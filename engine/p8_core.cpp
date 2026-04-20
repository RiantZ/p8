#include "p8_client_api.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <exception>

extern "C"
{

    bool p8_initialize(const struct s_p8_config *ip_config)
    {
        if(!ip_config)
        {
            std::fprintf(stderr, "p8_initialize: NULL config\n");
            return false;
        }
        if(!ip_config->mp_json_config)
        {
            std::fprintf(stderr, "p8_initialize: mp_json_config is NULL\n");
            return false;
        }

        // Parse to validate the JSON; consumption of individual parameters will
        // be wired in later, once the schema is known.
        try
        {
            (void)nlohmann::json::parse(ip_config->mp_json_config);
        }
        catch(const nlohmann::json::parse_error &ir_err)
        {
            std::fprintf(stderr, "p8_initialize: JSON parse error: %s\n", ir_err.what());
            return false;
        }
        catch(const std::exception &ir_err)
        {
            std::fprintf(stderr, "p8_initialize: unexpected error: %s\n", ir_err.what());
            return false;
        }

        return true;
    }

    bool p8_get_initialized()
    {
        return false;
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
