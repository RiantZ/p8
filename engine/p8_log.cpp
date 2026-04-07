#include "p8_client_api.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions called by cp8_thread (declared above, implemented here)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool p8_log_register_current_thread(const char *)
{
    return false;
}

void p8_log_unregister_current_thread()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Logging API
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void p8_log_set_verbosity(p_p8_module, enum e_p8_level)
{
}

enum e_p8_level p8_log_get_verbosity(p_p8_module)
{
    return e_p8_trace0;
}

bool p8_log_register_module(const char *, enum e_p8_level, p_p8_module *op_module)
{
    if(op_module)
    {
        *op_module = P8_MODULE_INVALID_ID;
    }
    return false;
}

p_p8_module p8_log_find_module(const char *)
{
    return P8_MODULE_INVALID_ID;
}

bool p8_log_sent(enum e_p8_level,
                 p_p8_module,
                 uint64_t,
                 uint32_t,
                 const char *,
                 const char *,
                 size_t,
                 const struct s_p8_attr_val *,
                 const char *,
                 ...)
{
    return false;
}

bool p8_log_sent_emb(enum e_p8_level,
                     p_p8_module,
                     uint64_t,
                     uint32_t,
                     const char *,
                     const char *,
                     size_t,
                     const struct s_p8_attr_val *,
                     const char **,
                     va_list *)
{
    return false;
}
