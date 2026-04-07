# P7 library
## Building

### Prerequisites

- CMake 3.20+
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

Build output is placed in a platform-specific directory at the project root:

| Platform | Directory     |
|----------|---------------|
| Windows  | `_Build_wnd/` |
| macOS    | `_Build_mac/` |
| Linux    | `_Build_lnx/` |

On Windows the script auto-detects available Visual Studio generators and prompts to choose one if several are found.
