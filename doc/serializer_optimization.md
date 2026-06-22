# Передача сервисных данных в Sink — план оптимизации

Документ описывает архитектуру доставки сервисных данных (log descriptors,
attribute descriptors, thread descriptors, trace descriptors, metric descriptors,
log module descriptors) в Sink при сохранении пропускной способности
**на уровне сотен миллионов событий в секунду**. План родился из обсуждения
открытых вопросов в `doc/todo.md` («Создание синков») и фиксирует решения,
к которым мы пришли.

## 1. Контекст

Текущее состояние (после `4829f69 feat(log): binary protocol, fragmentation, and serialization refactoring`):

- `cp8_core` — синглтон с buffer pool, реестрами log descriptors и attribute descriptors.
- `cp8_tls_writer` — базовый TLS-писатель, владеет текущим буфером и списком фрагментов.
- `cp8_log : cp8_tls_writer` — `thread_local`-инстанс с локальной картой descriptor'ов по hash.
- Бинарный протокол: `s_p8_hdr` (hello), `s_p8_data_buf_hdr`, `s_p8_log_item_hdr`.
- `P8_PACKET_SERVICE = 4` зарезервирован, но не используется.

Чего не хватает:

- Sink (буферы сейчас просто возвращаются в свободный пул).
- Consumer-нити, которая дренирует «готовые» буферы.
- Сериализованного представления сервисных данных.
- Версионирования / инкрементальной доставки.
- Reset-семантики при ротации файла или разрыве сети.

## 2. Цели и ограничения

Полученные от заказчика жёсткие требования:

