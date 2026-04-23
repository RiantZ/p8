#pragma once

#define XXH_STATIC_LINKING_ONLY
#define XXH_INLINE_ALL
#include "xxhash.h"

#include <string.h>

#define P8_XXH3_LOG_HASH(ip_file_name, ii_file_line, ou_hash)                                                         \
    do                                                                                                                \
    {                                                                                                                 \
        XXH3_state_t lo_xxh_state_;                                                                                   \
        XXH3_64bits_reset(&lo_xxh_state_);                                                                            \
        XXH3_64bits_update(&lo_xxh_state_, (ip_file_name), strlen((ip_file_name)));                                   \
        int li_local_copy = ii_file_line;                                                                             \
        XXH3_64bits_update(&lo_xxh_state_, &li_local_copy, sizeof(li_local_copy));                                    \
        (ou_hash) = XXH3_64bits_digest(&lo_xxh_state_);                                                               \
    } while(0)
