#include "p8_core.hpp"
#include "p8_config_keys.hpp"
#include "p8_hash.hpp"
#include "p8_log.hpp"
#include "p8_tls_writer.hpp"

#include "kit/endian.hpp"
#include "kit/shared_mem.hpp"
#include "kit/system.hpp"
#include "kit/thread.hpp"
#include "kit/time.hpp"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <strings.h>
#include <chrono>
#include <new>
#include <string>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Protocol structures must be 8-byte aligned so that uint64_t members can be accessed without unaligned loads when
// items are packed sequentially in shared-memory buffers.  A misaligned 64-bit read is either a fault (strict-align
// architectures) or a silent performance penalty (x86), so we enforce the invariant at compile time.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static_assert(sizeof(struct s_p8_hdr) % 8 == 0, "s_p8_hdr size must be a multiple of 8 bytes");
static_assert(sizeof(struct s_p8_data_buf_hdr) % 8 == 0, "s_p8_data_buf_hdr size must be a multiple of 8 bytes");
static_assert(sizeof(struct s_p8_log_item_dat) % 8 == 0, "s_p8_log_item_hdr size must be a multiple of 8 bytes");

static_assert(alignof(struct s_p8_hdr) >= 8, "s_p8_hdr must have at least 8-byte alignment");
static_assert(alignof(struct s_p8_data_buf_hdr) >= 8, "s_p8_data_buf_hdr must have at least 8-byte alignment");
static_assert(alignof(struct s_p8_log_item_dat) >= 8, "s_p8_log_item_hdr must have at least 8-byte alignment");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Wire-format layout contract.  These structures are written to / read from shared memory and transmitted over the
// network between hosts that may build this header with different compilers (MSVC / GCC / Clang) and bitness (the
// alignment of uint64_t is 4 on the i386 System V ABI but 8 elsewhere).  The exact size and field offsets below are
// the binary contract: any field reorder, insert, remove or type change that alters the on-wire layout must break the
// build here, not silently corrupt data on the receiving end.  Endianness is handled separately (header is always
// little-endian); these asserts only freeze the byte layout.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// s_p8_hdr
static_assert(sizeof(struct s_p8_hdr) == 568, "s_p8_hdr wire size changed");
static_assert(offsetof(struct s_p8_hdr, mu_packet_type) == 0, "s_p8_hdr::mu_packet_type offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_protocol_version) == 1, "s_p8_hdr::mu_protocol_version offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_is_big_endian) == 2, "s_p8_hdr::mu_is_big_endian offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_padding) == 3, "s_p8_hdr::mu_padding offset changed");
static_assert(offsetof(struct s_p8_hdr, mi_utc_sec_offset) == 4, "s_p8_hdr::mi_utc_sec_offset offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_size) == 8, "s_p8_hdr::mu_size offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_process_id) == 12, "s_p8_hdr::mu_process_id offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_system_time) == 16, "s_p8_hdr::mu_system_time offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_hires_tick) == 24, "s_p8_hdr::mu_hires_tick offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_hires_freq_numer) == 32, "s_p8_hdr::mu_hires_freq_numer offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_hires_freq_denom) == 40, "s_p8_hdr::mu_hires_freq_denom offset changed");
static_assert(offsetof(struct s_p8_hdr, mp_process_name) == 48, "s_p8_hdr::mp_process_name offset changed");
static_assert(offsetof(struct s_p8_hdr, mp_host_name) == 304, "s_p8_hdr::mp_host_name offset changed");
static_assert(offsetof(struct s_p8_hdr, mu_hash) == 560, "s_p8_hdr::mu_hash offset changed");

// s_p8_svc_hdr
static_assert(sizeof(struct s_p8_svc_hdr) == 4, "s_p8_svc_hdr wire size changed");
static_assert(offsetof(struct s_p8_svc_hdr, mu_type) == 0, "s_p8_svc_hdr::mu_type offset changed");
static_assert(offsetof(struct s_p8_svc_hdr, mu_flags) == 1, "s_p8_svc_hdr::mu_flags offset changed");
static_assert(offsetof(struct s_p8_svc_hdr, mu_size) == 2, "s_p8_svc_hdr::mu_size offset changed");