1. **Producer hot-path** — сериализация descriptor'а выполняется **один раз** в момент
   первой регистрации (внутри уже существующего mutex'а). На сам emit добавляется
   только минимально необходимая работа — её цена обсуждается в п.4.
2. **Никакого центрального сканирования** payload'а на consumer-нити —
   при сотнях миллионов событий/сек это создало бы непреодолимое бутылочное горлышко.
3. **Sink** — единый интерфейс с двумя реализациями: файловый (write-only,
   допускает ротацию) и сетевой (TCP к коллектору, разрывы соединения).
   Multi-sink не поддерживается.
4. **Reset-семантика** (ротация файла, восстановление сетевого соединения):
   pending data-буферы из очереди ядра **доставляются** в новый sink, и перед ними
   досылаются **только те** service-entries, на которые они ссылаются (а не вся
   история регистраций).
5. **Иммутабельность сервисных данных** — после регистрации descriptor живёт
   неизменным до конца процесса. Этим пользуемся.
6. **Опциональность REFS-механизма** — REFS-pipeline (mini-буферы, dedup
   через `mu_current_buf_seq`, footer-сборка) активен **только** для sink'ов,
   которые этого требуют (сетевой). Файловый sink сохраняет service-данные
   персистентно в отдельный не-ротируемый side-файл и **не нуждается** в
   REFS — соответственно весь REFS-pipeline должен **полностью выключаться**
   на producer-стороне без overhead'а сверх одной branch-предикции. Подробно
   в разделе 7.5.

## 3. Общая архитектура

```
┌──────────────────┐     ┌──────────────────────┐     ┌───────────────────┐
│  TLS writer #N   │     │       cp8_core       │     │   Consumer thread │
│  (cp8_log/trc/   │     │                      │     │   (high priority) │
│   mtk per thread)│     │  data pool (8 KB)    │     │                   │
│                  │     │  mini pool (1 KB)    │     │ pop pair:         │
│ - data buffer    │     │  both grow on demand │     │   REFS → IDs →    │
│ - mini buffer    │ ──► │  ready queue ◄───────┼──── │   lookup registry │
│   (footer-refs)  │     │  registries with     │ ──► │   build SERVICE   │
│ - dedup state    │     │  pre-serialized      │     │ sink.write(SVC)   │
└──────────────────┘     │  service bytes       │     │ sink.write(DATA)  │
                         └──────────────────────┘     │ recycle pair      │
                                                      └────────┬──────────┘
                                                               ▼
                                                       ┌──────────────────┐
                                                       │      Sink        │
                                                       │ (file | network) │
                                                       └──────────────────┘
```

Ключевые элементы:

- **Producer (TLS writer)** — пишет data items в data-буфер, попутно (с дедупом)
  пишет ID референсы в отдельный **mini-buffer** ("REFS"-буфер) из mini-pool ядра.
- **Core** — два buffer pool'а с независимой геометрией (data 8 KB, mini 1 KB),
  оба расширяются по требованию до общего `mz_max_memory_size`; реестры
  descriptor'ов с pre-serialized bytes; ready-queue, в которую producer кладёт
  **logical bundle** атомарно. Bundle = `M mini-фрагментов + N data-фрагментов`
  (`M ≥ 0`, `N ≥ 1`), submit'нутых под одним lock'ом в порядке
  `[mini_0, mini_1, …, data_0, data_1, …]`.
- **Consumer thread** — единственная нить ядра с максимально доступным
  приоритетом; pop'ает bundle из ready-queue, парсит mini-цепочку (REFS),
  делает lookup в реестрах, строит `P8_PACKET_SERVICE` буфер, отправляет
  sink'у service затем data-цепочку, возвращает все буферы в свои pool'ы.
- **Sink** — интерфейс из двух методов; на проводе видит только финальные
  packet'ы `[SERVICE][LOGS|TRACES|METRICS]`, ничего не знает про REFS —
  это internal packet между producer'ом и consumer'ом.

Mini submit'ится **одновременно с data**, в том же bundle — никакого
ожидания заполнения mini не происходит. Mini может быть от нуля до
нескольких фрагментов; data всегда хотя бы один. Естественный инвариант:
`live(mini_buf) ≤ live(data_buf)` (один mini не сопровождает больше data,
чем сам data-bundle содержит). Mini-pool накладной расход памяти ограничен
~12.5% от data-pool'а (1 KB на каждые 8 KB).

**Два режима**: описанный выше pipeline (REFS-enabled) активен только при
сетевом sink'е. Файловый sink **выключает** REFS-pipeline полностью —
mini-pool не allocate'ится, producer'ы пропускают dedup-check за одну
branch-предикцию, а service-данные сохраняются sink'ом в **отдельный
персистентный файл**, который не ротируется. Подробности в разделе 7.4.

## 4. Producer-side: дедуп через seq-counter, парный mini-buffer

### 4.1 Расширения TLS state

В `cp8_tls_writer` добавляются:

```cpp
class cp8_tls_writer
{
    // existing
    uint8_t                            *mp_buffer    = nullptr;   // data-буфер 8 KB
    size_t                              mz_buf_used  = 0;
    size_t                              mz_buf_max   = 0;
    std::vector<const s_p8_attr_desc *> mo_attr_cache;
    kit::c_lst<uint8_t *>               mo_fragments { 16 };
    uint32_t                            mu_thread_id = 0;

    // NEW: дедуп-state
    uint32_t                            mu_current_buf_seq      = 0;
    bool                                mb_thread_desc_emitted  = false;
    std::vector<uint32_t>               mo_attr_last_seq;       // parallel to mo_attr_cache

    // NEW: mini-buffer для footer-refs (отдельный буфер из mini-pool ядра)
    uint8_t                            *mp_mini_buffer = nullptr;   // 1 KB, lazy-alloc
    size_t                              mz_mini_used   = 0;
    size_t                              mz_mini_max    = 0;          // = mz_mini_buffer_size из core
    uint16_t                            mu_section_counts[e_p8_svc_type_count] = {};
    kit::c_lst<uint8_t *>               mo_mini_fragments { 4 };     // для цепочки fragments

    // NEW: cached флаг "нужны ли REFS sink'у". Обновляется при start_new_bundle.
    // В файловом режиме всегда false → весь REFS-pipeline схлопывается (см. 7.5).
    bool                                mb_refs_enabled = false;
};
```

`mp_mini_buffer` берётся из mini-pool ядра одновременно с `mp_buffer` при
открытии нового логического буфера — на практике первый же emit нового
буфера всегда даст miss хотя бы по log_desc (после flush'а `mu_current_buf_seq`
инкрементируется, и все TLS-cache entries становятся stale для нового буфера).
Lazy-alloc внутри `cp8_tls_writer` нужен только чтобы не держать пустую пару
буферов, пока writer ни разу ничего не emit'нул.

В `cp8_log` карта descriptor'ов превращается из `std::map<uint64_t, s_p8_log_desc *>`
в `std::unordered_map<uint64_t, s_log_desc_tls>` ради O(1) lookup'а:

```cpp
struct s_log_desc_tls
{
    s_p8_log_desc *mp_desc;
    uint32_t       mu_last_buf_seq;
};

std::unordered_map<uint64_t, s_log_desc_tls> mo_desc_map;
```

В `cp8_trc` аналогично будет `std::unordered_map<uint64_t, s_trace_last_seq>` для отслеживания
видимости trace_id в текущем буфере.

### 4.2 Hot-path emit

Идея: дедуп выполняется **за счёт сравнения seq-counter'а** записи в TLS-cache
с `mu_current_buf_seq`. На emit к существующей работе добавляется ровно один
такой compare и, при miss'е, append в mini-buffer:

```cpp
// 1. существующий путь: lookup в TLS desc-cache
auto it = mo_desc_map.find(lu_hash);
s_log_desc_tls *lp_tls;
if(it == mo_desc_map.end()) [[unlikely]]
{
    s_p8_log_desc *lp_desc = mp_core->resolve_log_desc(lu_hash, ip_file, iu_line, ip_function, ip_format);
    if(!lp_desc) [[unlikely]]
    {
        return false;
    }
    lp_tls = &mo_desc_map.emplace(lu_hash, s_log_desc_tls { lp_desc, 0 }).first->second;
}
else
{
    lp_tls = &it->second;
}

// 2. НОВОЕ: дедуп-check + append в mini-buffer — ТОЛЬКО если sink требует REFS
if(mb_refs_enabled && lp_tls->mu_last_buf_seq != mu_current_buf_seq) [[unlikely]]
{
    lp_tls->mu_last_buf_seq = mu_current_buf_seq;
    mini_append_u64(e_p8_svc_log_desc, lu_hash);   // см. 4.6 — может вызвать flush
}

// 3. существующий путь: write log_item в data area
```

Поле `mb_refs_enabled` — кэш atomic-флага ядра, обновляется один раз при
старте каждого bundle (см. 4.4 и 7.5). В режиме файлового sink'а оно `false`
и весь REFS-блок схлопывается в одну branch-предикцию (~1 ns), которая
почти всегда правильно предсказывается (поток "знает" свой sink-режим).
То же выключение применяется к проверкам attr/trace/thread/module/mtk —
все они под одним `if(mb_refs_enabled)`.

Аналогичные проверки для attr (через `mo_attr_last_seq[attr_id]`), trace_id
(через `mo_trace_last_seq`), thread_desc (через `mb_thread_desc_emitted` — флаг
ровно один на буфер), module/mtk (по handle/id).

`mini_append_*` пишет напрямую в `mp_mini_buffer + mz_mini_used`. Если
`mp_mini_buffer == nullptr` — lazy `acquire_mini_buffer()` под капотом.
Если в mini-буфере не хватает места — flush пары `(mini, data)` и acquire
свежей пары (детали в 4.6).

Стоимость на cache-hit (hot case): один `cmp + branch-not-taken` ≈ **2 ns**.
На cache-miss (~1 раз на ID за весь буфер): один `store + memcpy 8B` ≈ **+10 ns**.

### 4.3 Дедуп by construction

Любой повторный emit с тем же ID в том же буфере видит
`lp_tls->mu_last_buf_seq == mu_current_buf_seq` и пропускает append.
**Никакого скана mini-буфера никогда не происходит** — ни на write, ни на read.
Дедуп возникает как побочный эффект порядка "проверить counter → set counter
→ append", тождественно паттерну generation-counter в game engines и memory
allocators.

### 4.4 Buffer rotation

В общем виде (включая fragmentation) логика flush bundle описана в 4.5.
Для простейшего случая `M=1, N=1` (один data-буфер + один mini-буфер,
оба заполнились без переполнения mid-record) flow выглядит так:

```cpp
// 1. финализировать заголовки
auto *lp_data_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(mp_buffer);
lp_data_hdr->mu_size = static_cast<uint16_t>(mz_buf_used);

if(mp_mini_buffer)
{
    auto *lp_mini_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(mp_mini_buffer);
    lp_mini_hdr->mu_size = static_cast<uint16_t>(mz_mini_used);
}

// 2. подготовить chain'ы (для случая M=1, N=1)
kit::c_lst<uint8_t *> lo_minis;
kit::c_lst<uint8_t *> lo_datas;
if(mp_mini_buffer) lo_minis.push_last(mp_mini_buffer);
lo_datas.push_last(mp_buffer);

// 3. atomic submit bundle в ready-queue под одним lock'ом
mp_core->submit_chain(lo_minis, lo_datas);
mp_buffer      = nullptr;
mp_mini_buffer = nullptr;

// 4. сброс state
mz_buf_used  = 0;
mz_mini_used = 0;
memset(mu_section_counts, 0, sizeof(mu_section_counts));
mb_thread_desc_emitted = false;

// 5. инкремент seq (все existing TLS entries автоматически stale для новой bundle)
mu_current_buf_seq++;

// 6. refresh cached флага sink'а — atomic_load(memory_order_relaxed), ~1 ns
mb_refs_enabled = mp_core->get_refs_enabled();
```

Следующий emit на этой нити acquire'ит новый data-буфер; mini-буфер
lazy-acquire'ится на первом cache-miss'е (только если `mb_refs_enabled`).
В файловом режиме mini-буфер не acquire'ится вообще никогда.

**Никакого memcpy** при flush'е — буферы уже заполнены, нужно только их
финализировать (записать `mu_size`) и положить в очередь. **Никакого O(N)
sweep'а** TLS-cache при ротации — все entries становятся stale естественным
образом через несоответствие seq. Это критично: при сотнях миллионов emit'ов
в секунду ротация bundle может происходить тысячи раз/сек, и любой sweep
O(N) был бы недопустим.

### 4.5 Logical bundle — N data-буферов + M mini-буферов

Существующий `P8_DATA_FLAG_FRAGMENT` означает, что одна логическая запись
растянута на несколько физических буферов. В обновлённой модели
**одна logical bundle = одна invocation `submit_chain`** содержит:

- `N ≥ 1` data-буферов (типично 1, при fragmentation logical-записи — больше)
- `M ≥ 0` mini-буферов (типично 1, при service-burst'е — больше; 0 в крайнем
  случае, если ни одного нового ID не появилось — см. ниже)

Правила формирования bundle:

- `mu_current_buf_seq` инкрементируется **только** при старте новой logical
  bundle, не на каждый физический фрагмент.
- Когда **data-буфер** заполняется, но logical-запись ещё не закрыта:
  data-буфер уходит в `mo_fragments`, acquire'ится новый data-буфер.
  Mini-буфер при этом **продолжает заполняться тем же экземпляром** — он
  относится ко всей logical bundle, не к одному физическому data-буферу.
- Если переполняется **сам mini-буфер**, текущий mini-буфер уходит
  в `mo_mini_fragments`, acquire'ится новый mini-буфер. Цепочка mini-буферов
  логически конкатенируется consumer'ом.
- На flush bundle (когда logical-запись закрыта) producer делает
  `submit_chain(io_minis, io_datas)` — атомарный submit обеих цепочек под
  одним lock'ом, mini-цепочка идёт первой:

  ```cpp
  void cp8_core::submit_chain(kit::c_lst<uint8_t *> &io_minis,
                              kit::c_lst<uint8_t *> &io_datas)
  {
      std::lock_guard<kit::c_spin_lock> lo_guard(mo_ready_lock);
      while(io_minis.size() > 0) mo_ready_queue.push_last(io_minis.pop_first());
      while(io_datas.size() > 0) mo_ready_queue.push_last(io_datas.pop_first());
  }
  ```

- Consumer pop'ает буферы в порядке submit'а; собирает mini-цепочку до
  отсутствия FRAGMENT-флага, затем data-цепочку аналогично.

**Mini submit'ится не дожидаясь заполнения** — он уходит в bundle с тем
content'ом, что накоплен на момент закрытия logical-записи. Если bundle
содержит даже один новый ID — mini не пустой; если ни одного — mini
вообще не acquired'ится (lazy), и в bundle уйдут только data-буфера.

#### Примеры bundle

```
Случай 1 (типичный, M=0): bundle = [data]
  emit(A, X) — все известны для текущего seq, в mini ничего не добавилось,
  data full → submit_chain([], [data]).

Случай 2 (типичный, M=1, N=1): bundle = [mini, data]
  emit(A=new, X) → mini += A, data += item
  ... 100 emit'ов ...
  data full → submit_chain([mini], [data]).

Случай 3 (data-fragmentation, M=1, N=2): bundle = [mini, data_0, data_1]
  emit(A=new), emit(...), emit(...), data overflows item → data_0 → mo_fragments
  emit(item-continuation) → data_1
  logical record closes → submit_chain([mini], [data_0, data_1]).

Случай 4 (service-burst, M=2, N=1): bundle = [mini_0, mini_1, data]
  emit(A=new, X=new, Y=new, ... 130 new IDs)
  mini полнится → mini_0 → mo_mini_fragments
  продолжаем заполнять mini_1
  data full → submit_chain([mini_0, mini_1], [data]).

Случай 5 (всё fragment'ит, M=2, N=3): bundle = [mini_0, mini_1, data_0, data_1, data_2]
  большая logical-запись с burst'ом новых IDs.
```

Никаких persistent mini между разными bundle'ами нет — каждая bundle
self-contained: содержит все REFS, на которые ссылаются её data-фрагменты.
Это сохраняет инвариант "pending bundle переживает Reset" из раздела 7.3:
consumer обрабатывает bundle как атомарную единицу, парсит mini-цепочку,
догоняет sink_known, отправляет SERVICE + DATA.

### 4.6 Защита от переполнения mini-буфера

Никакого worst-case reservation на emit делать не нужно — `mini_append_*`
сам проверяет наличие места и при необходимости fragment'ит цепочку
mini-буферов:

```cpp
void cp8_tls_writer::mini_append_u64(uint8_t iu_type, uint64_t iu_id)
{
    size_t lz_needed = sizeof(s_mini_entry_u64);    // type:u8 + id:u64

    if(!mp_mini_buffer) [[unlikely]]
    {
        mp_mini_buffer = mp_core->acquire_mini_buffer();   // lazy
        if(!mp_mini_buffer) [[unlikely]]
        {
            // OOM: эмиссия будет дропнута выше по стеку
            return;
        }
        init_mini_buf_hdr(mp_mini_buffer);
        mz_mini_used = sizeof(s_p8_data_buf_hdr);
    }

    if(mz_mini_used + lz_needed > mz_mini_max) [[unlikely]]
    {
        // fragment mini-chain, продолжаем заполнять
        auto *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(mp_mini_buffer);
        lp_hdr->mu_flags |= P8_DATA_FLAG_FRAGMENT;
        lp_hdr->mu_size   = static_cast<uint16_t>(mz_mini_used);
        mo_mini_fragments.push_last(mp_mini_buffer);

        mp_mini_buffer = mp_core->acquire_mini_buffer();
        if(!mp_mini_buffer) [[unlikely]]
        {
            return;     // OOM
        }
        init_mini_buf_hdr(mp_mini_buffer);
        mz_mini_used = sizeof(s_p8_data_buf_hdr);
    }

    // append entry
    mp_mini_buffer[mz_mini_used++] = iu_type;
    memcpy(mp_mini_buffer + mz_mini_used, &iu_id, sizeof(uint64_t));
    mz_mini_used += sizeof(uint64_t);
    mu_section_counts[iu_type]++;
}
```

Преимущества по сравнению с inline-scratch'ом:

- Нет жёсткого верхнего лимита на размер footer'а — fragmentation работает
  на mini-цепочке как на data-цепочке.
- Нет worst-case reservation расчёта перед каждым emit'ом.
- Нет различия в коде между "footer влезает" и "не влезает" — один путь.

Цена: один атомарный pool-acquire на каждые ~120 уникальных ID
(при `mz_mini_buffer_size = 1024`). На реалистичной нагрузке (≤ 50
уникальных ID/буфер) — **ноль дополнительных acquire'ов сверх одного
на буфер**.

### 4.7 Регистрация TLS writer'а в core

Чтобы `p8_exceptional_flush` (см. 7.6) мог достать данные in-flight у каждой
нити, каждый `cp8_tls_writer` при создании регистрируется в глобальном
intrusive-списке `cp8_core`, а при destruction — отписывается. Это нужно
**только** для crash-flush'а; в нормальной операции список никем не
используется.

```cpp
class cp8_tls_writer
{
    // ...
    cp8_tls_writer *mp_next = nullptr;       // NEW: intrusive list для registration
    cp8_tls_writer *mp_prev = nullptr;
};

// в конструкторе
cp8_tls_writer::cp8_tls_writer()
    : mp_core(cp8_core::get_global_core(P8_CORE_ACQUIRE_TIMEOUT_MS))
    , mu_thread_id(...)
{
    if(mp_core)
    {
        mp_core->addref();
        mz_buf_max = cp8_core::mz_data_buffer_size;
        mp_core->register_writer(this);   // NEW
    }
}

// в деструкторе
cp8_tls_writer::~cp8_tls_writer()
{
    if(mp_core)
    {
        mp_core->unregister_writer(this);  // NEW
        // ... существующая очистка буферов ...
    }
}
```

Стоимость: один acquire `kit::c_spin_lock` на создание/destruction TLS
writer'а — то есть **один раз на жизнь нити**. На hot-path emit'е — ноль
(список вообще не трогается).

## 5. Core: registry с pre-serialized cache

### 5.1 Расширение существующих descriptor'ов

Каждый descriptor получает поле с заранее сериализованными байтами, ready-to-memcpy:

```cpp
struct s_p8_log_desc
{
    // existing
    uint64_t      mu_hash;
    const char   *mp_format;
    const char   *mp_file;
    const char   *mp_function;
    uint32_t      mu_line;
    size_t        mz_args_count;
    size_t        mz_fixed_args_size;
    s_p8_log_varg ma_args[P8_LOG_MAX_ARGS];

    // NEW: pre-serialized service entry
    uint16_t  mu_serialized_size;
    uint8_t  *mp_serialized;    // alloc один раз в resolve_log_desc()
};

struct s_p8_attr_desc
{
    p8_attr_id          mi_id;
    enum e_p8_attr_type me_type;
    char               *mp_name;

    // NEW
    uint16_t  mu_serialized_size;
    uint8_t  *mp_serialized;
};
```

Аналогичные поля у будущих `s_p8_thread_desc`, `s_p8_trace_desc`,
`s_p8_mtk_desc`, `s_p8_module_desc`.

`mp_serialized` aллоцируется **один раз** в момент регистрации, под уже
существующим mutex'ом, и **никогда** не изменяется (иммутабельность сервисных
данных). Чтение блока с любой нити — без локов.

