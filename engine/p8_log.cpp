#include "p8_log.hpp"
#include "p8_protocol.h"

#include <stdint.h>
#include <string.h>
#include <wchar.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum e_prefix_type
{
    e_prefix_i64 = 0,
    e_prefix_i32,
    e_prefix_ll,
    e_prefix_l,
    e_prefix_hh,
    e_prefix_h,
    e_prefix_i,
    e_prefix_w,
    e_prefix_j,
    e_prefix_upper_l,
    e_prefix_unknown
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct s_prefix_desc
{
    const char        *mp_prefix;
    uint32_t           mu_len;
    enum e_prefix_type me_type;
};

// longest-match-first order is critical
static const struct s_prefix_desc gs_prefixes[] = {
    { "I64", 3, e_prefix_i64     },
    { "I32", 3, e_prefix_i32     },
    { "ll",  2, e_prefix_ll      },
    { "hh",  2, e_prefix_hh      },
    { "h",   1, e_prefix_h       },
    { "l",   1, e_prefix_l       },
    { "L",   1, e_prefix_upper_l },
    { "I",   1, e_prefix_i       },
    { "z",   1, e_prefix_i       },
    { "t",   1, e_prefix_i       },
    { "w",   1, e_prefix_w       },
    { "j",   1, e_prefix_j       },
    { NULL,  0, e_prefix_unknown }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const struct s_prefix_desc *find_prefix(const char *ip_fmt)
{
    const struct s_prefix_desc *lp_cur = &gs_prefixes[0];

    while(lp_cur->mu_len)
    {
        if(0 == strncmp(ip_fmt, lp_cur->mp_prefix, lp_cur->mu_len))
        {
            return lp_cur;
        }
        lp_cur++;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t log_parse_format_string(struct sP7Trace_Arg *op_args, size_t iz_args_max, const char *ip_format)
{
    size_t             lz_count     = 0;
    const char        *lp_iter      = ip_format;
    bool               lb_percent   = false;
    enum e_prefix_type le_prefix    = e_prefix_unknown;
    uint8_t            lu_has_width = 0;

    if(!ip_format || !op_args || 0 == iz_args_max)
    {
        return 0;
    }

    while(*lp_iter)
    {
        if(!lb_percent)
        {
            if('%' == *lp_iter)
            {
                lb_percent   = true;
                le_prefix    = e_prefix_unknown;
                lu_has_width = 0;
            }
        }
        else
        {
            switch(*lp_iter)
            {
            case '*':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }
                op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
                lz_count++;
                lu_has_width = 1;
                break;
            }
            case 'I':
            case 'l':
            case 'h':
            case 'w':
            case 'z':
            case 'j':
            case 't':
            case 'L':
            {
                const struct s_prefix_desc *lp_pfx = find_prefix(lp_iter);
                if(lp_pfx)
                {
                    le_prefix = lp_pfx->me_type;
                    if(1 < lp_pfx->mu_len)
                    {
                        lp_iter += (lp_pfx->mu_len - 1);
                    }
                }
                break;
            }
            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
            case 'b':
            case 'B':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }

                if(e_prefix_unknown == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
                }
                else if(e_prefix_ll == le_prefix || e_prefix_i64 == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT64;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint64_t);
                }
                else if(e_prefix_h == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT16;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint16_t);
                }
                else if(e_prefix_hh == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT8;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint8_t);
                }
                else if(e_prefix_l == le_prefix)
                {
#if defined(G_OS_WINDOWS)
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
#elif defined(GTX64)
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT64;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint64_t);
#else
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
#endif
                }
                else if(e_prefix_i32 == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
                }
                else if(e_prefix_i == le_prefix)
                {
#ifdef GTX64
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT64;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint64_t);
#else
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
#endif
                }
                else if(e_prefix_j == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INTMAX;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uintmax_t);
                }
                else
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_INT32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(uint32_t);
                }

                lz_count++;
                lb_percent = false;
                break;
            }
            case 's':
            case 'S':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }

                if(e_prefix_h == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_STRA;
                    op_args[lz_count].mu_size = lu_has_width;
                }
                else if('S' == *lp_iter)
                {
#if defined(G_OS_WINDOWS)
                    op_args[lz_count].mu_type = P8_ARG_TYPE_STRA;
#else
                    op_args[lz_count].mu_type = P8_ARG_TYPE_USTR32;
#endif
                    op_args[lz_count].mu_size = lu_has_width;
                }
                else if(e_prefix_l == le_prefix)
                {
#if defined(G_OS_WINDOWS)
                    op_args[lz_count].mu_type = P8_ARG_TYPE_USTR16;
#else
                    op_args[lz_count].mu_type = P8_ARG_TYPE_USTR32;
#endif
                    op_args[lz_count].mu_size = lu_has_width;
                }
                else
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_USTR8;
                    op_args[lz_count].mu_size = lu_has_width;
                }

                lz_count++;
                lb_percent = false;
                break;
            }
            case 'p':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }
                op_args[lz_count].mu_type = P8_ARG_TYPE_PVOID;
                op_args[lz_count].mu_size = P8_SIZE_OF_ARG(void *);
                lz_count++;
                lb_percent = false;
                break;
            }
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'a':
            case 'A':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }

                if(e_prefix_upper_l == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_LDOUBLE;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(long double);
                }
                else
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_DOUBLE;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(double);
                }

                lz_count++;
                lb_percent = false;
                break;
            }
            case 'c':
            case 'C':
            {
                if(lz_count >= iz_args_max)
                {
                    return lz_count;
                }

                if(e_prefix_h == le_prefix)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_CHAR;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(char);
                }
                else if(e_prefix_l == le_prefix)
                {
#if defined(G_OS_WINDOWS)
                    op_args[lz_count].mu_type = P8_ARG_TYPE_CHAR16;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(wchar_t);
#else
                    op_args[lz_count].mu_type = P8_ARG_TYPE_CHAR32;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(wchar_t);
#endif
                }
                else if('c' == *lp_iter)
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_CHAR;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(char);
                }
                else
                {
                    op_args[lz_count].mu_type = P8_ARG_TYPE_CHAR;
                    op_args[lz_count].mu_size = P8_SIZE_OF_ARG(char);
                }

                lz_count++;
                lb_percent = false;
                break;
            }
            case '%':
            {
                lb_percent = false;
                break;
            }
            } // switch

            if(lb_percent && *lp_iter >= '0' && *lp_iter <= '9')
            {
                lu_has_width = 1;
            }
        }

        lp_iter++;
    }

    return lz_count;
}

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
