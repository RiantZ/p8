#pragma once

#include "kit/ts_helpers.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Read a file fully into a malloc'd, NUL-terminated buffer.
    ///
    /// The path argument uses `tXCHAR` from `kit/ts_helpers.h`:
    ///   * POSIX (Linux, macOS): `char`    - UTF-8 bytes.
    ///   * Windows:              `wchar_t` - UTF-16 code units (native Windows path encoding).
    /// Callers can pass argv / GetCommandLineW() entries verbatim, no conversion needed.
    ///
    /// @param ip_path [in]  path to the file (never NULL)
    /// @param op_size [out] optional; receives the byte count read from disk
    ///                      (NOT counting the terminating NUL). Pass NULL if unused.
    /// @return pointer to a newly allocated buffer of `(*op_size) + 1` bytes, with
    ///         a trailing `'\0'` byte for convenience when the content is textual.
    ///         Returns NULL on any failure (open / seek / alloc / short read).
    ///         Caller must release the buffer with `free()`.
    char *p8_config_read_file(const tXCHAR *ip_path, size_t *op_size);

#ifdef __cplusplus
}
#endif