Для cache-friendliness рекомендуется выделять `mp_serialized` blob'ы slab'ом
(один большой preallocated kit-list / arena), чтобы все service-bytes одного
типа лежали в соседних cache lines.

### 5.2 Изменения registry'ев

| Registry | Текущее состояние | Целевое состояние | Lookup |
|----------|-------------------|-------------------|--------|
| `mo_log_descs` | `std::map<uint64_t, s_p8_log_desc *>` | `std::unordered_map<uint64_t, s_p8_log_desc *>` | O(1) amortized |
| `mo_attr_descs` | `std::vector<s_p8_attr_desc *>` | без изменений | O(1) array index |
| trace registry | отсутствует | `std::unordered_map<uint64_t, s_p8_trace_desc *>` | O(1) amortized |
| thread registry | отсутствует | `std::vector<s_p8_thread_desc *>` indexed by slot | O(1) |
| mtk registry | отсутствует | `std::vector<s_p8_mtk_desc *>` indexed by `h_p8_mtk_id` | O(1) |
| module registry | отсутствует | `std::vector<s_p8_module_desc *>` indexed by handle | O(1) |

### 5.3 Pool'ы, submit_chain и buffer lifecycle

Существующий монолитный buffer pool рефакторится в **отдельный класс**
`cp8_buffer_pool`, и в `cp8_core` живут два его экземпляра — для data-буферов
(8 KB) и для mini-буферов (1 KB). Учёт общего memory budget'а вынесен в
отдельный класс `cp8_memory_budget`, который оба pool'а используют как
shared resource. Это устраняет дублирование, делает pool тестируемым
изолированно и оставляет `cp8_core` отвечать только за координацию
(registries, sink, consumer-thread).

#### `cp8_memory_budget`

Atomic-счётчик shared memory budget'а. Используется pool'ами при
`grow-on-demand` чтобы не превысить общий лимит:

```cpp
class cp8_memory_budget
{
public:
    explicit cp8_memory_budget(size_t iz_max_bytes);

    // Atomic reservation. true если резерв удался.
    bool try_reserve(size_t iz_bytes);
    void release(size_t iz_bytes);

    size_t get_used() const { return mu_used.load(std::memory_order_relaxed); }
    size_t get_max() const { return mz_max; }

private:
    std::atomic<size_t> mu_used { 0 };
    size_t              mz_max;
};
```

#### `cp8_buffer_pool`

Buffer pool фиксированного размера буфера с grow-on-demand. `mp_budget` —
non-owning указатель на общий budget (которым владеет `cp8_core`):

```cpp
class cp8_buffer_pool
{
public:
    // iz_buffer_size — фиксирован для всех буферов в этом pool'е.
    cp8_buffer_pool(size_t iz_buffer_size, cp8_memory_budget *ip_budget);
    ~cp8_buffer_pool();

    cp8_buffer_pool(const cp8_buffer_pool &)            = delete;
    cp8_buffer_pool &operator=(const cp8_buffer_pool &) = delete;
    cp8_buffer_pool(cp8_buffer_pool &&)                 = delete;
    cp8_buffer_pool &operator=(cp8_buffer_pool &&)      = delete;

    // Pre-allocate iz_initial_count буферов (если budget позволяет).
    // Возвращает реально pre-allocated count.
    // init(0) — корректное "не аллоцировать ничего сейчас";
    // grow-on-demand всё равно сработает при первом acquire.
    size_t init(size_t iz_initial_count);

    // Acquire свободный буфер. nullptr если budget исчерпан или malloc упал.
    uint8_t *acquire();

    // Вернуть в pool после использования consumer'ом.
    void     recycle(uint8_t *ip_buf);

    size_t   get_buffer_size() const { return mz_buffer_size; }
    size_t   get_total_allocated() const;
    size_t   get_free_count() const;

private:
    size_t                          mz_buffer_size;
    cp8_memory_budget              *mp_budget;       // shared, не owning
    kit::c_lst<uint8_t *>           mo_free;
    kit::c_lst<uint8_t *>           mo_all;
    std::atomic<size_t>             mu_total_allocated { 0 };
    std::mutex                      mo_mutex;
};
```

