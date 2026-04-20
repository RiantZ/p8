#include "p8_core.hpp"

#include "kit/shared_mem.hpp"

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdio>
#include <exception>
#include <new>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// singleton state
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static cp8_core             *gp_instance   = nullptr;
static c_shared::h_shared    gp_shm_handle = nullptr;
static const tXCHAR         *gp_shm_name   = TM("p8");
static std::atomic<uint32_t> gu_instance_count { 0 };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// cp8_core implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cp8_core::cp8_core(const struct s_p8_config *)
    : mb_initialized(true)
{
    gu_instance_count.fetch_add(1, std::memory_order_relaxed);
}

cp8_core::~cp8_core()
{
    gu_instance_count.fetch_sub(1, std::memory_order_relaxed);
}

bool cp8_core::get_initialized() const
{
    return mb_initialized;
}

void cp8_core::exceptional_flush()
{
}

bool cp8_core::register_current_thread(const char *)
{
    return false;
}

void cp8_core::unregister_current_thread()
{
}

p8_attr_id cp8_core::attr_register(const char *, enum e_p8_attr_type)
{
    return P8_ATTR_ERROR_NOT_IMPLEMENTED;
}

p8_attr_id cp8_core::attr_get(const char *) const
{
    return P8_ATTR_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// extern "C" API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{

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

        if(!ip_config->mp_json_config)
        {
            std::fprintf(stderr, "p8_initialize: mp_json_config is NULL\n");
            lb_error = true;
            goto lbl_exit;
        }

        try
        {
            (void)nlohmann::json::parse(ip_config->mp_json_config,
                                        /*callback=*/nullptr,
                                        /*allow_exceptions=*/true,
                                        /*ignore_comments=*/true);
        }
        catch(const nlohmann::json::parse_error &ir_err)
        {
            std::fprintf(stderr, "p8_initialize: JSON parse error: %s\n", ir_err.what());
            lb_error = true;
            goto lbl_exit;
        }
        catch(const std::exception &ir_err)
        {
            std::fprintf(stderr, "p8_initialize: unexpected error: %s\n", ir_err.what());
            lb_error = true;
            goto lbl_exit;
        }

        // 4. create core instance
        lp_new = new(std::nothrow) cp8_core(ip_config);
        if(!lp_new)
        {
            std::fprintf(stderr, "p8_initialize: allocation failed\n");
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

    bool p8_get_initialized()
    {
        return gp_instance && gp_instance->get_initialized();
    }

    void p8_exceptional_flush()
    {
    }

    bool p8_register_current_thread(const char *)
    {
        return false;
    }

    void p8_unregister_current_thread()
    {
    }

    p8_attr_id p8_attr_register(const char *, enum e_p8_attr_type)
    {
        return P8_ATTR_ERROR_NOT_IMPLEMENTED;
    }

    p8_attr_id p8_attr_get(const char *)
    {
        return P8_ATTR_ERROR_NOT_IMPLEMENTED;
    }

} // extern "C"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test helpers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef P8_TESTING

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

uint32_t p8_test_get_instance_count()
{
    return gu_instance_count.load(std::memory_order_relaxed);
}

#endif // P8_TESTING
