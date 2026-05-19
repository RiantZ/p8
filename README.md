# Protocol8
## Building

### Prerequisites

- CMake 3.20+ (tested up to 4.0)
- Python 3
- C++20 compatible compiler (MSVC, GCC, Clang)

### Build

All builds go through the helper script `scripts/cmake_build.py`:

```bash
# Configure only
python scripts/cmake_build.py

# Configure + build
python scripts/cmake_build.py --build

# Configure + build using presets
python scripts/cmake_build.py --preset macos --build

# Clean build (wipe build directory, then configure + build)
python scripts/cmake_build.py --clean --build
```

The script uses `CMakePresets.json` to select the right preset for each platform automatically.

Build output is placed in a platform-specific directory at the project root:

| Platform | Directory     | Preset    |
|----------|---------------|-----------|
| Windows  | `_Build_wnd/` | `windows` |
| macOS    | `_Build_mac/` | `macos`   |
| Linux    | `_Build_lnx/` | `linux`   |

You can also invoke CMake directly with a preset: `cmake --preset macos`.

### Formatting
The pre-commit hooks required code to be formatted using Clang format, to format code please use command:
```bash
python scripts/code_format.py --format --staged
```

### Testing

```bash
# Run regression tests (macOS example; substitute _Build_wnd or _Build_lnx on other platforms)
cd _Build_mac && ctest

# Run performance tests (disabled by default, must be requested explicitly)
cd _Build_mac && ./tests/regression/P8_RegressionTests --gtest_filter="c_log_perf_test.*" --gtest_also_run_disabled_tests
```

### Custom presets

To override settings without changing tracked files, create `CMakeUserPresets.json` (git-ignored) in the project root. A preset defined there can inherit from any base preset in `CMakePresets.json`.

Example — use a local kit checkout instead of fetching from GitHub:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "macos-local-kit",
      "inherits": "macos",
      "cacheVariables": {
        "FETCHCONTENT_SOURCE_DIR_KIT": "${sourceDir}/../kit"
      }
    }
  ]
}
```

Then build with:

```bash
python scripts/cmake_build.py --preset macos-local-kit --build
```