// s_p8_data_buf_hdr
static_assert(sizeof(struct s_p8_data_buf_hdr) == 24, "s_p8_data_buf_hdr wire size changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_packet_type) == 0,
              "s_p8_data_buf_hdr::mu_packet_type offset changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_flags) == 1, "s_p8_data_buf_hdr::mu_flags offset changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_size) == 2, "s_p8_data_buf_hdr::mu_size offset changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_thread_id) == 4, "s_p8_data_buf_hdr::mu_thread_id offset changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_start_time) == 8,
              "s_p8_data_buf_hdr::mu_start_time offset changed");
static_assert(offsetof(struct s_p8_data_buf_hdr, mu_stop_time) == 16,
              "s_p8_data_buf_hdr::mu_stop_time offset changed");

// s_p8_log_varg
static_assert(sizeof(struct s_p8_log_varg) == 2, "s_p8_log_varg wire size changed");
static_assert(offsetof(struct s_p8_log_varg, mu_type) == 0, "s_p8_log_varg::mu_type offset changed");
static_assert(offsetof(struct s_p8_log_varg, mu_size) == 1, "s_p8_log_varg::mu_size offset changed");

// s_p8_log_item_svc
static_assert(sizeof(struct s_p8_log_item_svc) == 24, "s_p8_log_item_svc wire size changed");
static_assert(offsetof(struct s_p8_log_item_svc, ms_hdr) == 0, "s_p8_log_item_svc::ms_hdr offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_line) == 4, "s_p8_log_item_svc::mu_line offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_hash) == 8, "s_p8_log_item_svc::mu_hash offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_format_len) == 16,
              "s_p8_log_item_svc::mu_format_len offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_file_len) == 18, "s_p8_log_item_svc::mu_file_len offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_function_len) == 20,
              "s_p8_log_item_svc::mu_function_len offset changed");
static_assert(offsetof(struct s_p8_log_item_svc, mu_args_count) == 22,
              "s_p8_log_item_svc::mu_args_count offset changed");

// s_p8_log_item_dat
static_assert(sizeof(struct s_p8_log_item_dat) == 40, "s_p8_log_item_dat wire size changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_hash) == 0, "s_p8_log_item_dat::mu_hash offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_timestamp) == 8, "s_p8_log_item_dat::mu_timestamp offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_trace_id) == 16, "s_p8_log_item_dat::mu_trace_id offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_thread_id) == 24,
              "s_p8_log_item_dat::mu_thread_id offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_level) == 28, "s_p8_log_item_dat::mu_level offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_processor) == 29,
              "s_p8_log_item_dat::mu_processor offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_args_size) == 30,
              "s_p8_log_item_dat::mu_args_size offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_size) == 32, "s_p8_log_item_dat::mu_size offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_attrs_count) == 36,
              "s_p8_log_item_dat::mu_attrs_count offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_padding_size) == 38,
              "s_p8_log_item_dat::mu_padding_size offset changed");
