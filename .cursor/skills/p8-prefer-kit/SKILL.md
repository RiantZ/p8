---
name: p8-prefer-kit
description: Enforces preferring the in-tree `dep/kit` library over the C++ standard library, libc, and POSIX/Win32 when equivalent functionality exists. Use whenever writing, generating, refactoring, or reviewing C/C++ code in this repository - any time a container, lock, shared memory, platform detection, text/char portability, string duplication, visibility macro, packing/alignment attribute, or similar low-level primitive is about to be added.
---

# Prefer `dep/kit` over std / libc / POSIX

This project ships with an in-tree toolset library at `dep/kit` (public headers under `dep/kit/include/kit/`). When **both** kit and the standard library (or libc, or POSIX/Win32) can do the job, **pick kit**. This keeps p8 portable across Linux, macOS, Windows and avoids re-solving problems kit already solved.

> Kit is evolving. New headers and modules are added over time. The source of truth is the `dep/kit/include/kit/` directory and `dep/kit/README.md`, **not** the cheat sheet below. Always rediscover before deciding.

## Decision Rule (run for every C/C++ addition)

1. **Discover.** Before picking any non-trivial primitive, list the current kit surface:
   - Public headers: everything under `dep/kit/include/kit/` (including newly added files).
   - Module table: the "Modules" section of `dep/kit/README.md`.
   - If the task is niche, `grep` kit headers for relevant type/function names (e.g. `ring`, `pool`, `thread`, `channel`, `atomic`, ...).
2. **Match.** If any kit module covers the task - use kit. Do not include the std/POSIX/Win32 equivalent.
3. **Fallback.** If kit does **not** cover it - use the most portable standard option (prefer `<cstdint>`, `<cstddef>`, `<string_view>`, `<atomic>`, ... over libc or POSIX). Only touch POSIX/Win32 directly when kit does not wrap it **and** the feature is clearly out of kit's current scope.
4. **Unsure.** If you cannot tell whether kit covers it after step 1, say so explicitly and ask the user before falling back to std/libc/POSIX.

## Kit Cheat Sheet (non-authoritative snapshot)

This table reflects the kit API at the time this skill was written. Treat it as a **hint**, not a closed list. If step 1 of the Decision Rule reveals a kit header not listed here, prefer that header over std/libc/POSIX anyway and mention the gap to the user so the skill can be updated.

| Task                                   | Use (kit)                          | Do NOT use                                                |
|----------------------------------------|------------------------------------|-----------------------------------------------------------|
| Doubly-linked list                     | `kit::c_lst<T>` (`kit/list.hpp`)   | `std::list`, hand-rolled linked lists                     |
| Fast / recursive lock on hot paths     | `kit::c_spin_lock` (`kit/spin_lock.hpp`) | `std::mutex`, `std::recursive_mutex`, `pthread_mutex_t`, `CRITICAL_SECTION` for hot paths |
| CPU spin / pause primitive             | `kit/plasma.hpp`                   | Inline `__asm__("pause")`, `_mm_pause()` scattered in code|
| Named shared memory + named semaphore  | `c_shared` (`kit/shared_mem.hpp`)  | `shm_open` + `mmap` + `sem_open`, `CreateFileMapping` + `CreateMutex` |
| OS detection at compile time           | `G_OS_LINUX` / `G_OS_DARWIN` / `G_OS_WINDOWS` (`kit/ts_helpers.h`) | Raw `__linux__` / `__APPLE__` / `_WIN32` checks in p8 code |
| 32/64-bit arch detection               | `GTX32` / `GTX64` (`kit/ts_helpers.h`) | Raw `__x86_64__` / `_WIN64` checks                        |
| Portable char type (UTF-8 on POSIX, `wchar_t` on Windows) | `XCHAR`, `tXCHAR` (`kit/ts_helpers.h`, `kit/types.h`) | Bare `char *` / `wchar_t *` in cross-platform paths       |
| String literal that auto-picks char width | `TM("...")` / `TMM(...)` (`kit/ts_helpers.h`) | `L"..."` / `"..."` duplicated per platform                |
| Duplicate a platform string            | `pstr_dup` / `pstr_free_dup` (`kit/types.h`) | `strdup`, `_wcsdup`, manual `malloc`+`memcpy`             |
| Packing / alignment attribute          | `PRAGMA_PACK_ENTER` / `ATTR_PACK` / `ATTR_ALIGN` (`kit/ts_helpers.h`) | Raw `__attribute__((packed))`, `#pragma pack` scattered    |
| Force-inline                           | `__forceinline` (defined in `kit/ts_helpers.h` on POSIX) | Compiler-specific `__attribute__((always_inline))` directly |
| Mark argument unused                   | `UNUSED_ARG(x)` (`kit/ts_helpers.h`) | `(void)x;` scattered, `[[maybe_unused]]` in C code        |
| Export symbol for shared lib           | `P7_EXPORT` / `KIT_API` (`kit/ts_helpers.h`, `kit/export.h`) | Raw `__declspec(dllexport)` / `__attribute__((visibility))` |
| Unique identifier generator            | `MAKE_UNIQUE(x)` (`kit/ts_helpers.h`) | Hand-rolled `__COUNTER__` glue                            |
| Stringify macro arg                    | `TOSTR(x)` / `STR_HELPER(x)` (`kit/ts_helpers.h`) | Local `#` / `##` helpers                                  |

