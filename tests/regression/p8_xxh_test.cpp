#include "p8_xxh.hpp"

#include <gtest/gtest.h>

TEST(c_p8_xxh_test, hash_produces_nonzero)
{
    XXH64_hash_t lu_hash = 0;
    P8_XXH3_LOG_HASH("hello", 42, lu_hash);
    EXPECT_NE(lu_hash, 0u);
}

TEST(c_p8_xxh_test, hash_is_deterministic)
{
    XXH64_hash_t lu_hash1 = 0;
    XXH64_hash_t lu_hash2 = 0;
    P8_XXH3_LOG_HASH("hello", 42, lu_hash1);
    P8_XXH3_LOG_HASH("hello", 42, lu_hash2);
    EXPECT_EQ(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, different_string_different_hash)
{
    XXH64_hash_t lu_hash1 = 0;
    XXH64_hash_t lu_hash2 = 0;
    P8_XXH3_LOG_HASH("hello", 42, lu_hash1);
    P8_XXH3_LOG_HASH("world", 42, lu_hash2);
    EXPECT_NE(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, different_int_different_hash)
{
    XXH64_hash_t lu_hash1 = 0;
    XXH64_hash_t lu_hash2 = 0;
    P8_XXH3_LOG_HASH("hello", 1, lu_hash1);
    P8_XXH3_LOG_HASH("hello", 2, lu_hash2);
    EXPECT_NE(lu_hash1, lu_hash2);
}

TEST(c_p8_xxh_test, empty_string_ok)
{
    XXH64_hash_t lu_hash = 0;
    P8_XXH3_LOG_HASH("", 0, lu_hash);
    EXPECT_NE(lu_hash, 0u);
}
