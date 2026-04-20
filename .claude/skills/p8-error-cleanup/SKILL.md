---
name: p8-error-cleanup
description: >-
  Enforces the p8 single-exit error-handling pattern for C and C-style C++
  functions - all locals declared up front, one `lbl_exit:` label, every failure
  (of any kind - syscall, library call, allocation, validation, invariant, logic
  check) logs and does `goto lbl_exit`, and every resource acquired earlier is
  released once in an idempotent cleanup section. Use whenever writing or
  refactoring any C or `extern "C"` function that acquires at least one
  releasable resource (file handle, malloc, mutex, shared mem, socket, thread)
  and can fail at any step after that acquisition.
  Reference implementation: engine/p8_config.cpp::p8_config_read_file.
---

# p8 Single-Exit Error Cleanup (`goto lbl_exit`)

In C and in `extern "C"` C++ we do not have destructors / RAII to lean on, so cleanup is the programmer's job. The p8 convention for any non-trivial function that acquires several resources is a **single-exit, goto-based cleanup** pattern. Reference implementation: `engine/p8_config.cpp::p8_config_read_file`.

Do **not** invent ad-hoc cleanup schemes: nested `if`s with repeated `fclose`/`free`, multiple `return` statements each rolling back its own subset, or "cleanup only this resource in this branch". They are bug magnets in p8 — use the pattern below.

## When This Applies

The trigger is a single question:

> *Is there any code path in this function where a failure can happen **after** a releasable resource was acquired, such that on that failure the resource must be released before returning?*

If the answer is yes — **use this pattern**. Do not argue about "only one malloc" or "only two branches"; the moment a second error-capable step lives after an acquisition, ad-hoc cleanup starts leaking or double-freeing the first time someone adds a branch.

Concretely, use the pattern in any C or `extern "C"` function where **any** of these holds:

- Acquires 1+ resource that needs explicit release (`fopen`, `malloc`/`calloc`, `shm_open` + `mmap`, `sem_open`, `pthread_create`, `socket`, `CreateFile`, `lock`, ref-count bump, ...) AND has at least one subsequent step that can fail.
- Acquires 2+ resources that need explicit release, regardless of failure-point count.
- Returns a freshly-allocated resource to the caller, with `nullptr` / `false` on failure.

The **kinds of failure** you must handle uniformly through `goto lbl_exit` include, but are not limited to:

- Return codes from syscalls / libc / POSIX / Win32 (`fseek`, `fread`, `mmap`, `read`, `write`, `fstat`, ...).
- Return codes from kit / project APIs (anything returning `bool`, error enum, negative value, or `nullptr`).
- Allocation failures (`malloc` / `calloc` / `realloc` / `new(std::nothrow)` / kit allocators returning null).
- Input validation (null pointer, out-of-range size, wrong enum value, wrong state/phase).
- Invariant / sanity checks (`fread` returned fewer bytes than requested, parsed value out of expected set, CRC mismatch, capacity exceeded).
- Business-logic errors (not-found, not-authorized, already-exists, wrong version).
- C++ exceptions from callees — caught at the boundary of the function and converted to `goto lbl_exit`.
- Cancellation / shutdown signals observed mid-work.

If the function has acquired *anything* and any of the above happens, the answer is always the same: **log, `goto lbl_exit`, let the cleanup section release what was acquired so far**.

For trivial functions that acquire no resources (pure computation) or that acquire exactly one resource and have zero failure points after the acquisition, a direct `return` is still fine.

## The Canonical Shape

```cpp
char *p8_module_do_something(const tXCHAR *ip_path, size_t *op_size)
{
    // 1. EVERY local is declared at the top with a safe default.
    //    Required: C++ forbids `goto` jumping over a variable's initialization.
    FILE  *lp_file   = nullptr;
    char  *lp_buf    = nullptr;
    char  *lp_result = nullptr;
    size_t lz_size   = 0;

    // 2. Normalize output params up front so the caller sees a well-defined
    //    state even on the earliest error.
    if(op_size)
    {
        *op_size = 0;
    }

    // 3. Every validation / acquisition: log on failure, goto lbl_exit.
    if(!ip_path)
    {
        std::fprintf(stderr, "p8_module_do_something: NULL path\n");
        goto lbl_exit;
    }

    lp_file = std::fopen(ip_path, "rb");
    if(!lp_file)
    {
        std::fprintf(stderr, "p8_module_do_something: failed to open file\n");
        goto lbl_exit;
    }

    lp_buf = static_cast<char *>(std::malloc(lz_size + 1));
    if(!lp_buf)
    {
        std::fprintf(stderr, "p8_module_do_something: malloc of %zu bytes failed\n", lz_size + 1);
        goto lbl_exit;
    }

    // ... real work ...

    // 4. Success epilogue: transfer ownership of each returned resource
    //    out of its "owned locally" slot. The cleanup section below then
    //    becomes a no-op for that resource.
    lp_result = lp_buf;
    lp_buf    = nullptr;

    if(op_size)
    {
        *op_size = lz_size;
    }

lbl_exit:
    // 5. Idempotent cleanup. Each `if(lp_xxx)` guard makes the section safe
    //    regardless of how far we got before jumping here.
    if(lp_file)
    {
        std::fclose(lp_file);
        lp_file = nullptr;
    }
    if(lp_buf)
    {
        std::free(lp_buf);
        lp_buf = nullptr;
    }
    return lp_result;
}
```

