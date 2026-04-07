#include "p8_client_api.h"

bool p8_initialize(const struct s_p8_config *)
{
    return false;
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
    return P8_ATTR_INVALID;
}

p8_attr_id p8_attr_get(const char *)
{
    return P8_ATTR_INVALID;
}
