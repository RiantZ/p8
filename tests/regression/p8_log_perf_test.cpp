#include "p8_client_api.h"
#include "p8_core.hpp"
#include "p8_config_keys.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>

class c_log_perf_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        struct s_p8_config lo_config = {};
        lo_config.mp_json_config     = "{"
                                       "\"" P8_CFG_KEY_MAX_MEMORY_SIZE "\": \"16MB\","
                                       "\"" P8_CFG_KEY_INITIAL_MEMORY_SIZE "\": \"16MB\""
                                       "}";

        ASSERT_TRUE(p8_initialize(&lo_config));
    }

    void TearDown() override
    {
        p8_release();
    }
};

TEST_F(c_log_perf_test, DISABLED_send_hello_d_no_attrs)
{
    static constexpr uint32_t lu_warmup     = 10'000;
    static constexpr uint32_t lu_iterations = 1'000'000;

    for(uint32_t lu_i = 0; lu_i < lu_warmup; ++lu_i)
    {
        p8_log_sent(e_p8_trace0,
                    nullptr,
                    0,
                    static_cast<uint32_t>(__LINE__),
                    __FILE__,
                    __FUNCTION__,
                    0,
                    nullptr,
                    "hello %d",
                    static_cast<int>(lu_i));
    }

    auto lo_start = std::chrono::steady_clock::now();

    for(uint32_t lu_i = 0; lu_i < lu_iterations; ++lu_i)
    {
        p8_log_sent(e_p8_trace0,
                    nullptr,
                    0,
                    static_cast<uint32_t>(__LINE__),
                    __FILE__,
                    __FUNCTION__,
                    0,
                    nullptr,
                    "hello %d",
                    static_cast<int>(lu_i));
    }

    auto lo_end     = std::chrono::steady_clock::now();
    auto lo_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(lo_end - lo_start);
    auto lu_ns_per_call
        = std::chrono::duration_cast<std::chrono::nanoseconds>(lo_end - lo_start).count() / lu_iterations;

    std::printf("\n");
    std::printf("  warmup     : %u\n", lu_warmup);
    std::printf("  iterations : %u\n", lu_iterations);
    std::printf("  total      : %lld us\n", static_cast<long long>(lo_elapsed.count()));
    std::printf("  per call   : %lld ns\n", static_cast<long long>(lu_ns_per_call));
    std::printf("\n");
}
