#pragma once

#include "p8_client_api.h"

#include "kit/list.hpp"

#include <cstdint>
#include <mutex>
#include <vector>

class cp8_core
{
public:
    explicit cp8_core(const struct s_p8_config *ip_config);
    ~cp8_core();

    // core
    bool get_initialized() const;
    void exceptional_flush();

    // thread
    bool register_current_thread(const char *ip_name);
    void unregister_current_thread();

    // attributes
    p8_attr_id attr_register(const char *ip_name, enum e_p8_attr_type ie_type);
    p8_attr_id attr_get(const char *ip_name) const;

    // buffer pool
    uint8_t *acquire_buffer();
    void     release_buffer(uint8_t *ip_buffer);

    // non-copyable, non-movable
    cp8_core(const cp8_core &)            = delete;
    cp8_core &operator=(const cp8_core &) = delete;
    cp8_core(cp8_core &&)                 = delete;
    cp8_core &operator=(cp8_core &&)      = delete;

private:
    bool mb_initialized                        = false;

    // config — buffer pool
    const static size_t mz_buffer_size         = 8192;
    size_t              mz_max_memory_size     = std::numeric_limits<size_t>::max();
    size_t              mz_initial_memory_size = 1024 * 1024;

    // buffer pool state
    kit::c_lst<uint8_t *> mo_free_buffers;
    kit::c_lst<uint8_t *> mo_all_buffers;
    size_t                mz_total_allocated;
    std::mutex            mo_pool_mutex;

#ifdef P8_TESTING
    friend size_t p8_test_get_buffer_size();
    friend size_t p8_test_get_free_buffers_count();
    friend size_t p8_test_get_total_allocated();
    friend size_t p8_test_get_all_buffers_count();
#endif
};

#ifdef P8_TESTING
void     p8_test_reset();
uint32_t p8_test_get_instance_count();
size_t   p8_test_get_buffer_size();
size_t   p8_test_get_free_buffers_count();
size_t   p8_test_get_total_allocated();
size_t   p8_test_get_all_buffers_count();
uint8_t *p8_test_acquire_buffer();
void     p8_test_release_buffer(uint8_t *ip_buffer);
#endif
