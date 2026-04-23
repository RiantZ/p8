#include "p8_log.hpp"

static thread_local cp8_log go_tls_log;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_log::cp8_log()
    : mp_core(cp8_core::get_global_core(P8_CORE_ACQUIRE_TIMEOUT_MS))
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_log::set_verbosity(p_p8_module ip_module, enum e_p8_level ie_verbosity)
{
    (void)ip_module;
    (void)ie_verbosity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum e_p8_level cp8_log::get_verbosity(p_p8_module ip_module)
{
    (void)ip_module;
    return e_p8_trace0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_log::register_module(const char *ip_name, enum e_p8_level ie_verbosity, p_p8_module *op_module)
{
    (void)ip_name;
    (void)ie_verbosity;
    if(op_module)
    {
        *op_module = P8_MODULE_INVALID_ID;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
p_p8_module cp8_log::find_module(const char *ip_name)
{
    (void)ip_name;
    return P8_MODULE_INVALID_ID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_log::send(enum e_p8_level             ie_level,
                   p_p8_module                 ip_module,
                   uint64_t                    iu_trace_id,
                   uint32_t                    iu_line,
                   const char                 *ip_file,
                   const char                 *ip_function,
                   size_t                      iz_attrs,
                   const struct s_p8_attr_val *ip_attrs,
                   const char                 *ip_format,
                   va_list                     io_args)
{
    (void)ie_level;
    (void)ip_module;
    (void)iu_trace_id;
    (void)iu_line;
    (void)ip_file;
    (void)ip_function;
    (void)iz_attrs;
    (void)ip_attrs;
    (void)ip_format;
    (void)io_args;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_log::send_emb(enum e_p8_level             ie_level,
                       p_p8_module                 ip_module,
                       uint64_t                    iu_trace_id,
                       uint32_t                    iu_line,
                       const char                 *ip_file,
                       const char                 *ip_function,
                       size_t                      iz_attrs,
                       const struct s_p8_attr_val *ip_attrs,
                       const char                **ip_format,
                       va_list                    *ip_va_list)
{
    (void)ie_level;
    (void)ip_module;
    (void)iu_trace_id;
    (void)iu_line;
    (void)ip_file;
    (void)ip_function;
    (void)iz_attrs;
    (void)ip_attrs;
    (void)ip_format;
    (void)ip_va_list;
    return false;
}

extern "C"
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void p8_log_set_verbosity(p_p8_module ip_module, enum e_p8_level ie_verbosity)
    {
        go_tls_log.set_verbosity(ip_module, ie_verbosity);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    enum e_p8_level p8_log_get_verbosity(p_p8_module ip_module)
    {
        return go_tls_log.get_verbosity(ip_module);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_log_register_module(const char *ip_name, enum e_p8_level ie_verbosity, p_p8_module *op_module)
    {
        return go_tls_log.register_module(ip_name, ie_verbosity, op_module);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    p_p8_module p8_log_find_module(const char *ip_name)
    {
        return go_tls_log.find_module(ip_name);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_log_sent(enum e_p8_level             ie_level,
                     p_p8_module                 ip_module,
                     uint64_t                    iu_trace_id,
                     uint32_t                    iu_line,
                     const char                 *ip_file,
                     const char                 *ip_function,
                     size_t                      iz_attrs,
                     const struct s_p8_attr_val *ip_attrs,
                     const char                 *ip_format,
                     ...)
    {
        va_list lo_args;
        va_start(lo_args, ip_format);
        bool lb_result = go_tls_log.send(ie_level,
                                         ip_module,
                                         iu_trace_id,
                                         iu_line,
                                         ip_file,
                                         ip_function,
                                         iz_attrs,
                                         ip_attrs,
                                         ip_format,
                                         lo_args);
        va_end(lo_args);
        return lb_result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_log_sent_emb(enum e_p8_level             ie_level,
                         p_p8_module                 ip_module,
                         uint64_t                    iu_trace_id,
                         uint32_t                    iu_line,
                         const char                 *ip_file,
                         const char                 *ip_function,
                         size_t                      iz_attrs,
                         const struct s_p8_attr_val *ip_attrs,
                         const char                **ip_format,
                         va_list                    *ip_va_list)
    {
        return go_tls_log.send_emb(ie_level,
                                   ip_module,
                                   iu_trace_id,
                                   iu_line,
                                   ip_file,
                                   ip_function,
                                   iz_attrs,
                                   ip_attrs,
                                   ip_format,
                                   ip_va_list);
    }

} // extern "C"