## Hard Rules

1. **One label, one name.** Exactly one cleanup label per function, named `lbl_exit:`. No `lbl_fail`, `cleanup`, `done`, `out`. Consistency is the point.
2. **All locals declared at the top, with safe defaults.** `nullptr` for pointers, `0` for integers, `false` for bools, invalid-sentinel for handles (e.g. `-1`, `P8_MODULE_INVALID_ID`). This is not style — in C++ `goto` may not cross a variable's initialization, so late-declared locals break the pattern outright. It is also what makes the cleanup section safe (see rule 4).
3. **Never `goto` upward or sideways.** Only forward, only to `lbl_exit`. No nested labels, no `goto` out of one cleanup block into another.
4. **Cleanup is idempotent AND total.** Every resource the function *can* acquire has exactly one matching release in `lbl_exit`, guarded by `if(lp_xxx)` (or an equivalent acquired-check such as `if(lh_handle != INVALID_HANDLE)`). The section must work correctly whether the jump came from the very first validation (nothing acquired) or the very last line before success (everything acquired). The default-initialized values from rule 2 are what make each guard reliably falsy before the corresponding acquisition.
5. **Every error path goes through `lbl_exit`, not just the "obvious" ones.** Any operation that can fail — syscall, library call, allocation, validation, invariant check, enum/range check, exception caught at boundary — routes its failure via `goto lbl_exit`. No silent ignoring of return codes. No `return` sneaking out past the cleanup. If you write a call that returns a status, you either use the status or document (in a comment) why ignoring it is safe.
6. **Log on every error path, exactly once.** The error site knows *what* failed — log there, not in `lbl_exit`. Format: `"<function_name>: <what failed>\n"`, optionally with useful context (size, errno, index, returned value). See "Logging" below.
7. **No `return` before `lbl_exit`** inside the function body. Every exit path, including success, flows through the label. One-liner early returns in *pure input validation before any acquisition* are tolerated only when the function has not yet touched any resource and the declaration section is done — prefer `goto lbl_exit` even there for uniformity.
8. **Order of releases in `lbl_exit` is the reverse of acquisitions** when one resource depends on another (e.g. `munmap` before `close` of the backing fd, `pthread_join` before freeing thread args). When there is no dependency, keep a stable, reproducible order (top of file to bottom, matching declaration order) so that diffs stay small.
9. **Do not mix this pattern with C++ exceptions** inside the same function. If a callee can throw, either wrap it in `try { ... } catch(...) { log; goto lbl_exit; }` at the call site, or rewrite the function in RAII style (and then the whole `goto` scaffold goes away). Letting an exception fly past a partially-acquired state inside a `goto`-managed function leaks every resource already acquired.
10. **Nullify every pointer / reset every handle immediately after releasing it.** After `delete p`, `std::free(p)`, `std::fclose(f)`, `::munmap(p, …)`, `::close(fd)`, or any other release call, the very next statement must set the variable back to its "unacquired" default (`nullptr`, `-1`, etc.). This prevents double-free / double-close bugs that surface when future edits add a second `goto lbl_exit` after the cleanup, or when shutdown logic calls the cleanup section more than once. The rule applies both in the `lbl_exit` cleanup section and in the normal function body (e.g. mid-body `delete` before a race-retry path).

## Logging

p8 will eventually route through `P8ERR0(mp_module, ...)` (see `api/p8_client_api.h`), but `p8_log_sent` is currently a stub. Until the backend is wired up, follow the existing convention in `engine/p8_core.cpp` / `engine/p8_config.cpp`:

