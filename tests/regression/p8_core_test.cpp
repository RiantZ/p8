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
    static constexpr uint32_t lu_thread_count = 16;

    struct s_p8_config lo_config              = {};
    lo_config.mp_json_config                  = "{}";

    std::latch               lo_start_latch(lu_thread_count);
    std::vector<std::thread> lo_threads;
    std::vector<uint8_t>     lo_results(lu_thread_count, 0);

    lo_threads.reserve(lu_thread_count);
    for(uint32_t lu_i = 0; lu_i < lu_thread_count; ++lu_i)
    {
        lo_threads.emplace_back(
            [&, lu_i]()
            {
                lo_start_latch.arrive_and_wait();
                lo_results[lu_i] = p8_initialize(&lo_config);
            });
    }

    for(auto &lo_t : lo_threads)
    {
        lo_t.join();
    }

    EXPECT_TRUE(p8_get_initialized());
    EXPECT_EQ(p8_test_get_instance_count(), 1u);

    for(uint32_t lu_i = 0; lu_i < lu_thread_count; ++lu_i)
    {
        EXPECT_TRUE(lo_results[lu_i]) << "thread " << lu_i << " failed";
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// buffer pool tests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_F(c_p8_core_test, buffer_pool_preallocated)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = R"({
        "max_memory_size": "10KB",
        "max_buffers_count": "10",
        "initial_memory_size": "2MB"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_EQ(p8_test_get_buffer_size(), 1024u);
    EXPECT_EQ(p8_test_get_free_buffers_count(), 10u);
    EXPECT_EQ(p8_test_get_all_buffers_count(), 10u);
}

TEST_F(c_p8_core_test, buffer_pool_small_prealloc_allocates_all)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = R"({
        "max_memory_size": "10KB",
        "max_buffers_count": "10",
        "initial_memory_size": "500"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_EQ(p8_test_get_free_buffers_count(), 10u);
}

TEST_F(c_p8_core_test, buffer_acquire_release)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = R"({
        "max_memory_size": "10KB",
        "max_buffers_count": "10",
        "initial_memory_size": "2MB"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));

    size_t   lz_free_before = p8_test_get_free_buffers_count();
    uint8_t *lp_buf         = p8_test_acquire_buffer();
    ASSERT_NE(lp_buf, nullptr);
    EXPECT_EQ(p8_test_get_free_buffers_count(), lz_free_before - 1);

    p8_test_release_buffer(lp_buf);
    EXPECT_EQ(p8_test_get_free_buffers_count(), lz_free_before);
}

TEST_F(c_p8_core_test, buffer_acquire_on_demand)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = R"({
        "max_memory_size": "2KB",
        "max_buffers_count": "2",
        "initial_memory_size": "2MB"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));
    EXPECT_EQ(p8_test_get_free_buffers_count(), 2u);

    uint8_t *lp_buf1 = p8_test_acquire_buffer();
    uint8_t *lp_buf2 = p8_test_acquire_buffer();
    ASSERT_NE(lp_buf1, nullptr);
    ASSERT_NE(lp_buf2, nullptr);
    EXPECT_EQ(p8_test_get_free_buffers_count(), 0u);

    // limit reached — on-demand should fail
    uint8_t *lp_buf3 = p8_test_acquire_buffer();
    EXPECT_EQ(lp_buf3, nullptr);

    p8_test_release_buffer(lp_buf1);
    p8_test_release_buffer(lp_buf2);
}

TEST_F(c_p8_core_test, buffer_acquire_on_demand_within_limit)
{
    struct s_p8_config lo_config = {};
    lo_config.mp_json_config     = R"({
        "max_memory_size": "10MB",
        "max_buffers_count": "100",
        "initial_memory_size": "1MB"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));

    size_t lz_pre_count = p8_test_get_free_buffers_count();
    EXPECT_EQ(lz_pre_count, 10u);
    EXPECT_EQ(p8_test_get_all_buffers_count(), 10u);

    // acquire all pre-allocated
    std::vector<uint8_t *> lo_bufs;
    lo_bufs.reserve(lz_pre_count);
    for(size_t lz_i = 0; lz_i < lz_pre_count; ++lz_i)
    {
        uint8_t *lp_buf = p8_test_acquire_buffer();
        ASSERT_NE(lp_buf, nullptr);
        lo_bufs.push_back(lp_buf);
    }
    EXPECT_EQ(p8_test_get_free_buffers_count(), 0u);

    // next acquire triggers on-demand allocation
    uint8_t *lp_on_demand = p8_test_acquire_buffer();
    ASSERT_NE(lp_on_demand, nullptr);
    EXPECT_EQ(p8_test_get_all_buffers_count(), lz_pre_count + 1);

    // release all
    p8_test_release_buffer(lp_on_demand);
    for(uint8_t *lp_buf : lo_bufs)
    {
        p8_test_release_buffer(lp_buf);
    }
}

TEST_F(c_p8_core_test, buffer_concurrent_acquire_release)
{
    static constexpr uint32_t lu_thread_count = 16;
    static constexpr uint32_t lu_iterations   = 100;

    struct s_p8_config lo_config              = {};
    lo_config.mp_json_config                  = R"({
        "max_memory_size": "1MB",
        "max_buffers_count": "1000",
        "initial_memory_size": "1MB"
    })";

    EXPECT_TRUE(p8_initialize(&lo_config));
    size_t lz_initial_free = p8_test_get_free_buffers_count();

    std::latch               lo_start_latch(lu_thread_count);
    std::vector<std::thread> lo_threads;
    lo_threads.reserve(lu_thread_count);

    for(uint32_t lu_i = 0; lu_i < lu_thread_count; ++lu_i)
    {
        lo_threads.emplace_back(
            [&]()
            {
                lo_start_latch.arrive_and_wait();
                for(uint32_t lu_j = 0; lu_j < lu_iterations; ++lu_j)
                {
                    uint8_t *lp_buf = p8_test_acquire_buffer();
                    if(lp_buf)
                    {
                        p8_test_release_buffer(lp_buf);
                    }
                }
            });
    }

    for(auto &lo_t : lo_threads)
    {
        lo_t.join();
    }

    EXPECT_EQ(p8_test_get_free_buffers_count(), lz_initial_free);
    EXPECT_EQ(p8_test_get_all_buffers_count(), lz_initial_free);
}
