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
#define P8_ARGS_TYPE_COUNT  0x0F

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
