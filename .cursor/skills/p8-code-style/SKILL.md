---
name: p8-code-style
description: Enforces the p8 project C/C++ naming and code style (lowercase snake_case, mandatory type/visibility prefixes, file naming, define casing) defined in doc/code_style.md. Use whenever writing, generating, refactoring, renaming, or reviewing C, C++, or header code in this repository, including any new file, function, class, struct, enum, typedef, variable, parameter, or macro.
---

# p8 Code Style

The full reference is `doc/code_style.md`. This skill is the operational summary the agent MUST follow when producing any C/C++ code in this repo. Defaults are non-negotiable unless the user explicitly overrides them.

## Golden Rules

1. All identifiers (files, types, functions, variables) are **lowercase** with `_` as the only word separator. No camelCase, no PascalCase.
2. **Every** type and **every** variable carries a prefix. No prefix = wrong.
3. Preprocessor `#define` names are the **only** ALL_CAPS identifiers.
4. C functions are prefixed with the module name (e.g. `p8_`). C++ class methods start with a verb (`do_`, `get_`, `set_`, `is_`, `create_`, ...).
5. Format with the repo's `.clang-format` before finishing.

## File Naming

- `my_file_for_something.cpp`, `my_file_for_something.hpp`, `my_file_for_something.h`
- Never `MyFile.cpp`, never `myFile.cpp`, never `my-file.cpp`.

## Type Prefixes (used when declaring a type)

Single-letter prefix + `_` + lowercase name.

| Prefix | Meaning              | Example                   |
|--------|----------------------|---------------------------|
| `i`    | signed integer       | `typedef int32_t i_count;`|
| `u`    | unsigned integer     | `uint32_t mu_val;`        |
| `x`    | utf char             | `x_char ix_letter;`       |
| `z`    | size_t               | `size_t lz_len;`          |
| `b`    | bool                 | `bool ib_flag;`           |
| `c`    | class                | `class c_my_class`        |
| `d`    | double               | `double md_value;`        |
| `f`    | float                | `float lf_ratio;`         |
| `p`    | pointer              | `void *mp_data;`          |
| `r`    | reference            | `c_obj &or_out;`          |
| `v`    | r-value reference    | `c_obj &&iv_tmp;`         |
| `o`    | object (instance)    | `c_my_class lo_inst;`     |
| `s`    | structure            | `struct s_my_struct`      |
| `e`    | enum type / values   | `enum class e_state { e_idle, e_run };` |
| `t`    | template             | `template<typename t_x>`  |
| `l`    | function ptr / lambda| `typedef int (*l_cb)(int);`|
| `h`    | handle / custom type | `typedef int32_t h_p8_id;`|

## Variable Prefixes (access/visibility, then type)

`<access><type>_name`

Access part:

- Function parameters: `i` (in), `io` (in/out), `o` (out, input ignored)
- Scope: `g` (global / file-static), `l` (local)
- Members: `m` (class/struct member)

Examples:
- `ip_ctx`     — input pointer
- `iou_count`  — in/out unsigned
- `op_result`  — output pointer
- `gs_config`  — global struct instance
- `lz_size`    — local size_t
- `mp_data`    — member pointer
- `ml_timer_value` — member function-pointer/lambda
- `mu_val1`    — member unsigned

### `o` Disambiguation

- `o` as the **first** letter = output parameter (access).
- `o` as the **second** letter = object (type).
- Compound prefix order is therefore `<access><type>`, e.g. `oc_data` = output class, `lo_inst` = local object, `moc_child` is **wrong** (members can't be outputs).

## Functions

- C function: `<module>_<verb>_<noun>` → `p8_register_current_thread`, `p8_mtk_emit`.
- C++ method: starts with a verb in lowercase snake_case → `do_something`, `get_size`, `set_name`, `is_ready`.
- Constructors/destructors follow class name (`c_my_class::c_my_class(...)`).

## Preprocessor Defines

ALL_CAPS_WITH_UNDERSCORES. This is the **only** exception to the lowercase rule.

```cpp
#define P8_MAX_CHANNELS 64
#define MY_DEFINE       0
```

## Canonical Example

```cpp
#include "my_header_file.h"

#define MY_DEFINE 0

enum class e_my_enum
{
    e_val_1,
    e_val_2,
    e_vals_count
};

struct s_my_struct
{
    uint32_t mu_val1;
    int32_t  mi_val2;
};

static const s_my_struct gs_my_global_val = {0, 0};

class c_my_class
{
    void *mp_data = nullptr;
public:
    explicit c_my_class(uint32_t iu_init_var);
    bool do_process(const s_my_struct *ip_in, s_my_struct *op_out);
};

void my_function(c_my_class *&or_object)
{
    or_object = new c_my_class(10);
}
```

## Pre-Submit Checklist (run mentally before finishing any code change)

- [ ] All new files use `lower_snake_case.{c,cpp,h,hpp}`.
- [ ] Every new type has a single-letter type prefix (`c_`, `s_`, `e_`, `h_`, `l_`, `t_`).
- [ ] Every new variable has `<access><type>_` prefix; no bare names.
- [ ] Function parameters use `i`/`io`/`o` access correctly.
- [ ] No camelCase or PascalCase anywhere except inside untouched third-party code.
- [ ] `#define`s are ALL_CAPS; no other identifier is.
- [ ] C functions begin with the module prefix; C++ methods begin with a verb.
- [ ] Code is formatted with the project `.clang-format`.

## When in Doubt

Open `doc/code_style.md` and follow it literally. If a rule appears to conflict with surrounding existing code, match the existing file's style locally and flag the inconsistency to the user instead of silently mixing styles.
