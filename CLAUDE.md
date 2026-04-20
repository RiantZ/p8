# Protocol8 (p8)

C/C++ instrumentation and telemetry library: metrics, logs, distributed traces.
Early development — API defined in `api/p8_client_api.h`, most implementations are stubs.

## Build & Test

```bash
# Configure + build (auto-detects platform)
python scripts/cmake_build.py --build

# Clean build
python scripts/cmake_build.py --clean --build

# Run tests (macOS example; substitute _Build_wnd or _Build_lnx on other platforms)
cd _Build_mac && ctest
```

| Platform | Build directory | Preset    |
|----------|-----------------|-----------|
| Windows  | `_Build_wnd/`   | `windows` |
| macOS    | `_Build_mac/`   | `macos`   |
| Linux    | `_Build_lnx/`   | `linux`   |

Custom presets: create `CMakeUserPresets.json` (git-ignored) inheriting from a base preset.

## Code Formatting

- Config: `.clang-format` at project root.
- Pre-commit hook runs `python scripts/code_format.py --check --staged` automatically.
- Always format before committing.

## Project Structure

```
api/              Public C headers (p8_client_api.h, p8_config.h), C language by default, C++ only if supported by compiler
engine/           Implementation (.cpp)
dep/              Dependencies via CMake FetchContent (kit, nlohmann_json, GoogleTest)
tests/regression/ GoogleTest-based tests
examples/         Sample config.json
scripts/          Build helpers (cmake_build.py, code_format.py, git hooks)
doc/              Design docs, code style reference
```

## Dependencies

- **kit** (`dep/kit`) — in-tree toolset library. Prefer kit over std/libc/POSIX when both can do the job. Details in `.claude/rules/p8-prefer-kit.md`.
- **nlohmann_json** v3.12.0 — JSON config parsing.
- **GoogleTest** v1.17.0 — test framework.

All fetched automatically via CMake FetchContent.

## Error Handling (C / extern "C" functions)

All C/C++ functions that acquire releasable resources use the **single-exit goto cleanup** pattern:
- All locals declared at the top with safe defaults (`nullptr`, `0`, `false`).
- One `lbl_exit:` label per function.
- Every failure logs and does `goto lbl_exit`.
- Cleanup section: idempotent, guarded release of each resource in reverse acquisition order.

Reference implementation: `engine/p8_config.cpp::p8_config_read_file`.
Full pattern specification: `/p8-error-cleanup` skill (`.claude/skills/p8-error-cleanup/SKILL.md`).

## Code Style

Lowercase snake_case everywhere, mandatory type/visibility prefixes on all variables and types.
Full rules loaded automatically from `.claude/rules/p8-code-style.md` when working with C/C++ files.
Canonical reference: `doc/code_style.md`.
