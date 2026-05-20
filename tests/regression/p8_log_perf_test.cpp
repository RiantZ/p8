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

TEST_F(c_log_perf_test, DISABLED_send_hello_d_3_attrs)
{
    static constexpr uint32_t lu_warmup     = 10'000;
    static constexpr uint32_t lu_iterations = 1'000'000;

    p8_attr_id li_str                       = p8_attr_register("perf_label", e_p8_attr_str);
    p8_attr_id li_i64                       = p8_attr_register("perf_count", e_p8_attr_i64);
    p8_attr_id li_f64                       = p8_attr_register("perf_ratio", e_p8_attr_f64);
    ASSERT_TRUE(P8_IS_ATTR_VALID(li_str));
    ASSERT_TRUE(P8_IS_ATTR_VALID(li_i64));
    ASSERT_TRUE(P8_IS_ATTR_VALID(li_f64));

    struct s_p8_attr_val la_attrs[3] = {};
    la_attrs[0].m_id                 = li_str;
    la_attrs[0].mp_str               = "request_handler";
    la_attrs[1].m_id                 = li_i64;
    la_attrs[1].mi_i64               = 42;
    la_attrs[2].m_id                 = li_f64;
    la_attrs[2].md_f64               = 3.14;

    for(uint32_t lu_i = 0; lu_i < lu_warmup; ++lu_i)
    {
        la_attrs[1].mi_i64 = static_cast<int64_t>(lu_i);
        p8_log_sent(e_p8_trace0,
                    nullptr,
                    0,
                    static_cast<uint32_t>(__LINE__),
                    __FILE__,
                    __FUNCTION__,
                    3,
                    la_attrs,
                    "hello %d",
                    static_cast<int>(lu_i));
    }

    auto lo_start = std::chrono::steady_clock::now();

    for(uint32_t lu_i = 0; lu_i < lu_iterations; ++lu_i)
    {
        la_attrs[1].mi_i64 = static_cast<int64_t>(lu_i);
        p8_log_sent(e_p8_trace0,
                    nullptr,
                    0,
                    static_cast<uint32_t>(__LINE__),
                    __FILE__,
                    __FUNCTION__,
                    3,
                    la_attrs,
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
