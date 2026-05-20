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

#define P8_PACKET_MAX_SIZE      (1 << 16) // 64KB max page

#define P8_PACKET_MAIN          0
#define P8_PACKET_LOGS          1
#define P8_PACKET_TRACES        2
#define P8_PACKET_METRICS       3
#define P8_PACKET_SERVICE       4

#define P8_PROTOCOL_VERSION     1

#define P8_PROCESS_NAME_MAX_LEN 256
#define P8_HOST_NAME_MAX_LEN    256

    // Base hello message from P8, the first packet to be sent
    // WARNING: the header is always in little-endian!
    struct s_p8_hdr
    {
        uint8_t  mu_packet_type;      // P8_PACKET_MAIN
        uint8_t  mu_protocol_version; // P8_PROTOCOL_VERSION
        uint8_t  mu_is_big_endian;    // 1 if host is big-endian, 0 - 0 otherwise
        uint8_t  mu_padding;          // padding
        int32_t  mi_utc_sec_offset;   // utc offset in seconds
        uint32_t mu_size;             // alligned on 8 bytes!
        uint32_t mu_process_id;       // host process ID
        uint64_t mu_system_time;      // system time, 100-nanosecond intervals since January 1, 1601 (UTC)
        uint64_t mu_hires_tick;       // high resolution monotonic time value
        uint64_t mu_hires_freq_numer; // high resolution monotonic time frequency numerator
        uint64_t mu_hires_freq_denom; // high resolution monotonic time frequency denominator
        char     mp_process_name[P8_PROCESS_NAME_MAX_LEN];
        char     mp_host_name[P8_HOST_NAME_MAX_LEN];
        uint64_t mu_hash; // header control hash
    };

#define P8_DATA_FLAG_FRAGMENT   (1 << 0) // buffer tail data continues in the next buffer for the same thread
#define P8_DATA_FLAG_RESERVED_1 (1 << 1)
#define P8_DATA_FLAG_RESERVED_2 (1 << 2)
#define P8_DATA_FLAG_RESERVED_3 (1 << 3)
#define P8_DATA_FLAG_RESERVED_4 (1 << 4)
#define P8_DATA_FLAG_RESERVED_5 (1 << 5)
#define P8_DATA_FLAG_RESERVED_6 (1 << 6)
#define P8_DATA_FLAG_RESERVED_7 (1 << 7)

    struct s_p8_data_buf_hdr
    {
        uint8_t  mu_packet_type; // P8_PACKET_XXXX
        uint8_t  mu_flags;       // P8_DATA_FLAG_XXX
        uint16_t mu_size;        // size in bytes
        uint32_t mu_thread_id;   // thread ID which emits data
        uint64_t mu_start_time;  // first timestamp
        uint64_t mu_stop_time;   // last timestamp
    };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LOGS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

    struct s_p8_log_varg
    {
        uint8_t mu_type; // var arg type: P8_ARG_TYPE_XXXXXX
        uint8_t mu_size; // var arg size in bytes
    };

#define P8_LOG_MAX_ARGS         32
#define P8_LOG_MIN_BUFFER_SPACE 32
    struct s_p8_log_item_hdr
    {
        uint64_t mu_hash;      // log descriptor hash (key into descriptor tree)
        uint64_t mu_timestamp; // monotonic clock timestamp
        uint64_t mu_trace_id;  // distributed trace correlation ID

        // 64 bits
        uint32_t mu_thread_id; // OS thread ID
        uint8_t  mu_level;     // e_p8_level
        uint8_t  mu_processor; // CPU core ID
        uint16_t mu_args_size; // serialized variable arguments size in bytes

        // 64 bits
        uint32_t mu_size;         // total item size in bytes (header + args + attrs)
        uint16_t mu_attrs_count;  // number of serialized attributes
        uint8_t  mu_padding_size; // log item data is alligned in 8 bytes, the value is padding length in bytes
        uint8_t  mu_flags;        // todo
        //* Serialized Variable agruments [int32][int64][double][length in bytes (16b) + string data] ...
        //* Serialized attributes [p8_attr_id + data][...]
        //* padding to aling size on 8 bytes boundary
    };

#ifdef __cplusplus
}
#endif