#### Grow-on-demand (внутри `cp8_buffer_pool::acquire`)

```cpp
uint8_t *cp8_buffer_pool::acquire()
{
    std::lock_guard<std::mutex> lo_guard(mo_mutex);

    if(mo_free.size() > 0)
    {
        return mo_free.pop_first();                         // hot path
    }

    if(!mp_budget->try_reserve(mz_buffer_size))
    {
        return nullptr;                                      // budget exceeded
    }

    uint8_t *lp_new = static_cast<uint8_t *>(malloc(mz_buffer_size));
    if(!lp_new) [[unlikely]]
    {
        mp_budget->release(mz_buffer_size);                  // rollback
        return nullptr;
    }
    mo_all.push_last(lp_new);
    mu_total_allocated.fetch_add(mz_buffer_size, std::memory_order_relaxed);
    return lp_new;
}
```

`recycle` симметрично возвращает в `mo_free` под lock'ом.

#### `cp8_core` после рефакторинга

```cpp
class cp8_core
{
    // ─── memory budget (shared между pool'ами) ───
    cp8_memory_budget               mo_memory_budget;

    // ─── два экземпляра pool'а с разной геометрией ───
    cp8_buffer_pool                 mo_data_pool;     // 8 KB
    cp8_buffer_pool                 mo_mini_pool;     // 1 KB

    // ─── ready queue ───
    kit::c_lst<uint8_t *>           mo_ready_queue;
    kit::c_spin_lock                mo_ready_lock;

    // ─── sink и REFS-режим ───
    cp8_sink_iface                 *mp_sink           = nullptr;
    std::atomic<bool>               mb_refs_enabled { false };

    // ─── intrusive list зарегистрированных TLS writer'ов (для exceptional_flush) ───
    cp8_tls_writer                 *mp_writers_head   = nullptr;
    kit::c_spin_lock                mo_writers_lock;

public:
    cp8_core(const s_p8_config *ip_config);

    // pool-facades (inline → компилятор inlin'ит, ноль indirection в release build'е)
    uint8_t *acquire_data_buffer() { return mo_data_pool.acquire(); }
    uint8_t *acquire_mini_buffer() { return mo_mini_pool.acquire(); }

    // registration of TLS writers — вызывается из их конструктора/деструктора (см. 4.7)
    void     register_writer(cp8_tls_writer *ip_writer);
    void     unregister_writer(cp8_tls_writer *ip_writer);

    // ОПАСНО: вызывается только из p8_exceptional_flush (signal context).
    // Делает lock-free traversal списка writer'ов, не acquire'ит ни одного mutex'а.
    // См. раздел 7.6.
    void     exceptional_flush_signal_safe();

    bool     get_refs_enabled() const
    {
        return mb_refs_enabled.load(std::memory_order_relaxed);
    }

    // atomic submit chain: mini'ы первыми, data вторыми; один lock на всё
    void     submit_chain(kit::c_lst<uint8_t *> &io_minis,
                          kit::c_lst<uint8_t *> &io_datas);

    // routing recycle по mu_packet_type (P8_PACKET_REFS → mini-pool, иначе → data-pool)
    void     recycle_buffer(uint8_t *ip_buf);
};
```

#### Инициализация в `cp8_core::cp8_core`

```cpp
cp8_core::cp8_core(const s_p8_config *ip_config)
    : mo_memory_budget(parse_max_memory(ip_config))
    , mo_data_pool(8192, &mo_memory_budget)
    , mo_mini_pool(parse_mini_buffer_size(ip_config), &mo_memory_budget)
{
    size_t lz_initial_data = parse_initial_memory(ip_config) / 8192;
    mo_data_pool.init(lz_initial_data);

    // mini-pool инициализируется ПОСЛЕ выбора sink'а:
    // если sink не требует REFS → mo_mini_pool.init(0), иначе разумное initial
    // (см. 7.4)
}
```

#### Конфигурация

`mz_mini_buffer_size` — поле в `s_p8_config`:

```cpp
struct s_p8_config
{
    // existing fields ...
    size_t mz_mini_buffer_size;   // default 1024; 0 = use library default
};
```

Default 1024 байта покрывает ~120 уникальных ID на буфер при entry-size 8 B
(`type:u8 + id:u64`). Tuning имеет смысл если профилирование показывает
частую fragmentation mini-цепочки.

#### Atomic submit_chain (по-прежнему в `cp8_core`)

Под одним spin-lock'ом — гарантирует FIFO порядок `mini → data` в очереди
даже при concurrent push'ах разных нитей:

```cpp
void cp8_core::submit_chain(kit::c_lst<uint8_t *> &io_minis,
                            kit::c_lst<uint8_t *> &io_datas)
{
    std::lock_guard<kit::c_spin_lock> lo_guard(mo_ready_lock);
    while(io_minis.size() > 0)
    {
        mo_ready_queue.push_last(io_minis.pop_first());
    }
    while(io_datas.size() > 0)
    {
        mo_ready_queue.push_last(io_datas.pop_first());
    }
}
```

#### Recycle routing

Один публичный `recycle_buffer` диспатчит на нужный pool по типу packet'а:

```cpp
void cp8_core::recycle_buffer(uint8_t *ip_buf)
{
    auto *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(ip_buf);
    if(lp_hdr->mu_packet_type == P8_PACKET_REFS)
    {
        mo_mini_pool.recycle(ip_buf);
    }
    else
    {
        mo_data_pool.recycle(ip_buf);
    }
}
```

Инвариант: `live(mini_buf) ≤ live(data_buf)` — поэтому mini-pool никогда
не вырастет значительно больше data-pool'а в долях общего budget'а
(~12.5% при дефолтной геометрии).

#### Тестируемость

После рефакторинга `cp8_buffer_pool` и `cp8_memory_budget` тестируются
**изолированно**, без поднятия всего `cp8_core`:

- pool acquire/recycle/grow/exhaust
- concurrent acquire из нескольких нитей
- budget reservation/release под contention
- correct cleanup в destructor'е

`cp8_core` после рефакторинга остаётся в основном координационным слоем
(registries, sink lifecycle, consumer-thread).

## 6. Wire format

### 6.1 Packet types и buffer header

`s_p8_data_buf_hdr` **не расширяется** — footer теперь живёт в отдельном
mini-буфере, не в data-буфере. Достаточно добавить новый packet type:

```c
#define P8_PACKET_MAIN     0
#define P8_PACKET_LOGS     1
#define P8_PACKET_TRACES   2
#define P8_PACKET_METRICS  3
#define P8_PACKET_SERVICE  4
#define P8_PACKET_REFS     5    // NEW: mini-буфер с ID-set'ом
```

`P8_PACKET_REFS` — это **internal** packet между producer'ом и consumer'ом.
На проводе sink его никогда не видит — consumer материализует его в
`P8_PACKET_SERVICE` через lookup'ы в registry.

`P8_PACKET_SERVICE` — внешний packet, который consumer строит на лету
из REFS-буфера и pre-serialized blob'ов из registry; именно его получает sink.

### 6.2 REFS-буфер: формат тела

Тело REFS-буфера (после `s_p8_data_buf_hdr` с `mu_packet_type = P8_PACKET_REFS`)
— flat-список entry, типизированных по `type:u8`. Каждый entry самодостаточен,
парсер просто идёт вперёд от начала body до `mu_size`:

```
[s_p8_data_buf_hdr]                              // packet_type=P8_PACKET_REFS
[entry: type:u8, id:type-specific (4 или 8 байт)]
[entry: type:u8, id:type-specific]
...
```

Альтернативно — sections (один section header на тип):

```
[s_p8_data_buf_hdr]
[section_count:u8]
[section_1]
  [type:u8]
  [count:u16]
  [id_payload : type-specific bytes × count]
[section_2]
  ...
```

Sections экономят байты при множестве entry'ов одного типа (например, 50 attr'ов
дают section header 4 байта + 50×4 = 204 vs flat 50×(1+4) = 250). Подробный
выбор формата — детально на шаге 4 реализации, основываясь на наблюдаемом
распределении типов в REFS-буферах.

Размер `id_payload` зависит от типа:

| `type` | id_size | id type |
|--------|---------|---------|
| `e_p8_svc_log_desc` | 8 | u64 hash |
| `e_p8_svc_attr` | 4 | p8_attr_id (i32) |
| `e_p8_svc_thread` | 4 | u32 thread_id |
| `e_p8_svc_trace` | 8 | u64 trace_id |
| `e_p8_svc_mtk` | 4 | h_p8_mtk_id (i32) |
| `e_p8_svc_module` | 4 | u32 module handle |

