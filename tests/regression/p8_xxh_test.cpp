#include "p8_hash.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>

TEST(c_p8_xxh_test, hash_produces_nonzero)
{
    XXH64_hash_t lu_hash = P8_GET_LOG_HASH("hello", 42);
    EXPECT_NE(lu_hash, 0u);
}

TEST(c_p8_xxh_test, hash_is_deterministic)
{
    XXH64_hash_t lu_hash1 = P8_GET_LOG_HASH("hello", 42);
    XXH64_hash_t lu_hash2 = P8_GET_LOG_HASH("hello", 42);
    EXPECT_EQ(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, different_string_different_hash)
{
    XXH64_hash_t lu_hash1 = P8_GET_LOG_HASH("hello", 42);
    XXH64_hash_t lu_hash2 = P8_GET_LOG_HASH("world", 42);
    EXPECT_NE(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, different_int_different_hash)
{
    XXH64_hash_t lu_hash1 = P8_GET_LOG_HASH("hello", 1);
    XXH64_hash_t lu_hash2 = P8_GET_LOG_HASH("hello", 2);
    EXPECT_NE(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, empty_string_ok)
{
    XXH64_hash_t lu_hash = P8_GET_LOG_HASH("", 0);
    EXPECT_NE(lu_hash, 0u);
}

TEST(c_p8_xxh_test, DISABLED_benchmark_128b)
{
    static constexpr uint32_t lu_iterations = 1'000'000;
    static constexpr size_t   lz_str_len    = 128;

    char lp_str[lz_str_len + 1];
    memset(lp_str, 'A', lz_str_len);
    lp_str[lz_str_len]    = '\0';

    auto         lo_start = std::chrono::high_resolution_clock::now();
    XXH64_hash_t lu_hash;

    for(uint32_t lu_i = 0; lu_i < lu_iterations; ++lu_i)
    {
        lu_hash = P8_GET_LOG_HASH(lp_str, 7);
    }

    auto   lo_elapsed = std::chrono::high_resolution_clock::now() - lo_start;
    double ld_sec     = std::chrono::duration<double>(lo_elapsed).count();
    double ld_calls   = lu_iterations / ld_sec;
    double ld_bytes   = (lz_str_len + sizeof(int)) * lu_iterations / ld_sec;

    printf("  %u calls in %.3f s  |  %.1f Mcalls/s  |  %.1f MB/s\n",
           lu_iterations,
           ld_sec,
           ld_calls / 1e6,
           ld_bytes / (1024.0 * 1024.0));

    EXPECT_NE(lu_hash, 0u);
}
