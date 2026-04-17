---
name: p8-prefer-kit
description: Enforces preferring the in-tree `dep/kit` library over the C++ standard library, libc, and POSIX/Win32 when equivalent functionality exists. Use whenever writing, generating, refactoring, or reviewing C/C++ code in this repository - any time a list/container, mutex/lock, shared memory, platform detection, text/char portability, string duplication, visibility macro, or similar low-level primitive is about to be added.
---

# Prefer `dep/kit` over std / libc / POSIX

This project ships with an in-tree toolset library at `dep/kit` (public headers under `dep/kit/include/kit/`). When **both** kit and the standard library (or libc, or POSIX/Win32) can do the job, **pick kit**. This keeps p8 portable across Linux, macOS, Windows and avoids re-solving problems kit already solved.

## Decision Rule

1. Need a primitive? First check `dep/kit/include/kit/` for a match.
2. If kit has it → use kit. Do **not** include `<list>`, `<mutex>`, POSIX `<pthread.h>`, POSIX `<sys/mman.h>`, Win32 `CreateFileMapping`, etc. for that purpose.
3. If kit does **not** have it → use the most portable option available (prefer `<cstdint>`, `<cstddef>`, `<string_view>`, ... over libc/POSIX). Do not reach for POSIX/Win32 APIs directly unless kit already wraps them or the feature is explicitly out of kit's scope.
4. If you are unsure whether kit covers it, **grep `dep/kit/include/kit/` first**, then ask the user before falling back to std/libc.

## Kit Cheat Sheet

Use these mappings verbatim when choosing APIs.

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

Use them only when kit does **not** offer an alternative. In particular:

- Fixed-width integer types from `<cstdint>` / `<stdint.h>` (`uint32_t`, `int64_t`, ...).
- `<cstddef>` / `<stddef.h>` (`size_t`, `ptrdiff_t`).
- `<atomic>` for atomic ops not covered by `plasma.hpp`.
- `<chrono>` for time (kit does not wrap it).
- `<thread>` for thread creation (kit does not wrap thread lifecycle).
- `<string>` / `<string_view>` for general string handling (kit does not replace these).
- `<vector>` / `<array>` / `<unordered_map>` (only `std::list` has a kit replacement).
- `<cstdio>` / file I/O (kit does not wrap it).
- Plain `printf` / `memcpy` / `memset` / `strlen` from libc.
- Direct POSIX/Win32 calls only when they are **explicitly** the thing you need and kit does not expose them (e.g. `epoll`, `kqueue`, `WaitForSingleObject`).

## Include Style

- Include kit headers with the `kit/` prefix: `#include "kit/ts_helpers.h"`, `#include "kit/list.hpp"`.
- Put kit includes together, after the module's own headers and before system headers, so diffs stay small and `.clang-format` ordering is stable.

## Pre-Submit Checklist

Before finishing any new/edited C/C++ code, verify:

- [ ] No `#include <list>` — replaced by `kit/list.hpp` if a list is actually needed.
- [ ] No `std::mutex` / `std::recursive_mutex` on a documented hot path — replaced by `kit::c_spin_lock`.
- [ ] No raw `__linux__` / `__APPLE__` / `_WIN32` in p8 sources — replaced by `G_OS_LINUX` / `G_OS_DARWIN` / `G_OS_WINDOWS`.
- [ ] No raw `L"..."` / `wchar_t *` in cross-platform code — replaced by `TM("...")` / `tXCHAR`.
- [ ] No direct `shm_open` / `mmap` / `CreateFileMapping` — replaced by `c_shared` from `kit/shared_mem.hpp`.
- [ ] No bare `(void)arg;` and no compiler-specific `always_inline` — replaced by `UNUSED_ARG` / `__forceinline`.
- [ ] Any remaining std/libc/POSIX use is justified by the "When std / libc / POSIX Is Still Correct" section.

## When In Doubt

List `dep/kit/include/kit/` and open the header that sounds closest. If nothing fits, say so and ask the user before falling back to std/libc/POSIX.
