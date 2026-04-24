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

    enum e_p8_arg_type
    {
        P8_ARG_TYPE_UNK     = 0x00,
        P8_ARG_TYPE_CHAR    = 0x01,
        P8_ARG_TYPE_INT8    = 0x01,
        P8_ARG_TYPE_CHAR16  = 0x02,
        P8_ARG_TYPE_INT16   = 0x03,
        P8_ARG_TYPE_INT32   = 0x04,
        P8_ARG_TYPE_INT64   = 0x05,
        P8_ARG_TYPE_DOUBLE  = 0x06,
        P8_ARG_TYPE_PVOID   = 0x07,
        P8_ARG_TYPE_USTR16  = 0x08,
        P8_ARG_TYPE_STRA    = 0x09,
        P8_ARG_TYPE_USTR8   = 0x0A,
        P8_ARG_TYPE_USTR32  = 0x0B,
        P8_ARG_TYPE_CHAR32  = 0x0C,
        P8_ARG_TYPE_INTMAX  = 0x0D,
        P8_ARG_TYPE_LDOUBLE = 0x0E,
        P8_ARGS_TYPE_COUNT
    };

    PRAGMA_PACK_ENTER(2)

    struct sP7Trace_Arg
    {
        uint8_t mu_type;
        uint8_t mu_size;
    } ATTR_PACK(2);

    PRAGMA_PACK_EXIT()

    size_t log_parse_format_string(struct sP7Trace_Arg *op_args, size_t iz_args_max, const char *ip_format);

#ifdef __cplusplus
}
#endif
