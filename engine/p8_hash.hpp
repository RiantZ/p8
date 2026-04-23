#pragma once

#define XXH_INLINE_ALL
#include "xxhash.h"

#include <string.h>

#define P8_GET_LOG_HASH(ip_file_name, ii_file_line)                                                                   \
    XXH3_64bits_withSeed((ip_file_name), strlen((ip_file_name)), (XXH64_hash_t)(ii_file_line))