static_assert(offsetof(struct s_p8_log_item_dat, mu_flags) == 39, "s_p8_log_item_dat::mu_flags offset changed");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// singleton state
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static cp8_core             *gp_instance   = nullptr;
static c_shared::h_shared    gp_shm_handle = nullptr;
static const tXCHAR         *gp_shm_name   = TM("p8");
static std::atomic<uint32_t> gu_instance_count { 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool parse_size(const char *ip_str, size_t &oz_result)
{
    char   la_buf[128];
    size_t lz_dst = 0;

    for(size_t lz_i = 0; ip_str[lz_i] != '\0' && lz_dst < sizeof(la_buf) - 1; ++lz_i)
    {
        if(ip_str[lz_i] != ' ' && ip_str[lz_i] != '\t')
        {
            la_buf[lz_dst++] = ip_str[lz_i];
        }
    }
    la_buf[lz_dst] = '\0';

    if(strcasecmp(la_buf, "infinite") == 0)
    {
        oz_result = std::numeric_limits<size_t>::max();
        return true;
    }

    char              *lp_end = nullptr;
    unsigned long long lu_val = std::strtoull(la_buf, &lp_end, 10);

    if(lp_end == la_buf)
    {
        return false;
    }

    if(*lp_end == '\0')
    {
        oz_result = static_cast<size_t>(lu_val);
        return true;
    }

    if(strcasecmp(lp_end, "KB") == 0 || strcasecmp(lp_end, "Ki") == 0)
    {
        oz_result = static_cast<size_t>(lu_val * 1024);
        return true;
    }

    if(strcasecmp(lp_end, "MB") == 0 || strcasecmp(lp_end, "Mi") == 0)
    {
        oz_result = static_cast<size_t>(lu_val * 1024 * 1024);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cp8_core implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core::cp8_core(const struct s_p8_config *ip_config)
{
    gu_instance_count.fetch_add(1, std::memory_order_relaxed);

    if(!ip_config->mp_json_config)
    {
        std::fprintf(stderr, "cp8_core: mp_json_config is NULL\n");
        return;
    }

    nlohmann::json lo_json;

    try
    {
        lo_json = nlohmann::json::parse(ip_config->mp_json_config,
                                        nullptr, // callback
                                        true,    // allow_exceptions
                                        true);   // ignore_comments
    }
    catch(const nlohmann::json::parse_error &ir_err)
    {
        std::fprintf(stderr, "cp8_core: JSON parse error: %s\n", ir_err.what());
        return;
    }
    catch(const std::exception &ir_err)
    {
        std::fprintf(stderr, "cp8_core: unexpected error: %s\n", ir_err.what());
        return;
    }

    if(lo_json.contains(P8_CFG_KEY_SINK))
    {
        // TODO: configure sync mode (file.bin, network.tcp)
    }

    if(lo_json.contains(P8_CFG_KEY_DESTINATION))
    {
        // TODO: configure destination path or network endpoint
    }

    std::string ls_max_mem;
    std::string ls_init_mem;

    if(lo_json.contains(P8_CFG_KEY_MAX_MEMORY_SIZE))
    {
        ls_max_mem = lo_json[P8_CFG_KEY_MAX_MEMORY_SIZE].get<std::string>();
    }

    if(lo_json.contains(P8_CFG_KEY_INITIAL_MEMORY_SIZE))
    {
        ls_init_mem = lo_json[P8_CFG_KEY_INITIAL_MEMORY_SIZE].get<std::string>();
    }

    if(!init_buffer_pool(ls_max_mem.empty() ? nullptr : ls_max_mem.c_str(),
                         ls_init_mem.empty() ? nullptr : ls_init_mem.c_str()))
    {
        return;
    }

    init_header(ip_config);

    mb_initialized = true;

    start_worker();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core::~cp8_core()
{
    stop_worker();

    for(auto &lo_pair : mo_log_descs)
    {
        delete lo_pair.second;
    }
    mo_log_descs.clear();

    for(s_p8_attr_desc *lp_desc : mo_attr_descs)
    {
        std::free(lp_desc->mp_name);
        delete lp_desc;
    }
    mo_attr_descs.clear();
    mo_attr_name_map.clear();

    delete mp_data_pool;
    mp_data_pool = nullptr;
    mp_memory_budget.reset();

    gu_instance_count.fetch_sub(1, std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::addref()
{
    mu_ref_count.fetch_add(1, std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::release()
{
    if(mu_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        delete this;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::init_buffer_pool(const char *ip_max_memory_size, const char *ip_initial_memory_size)
{
    size_t lz_max_memory_size     = std::numeric_limits<size_t>::max();
    size_t lz_initial_memory_size = 1024 * 1024;

    if(ip_max_memory_size)
    {
        if(!parse_size(ip_max_memory_size, lz_max_memory_size))
        {
            std::fprintf(stderr, "cp8_core::init_buffer_pool: invalid max_memory_size value\n");
            return false;
        }
    }

    if(ip_initial_memory_size)
    {
        if(!parse_size(ip_initial_memory_size, lz_initial_memory_size))
        {
            std::fprintf(stderr, "cp8_core::init_buffer_pool: invalid initial_memory_size value\n");
            return false;
        }
    }

    if(lz_initial_memory_size > lz_max_memory_size)
    {
        lz_initial_memory_size = lz_max_memory_size;
    }

    try
    {
        mp_memory_budget = std::make_shared<cp8_memory_budget>(lz_max_memory_size);
    }
    catch(const std::bad_alloc &)
    {
        std::fprintf(stderr, "cp8_core::init_buffer_pool: budget allocation failed\n");
        return false;
    }

    mp_data_pool = new(std::nothrow) cp8_buffer_pool(mz_data_buffer_size, mp_memory_budget);
    if(!mp_data_pool)
    {
        std::fprintf(stderr, "cp8_core::init_buffer_pool: data pool allocation failed\n");
        mp_memory_budget.reset();
        return false;
    }

    mp_data_pool->init(lz_initial_memory_size / mz_data_buffer_size);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::init_header(const struct s_p8_config *ip_config)
{
    std::memset(&mo_hdr, 0, sizeof(mo_hdr));

    mo_hdr.mu_packet_type      = P8_PACKET_MAIN;
    mo_hdr.mu_protocol_version = P8_PROTOCOL_VERSION;

    uint32_t lu_endian         = 0x1;
    mo_hdr.mu_is_big_endian    = (*reinterpret_cast<uint8_t *>(&lu_endian) == 0) ? 1 : 0;

    mo_hdr.mi_utc_sec_offset   = static_cast<int8_t>(kit::get_utc_offset_seconds() / 3600);
    mo_hdr.mu_size             = static_cast<uint32_t>(sizeof(struct s_p8_hdr));
    mo_hdr.mu_process_id       = kit::get_process_id();
    mo_hdr.mu_system_time      = kit::get_system_time();

    // TODO: use this callbacks by logs, traces, metrics
    if(ip_config->ml_timer_value && ip_config->ml_timer_frequency)
    {
        mo_hdr.mu_hires_tick       = ip_config->ml_timer_value(ip_config->mp_ctx_timer);
        uint64_t lu_freq           = ip_config->ml_timer_frequency(ip_config->mp_ctx_timer);
        mo_hdr.mu_hires_freq_numer = 1000000000ULL;
        mo_hdr.mu_hires_freq_denom = lu_freq;
    }
    else
    {
        mo_hdr.mu_hires_tick = kit::get_hires_ticks();
        kit::get_hires_ticks_freq(mo_hdr.mu_hires_freq_numer, mo_hdr.mu_hires_freq_denom);
    }

    kit::get_process_name(mo_hdr.mp_process_name, sizeof(mo_hdr.mp_process_name));
    kit::get_host_name(mo_hdr.mp_host_name, sizeof(mo_hdr.mp_host_name));

    if(mo_hdr.mu_is_big_endian)
    {
        mo_hdr.mu_size             = kit::bswap32(mo_hdr.mu_size);
        mo_hdr.mi_utc_sec_offset   = kit::bswap32(mo_hdr.mi_utc_sec_offset);
        mo_hdr.mu_process_id       = kit::bswap32(mo_hdr.mu_process_id);
        mo_hdr.mu_system_time      = kit::bswap64(mo_hdr.mu_system_time);
        mo_hdr.mu_hires_tick       = kit::bswap64(mo_hdr.mu_hires_tick);
        mo_hdr.mu_hires_freq_numer = kit::bswap64(mo_hdr.mu_hires_freq_numer);
        mo_hdr.mu_hires_freq_denom = kit::bswap64(mo_hdr.mu_hires_freq_denom);
    }

    mo_hdr.mu_hash = XXH3_64bits(&mo_hdr, offsetof(struct s_p8_hdr, mu_hash));

    if(mo_hdr.mu_is_big_endian)
    {
        mo_hdr.mu_hash = kit::bswap64(mo_hdr.mu_hash);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::get_initialized() const
{
    return mb_initialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::exceptional_flush()
{
    if(!mb_initialized)
    {
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::start_worker()
{
    const c_event::e_type la_types[] = { c_event::e_single_auto, c_event::e_multi };

    if(!mo_worker_event.init(2, la_types))
    {
        std::fprintf(stderr, "cp8_core::start_worker: event init failed\n");
        return;
    }

    mo_worker_thread  = std::thread(&cp8_core::worker_main, this);
    mb_worker_running = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::stop_worker()
{
    if(!mb_worker_running)
    {
        return;
    }

    mo_worker_event.set(mu_event_stop);

    if(mo_worker_thread.joinable())
    {
        mo_worker_thread.join();
    }

    mb_worker_running = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::worker_main()
{
    // best-effort: raising priority typically needs elevated privileges, failure is non-fatal
    kit::set_thread_priority(kit::e_tp_time_critical);

    for(;;)
    {
        uint32_t lu_signal = mo_worker_event.wait(P8_CORE_THREAD_TIMEOUT_MS);

        if(lu_signal == mu_event_stop)
        {
            break;
        }

        // lu_signal == mu_event_wake (external event) or c_event::timeout
        do_iteration();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::notify()
{
    mo_worker_event.set(mu_event_wake);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::do_iteration()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::register_writer(cp8_tls_writer *ip_writer)
{
    std::lock_guard<kit::c_spin_lock> lo_guard(mo_writers_lock);
    ip_writer->mp_prev_writer = nullptr;
    ip_writer->mp_next_writer = mp_writers_head;
    if(mp_writers_head)
    {
        mp_writers_head->mp_prev_writer = ip_writer;
    }
    mp_writers_head = ip_writer;
    ++mz_writers_count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::unregister_writer(cp8_tls_writer *ip_writer)
{
    std::lock_guard<kit::c_spin_lock> lo_guard(mo_writers_lock);
    if(ip_writer->mp_prev_writer)
    {
        ip_writer->mp_prev_writer->mp_next_writer = ip_writer->mp_next_writer;
    }
    else
    {
        mp_writers_head = ip_writer->mp_next_writer;
    }
    if(ip_writer->mp_next_writer)
    {
        ip_writer->mp_next_writer->mp_prev_writer = ip_writer->mp_prev_writer;
    }
    ip_writer->mp_next_writer = nullptr;
    ip_writer->mp_prev_writer = nullptr;
    --mz_writers_count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::register_current_thread(const char *)
{
    if(!mb_initialized)
    {
        return false;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::unregister_current_thread()
{
    if(!mb_initialized)
    {
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
p8_attr_id cp8_core::attr_register(const char *ip_name, enum e_p8_attr_type ie_type)
{
    if(!mb_initialized)
    {
        return P8_ATTR_ERROR_NOT_INITIALIZED;
    }

    if(!ip_name || ip_name[0] == '\0')
    {
        return P8_ATTR_ERROR_INVALID_NAME;
    }

    std::lock_guard<std::mutex> lo_lock(mo_attr_mutex);

    auto lo_it = mo_attr_name_map.find(ip_name);
    if(lo_it != mo_attr_name_map.end())
    {
        p8_attr_id li_existing = lo_it->second;
        if(mo_attr_descs[static_cast<size_t>(li_existing)]->me_type != ie_type)
        {
            return P8_ATTR_ERROR_TYPE_MISMATCH;
        }
        return li_existing;
    }

    s_p8_attr_desc *lp_desc = new(std::nothrow) s_p8_attr_desc;
    if(!lp_desc)
    {
        return P8_ATTR_ERROR_ALLOC_FAILED;
    }

    lp_desc->mp_name = strdup(ip_name);
    if(!lp_desc->mp_name)
    {
        delete lp_desc;
        return P8_ATTR_ERROR_ALLOC_FAILED;
    }

    lp_desc->mi_id   = static_cast<p8_attr_id>(mo_attr_descs.size());
    lp_desc->me_type = ie_type;

    mo_attr_descs.push_back(lp_desc);
    mo_attr_name_map[ip_name] = lp_desc->mi_id;

    return lp_desc->mi_id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
p8_attr_id cp8_core::attr_get(const char *ip_name) const
{
    if(!mb_initialized)
    {
        return P8_ATTR_ERROR_NOT_INITIALIZED;
    }

    if(!ip_name || ip_name[0] == '\0')
    {
        return P8_ATTR_ERROR_NOT_FOUND;
    }

    std::lock_guard<std::mutex> lo_lock(mo_attr_mutex);

    auto lo_it = mo_attr_name_map.find(ip_name);
    if(lo_it != mo_attr_name_map.end())
    {
        return lo_it->second;
    }

    return P8_ATTR_ERROR_NOT_FOUND;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::sync_attr_cache(std::vector<const s_p8_attr_desc *> &io_cache)
{
    std::lock_guard<std::mutex> lo_lock(mo_attr_mutex);

    size_t lz_old = io_cache.size();
    size_t lz_new = mo_attr_descs.size();

    if(lz_old >= lz_new)
    {
        return;
    }

    io_cache.resize(lz_new);
    for(size_t lz_i = lz_old; lz_i < lz_new; ++lz_i)
    {
        io_cache[lz_i] = mo_attr_descs[lz_i];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t *cp8_core::acquire_buffer()
{
    if(!mb_initialized)
    {
        return nullptr;
    }

    return mp_data_pool->acquire();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::release_buffer(uint8_t *ip_buffer)
{
    if(!ip_buffer || !mb_initialized)
    {
        return;
    }

#ifdef P8_TESTING
    if(mb_capture_enabled)
    {
        std::lock_guard<std::mutex> lo_lock(mo_capture_mutex);
        mo_captured_buffers.emplace_back(ip_buffer, ip_buffer + mz_data_buffer_size);
    }
#endif

    mp_data_pool->recycle(ip_buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::release_buffers(kit::c_lst<uint8_t *> &io_buffers)
{
    if(!mb_initialized)
    {
        return;
    }

    if(0 == io_buffers.size())
    {
        return;
    }

    io_buffers.clear(
        [this](uint8_t *ip_buf)
        {
#ifdef P8_TESTING
            if(mb_capture_enabled)
            {
                std::lock_guard<std::mutex> lo_lock(mo_capture_mutex);
                mo_captured_buffers.emplace_back(ip_buf, ip_buf + mz_data_buffer_size);
            }
#endif
            mp_data_pool->recycle(ip_buf);
        });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t cp8_core::get_buffer_size()
{
    return mz_data_buffer_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct s_p8_log_desc *cp8_core::resolve_log_desc(uint64_t    iu_hash,
                                                 const char *ip_file,
                                                 uint32_t    iu_line,
                                                 const char *ip_function,
                                                 const char *ip_format)
{
    std::lock_guard<std::mutex> lo_lock(mo_log_desc_mutex);

    auto lo_it = mo_log_descs.find(iu_hash);
    if(lo_it != mo_log_descs.end())
    {
        return lo_it->second;
    }

    struct s_p8_log_desc *lp_desc = new(std::nothrow) struct s_p8_log_desc;
    if(!lp_desc)
    {
        return nullptr;
    }

    lp_desc->mu_hash            = iu_hash;
    lp_desc->mp_format          = ip_format;
    lp_desc->mp_file            = ip_file;
    lp_desc->mp_function        = ip_function;
    lp_desc->mu_line            = iu_line;
    lp_desc->mz_args_count      = cp8_log::parse_format_string(lp_desc->ma_args, P8_LOG_MAX_ARGS, ip_format);

    lp_desc->mz_fixed_args_size = 0;
    for(size_t lz_i = 0; lz_i < lp_desc->mz_args_count; lz_i++)
    {
        uint8_t lu_type = lp_desc->ma_args[lz_i].mu_type;
        if(lu_type != P8_ARG_TYPE_USTR8 && lu_type != P8_ARG_TYPE_USTR16 && lu_type != P8_ARG_TYPE_USTR32
           && lu_type != P8_ARG_TYPE_STRA)
        {
            lp_desc->mz_fixed_args_size += lp_desc->ma_args[lz_i].mu_size;
        }
    }

    mo_log_descs[iu_hash] = lp_desc;
    return lp_desc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core *cp8_core::get_global_core(uint32_t iu_timeoutms)
{
    if(gp_instance)
    {
        return gp_instance;
    }

    uint32_t lu_elapsed = 0;

    while(lu_elapsed < iu_timeoutms)
    {
        cp8_core       *lp_found = nullptr;
        c_shared::h_sem lp_sem   = nullptr;

        if(c_shared::lock(gp_shm_name, lp_sem, 10) == c_shared::e_ok)
        {
            c_shared::read(gp_shm_name, &lp_found, sizeof(lp_found));
            c_shared::unlock(lp_sem);
        }

        if(lp_found)
        {
            gp_instance = lp_found;
            return gp_instance;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lu_elapsed += 10;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// extern "C" API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_initialize(const struct s_p8_config *ip_config)
    {
        cp8_core          *lp_new        = nullptr;
        cp8_core          *lp_existing   = nullptr;
        c_shared::h_shared lp_shm_handle = nullptr;
        c_shared::h_sem    lp_sem        = nullptr;
        bool               lb_error      = false;

        // 1. already initialized in this DSO
        if(gp_instance)
        {
            return true;
        }

        // 2. instance published by another DSO via shared memory
        if(c_shared::read(gp_shm_name, &lp_existing, sizeof(lp_existing)) && lp_existing)
        {
            gp_instance = lp_existing;
            return true;
        }

        // 3. validate config
        if(!ip_config)
        {
            std::fprintf(stderr, "p8_initialize: NULL config\n");
            lb_error = true;
            goto lbl_exit;
        }

        // 4. create core instance (constructor parses JSON, sets mb_initialized on failure)
        lp_new = new(std::nothrow) cp8_core(ip_config);
        if(!lp_new)
        {
            std::fprintf(stderr, "p8_initialize: allocation failed\n");
            lb_error = true;
            goto lbl_exit;
        }

        if(!lp_new->get_initialized())
        {
            lb_error = true;
            goto lbl_exit;
        }

        // 5. atomically publish pointer via shared memory (O_CREAT | O_EXCL)
        if(c_shared::create(&lp_shm_handle, gp_shm_name, reinterpret_cast<const uint8_t *>(&lp_new), sizeof(lp_new)))
        {
            gp_shm_handle = lp_shm_handle;
            gp_instance   = lp_new;
            lp_new        = nullptr;
        }
        else
        {
            // 6. another DSO won the race — read its pointer
            delete lp_new;
            lp_new      = nullptr;
            lp_existing = nullptr;

            if(c_shared::lock(gp_shm_name, lp_sem, 5000) == c_shared::e_ok)
            {
                c_shared::read(gp_shm_name, &lp_existing, sizeof(lp_existing));
                c_shared::unlock(lp_sem);
            }

            if(lp_existing)
            {
                gp_instance = lp_existing;
            }
            else
            {
                std::fprintf(stderr, "p8_initialize: failed to obtain instance from shared memory\n");
                lb_error = true;
                goto lbl_exit;
            }
        }

lbl_exit:
        if(lb_error && lp_new)
        {
            delete lp_new;
            lp_new = nullptr;
        }

        return !lb_error;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_get_initialized()
    {
        return gp_instance && gp_instance->get_initialized();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_exceptional_flush()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_register_current_thread(const char *)
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_unregister_current_thread()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    p8_attr_id p8_attr_register(const char *ip_name, enum e_p8_attr_type ie_type)
    {
        if(!gp_instance)
        {
            return P8_ATTR_ERROR_NOT_INITIALIZED;
        }
        return gp_instance->attr_register(ip_name, ie_type);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    p8_attr_id p8_attr_get(const char *ip_name)
    {
        if(!gp_instance)
        {
            return P8_ATTR_ERROR_NOT_FOUND;
        }
        return gp_instance->attr_get(ip_name);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_release()
    {
        if(!gp_instance)
        {
            return;
        }

        if(gp_shm_handle)
        {
            c_shared::close(gp_shm_handle);
            gp_shm_handle = nullptr;
        }
        c_shared::unlink(gp_shm_name);

        gp_instance->release();
        gp_instance = nullptr;
    }

} // extern "C"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef P8_TESTING

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_reset()
{
    p8_release();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t p8_test_get_instance_count()
{
    return gu_instance_count.load(std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_buffer_size()
{
    return gp_instance ? gp_instance->mz_data_buffer_size : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_free_buffers_count()
{
    if(!gp_instance || !gp_instance->mp_data_pool)
    {
        return 0;
    }
    return gp_instance->mp_data_pool->get_free_count();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_total_allocated()
{
    if(!gp_instance || !gp_instance->mp_data_pool)
    {
        return 0;
    }
    return gp_instance->mp_data_pool->get_total_allocated();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_all_buffers_count()
{
    if(!gp_instance || !gp_instance->mp_data_pool)
    {
        return 0;
    }
    return gp_instance->mp_data_pool->get_total_allocated() / gp_instance->mz_data_buffer_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t *p8_test_acquire_buffer()
{
    return gp_instance ? gp_instance->acquire_buffer() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_release_buffer(uint8_t *ip_buffer)
{
    if(gp_instance)
    {
        gp_instance->release_buffer(ip_buffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_enable_buffer_capture()
{
    if(gp_instance)
    {
        std::lock_guard<std::mutex> lo_lock(gp_instance->mo_capture_mutex);
        gp_instance->mo_captured_buffers.clear();
        gp_instance->mb_capture_enabled = true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_disable_buffer_capture()
{
    if(gp_instance)
    {
        std::lock_guard<std::mutex> lo_lock(gp_instance->mo_capture_mutex);
        gp_instance->mb_capture_enabled = false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_captured_count()
{
    if(!gp_instance)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lo_lock(gp_instance->mo_capture_mutex);
    return gp_instance->mo_captured_buffers.size();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::vector<std::vector<uint8_t>> &p8_test_get_captured_buffers()
{
    static const std::vector<std::vector<uint8_t>> lo_empty;
    return gp_instance ? gp_instance->mo_captured_buffers : lo_empty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_clear_captured_buffers()
{
    if(gp_instance)
    {
        std::lock_guard<std::mutex> lo_lock(gp_instance->mo_capture_mutex);
        gp_instance->mo_captured_buffers.clear();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t cp8_core::get_writer_count()
{
    std::lock_guard<kit::c_spin_lock> lo_guard(mo_writers_lock);
    return mz_writers_count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_tls_writer *cp8_core::get_writers_head()
{
    return mp_writers_head;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_writer_count()
{
    return gp_instance ? gp_instance->get_writer_count() : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_tls_writer *p8_test_get_writers_head()
{
    return gp_instance ? gp_instance->get_writers_head() : nullptr;
}

#endif // P8_TESTING
