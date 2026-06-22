#include "p8_client_api.h"
#include "p8_core.hpp"
#include "p8_config_keys.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>

namespace
{
static constexpr uint32_t lu_warmup_iters = 10'000;
static constexpr uint32_t lu_batch_count  = 20;
static constexpr uint32_t lu_batch_iters  = 100'000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static double percentile(std::vector<double> &or_xs, double id_p)
{
    if(or_xs.empty())
    {
        return 0.0;
    }
    std::sort(or_xs.begin(), or_xs.end());
    const double ld_rank = (static_cast<double>(or_xs.size()) - 1.0) * id_p;
    const size_t lz_lo   = static_cast<size_t>(ld_rank);
    const size_t lz_hi   = std::min(lz_lo + 1, or_xs.size() - 1);
    const double ld_frac = ld_rank - static_cast<double>(lz_lo);
    return or_xs[lz_lo] + (or_xs[lz_hi] - or_xs[lz_lo]) * ld_frac;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void report(const char *ip_label, std::vector<double> &or_samples_ns)
{
    const size_t lz_n   = or_samples_ns.size();
    double       ld_sum = 0.0;
    for(double ld_x : or_samples_ns)
    {
        ld_sum += ld_x;
    }
    const double ld_mean = ld_sum / static_cast<double>(lz_n);

    double ld_sq         = 0.0;
    for(double ld_x : or_samples_ns)
    {
        const double ld_d  = ld_x - ld_mean;
        ld_sq             += ld_d * ld_d;
    }
    const double ld_stdev       = std::sqrt(ld_sq / static_cast<double>(lz_n));

    std::vector<double> lo_copy = or_samples_ns;
    const double        ld_min  = *std::min_element(lo_copy.begin(), lo_copy.end());
    const double        ld_max  = *std::max_element(lo_copy.begin(), lo_copy.end());
    const double        ld_med  = percentile(lo_copy, 0.50);
    const double        ld_p95  = percentile(lo_copy, 0.95);

    std::printf("\n");
    std::printf("  test       : %s\n", ip_label);
    std::printf("  batches    : %zu x %u iters (warmup %u)\n", lz_n, lu_batch_iters, lu_warmup_iters);
    for(size_t lz_i = 0; lz_i < lz_n; ++lz_i)
    {
        std::printf("  batch %2zu   : %.3f ns/call\n", lz_i, or_samples_ns[lz_i]);
    }
    std::printf("  --- summary (ns/call) ---\n");
    std::printf("  n          : %zu\n", lz_n);
    std::printf("  min        : %.3f\n", ld_min);
    std::printf("  median     : %.3f\n", ld_med);
    std::printf("  mean       : %.3f\n", ld_mean);
    std::printf("  p95        : %.3f\n", ld_p95);
    std::printf("  max        : %.3f\n", ld_max);
    std::printf("  stdev      : %.3f\n", ld_stdev);
    std::printf("\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void run_batches(const std::function<void(uint32_t)> &ir_call, std::vector<double> &or_samples_ns)
{
    for(uint32_t lu_w = 0; lu_w < lu_warmup_iters; ++lu_w)
    {
        ir_call(lu_w);
    }

    or_samples_ns.clear();
    or_samples_ns.reserve(lu_batch_count);

    for(uint32_t lu_b = 0; lu_b < lu_batch_count; ++lu_b)
    {
        const auto lo_start = std::chrono::steady_clock::now();
        for(uint32_t lu_i = 0; lu_i < lu_batch_iters; ++lu_i)
        {
            ir_call(lu_i);
        }
        const auto lo_end = std::chrono::steady_clock::now();
        const auto lo_dt  = std::chrono::duration_cast<std::chrono::nanoseconds>(lo_end - lo_start).count();
        or_samples_ns.push_back(static_cast<double>(lo_dt) / static_cast<double>(lu_batch_iters));
    }
}
} // namespace

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
    std::vector<double> lo_samples;
    run_batches(
        [](uint32_t iu_i)
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
                        static_cast<int>(iu_i));
        },
        lo_samples);

    report("send_hello_d_no_attrs", lo_samples);
}

TEST_F(c_log_perf_test, DISABLED_send_hello_d_3_attrs)
{
    p8_attr_id li_str = p8_attr_register("perf_label", e_p8_attr_str);
    p8_attr_id li_i64 = p8_attr_register("perf_count", e_p8_attr_i64);
    p8_attr_id li_f64 = p8_attr_register("perf_ratio", e_p8_attr_f64);
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

    std::vector<double> lo_samples;
    run_batches(
        [&la_attrs](uint32_t iu_i)
        {
            la_attrs[1].mi_i64 = static_cast<int64_t>(iu_i);
            p8_log_sent(e_p8_trace0,
                        nullptr,
                        0,
                        static_cast<uint32_t>(__LINE__),
                        __FILE__,
                        __FUNCTION__,
                        3,
                        la_attrs,
                        "hello %d",
                        static_cast<int>(iu_i));
        },
        lo_samples);

    report("send_hello_d_3_attrs", lo_samples);
}
