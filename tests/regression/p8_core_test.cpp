#include "p8_client_api.h"
#include "p8_core.hpp"

#include <gtest/gtest.h>

#include <latch>
#include <thread>
#include <vector>

class c_p8_core_test : public ::testing::Test
{
protected:
    void TearDown() override
    {
        p8_test_reset();
    }
};

TEST_F(c_p8_core_test, single_thread_initialize)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = "{}";

    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_TRUE(p8_get_initialized());
    EXPECT_EQ(p8_test_get_instance_count(), 1u);
}

TEST_F(c_p8_core_test, double_initialize_same_thread)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = "{}";

    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_TRUE(p8_get_initialized());
    EXPECT_EQ(p8_test_get_instance_count(), 1u);
}

TEST_F(c_p8_core_test, concurrent_initialize)
{
    static constexpr uint32_t LU_THREAD_COUNT = 16;

    struct s_p8_config lo_config              = {};
    lo_config.mp_json_config                  = "{}";

    std::latch               lo_start_latch(LU_THREAD_COUNT);
    std::vector<std::thread> lo_threads;
    std::vector<uint8_t>     la_results(LU_THREAD_COUNT, 0);

    lo_threads.reserve(LU_THREAD_COUNT);
    for(uint32_t lu_i = 0; lu_i < LU_THREAD_COUNT; ++lu_i)
    {
        lo_threads.emplace_back(
            [&, lu_i]()
            {
                lo_start_latch.arrive_and_wait();
                la_results[lu_i] = p8_initialize(&lo_config);
            });
    }

    for(auto &lo_t : lo_threads)
    {
        lo_t.join();
    }

    EXPECT_TRUE(p8_get_initialized());
    EXPECT_EQ(p8_test_get_instance_count(), 1u);

    for(uint32_t lu_i = 0; lu_i < LU_THREAD_COUNT; ++lu_i)
    {
        EXPECT_TRUE(la_results[lu_i]) << "thread " << lu_i << " failed";
    }
}

TEST_F(c_p8_core_test, null_config_fails)
{
    EXPECT_FALSE(p8_initialize(nullptr));
    EXPECT_FALSE(p8_get_initialized());
}

TEST_F(c_p8_core_test, invalid_json_fails)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = "{not valid json}";

    EXPECT_FALSE(p8_initialize(&lo_config));
    EXPECT_FALSE(p8_get_initialized());
}