### 6.3 SERVICE-buffer payload

Consumer строит `P8_PACKET_SERVICE` буфер из последовательно склеенных
pre-serialized blob'ов. Каждый blob уже имеет внутренний header вида:

```
[svc_entry]
  [type:u8]
  [size:u16]      // размер payload не включая header
  [payload:bytes] // type-specific descriptor body
```

Конкретные форматы payload по типам (предварительные, детали при реализации):

```
log_desc payload:
  [hash:u64][line:u32]
  [file_len:u16][file:bytes]
  [function_len:u16][function:bytes]
  [format_len:u16][format:bytes]
  [args_count:u8][args : s_p8_log_varg × args_count]

attr payload:
  [id:i32][type:u8]
  [name_len:u16][name:bytes]

thread payload:
  [id:u32]
  [name_len:u16][name:bytes]

trace payload:
  [trace_id:u64][parent_id:u64][line:u32]
  [file_len:u16][file:bytes]
  [function_len:u16][function:bytes]
  [args_format_len:u16][args_format:bytes]

mtk payload:
  [id:i32][flags:u8] // push/pull, group, etc.
  [name_len:u16][name:bytes]
  [description_len:u16][description:bytes]
  [unit_len:u16][unit:bytes]
  [min:f64][max:f64][on:u8]

module payload:
  [handle:u32][verbosity:u8]
  [name_len:u16][name:bytes]
```

Pre-serialized blob иммутабелен — изменения верхних флагов (например,
`p8_log_set_verbosity`) транслируются в **отдельные** events типа
`P8_PACKET_MAIN` или новый mini-packet (out of scope для этого плана).

## 7. Consumer thread и Sink

### 7.1 Consumer thread

Единственная нить, владеется `cp8_core`. Старт — внутри `p8_initialize`,
остановка — внутри `p8_release`.

**Приоритет**: при старте нить пытается установить максимально доступный
системе приоритет (на POSIX — `SCHED_FIFO` / `SCHED_RR` через
`pthread_setschedparam`, на Windows — `THREAD_PRIORITY_TIME_CRITICAL` через
`SetThreadPriority`). Это minimises вероятность того, что под нагрузкой
consumer не успевает дренировать ready-queue и producer'ы упрутся
в `acquire_*_buffer() == nullptr` через выработку memory budget.
При отказе системы поднять приоритет (например, не root на Linux без
CAP_SYS_NICE) — логируем warning и продолжаем с дефолтным приоритетом.

Псевдокод цикла:

```cpp
void cp8_core::consumer_loop()
{
    set_max_thread_priority();   // best-effort

    while(!mb_shutdown_requested)
    {
        // pop логическую пару (mini-chain + data-chain)
        kit::c_lst<uint8_t *> lo_minis;
        kit::c_lst<uint8_t *> lo_datas;
        if(!pop_ready_bundle(lo_minis, lo_datas))    // blocking / timed-wait
        {
            continue;
        }

        // mini-chain — это P8_PACKET_REFS буферы; собрать недостающие IDs
        s_svc_diff lo_diff = compute_service_diff(lo_minis, mo_sink_known);
        if(lo_diff.mz_total_size > 0)
        {
            // материализуем SERVICE-buffer из pre-serialized blob'ов registry
            send_service_buffer(lo_diff);          // sink.write(P8_PACKET_SERVICE, ...)
            apply_diff(mo_sink_known, lo_diff);
        }

        // переслать data-chain
        for(uint8_t *lp_data : lo_datas)
        {
            auto *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(lp_data);
            if(!mp_sink->write(lp_hdr->mu_packet_type, lp_data, lp_hdr->mu_size))
            {
                handle_sink_reset();               // см. 7.3
                break;
            }
        }

        // recycle оба chain'а в соответствующие pool'ы
        while(lo_minis.size() > 0) recycle_mini_buffer(lo_minis.pop_first());
        while(lo_datas.size() > 0) recycle_data_buffer(lo_datas.pop_first());
    }
}
```

`pop_ready_bundle` — внутренний хелпер, который читает из ready-queue mini-chain
до отсутствия FRAGMENT-флага, затем data-chain до отсутствия FRAGMENT-флага.
Так как submit_chain гарантирует FIFO порядок `mini → data`, парсинг
тривиален.

### 7.2 Sink interface

```cpp
class cp8_sink_iface
{
public:
    virtual ~cp8_sink_iface() = default;

    // Отправить буфер. Возвращает true при успехе. False означает,
    // что sink в неработоспособном состоянии — consumer инициирует reset.
    virtual bool write(uint8_t iu_packet_type, const uint8_t *ip_bytes, size_t iz_size) = 0;

    // Жизненный цикл sink'а до/после reset. Дёргается consumer'ом.
    virtual void on_reset() = 0;

    // Опциональный hook: sink может persistить свой known-set и при старте
    // сообщить consumer'у, какие ID не нужно пересылать.
    //
    // По умолчанию ничего не делает — consumer считает sink-known-set пустым.
    virtual void prime_known(struct s_p8_sink_known &or_known) { (void)or_known; }

    // NEW: говорит ядру, нужны ли REFS-буфера в стриме.
    // Возвращает true → producer'ы будут собирать REFS, consumer строить SERVICE.
    // Возвращает false → весь REFS-pipeline выключается, sink сам отвечает за
    // персистентное хранение service-данных (через on_descriptor_registered).
    virtual bool needs_refs() const = 0;

    // NEW: вызывается ядром при первой регистрации descriptor'а под mutex'ом
    // регистрации, сразу после того как создан pre-serialized blob.
    // File sink использует этот hook для записи в side service-файл.
    // По умолчанию — noop.
    virtual void on_descriptor_registered(uint8_t        iu_svc_type,
                                          uint64_t       iu_id,
                                          const uint8_t *ip_blob,
                                          size_t         iz_size)
    {
        (void)iu_svc_type; (void)iu_id; (void)ip_blob; (void)iz_size;
    }

    // NEW: async-signal-safe write. Вызывается ТОЛЬКО из p8_exceptional_flush.
    // Реализация ДОЛЖНА:
    //   - не аллоцировать память (никаких malloc/new)
    //   - не брать lock'и, которые могут заблокировать
    //   - пользоваться только async-signal-safe syscall'ами:
    //       POSIX: write(2), send(2)
    //       Windows: WriteFile, WSASend (валидны в SEH __except)
    //   - писать напрямую в pre-opened fd / socket, не через FILE*/std::ostream
    //
    // Возвращаемое значение игнорируется — best-effort.
    virtual void write_signal_safe(uint8_t        iu_packet_type,
                                   const uint8_t *ip_bytes,
                                   size_t         iz_size) = 0;

    // NEW: flush'нуть собственный внутренний буфер sink'а через safe syscall.
    // Дёргается перед write_signal_safe loop'ом в exceptional_flush, чтобы
    // данные, накопленные в internal buffer'е, попали в файл/сокет.
    virtual void flush_internal_signal_safe() = 0;
};
```

#### Требования к internal-буферизации sink'а

