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