```cpp
std::fprintf(stderr, "p8_module_do_something: short read %zu of %zu bytes\n", lz_read, lz_size);
```

Rules:

- Prefix every message with the function name (`<function>: ...`). This survives grep and makes multi-thread logs usable.
- One `fprintf` per `goto lbl_exit` site. Don't batch. Don't log again inside `lbl_exit`.
- Include the values that made the check fail: requested size, returned count, `errno`, offending index. "fseek failed" is almost useless; "fseek to end failed, errno=%d" is actionable.
- When the p8 log backend goes live, these lines get mechanically rewritten to `P8ERR0(mp_module, "...", ...);` — keep message format consistent so that migration is a `sed`.

## Returning a Resource: Ownership Transfer (Preferred)

When the function returns a freshly-allocated resource to the caller, use the **ownership transfer** idiom:

```cpp
char *lp_buf    = nullptr;   // "owner" slot; cleaned up in lbl_exit
char *lp_result = nullptr;   // "handed to caller" slot; returned, never freed here

// ... on the success path, just before lbl_exit:
lp_result = lp_buf;
lp_buf    = nullptr;         // <-- key line: disown the buffer

lbl_exit:
    if(lp_buf)
    {
        std::free(lp_buf);
        lp_buf = nullptr;
    }
    return lp_result;
```

Why this is the default in p8:

- The cleanup section stays dumb and symmetric: one `if(lp_xxx) free(lp_xxx);` per resource, no branching on success/error.
- Correctness is preserved even if you forget to set an error flag, or later add a new error path that forgets to set one. The invariant "`lp_buf != nullptr` means this function still owns the buffer" is self-enforcing.
- Adding a second returned resource scales linearly: add `lp_result2` + `lp_buf2 = nullptr;` twin lines, done.

## Returning a Resource: Error Flag (Acceptable Alternative)

For the specific case of **exactly one** allocated resource returned to the caller, the error-flag variant used in `p8_config_read_file` is also acceptable:

```cpp
char *lp_buf    = nullptr;
bool  lb_error  = false;

// ... on every error site:
lb_error = true;
goto lbl_exit;

lbl_exit:
    if(lb_error && lp_buf)
    {
        std::free(lp_buf);
        lp_buf = nullptr;    // so we return nullptr to the caller
    }
    return lp_buf;
```

Use this variant **only** when all of:

- Exactly one heap-allocated resource is returned.
- The returned pointer itself is the "did we succeed" signal (`nullptr` ⇔ failure).
- You prefer the explicit `lb_error` flag for readability over the ownership-transfer indirection.

The ownership-transfer variant is still preferred for new code because it scales to multiple returned resources and makes it structurally impossible to forget updating the error flag.

## Multi-Resource Example

When the function owns several resources and several of them must be released even on very late failures, each resource gets its own default-initialized slot, its own acquisition site, and its own guarded release — in reverse order:

```cpp
bool p8_module_do_something(const tXCHAR *ip_path, size_t iz_len, void **op_view)
{
    bool     lb_ok       = false;
    int      li_fd       = -1;
    void    *lp_map      = nullptr;
    c_thing *lp_thing    = nullptr;

    if(op_view)
    {
        *op_view = nullptr;
    }

    if(!ip_path || !op_view || iz_len == 0)
    {
        std::fprintf(stderr, "p8_module_do_something: bad args\n");
        goto lbl_exit;
    }

    li_fd = ::open(ip_path, O_RDONLY);
    if(li_fd < 0)
    {
        std::fprintf(stderr, "p8_module_do_something: open failed, errno=%d\n", errno);
        goto lbl_exit;
    }

    lp_map = ::mmap(nullptr, iz_len, PROT_READ, MAP_PRIVATE, li_fd, 0);
    if(lp_map == MAP_FAILED)
    {
        lp_map = nullptr; // normalize sentinel so the cleanup guard is reliable
        std::fprintf(stderr, "p8_module_do_something: mmap failed, errno=%d\n", errno);
        goto lbl_exit;
    }

    lp_thing = new (std::nothrow) c_thing(lp_map, iz_len);
    if(!lp_thing)
    {
        std::fprintf(stderr, "p8_module_do_something: c_thing alloc failed\n");
        goto lbl_exit;
    }

    if(!lp_thing->do_validate())
    {
        std::fprintf(stderr, "p8_module_do_something: validate failed\n");
        goto lbl_exit;
    }

    // success: hand the view out, disown the local slots we don't want freed.
    *op_view = lp_map;
    lp_map   = nullptr;
    lb_ok    = true;

lbl_exit:
    // reverse-of-acquisition order; each guard relies on rule 2 defaults.
    if(lp_thing)
    {
        delete lp_thing;
        lp_thing = nullptr;
    }
    if(lp_map)
    {
        ::munmap(lp_map, iz_len);
        lp_map = nullptr;
    }
    if(li_fd >= 0)
    {
        ::close(li_fd);
        li_fd = -1;
    }
    return lb_ok;
}
```