Sink **должен** использовать собственный механизм буферизации (для batching
write'ов в нормальной работе), напрямую поверх raw fd/socket. Запрещено:

- `std::FILE *`, `std::ofstream`, `std::cout` — buffered I/O с внутренним
  lock'ом, не async-signal-safe
- любые аллокаторы во время `write_signal_safe`

Разрешено и рекомендуется:

- `int mi_fd` (POSIX) или `HANDLE mh_file` (Windows) — pre-opened при init
- собственный `uint8_t mp_write_buffer[N]` фиксированного размера
- batching через `write(2)` / `WriteFile` напрямую
- `O_APPEND` для атомарной записи в файл из нескольких context'ов

Две реализации:

- `cp8_sink_file` — пишет события в файл данных; `needs_refs() = false`;
  `on_descriptor_registered` дописывает blob в **отдельный persistent
  service-файл** (никогда не ротируется в течение жизни процесса);
  в `on_reset` закрывает и переоткрывает data-файл (например, после
  ротации); `prime_known` оставляет noop (REFS-механизм всё равно
  выключен).
- `cp8_sink_net` — TCP-клиент; `needs_refs() = true`; `write` блокирует
  на send; `on_reset` закрывает сокет и переподключается; `prime_known`
  может быть заполнен ответом удалённой стороны (по протоколу
  handshake — out of scope здесь); `on_descriptor_registered` —
  noop (descriptor'ы доедут через REFS).

### 7.3 Reset-семантика

Reset инициируется тремя путями:

1. `sink->write()` вернул `false` (например, TCP-разрыв или disk-full).
2. Внешний сигнал (например, file rotation по SIGHUP) через явный метод.
3. Программное закрытие через `p8_release`.

После reset'а consumer:

- Вызывает `sink->on_reset()`.
- **Не сбрасывает** ready-queue (pending data-буферы доставятся в новый sink).
- Очищает `mo_sink_known` (новый sink ничего не знает).
- Вызывает `sink->prime_known(mo_sink_known)` для возможной частичной
  инициализации (sink сам сообщает, что у него уже есть).
- Возвращается в цикл — первый же data-буфер из очереди вызовет
  re-send service entries для всех ID, которые в нём фигурируют (footer уже
  есть, так как был записан producer'ом до reset'а).

Это даёт **именно ту семантику**, которую заказчик хотел: «после долгого
разрыва шлём только то, что соответствует данным, которые ядро уже хранит
и собирается выслать в ближайшем будущем».

### 7.4 Двухрежимный pipeline: network vs file

Sink выбирает один из двух режимов при инициализации через `needs_refs()`.
Режим фиксирован на всё время жизни sink-инстанса.

#### Режим A: REFS-enabled (network)

Стандартный pipeline, описанный в разделах 4–7.

- `mp_core->mb_refs_enabled = true`
- producer'ы dedup'ят и пишут IDs в mini-буферы
- consumer строит SERVICE-buffer'ы из REFS + registry blob'ов
- `on_descriptor_registered` — noop (descriptor'ы доедут sink'у через
  обычный bundle flow)
- Reset clear'ит sink_known, pending bundle'а несут полную информацию

#### Режим B: REFS-disabled (file)

REFS-pipeline полностью выключен.

- `mp_core->mb_refs_enabled = false`
- producer'ы **не** делают dedup-check (один branch на emit, hot=false)
- mini-буфера **не acquire'ятся** ни одной нитью
- `mo_mini_pool.init(0)` — pool не allocate'ит initial slab; вся
  `mz_initial_memory_size` достаётся data-pool'у. Grow-on-demand технически
  по-прежнему доступен, но никогда не сработает, потому что producer'ы
  не вызывают `acquire_mini_buffer`.
- consumer на pop'е не ожидает REFS — bundle'а содержат только data
- sink_known не используется (нечего соотносить)

Service-данные доставляются через **отдельный персистентный файл**,
который sink пишет напрямую при регистрации descriptor'ов:

1. `cp8_core::resolve_log_desc` (и аналогичные функции регистрации) под
   уже существующим mutex'ом регистрации создают descriptor + pre-serialized
   blob, вставляют в registry.
2. Под тем же mutex'ом вызывается `mp_sink->on_descriptor_registered(...)`.
3. `cp8_sink_file::on_descriptor_registered` записывает blob в side-файл
   (open в `O_APPEND` режиме, fsync периодически).
4. Side-файл **никогда не ротируется** в течение жизни процесса —
   при ротации data-файла service-файл остаётся прежним.

При перезапуске процесса service-файл существует и может быть переиспользован:
новый процесс при первой регистрации того же descriptor'а вычисляет тот же
hash → seek в service-файле подсказывает что blob уже есть (опционально,
зависит от стратегии). Это deduplication между процессами — детали в
зоне реализации `cp8_sink_file`, out of scope для этого плана.

#### Стоимость "выключенного" режима на hot-path

| Операция | Стоимость в режиме A | Стоимость в режиме B |
|---|---|---|
| Lookup в `mo_desc_map` | ~10–20 ns | ~10–20 ns (без изменений) |
| `if(mb_refs_enabled)` branch | ~1 ns (taken) | ~1 ns (not taken, predicted) |
| Dedup-check + mini append | ~5–15 ns | **0 ns** (skip полностью) |
| mini-buffer acquire/recycle | ~50 ns / per buffer | **0** (никогда не acquire'ится) |

**Итог**: в режиме B producer добавляет к существующему hot-path'у ровно
**одну branch-предикцию** на emit (`mb_refs_enabled == false`, hot path
not-taken). При 10⁸ events/sec это ~100 ms/sec ≈ 10% one core в худшем
случае mispredict'ов и **<1%** при правильном предсказании (which is the
common case since flag не меняется в runtime'е).

#### Переключение sink'а в runtime

В первой версии sink **фиксируется** при `p8_initialize` и не меняется
до `p8_release`. Если нужен другой sink — рестарт процесса. Это
ограничение убирает следующие edge case'ы:

- pending bundle'а, заполненные в режиме A, попадают в sink режима B
  (или наоборот) — не происходит
- TLS-кэшированный `mb_refs_enabled` устаревает — не происходит
  (флаг обновляется только на старте каждого bundle, и не меняется
  в течение жизни процесса)

Динамический switch sink'а (например, fallback с network на file при
длительном disconnect) — отдельная фича, требует proectирования
синхронизации pending bundle'ов между режимами. Out of scope этого плана.

### 7.6 Exceptional flush — crash-handler путь

`p8_exceptional_flush` вызывается из crash-handler'а приложения (signal
handler на POSIX, `__except` block на Windows). Контекст накладывает
жёсткие ограничения:

- **Никаких аллокаций памяти** — `malloc`, `new`, std-контейнеры неприменимы.
- **Никаких ожидающих lock'ов** — async-signal-safe код не может звать
  `std::mutex::lock` или `pthread_mutex_lock`. `kit::c_spin_lock` мог бы
  завис в spin'е навечно, если crashed thread держит этот же lock.
- **Только async-signal-safe syscall'ы** — `write(2)`, `send(2)`, atomic'и,
  чтение памяти.
- **One-shot, terminal** — после вызова библиотека в неопределённом
  состоянии, дальнейшие P8-вызовы — UB. Это даёт нам право не заботиться
  о согласованности post-flush.

#### Ключевые проектные решения

1. **TLS writers зарегистрированы в core** (раздел 4.7) — exceptional flush
   итерирует intrusive-список `mp_writers_head` без участия producer'ов.
2. **Lock-free direct read** — exceptional flush **не пытается** acquire'ить
   ни `mo_writers_lock`, ни `mo_lock` каждого writer'а, ни pool-mutex'ы,
   ни `mo_ready_lock`. Все эти lock'и могут быть удержаны crashed нитью.
   Crash-flush просто **читает поля напрямую** (`mp_buffer`, `mz_buf_used`,
   `mp_mini_buffer`, `mz_mini_used`) — best-effort snapshot.
3. **Partial / corrupt data допустима** — если crash случился посреди
   серiaлизации log_item'а, прочитанный prefix может быть невалиден
   (например, `s_p8_log_item_hdr.mu_size` ещё не записан). Это OK:
   sink-сторонний parser должен уметь скипнуть broken packet и
   продолжить. Post-mortem-инструмент увидит максимум доступных данных.
4. **REFS не resolve'ятся** — в crash-context'е мы не строим SERVICE-буферы
   из REFS+registry. mini-буферы dump'ятся as-is. На приёмной стороне sink
   может увидеть unknown ID в data-буферах — это допустимо (раздел 2,
   ограничение #6 — full REFS pipeline валиден только в нормальной операции).
5. **Pool не используется** — никаких acquire/recycle. Буферы dump'ятся
   "as is", их не возвращают в pool после flush'а (процесс всё равно
   умирает).
6. **Свой syscall-уровень sink'а** — exceptional flush зовёт
   `sink->flush_internal_signal_safe()` затем `sink->write_signal_safe(...)`
   на каждом найденном буфере.

#### Псевдокод

```cpp
void p8_exceptional_flush()
{
    cp8_core *lp_core = cp8_core::get_global_core_no_wait();   // без acquire mutex'а
    if(!lp_core || !lp_core->mp_sink)
    {
        return;
    }

    lp_core->exceptional_flush_signal_safe();
}

void cp8_core::exceptional_flush_signal_safe()
{
    // 1. flush internal buffer sink'а — выкидываем все накопленные но не
    //    записанные в файл/сокет данные нормального flow
    mp_sink->flush_internal_signal_safe();

    // 2. dump ready-queue lock-free. Читаем head/tail напрямую, идём по
    //    next-pointer'ам. Возможен race с consumer'ом, который сейчас
    //    pop'ает — accept'им это как best-effort.
    for(cp8_lst_node *lp_node = mo_ready_queue.peek_first_unlocked();
        lp_node != nullptr;
        lp_node = lp_node->mp_next)
    {
        uint8_t *lp_buf = lp_node->mp_data;
        auto    *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(lp_buf);
        mp_sink->write_signal_safe(lp_hdr->mu_packet_type, lp_buf, lp_hdr->mu_size);
    }

    // 3. iterate registered TLS writers lock-free; читаем их in-flight состояние
    //    напрямую без acquire mo_lock
    for(cp8_tls_writer *lp_w = mp_writers_head;
        lp_w != nullptr;
        lp_w = lp_w->mp_next)
    {
        // 3a. fragments цепочки (уже накопленные fragment'ы)
        for(auto *lp_node = lp_w->mo_mini_fragments.peek_first_unlocked();
            lp_node; lp_node = lp_node->mp_next)
        {
            mp_sink->write_signal_safe(P8_PACKET_REFS, lp_node->mp_data,
                                       static_cast<s_p8_data_buf_hdr *>(...)->mu_size);
        }
        for(auto *lp_node = lp_w->mo_fragments.peek_first_unlocked();
            lp_node; lp_node = lp_node->mp_next)
        {
            // ... аналогично, packet_type из header'а ...
        }

        // 3b. current mini (если есть)
        uint8_t *lp_mini = lp_w->mp_mini_buffer;
        size_t   lz_mini = lp_w->mz_mini_used;
        if(lp_mini && lz_mini > sizeof(s_p8_data_buf_hdr))
        {
            // финализируем размер в header'е inplace перед write'ом
            auto *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(lp_mini);
            lp_hdr->mu_size = static_cast<uint16_t>(lz_mini);
            mp_sink->write_signal_safe(P8_PACKET_REFS, lp_mini, lz_mini);
        }

        // 3c. current data
        uint8_t *lp_data = lp_w->mp_buffer;
        size_t   lz_data = lp_w->mz_buf_used;
        if(lp_data && lz_data > sizeof(s_p8_data_buf_hdr))
        {
            auto *lp_hdr = reinterpret_cast<s_p8_data_buf_hdr *>(lp_data);
            lp_hdr->mu_size = static_cast<uint16_t>(lz_data);
            mp_sink->write_signal_safe(lp_hdr->mu_packet_type, lp_data, lz_data);
        }
    }

    // 4. final flush sink'а — пушим всё что write_signal_safe мог буферизировать
    mp_sink->flush_internal_signal_safe();
}
```

`peek_first_unlocked()` — новый метод (опционально расширение kit'а),
делает прямое чтение head pointer'а без acquire'а lock'а. Дальнейший
traversal — через `mp_next` указатели intrusive-списка.

#### Гарантии и не-гарантии

| Что гарантируется | Что не гарантируется |
|---|---|
| Доставка ready-queue, заполненной до момента crash'а | Что не будет race с consumer'ом, попавшим в pop в момент flush'а |
| Доставка in-flight state каждого зарегистрированного writer'а | Что crashed writer не оставил частично-записанный log_item с битым `mu_size` в header'е |
| Async-signal-safety — ни один lock не acquire'ится | Что parser sink'а увидит "красивые" границы — может потребоваться recovery-логика |
| Sink использует только raw syscall'ы | Что sink-fd / socket работоспособен в момент crash'а (TCP может быть broken) |

#### Sink-side recovery requirement

Из-за best-effort семантики sink-side parser (например, инструмент чтения
data-файла) **должен** уметь:

- Skip broken packet'ы (где `mu_size > buffer_remainder` или header
  явно поломан)
- Игнорировать unknown ID в data-payload (потенциально, если соответствующий
  REFS не доехал) — либо отображать "unknown" в UI, либо пропускать
- Не считать end-of-stream при первой ошибке — продолжать парс
  следующего валидного header'а

### 7.5 `s_p8_sink_known` представление

Per-type sparse set:

```cpp
struct s_p8_sink_known
{
    std::unordered_set<uint64_t> mo_log_descs;   // hashes
    std::unordered_set<p8_attr_id> mo_attrs;
    std::unordered_set<uint32_t> mo_threads;
    std::unordered_set<uint64_t> mo_traces;
    std::unordered_set<h_p8_mtk_id> mo_mtks;
    std::unordered_set<uint32_t> mo_modules;
};
```

Размер ограничен числом уникальных ID, никогда не превышает суммарного
количества регистраций в процессе (~единицы тысяч). Live-cost тривиален.

## 8. Стоимости и пропускная способность

Цифры приближённые, для оценки порядков.

### Producer hot-path (на emit)

В режиме A (network sink, REFS-enabled):

| Операция | Стоимость |
|----------|-----------|
| Hash + lookup в `mo_desc_map` (`unordered_map`) | ~10–20 ns (существовало) |
| Branch `if(mb_refs_enabled)` (taken) | ~1 ns (NEW) |
| Дедуп-check `mu_last_buf_seq` | **+2 ns** (NEW) |
| Append в mini-buffer (на miss, редко) | **+5–10 ns × #miss-types** (NEW) |
| Существующая сериализация args/attrs | без изменений |

Итог: на 10⁸ events/sec **+300 ms/sec = +30% one core** в худшем случае
(все ID — miss каждый буфер), реалистично ~+1–3%.

В режиме B (file sink, REFS-disabled):

| Операция | Стоимость |
|----------|-----------|
| Hash + lookup в `mo_desc_map` | ~10–20 ns (без изменений) |
| Branch `if(mb_refs_enabled)` (not-taken, predicted) | ~1 ns (NEW) |
| Дедуп-check, mini-append | **0 ns** (skip) |
| Существующая сериализация args/attrs | без изменений |

Итог: на 10⁸ events/sec **+100 ms/sec ≈ +1% one core** — близко к нулю.

### Buffer rotation

| Операция | Стоимость |
|----------|-----------|
| Финализация двух buffer header'ов | <5 ns |
| `submit_chain` под `kit::c_spin_lock` (2 push_last) | ~30–50 ns |
| reset counters + seq++ | <5 ns |

**Memcpy footer'а в хвост data-буфера отсутствует** — это устранено
переходом на пары `(mini, data)`.

### Consumer thread (на пару)

| Операция | Стоимость |
|----------|-----------|
| Parse REFS-буфера (flat-list walk) | ~100 ns |
| Per-ID lookup в `mo_sink_known` | ~10 ns × #IDs ≈ ~500 ns |
| Per-ID lookup в core registries (на miss) | ~20 ns × #miss |
| `memcpy` pre-serialized blob'ов в service-buffer | bound by sink-bandwidth |

В steady state большинство ID уже в `mo_sink_known` → consumer почти ничего не
делает кроме pass-through. Высокий приоритет нити гарантирует своевременный
drain'инг ready-queue под нагрузкой.

### Память

| | Размер |
|----------|--------|
| TLS dedup-state (per writer) | ~16 B на attr × #attrs + ~16 B на trace × #live-traces |
| Mini-buffer (live, per writer) | 1 KB (default) |
| Core data-pool | до `mz_max_memory_size` (растёт on demand) |
| Core mini-pool | ≤ ~12.5% от размера data-pool'а (тот же инвариант 1:1) |
| Core pre-serialized blobs | ~200 B × #descriptors ≈ десятки–сотни KB на процесс |

Mini-pool никогда не вырастет значительно больше data-pool'а из-за
инварианта `live(mini) ≤ live(data)`.

## 9. Этапы реализации

Рекомендованный порядок (каждый шаг закрывается независимо, с тестами):

1. **Pre-serialized blob в `s_p8_log_desc`** + сериализатор descriptor'а внутри
   `cp8_core::resolve_log_desc` (под существующим mutex'ом). Регрессионные тесты
   проверяют корректность сериализации без интеграции в стрим.
2. **Тот же blob для `s_p8_attr_desc`** в `cp8_core::attr_register`.
3a. **Рефакторинг pool'а в отдельные классы** (изоморфное преобразование без
    новой функциональности). Выделить `cp8_memory_budget` (atomic-счётчик
    shared budget'а) и `cp8_buffer_pool` (буферный pool с grow-on-demand,
    параметризованный размером буфера). Мигрировать существующий data-pool
    из `cp8_core` на эти классы. Все существующие тесты должны пройти без
    изменений. Добавить **новые** изолированные unit-тесты на
    `cp8_buffer_pool`: acquire/recycle/grow/exhaust/concurrent acquire/cleanup.

3b. **Добавление mini-pool как второго экземпляра `cp8_buffer_pool`** +
    `P8_PACKET_REFS` constant + `mz_mini_buffer_size` в `s_p8_config`.
    Поскольку pool — это уже отдельный класс, добавление второго экземпляра
    — это две строки в `cp8_core`. **Важно**: `mo_mini_pool.init(0)`, если
    sink не нуждается в REFS — pool не allocate'ит initial slab, но
    grow-on-demand сработает при первом acquire (что в режиме B никогда
    не происходит). Регрессионные тесты: оба pool'а под общим budget'ом
    корректно конкурируют за память; mini-pool в режиме B остаётся пустым.
4. **`cp8_sink_iface` минимальный + `mb_refs_enabled` в core**: только
   интерфейс `needs_refs()` и `on_descriptor_registered()`; in-memory
   test-sink, выставляющий флаг в обе позиции; `cp8_core::get_refs_enabled`;
   плагин в `resolve_log_desc` / `attr_register` для вызова hook'а. Это
   разблокирует параллельную разработку файлового пути и REFS-pipeline'а.
5. **Seq-based дедуп и mini-buffer в `cp8_tls_writer`**: поля
   `mu_current_buf_seq`, `mp_mini_buffer`, `mz_mini_used`,
   `mu_section_counts`, `mo_mini_fragments`, `mb_refs_enabled`;
   вспомогательные методы `mini_append_*`, `init_mini_buf_hdr`.
   `mb_refs_enabled` refresh'ится при start_new_bundle. Только plumbing —
   никаких клиентов.
6. **Использование mini-buffer'а в `cp8_log::send` под флагом**: дедуп
   log_desc + attr + thread, **только если `mb_refs_enabled`**.
   Регрессионные тесты проверяют (a) корректное содержимое REFS-буфера
   в режиме A на разных сценариях; (b) что в режиме B mini-buffer
   никогда не acquire'ится и hot-path схлопывается в одну branch.
7. **Переименование `release_buffer` → `submit_chain`** + введение
   ready-queue в `cp8_core` и atomic submit'ы под `kit::c_spin_lock`.
   Consumer-нить пока stub (recycle обоих буферов сразу).
8. **Consumer-нить** с высоким приоритетом и реальной отправкой в sink.
   Сначала test-sink (in-memory), затем `cp8_sink_file` режим A
   (получает REFS как все), для проверки полного flow.
9. **`cp8_sink_file` режим B (production)**: persistent service-файл,
   ротация только data-файла, `on_descriptor_registered` пишет в side-файл.
   Регрессионные тесты на: ротация data-файла не теряет service-данные;
   service-файл переоткрывается при перезапуске процесса.
10. **Reset-семантика для режима A**: `on_reset` hooks, `s_p8_sink_known`,
    `prime_known`.
11. **Exceptional flush (раздел 7.6)**: intrusive-список registered TLS
    writer'ов в `cp8_core`; методы `register_writer` / `unregister_writer`
    (вызываются из конструктора/деструктора `cp8_tls_writer`);
    `peek_first_unlocked` helper в kit (или внутренний эквивалент в p8);
    `write_signal_safe` / `flush_internal_signal_safe` в `cp8_sink_iface`
    и реализации в `cp8_sink_file`. Регрессионные тесты:
    (a) sink получает все ready-queue буферы; (b) sink получает in-flight
    TLS state нескольких живых writer'ов; (c) сценарий "одна нить
    в "псевдо-crash'е" (lock держит другая инструментированная нить) —
    crash-flush не deadlock'ается, скипает crashed writer и продолжает
    с другими (используем test-helper).
12. **`cp8_sink_net`** — отдельная задача, не входит в этот план; включает
    разработку handshake'а и стратегии переподключения.
13. **Остальные типы service-data**: thread/trace/mtk/module descriptors —
    каждый по схеме шагов 1–6 + `on_descriptor_registered` hook.

## 10. Открытые вопросы и риски

Сознательно не решено в этом документе — требуется отдельное обсуждение
перед началом соответствующих шагов:

- **Тип ready-queue**: один MPSC `kit::c_lst` под `kit::c_spin_lock`'ом
  vs. per-thread очереди + round-robin на consumer'е. На 100M+ events/sec
  и десятки producer'ов central spin-lock может стать точкой контеншена.
  Если профилирование на шаге 6 покажет проблему — мигрируем на
  per-thread MPSC. В kit lock-free очередей пока нет, может потребоваться
  добавить.
- **Backpressure**: что делает `acquire_data_buffer` / `acquire_mini_buffer`,
  если ready-queue не дренируется (медленный sink)? Сейчас возможен возврат
  `nullptr` при исчерпании `mz_max_memory_size`. Нужно решить — спин/sleep
  с timeout или сразу drop. Связано с поведением consumer'а под нагрузкой;
  поднятый приоритет нити снижает вероятность срабатывания этого пути.
- **File-sink sidecar** (`stream.svc` рядом с `stream.dat`) для persistent
  known-set между перезапусками процесса.
- **Net-sink handshake** — протокол согласования версий, опциональный
  back-channel «не присылай ID X, у меня уже есть».
- **Wraparound `mu_current_buf_seq`** при `uint32_t` — теоретически
  невозможно в realistic timescales (≈ 32 TB данных в один thread за
  ~95 дней непрерывной работы при 10⁸ events/sec). Безопаснее использовать
  `uint64_t` (+4 байта на TLS cache entry) и не закладываться на
  bias-detection при wrap'е.
- **Изменение verbosity на лету** (`p8_log_set_verbosity`) — должен
  доставляться как отдельное service-event, а не модификация blob'а.
- **Concurrent registration spike**: что если 100 потоков одновременно
  пытаются зарегистрировать descriptor под mutex'ом? Сериализуется через
  существующий mutex; pre-serialization внутри добавляет ~1 μs к
  registration time. Допустимо.

### Известные риски

| Риск | Влияние | План митигации |
|---|---|---|
| **`p8_exceptional_flush` против spin-locked нити** | Crashed нить может держать `mo_lock` или `mo_ready_lock`. Acquire этих lock'ов из signal handler'а → бесконечный spin. | **Решено** (7.6): registration TLS writers + lock-free direct read. Crash-flush ничего не acquire'ит — читает поля напрямую. Возможна частичная порча данных у crashed writer'а (partial log_item в момент crash'а) — допустимая best-effort семантика, sink-side parser должен уметь skip broken packet'ы. |
| **Partial / corrupt data в crash-dump'е** | Lock-free read TLS state без synchronization может прочитать поля в неконсистентном состоянии (например, `mz_buf_used` обновился, но в самом буфере ещё байты не записаны). | Принято как часть best-effort семантики `p8_exceptional_flush` (см. 7.6, ограничение #3). Sink-side parser должен gracefully скипнуть broken packet и продолжить с следующего валидного header'а. |
| **Sink-fd / socket мёртв в crash context'е** | TCP-соединение может быть оборвано, file fd закрыт другим signal handler'ом. `write(2)` вернёт error. | Best-effort: ошибки `write_signal_safe` игнорируются. Опциональный followup — pre-opened panic-fd, куда дампится backup-копия (out of scope этого плана). |
| **Slow consumer under load** | Если consumer-нить не успевает дренировать ready-queue, producer'ы упрутся в `acquire_*_buffer() == nullptr` и потеряют events. | Consumer стартует с максимально доступным приоритетом (SCHED_FIFO / TIME_CRITICAL). Если приоритет не выдан системой — warning в log. На системах под высокой нагрузкой может потребоваться pin'нуть consumer'а на dedicated core. |
| **Wraparound `mu_current_buf_seq`** | После ~95 дней непрерывной hot-нагрузки на одну нить seq обнулится, и стейлые `last_buf_seq == 0` entries будут восприняты как "уже в текущем буфере" — sink увидит unknown ID. | Использовать `uint64_t` для seq (исключает wrap в обозримом будущем). |
| **`mz_max_memory_size` exhaustion** | Оба pool'а делят общий бюджет; при медленном sink'е data-pool вытеснит mini-pool из общего budget'а или наоборот. | Раздельный учёт `mz_data_total_allocated` и `mz_mini_total_allocated` уже предусмотрен; рекомендация — выделять небольшую гарантированную квоту каждому pool'у при инициализации (например, 64 KB на mini-pool на старте). Уточнить при шаге 3. |
| **Mini-buf fragmentation overhead в burst'е** | На startup'е, когда регистрируются сотни callsites, mini-цепочка может стать длинной и consumer будет делать много lookups в registry. | Допустимо: это происходит один раз, на warm-up'е. После warm-up'а большинство ID уже в `mo_sink_known` → mini-цепочка короткая. |
| **Runtime switch sink'а (A↔B)** | Если sink меняется в runtime'е, pending bundle'а заполнены в одном режиме могут попасть в sink другого режима. TLS-кэшированный `mb_refs_enabled` устареет до следующего start_new_bundle. | В первой версии sink **фиксирован** при `p8_initialize` и не меняется. Будущие итерации (network ↔ file fallback) требуют отдельной проработки drain'а pending bundle'ов и инвалидации TLS-кэша всех нитей. |
| **Mismatch конфигурации (file sink + `mz_mini_buffer_size`)** | Пользователь конфигурирует файловый sink, но указывает `mz_mini_buffer_size` — память на mini-pool тратится впустую. | `cp8_core` при init вызывает `mo_mini_pool.init(0)` если `sink->needs_refs() == false`. `mz_mini_buffer_size` валидируется на корректное значение (для grow-on-demand при потенциальном будущем switch'е), но initial slab не allocate'ится. |
| **Service-файл vs ротация data-файла** | Если service-файл удалён или повреждён, а data-файлы ротируются — после перезапуска процесса часть data-файлов окажется "без service-данных". | Документировать: service-файл — критический артефакт, не подлежит отдельной очистке. Возможный followup — checksums / version markers в service-файле для обнаружения corruption. |

## 11. References

- `doc/todo.md` — оригинальные заметки заказчика, открытые вопросы.
- `api/p8_client_api.h` — описание сервисных типов: модули, нити, attrs,
  logs, metrics, traces.
- `api/p8_protocol.h` — текущий бинарный протокол.
- `engine/p8_core.{hpp,cpp}` — registries, buffer pool.
- `engine/p8_tls_writer.{hpp,cpp}` — базовый writer, сериализация attrs и
  UTF-8 строк.
- `engine/p8_log.{hpp,cpp}` — реализация `cp8_log::send`.
- `.claude/rules/p8-code-style.md` — naming conventions, используемые
  в этом документе.
- `.claude/rules/p8-prefer-kit.md` — приоритет kit над std/POSIX.