## When std / libc / POSIX Is Still Correct

Use them **only after** step 1 confirms kit does not cover the task. Categories that are *currently* outside kit's scope and thus typically fine:

- Fixed-width integer types: `<cstdint>` / `<stdint.h>` (`uint32_t`, `int64_t`, ...).
- Basic size/offset types: `<cstddef>` / `<stddef.h>` (`size_t`, `ptrdiff_t`).
- Atomic operations not covered by `plasma.hpp`: `<atomic>`.
- Time: `<chrono>`.
- Thread lifecycle: `<thread>`.
- General strings: `<string>`, `<string_view>`.
- Dynamic arrays / maps / sets: `<vector>`, `<array>`, `<unordered_map>`, `<unordered_set>`, ...
- File I/O: `<cstdio>`, `<fstream>`.
- Basic mem/str libc: `printf`, `memcpy`, `memset`, `strlen`, ...
- Platform-specific syscalls not wrapped by kit: `epoll`, `kqueue`, `WaitForSingleObject`, `inotify`, ...

**This list is tentative, not frozen.** If a future kit release introduces, for example, `kit/thread_pool.hpp`, `kit/ring_buffer.hpp`, or `kit/file.hpp`, the corresponding bullet above stops applying. The Decision Rule (step 1) takes precedence over this list.

## Include Style

- Include kit headers with the `kit/` prefix: `#include "kit/ts_helpers.h"`, `#include "kit/list.hpp"`.
- Group kit includes together, after the module's own headers and before system headers, so diffs stay small and `.clang-format` ordering is stable.

## Pre-Submit Checklist

Before finishing any new/edited C/C++ code, verify:

- [ ] Step 1 of the Decision Rule was actually performed (directory listed / README module table checked) for each non-trivial primitive added.
- [ ] Any std/libc/POSIX primitive chosen has a matching "currently out of kit scope" justification; if the current scope has changed since this skill was last updated, the kit header was used instead.
- [ ] No `#include <list>` unless you verified no kit list fits.
- [ ] No `std::mutex` / `std::recursive_mutex` on a hot path where `kit::c_spin_lock` would fit.
- [ ] No raw `__linux__` / `__APPLE__` / `_WIN32` in p8 sources - use `G_OS_LINUX` / `G_OS_DARWIN` / `G_OS_WINDOWS`.
- [ ] No raw `L"..."` / `wchar_t *` in cross-platform code - use `TM("...")` / `tXCHAR`.
- [ ] No direct `shm_open` / `mmap` / `CreateFileMapping` - use `c_shared`.
- [ ] No bare `(void)arg;` and no compiler-specific `always_inline` - use `UNUSED_ARG` / `__forceinline`.

## Keeping This Skill In Sync With Kit

Kit evolves independently of this skill. When you discover that:

- a kit header exists for a task that the Cheat Sheet does not mention, **or**
- the "When std / libc / POSIX Is Still Correct" list green-lights a task that kit now covers, **or**
- a kit API in the Cheat Sheet has been renamed, removed, or deprecated,

do not silently follow the outdated skill text. Use the current kit surface, and **tell the user** (one line is enough, e.g. *"kit now provides X, the `p8-prefer-kit` skill Cheat Sheet should be updated"*). The Decision Rule always wins over the tables.