Note how every post-acquisition failure — `mmap`, `new`, `do_validate()` — goes through the same label and relies on the same guards. Adding a 4th resource or a 5th validation step is a local edit: declare the slot at the top, add an `if(...) goto lbl_exit;` block, add one guarded release in the cleanup section in reverse order. Nothing else changes.

## Anti-Patterns (Do Not Do This)

```cpp
// BAD: late declaration, goto crosses initialization -> won't compile.
if(!ip_path) goto lbl_exit;
FILE *lp_file = std::fopen(ip_path, "rb");   // <-- goto jumps over this
if(!lp_file) goto lbl_exit;
```

```cpp
// BAD: per-branch cleanup. Adding a new step means editing every earlier branch.
if(std::fseek(lp_file, 0, SEEK_END) != 0)
{
    std::fclose(lp_file);
    return nullptr;
}
if((li_pos = std::ftell(lp_file)) < 0)
{
    std::fclose(lp_file);
    return nullptr;
}
```

```cpp
// BAD: returns freed pointer. Classic consequence of skipping the ownership
// transfer and "just freeing on error" with a single return variable.
lbl_exit:
    std::free(lp_buf);   // unconditional free
    return lp_buf;       // <-- use-after-free / returns dangling pointer
```

```cpp
// BAD: log duplicated in lbl_exit. The error site already logged the cause.
lbl_exit:
    if(lb_error)
    {
        std::fprintf(stderr, "p8_module_do_something: failed\n"); // noise
    }
```

```cpp
// BAD: multiple labels, non-idempotent cleanup.
lbl_free_buf:
    std::free(lp_buf);
lbl_close_file:
    std::fclose(lp_file);
    return nullptr;
```

## Pre-Submit Checklist

Before finishing a function that uses this pattern, verify:

- [ ] Exactly one `lbl_exit:` label; no other labels in the function.
- [ ] All locals declared and default-initialized at the top of the function body (`nullptr` / `0` / `false` / invalid sentinel).
- [ ] **Every** call that can fail — syscall, libc, POSIX/Win32, kit, project API, allocation, `new`, validation, invariant — has its return value checked. No silently ignored error codes.
- [ ] **Every** such failure routes through `goto lbl_exit;`. No silent `return`, no half-baked per-branch cleanup.
- [ ] Every error site logs once with `<function>: <what failed>` plus useful context (value, size, errno) before the `goto`.
- [ ] Every resource acquisition has exactly one matching guarded release in `lbl_exit`, in the reverse acquisition order when there is a dependency between resources.
- [ ] Each release in `lbl_exit` is wrapped in `if(lp_xxx)` (or an equivalent acquired-check) so that early-jump paths — where some resources were never acquired — are safe.
- [ ] Sentinel values returned by failed acquisitions (`MAP_FAILED`, `INVALID_HANDLE_VALUE`, negative fd, ...) are normalized to the same "unacquired" default used in the declaration, so the guarded release does not fire on them.
- [ ] Every release (`delete`, `std::free`, `std::fclose`, `::munmap`, `::close`, ...) is immediately followed by resetting the variable to its "unacquired" default (`nullptr`, `-1`, etc.) — both in `lbl_exit` and in the function body.
- [ ] If the function returns an owned resource: ownership transfer via `lp_result = lp_buf; lp_buf = nullptr;` is in place, OR the single-resource `lb_error` variant is used correctly.
- [ ] No `return` statement inside the function body other than the terminal one after `lbl_exit:`.
- [ ] Any callee that can throw a C++ exception is wrapped in `try { ... } catch(...) { log; goto lbl_exit; }` at the call site, so the cleanup section is not bypassed by a fly-through exception.
- [ ] Mentally walk every `goto lbl_exit;` in the function and confirm that exactly the resources acquired before that point are released by the guards, and nothing more, nothing less.

## Reference

See `engine/p8_config.cpp::p8_config_read_file` for the canonical implementation of the error-flag variant, and the "Ownership Transfer" section above for the preferred default. When in doubt, read the reference file before writing new code.
