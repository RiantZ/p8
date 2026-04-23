#pragma once

#include "p8_core.hpp"

class cp8_trc
{
public:
    cp8_trc();

    uint64_t begin(uint64_t                    iu_parent_trace_id,
                   uint32_t                    iu_line,
                   const char                 *ip_file,
                   const char                 *ip_function,
                   size_t                      iz_attrs,
                   const struct s_p8_attr_val *ip_attrs,
                   const char                 *ip_function_args,
                   va_list                     io_args);

    bool end(uint64_t iu_trace_id);

private:
    cp8_core *mp_core = nullptr;
};
