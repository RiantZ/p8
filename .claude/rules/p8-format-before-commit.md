---
paths:
  - "**/*.cpp"
  - "**/*.c"
  - "**/*.h"
  - "**/*.hpp"
---

# Format before commit

Before committing changes that include C/C++ files, **always** run the project formatter:

```bash
python scripts/code_format.py --format --staged
```

Then re-stage the formatted files before committing.

Do **not** use `clang-format` directly — the script handles file discovery, exclusions, and batch processing.
