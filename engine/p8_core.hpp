#pragma once

#include "p8_buffer_pool.hpp"
#include "p8_client_api.h"
#include "p8_memory_budget.hpp"
#include "p8_protocol.h"

#include "kit/list.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#define P8_CORE_ACQUIRE_TIMEOUT_MS 100

struct s_p8_log_desc;

struct s_p8_attr_desc
{
    p8_attr_id          mi_id;
    enum e_p8_attr_type me_type;
    char               *mp_name;
};

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
    void       sync_attr_cache(std::vector<const s_p8_attr_desc *> &io_cache);

    // buffer pool
    static size_t get_buffer_size();
    uint8_t      *acquire_buffer();
    void          release_buffer(uint8_t *ip_buffer);
    void          release_buffers(kit::c_lst<uint8_t *> &io_buffers);

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

#ifdef P8_TESTING
    cp8_buffer_pool *get_data_pool()
    {
        return mp_data_pool;
    }
    cp8_memory_budget *get_memory_budget()
    {
        return mp_memory_budget.get();
    }
#endif

private:
    bool init_buffer_pool(const char *ip_max_memory_size, const char *ip_initial_memory_size);
    bool init_header(const struct s_p8_config *ip_config);

    bool                  mb_initialized = false;
    std::atomic<uint32_t> mu_ref_count { 1 };

    struct s_p8_hdr mo_hdr             = {};

    // buffer pool geometry (kept on the core for backwards-compatible API)
    const static size_t mz_buffer_size = 8192;

    // shared memory budget + data buffer pool (built lazily in init_buffer_pool).
    // budget is shared_ptr because future revisions will pin a second pool
    // (mini-pool, see doc/serializer_optimization.md §5.3) to the same budget;
    // shared ownership lifts the destruction-order constraint between pools
    // and the budget. The pool itself is owned exclusively by cp8_core.
    std::shared_ptr<cp8_memory_budget> mp_memory_budget;
    cp8_buffer_pool                   *mp_data_pool = nullptr;

    // log descriptor registry (global, shared across all TLS cp8_log instances)
    std::map<uint64_t, struct s_p8_log_desc *> mo_log_descs;
    std::mutex                                 mo_log_desc_mutex;

    // attribute registry (global, TLS consumers sync via sync_attr_cache)
    std::vector<s_p8_attr_desc *>               mo_attr_descs;
    std::unordered_map<std::string, p8_attr_id> mo_attr_name_map;
    mutable std::mutex                          mo_attr_mutex;

#ifdef P8_TESTING
    friend size_t                                   p8_test_get_buffer_size();
    friend size_t                                   p8_test_get_free_buffers_count();
    friend size_t                                   p8_test_get_total_allocated();
    friend size_t                                   p8_test_get_all_buffers_count();
    friend void                                     p8_test_enable_buffer_capture();
    friend void                                     p8_test_disable_buffer_capture();
    friend size_t                                   p8_test_get_captured_count();
    friend const std::vector<std::vector<uint8_t>> &p8_test_get_captured_buffers();
    friend void                                     p8_test_clear_captured_buffers();

    bool                              mb_capture_enabled = false;
    std::mutex                        mo_capture_mutex;
    std::vector<std::vector<uint8_t>> mo_captured_buffers;
#endif
};

#ifdef P8_TESTING
void                                     p8_test_reset();
uint32_t                                 p8_test_get_instance_count();
size_t                                   p8_test_get_buffer_size();
size_t                                   p8_test_get_free_buffers_count();
size_t                                   p8_test_get_total_allocated();
size_t                                   p8_test_get_all_buffers_count();
uint8_t                                 *p8_test_acquire_buffer();
void                                     p8_test_release_buffer(uint8_t *ip_buffer);
void                                     p8_test_enable_buffer_capture();
void                                     p8_test_disable_buffer_capture();
size_t                                   p8_test_get_captured_count();
const std::vector<std::vector<uint8_t>> &p8_test_get_captured_buffers();
void                                     p8_test_clear_captured_buffers();
#endif
