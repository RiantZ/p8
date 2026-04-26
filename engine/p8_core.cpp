#include "p8_core.hpp"
#include "p8_config_keys.hpp"
#include "p8_log.hpp"

#include "kit/shared_mem.hpp"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <strings.h>
#include <chrono>
#include <new>
#include <string>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// singleton state
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static cp8_core             *gp_instance   = nullptr;
static c_shared::h_shared    gp_shm_handle = nullptr;
static const tXCHAR         *gp_shm_name   = TM("p8");
static std::atomic<uint32_t> gu_instance_count { 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool parse_size(const char *ip_str, size_t &oz_result)
{
    char   la_buf[128];
    size_t lz_dst = 0;

    for(size_t lz_i = 0; ip_str[lz_i] != '\0' && lz_dst < sizeof(la_buf) - 1; ++lz_i)
    {
        if(ip_str[lz_i] != ' ' && ip_str[lz_i] != '\t')
        {
            la_buf[lz_dst++] = ip_str[lz_i];
        }
    }
    la_buf[lz_dst] = '\0';

    if(strcasecmp(la_buf, "infinite") == 0)
    {
        oz_result = std::numeric_limits<size_t>::max();
        return true;
    }

    char              *lp_end = nullptr;
    unsigned long long lu_val = std::strtoull(la_buf, &lp_end, 10);

    if(lp_end == la_buf)
    {
        return false;
    }

    if(*lp_end == '\0')
    {
        oz_result = static_cast<size_t>(lu_val);
        return true;
    }

    if(strcasecmp(lp_end, "KB") == 0 || strcasecmp(lp_end, "Ki") == 0)
    {
        oz_result = static_cast<size_t>(lu_val * 1024);
        return true;
    }

    if(strcasecmp(lp_end, "MB") == 0 || strcasecmp(lp_end, "Mi") == 0)
    {
        oz_result = static_cast<size_t>(lu_val * 1024 * 1024);
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cp8_core implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core::cp8_core(const struct s_p8_config *ip_config)
{
    gu_instance_count.fetch_add(1, std::memory_order_relaxed);

    if(!ip_config->mp_json_config)
    {
        std::fprintf(stderr, "cp8_core: mp_json_config is NULL\n");
        return;
    }

    nlohmann::json lo_json;

    try
    {
        lo_json = nlohmann::json::parse(ip_config->mp_json_config,
                                        nullptr, // callback
                                        true,    // allow_exceptions
                                        true);   // ignore_comments
    }
    catch(const nlohmann::json::parse_error &ir_err)
    {
        std::fprintf(stderr, "cp8_core: JSON parse error: %s\n", ir_err.what());
        return;
    }
    catch(const std::exception &ir_err)
    {
        std::fprintf(stderr, "cp8_core: unexpected error: %s\n", ir_err.what());
        return;
    }

    if(lo_json.contains(P8_CFG_KEY_SINK))
    {
        // TODO: configure sync mode (file.bin, network.tcp)
    }

    if(lo_json.contains(P8_CFG_KEY_DESTINATION))
    {
        // TODO: configure destination path or network endpoint
    }

    std::string ls_max_mem;
    std::string ls_init_mem;

    if(lo_json.contains(P8_CFG_KEY_MAX_MEMORY_SIZE))
    {
        ls_max_mem = lo_json[P8_CFG_KEY_MAX_MEMORY_SIZE].get<std::string>();
    }

    if(lo_json.contains(P8_CFG_KEY_INITIAL_MEMORY_SIZE))
    {
        ls_init_mem = lo_json[P8_CFG_KEY_INITIAL_MEMORY_SIZE].get<std::string>();
    }

    if(!init_buffer_pool(ls_max_mem.empty() ? nullptr : ls_max_mem.c_str(),
                         ls_init_mem.empty() ? nullptr : ls_init_mem.c_str()))
    {
        return;
    }

    mb_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core::~cp8_core()
{
    for(auto &lo_pair : mo_log_descs)
    {
        delete lo_pair.second;
    }
    mo_log_descs.clear();

    for(uint8_t *lp_buf : mo_all_buffers)
    {
        delete[] lp_buf;
    }
    mo_all_buffers.clear();
    mo_free_buffers.clear();

    gu_instance_count.fetch_sub(1, std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::init_buffer_pool(const char *ip_max_memory_size, const char *ip_initial_memory_size)
{
    if(ip_max_memory_size)
    {
        if(!parse_size(ip_max_memory_size, mz_max_memory_size))
        {
            std::fprintf(stderr, "cp8_core: invalid max_memory_size value\n");
            return false;
        }
    }

    if(ip_initial_memory_size)
    {
        if(!parse_size(ip_initial_memory_size, mz_initial_memory_size))
        {
            std::fprintf(stderr, "cp8_core: invalid initial_memory_size value\n");
            return false;
        }
    }

    if(mz_initial_memory_size > mz_max_memory_size)
    {
        mz_initial_memory_size = mz_max_memory_size;
    }

    for(size_t lz_i = 0; lz_i < (mz_initial_memory_size / mz_buffer_size); ++lz_i)
    {
        uint8_t *lp_buf = new(std::nothrow) uint8_t[mz_buffer_size];
        if(!lp_buf)
        {
            std::fprintf(stderr, "cp8_core: memory allocation failed\n");
            break;
        }
        mo_free_buffers.push_last(lp_buf);
        mo_all_buffers.push_last(lp_buf);
        mz_total_allocated += mz_buffer_size;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::get_initialized() const
{
    return mb_initialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::exceptional_flush()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_core::register_current_thread(const char *)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::unregister_current_thread()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
p8_attr_id cp8_core::attr_register(const char *, enum e_p8_attr_type)
{
    return P8_ATTR_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
p8_attr_id cp8_core::attr_get(const char *) const
{
    return P8_ATTR_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t *cp8_core::acquire_buffer()
{
    std::lock_guard<std::mutex> lo_lock(mo_pool_mutex);

    if(mo_free_buffers.size() > 0)
    {
        return mo_free_buffers.pull_first();
    }

    // on-demand allocation
    if(mz_total_allocated + mz_buffer_size <= mz_max_memory_size)
    {
        uint8_t *lp_buf = new(std::nothrow) uint8_t[mz_buffer_size];
        if(lp_buf)
        {
            mo_all_buffers.push_last(lp_buf);
            mz_total_allocated += mz_buffer_size;
            return lp_buf;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_core::release_buffer(uint8_t *ip_buffer)
{
    if(!ip_buffer)
    {
        return;
    }
    std::lock_guard<std::mutex> lo_lock(mo_pool_mutex);
    mo_free_buffers.push_last(ip_buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t cp8_core::get_buffer_size()
{
    return mz_buffer_size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct s_p8_log_desc *cp8_core::resolve_log_desc(uint64_t    iu_hash,
                                                 const char *ip_file,
                                                 uint32_t    iu_line,
                                                 const char *ip_function,
                                                 const char *ip_format)
{
    std::lock_guard<std::mutex> lo_lock(mo_log_desc_mutex);

    auto lo_it = mo_log_descs.find(iu_hash);
    if(lo_it != mo_log_descs.end())
    {
        return lo_it->second;
    }

    struct s_p8_log_desc *lp_desc = new(std::nothrow) struct s_p8_log_desc;
    if(!lp_desc)
    {
        return nullptr;
    }

    lp_desc->mu_hash            = iu_hash;
    lp_desc->mp_format          = ip_format;
    lp_desc->mp_file            = ip_file;
    lp_desc->mp_function        = ip_function;
    lp_desc->mu_line            = iu_line;
    lp_desc->mz_args_count      = cp8_log::parse_format_string(lp_desc->ma_args, P8_LOG_MAX_ARGS, ip_format);

    lp_desc->mz_fixed_args_size = 0;
    for(size_t lz_i = 0; lz_i < lp_desc->mz_args_count; lz_i++)
    {
        uint8_t lu_type = lp_desc->ma_args[lz_i].mu_type;
        if(lu_type != P8_ARG_TYPE_USTR8 && lu_type != P8_ARG_TYPE_USTR16 && lu_type != P8_ARG_TYPE_USTR32
           && lu_type != P8_ARG_TYPE_STRA)
        {
            lp_desc->mz_fixed_args_size += lp_desc->ma_args[lz_i].mu_size;
        }
    }

    mo_log_descs[iu_hash] = lp_desc;
    return lp_desc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_core *cp8_core::get_global_core(uint32_t iu_timeoutms)
{
    if(gp_instance)
    {
        return gp_instance;
    }

    uint32_t lu_elapsed = 0;

    while(lu_elapsed < iu_timeoutms)
    {
        cp8_core       *lp_found = nullptr;
        c_shared::h_sem lp_sem   = nullptr;

        if(c_shared::lock(gp_shm_name, lp_sem, 10) == c_shared::e_ok)
        {
            c_shared::read(gp_shm_name, &lp_found, sizeof(lp_found));
            c_shared::unlock(lp_sem);
        }

        if(lp_found)
        {
            gp_instance = lp_found;
            return gp_instance;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lu_elapsed += 10;
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// extern "C" API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_initialize(const struct s_p8_config *ip_config)
    {
        cp8_core          *lp_new        = nullptr;
        cp8_core          *lp_existing   = nullptr;
        c_shared::h_shared lp_shm_handle = nullptr;
        c_shared::h_sem    lp_sem        = nullptr;
        bool               lb_error      = false;

        // 1. already initialized in this DSO
        if(gp_instance)
        {
            return true;
        }

        // 2. instance published by another DSO via shared memory
        if(c_shared::read(gp_shm_name, &lp_existing, sizeof(lp_existing)) && lp_existing)
        {
            gp_instance = lp_existing;
            return true;
        }

        // 3. validate config
        if(!ip_config)
        {
            std::fprintf(stderr, "p8_initialize: NULL config\n");
            lb_error = true;
            goto lbl_exit;
        }

        // 4. create core instance (constructor parses JSON, sets mb_initialized on failure)
        lp_new = new(std::nothrow) cp8_core(ip_config);
        if(!lp_new)
        {
            std::fprintf(stderr, "p8_initialize: allocation failed\n");
            lb_error = true;
            goto lbl_exit;
        }

        if(!lp_new->get_initialized())
        {
            lb_error = true;
            goto lbl_exit;
        }

        // 5. atomically publish pointer via shared memory (O_CREAT | O_EXCL)
        if(c_shared::create(&lp_shm_handle, gp_shm_name, reinterpret_cast<const uint8_t *>(&lp_new), sizeof(lp_new)))
        {
            gp_shm_handle = lp_shm_handle;
            gp_instance   = lp_new;
            lp_new        = nullptr;
        }
        else
        {
            // 6. another DSO won the race — read its pointer
            delete lp_new;
            lp_new      = nullptr;
            lp_existing = nullptr;

            if(c_shared::lock(gp_shm_name, lp_sem, 5000) == c_shared::e_ok)
            {
                c_shared::read(gp_shm_name, &lp_existing, sizeof(lp_existing));
                c_shared::unlock(lp_sem);
            }

            if(lp_existing)
            {
                gp_instance = lp_existing;
            }
            else
            {
                std::fprintf(stderr, "p8_initialize: failed to obtain instance from shared memory\n");
                lb_error = true;
                goto lbl_exit;
            }
        }

lbl_exit:
        if(lb_error && lp_new)
        {
            delete lp_new;
            lp_new = nullptr;
        }

        return !lb_error;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_get_initialized()
    {
        return gp_instance && gp_instance->get_initialized();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_exceptional_flush()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_register_current_thread(const char *)
    {
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_unregister_current_thread()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    p8_attr_id p8_attr_register(const char *, enum e_p8_attr_type)
    {
        return P8_ATTR_ERROR_NOT_IMPLEMENTED;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    p8_attr_id p8_attr_get(const char *)
    {
        return P8_ATTR_ERROR_NOT_IMPLEMENTED;
    }

} // extern "C"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef P8_TESTING

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_reset()
{
    if(gp_instance)
    {
        delete gp_instance;
        gp_instance = nullptr;
    }

    if(gp_shm_handle)
    {
        c_shared::close(gp_shm_handle);
        gp_shm_handle = nullptr;
    }
    else
    {
        c_shared::unlink(gp_shm_name);
    }

    gu_instance_count.store(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t p8_test_get_instance_count()
{
    return gu_instance_count.load(std::memory_order_relaxed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_buffer_size()
{
    return gp_instance ? gp_instance->mz_buffer_size : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_free_buffers_count()
{
    return gp_instance ? gp_instance->mo_free_buffers.size() : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_total_allocated()
{
    return gp_instance ? gp_instance->mz_total_allocated : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t p8_test_get_all_buffers_count()
{
    return gp_instance ? gp_instance->mo_all_buffers.size() : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint8_t *p8_test_acquire_buffer()
{
    return gp_instance ? gp_instance->acquire_buffer() : nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void p8_test_release_buffer(uint8_t *ip_buffer)
{
    if(gp_instance)
    {
        gp_instance->release_buffer(ip_buffer);
    }
}

#endif // P8_TESTING
