#pragma once

#include "kit/ts_helpers.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef GTX64
    #define P8_SIZE_OF_ARG(t) ((uint8_t)(((sizeof(t)) + 7u) & ~7u))
#else
    #define P8_SIZE_OF_ARG(t) ((uint8_t)(((sizeof(t)) + 3u) & ~3u))
#endif

#define P8_ARG_TYPE_UNK     0x00
#define P8_ARG_TYPE_CHAR    0x01
#define P8_ARG_TYPE_INT8    0x01
#define P8_ARG_TYPE_CHAR16  0x02
#define P8_ARG_TYPE_INT16   0x03
#define P8_ARG_TYPE_INT32   0x04
#define P8_ARG_TYPE_INT64   0x05
#define P8_ARG_TYPE_DOUBLE  0x06
#define P8_ARG_TYPE_PVOID   0x07
#define P8_ARG_TYPE_USTR16  0x08
#define P8_ARG_TYPE_STRA    0x09
#define P8_ARG_TYPE_USTR8   0x0A
#define P8_ARG_TYPE_USTR32  0x0B
#define P8_ARG_TYPE_CHAR32  0x0C
#define P8_ARG_TYPE_INTMAX  0x0D
#define P8_ARG_TYPE_LDOUBLE 0x0E

    struct s_p8_trace_arg
    {
        uint8_t mu_type; // var arg type: P8_ARG_TYPE_XXXXXX
        uint8_t mu_size; // var arg size in bytes
    };

#define P8_LOG_MAX_ARGS         32
#define P8_LOG_MIN_BUFFER_SPACE 256

    struct s_p8_log_item_hdr
    {
        uint64_t mu_hash;         // log descriptor hash (key into descriptor tree)
        uint64_t mu_timestamp_ns; // monotonic clock, nanoseconds
        uint64_t mu_trace_id;     // distributed trace correlation ID
        uint32_t mu_thread_id;    // OS thread ID
        uint16_t mu_size;         // total item size in bytes (header + args + attrs)
        uint8_t  mu_level;        // e_p8_level
        uint8_t  mu_processor;    // CPU core ID
        uint16_t mu_args_size;    // serialized variable arguments size in bytes
        uint8_t  mu_attrs_count;  // number of serialized attributes
        uint8_t  mu_padding[5];   // align to 8-byte boundary
    };

#ifdef __cplusplus
}
#endif
