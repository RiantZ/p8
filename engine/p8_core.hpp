#pragma once

#include "p8_client_api.h"

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

    // non-copyable, non-movable
    cp8_core(const cp8_core &)            = delete;
    cp8_core &operator=(const cp8_core &) = delete;
    cp8_core(cp8_core &&)                 = delete;
    cp8_core &operator=(cp8_core &&)      = delete;

private:
    bool mb_initialized;
};

#ifdef P8_TESTING
void     p8_test_reset();
uint32_t p8_test_get_instance_count();
#endif
