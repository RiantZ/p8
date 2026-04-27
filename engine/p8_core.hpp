#pragma once

#include "p8_client_api.h"

#include "kit/list.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

#define P8_CORE_ACQUIRE_TIMEOUT_MS 100

struct s_p8_log_desc;

class cp8_core
{
public:
    explicit cp8_core(const struct s_p8_config *ip_config);
    ~cp8_core();

    // lifetime
    void addref();
    void release();

    // core
    static cp8_core *get_global_core(uint32_t iu_timeoutms);
    bool             get_initialized() const;
    void             exceptional_flush();

    // thread
    bool register_current_thread(const char *ip_name);
    void unregister_current_thread();

    // attributes
    p8_attr_id attr_register(const char *ip_name, enum e_p8_attr_type ie_type);
    p8_attr_id attr_get(const char *ip_name) const;

    // buffer pool
    static size_t get_buffer_size();
    uint8_t      *acquire_buffer();
    void          release_buffer(uint8_t *ip_buffer);

    // log descriptors
    struct s_p8_log_desc *resolve_log_desc(uint64_t    iu_hash,
                                           const char *ip_file,
                                           uint32_t    iu_line,
                                           const char *ip_function,
                                           const char *ip_format);

    // non-copyable, non-movable
    cp8_core(const cp8_core &)            = delete;
    cp8_core &operator=(const cp8_core &) = delete;
    cp8_core(cp8_core &&)                 = delete;
    cp8_core &operator=(cp8_core &&)      = delete;

private:
    bool init_buffer_pool(const char *ip_max_memory_size, const char *ip_initial_memory_size);

    bool                  mb_initialized = false;
    std::atomic<uint32_t> mu_ref_count { 1 };

    // config — buffer pool
    const static size_t mz_buffer_size         = 8192;
    size_t              mz_max_memory_size     = std::numeric_limits<size_t>::max();
    size_t              mz_initial_memory_size = 1024 * 1024;

    // buffer pool state
    kit::c_lst<uint8_t *> mo_free_buffers;
    kit::c_lst<uint8_t *> mo_all_buffers;
    size_t                mz_total_allocated;
    std::mutex            mo_pool_mutex;

    // log descriptor registry (global, shared across all TLS cp8_log instances)
    std::map<uint64_t, struct s_p8_log_desc *> mo_log_descs;
    std::mutex                                 mo_log_desc_mutex;

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
