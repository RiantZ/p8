#pragma once

#include "p8_core.hpp"

class cp8_log
{
public:
    // log item descriptor
    //  format string
    //  file path
    //  file line
    //  function
    //  args list
    //  id
    //  module id
    // log item:
    //  id
    //  time
    //  serialized args
    //  level
    //  processor
    //  thread
    //  module id
    //
public:
    cp8_log();

    void            set_verbosity(p_p8_module ip_module, enum e_p8_level ie_verbosity);
    enum e_p8_level get_verbosity(p_p8_module ip_module);

    bool        register_module(const char *ip_name, enum e_p8_level ie_verbosity, p_p8_module *op_module);
    p_p8_module find_module(const char *ip_name);

    bool send(enum e_p8_level             ie_level,
              p_p8_module                 ip_module,
              uint64_t                    iu_trace_id,
              uint32_t                    iu_line,
              const char                 *ip_file,
              const char                 *ip_function,
              size_t                      iz_attrs,
              const struct s_p8_attr_val *ip_attrs,
              const char                 *ip_format,
              va_list                     io_args);

    bool send_emb(enum e_p8_level             ie_level,
                  p_p8_module                 ip_module,
                  uint64_t                    iu_trace_id,
                  uint32_t                    iu_line,
                  const char                 *ip_file,
                  const char                 *ip_function,
                  size_t                      iz_attrs,
                  const struct s_p8_attr_val *ip_attrs,
                  const char                **ip_format,
                  va_list                    *ip_va_list);

private:
    cp8_core *mp_core = nullptr;
};
