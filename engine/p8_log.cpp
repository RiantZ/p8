#include "p8_log.hpp"
#include "p8_hash.hpp"
#include "p8_protocol.h"

#include <functional>
#include <stdint.h>
#include <string.h>
#include <thread>
#include <wchar.h>

#include "kit/time.hpp"

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
size_t cp8_log::parse_format_string(struct s_p8_trace_arg *op_args, size_t iz_args_max, const char *ip_format)
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
    , mu_thread_id(static_cast<uint32_t>(std::hash<std::thread::id> {}(std::this_thread::get_id())))
{
    if(mp_core)
    {
        mp_core->addref();
        mz_buf_sz = mp_core->get_buffer_size();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_log::~cp8_log()
{
    if(mp_core)
    {
        if(mp_buffer)
        {
            mp_core->release_buffer(mp_buffer);
            mp_buffer = nullptr;
        }
        mp_core->release();
        mp_core = nullptr;
    }
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
static size_t serialize_utf8_string(uint8_t *op_dst, size_t iz_avail, const char *ip_str)
{
    uint16_t lu_len = 0;

    if(ip_str)
    {
        size_t lz_slen = strlen(ip_str);
        lu_len         = (lz_slen > UINT16_MAX) ? UINT16_MAX : static_cast<uint16_t>(lz_slen);
    }

    if(sizeof(lu_len) + lu_len > iz_avail)
    {
        return 0;
    }

    memcpy(op_dst, &lu_len, sizeof(lu_len));
    if(lu_len)
    {
        memcpy(op_dst + sizeof(lu_len), ip_str, lu_len);
    }

    return sizeof(lu_len) + lu_len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static size_t serialize_wide_string(uint8_t *op_dst, size_t iz_avail, const wchar_t *ip_str)
{
    uint16_t lu_len = 0;

    if(ip_str)
    {
        size_t lz_bytes = wcslen(ip_str) * sizeof(wchar_t);
        lu_len          = (lz_bytes > UINT16_MAX) ? UINT16_MAX : static_cast<uint16_t>(lz_bytes);
    }

    if(sizeof(lu_len) + lu_len > iz_avail)
    {
        return 0;
    }

    memcpy(op_dst, &lu_len, sizeof(lu_len));
    if(lu_len)
    {
        memcpy(op_dst + sizeof(lu_len), ip_str, lu_len);
    }

    return sizeof(lu_len) + lu_len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static size_t serialize_u16_string(uint8_t *op_dst, size_t iz_avail, const uint16_t *ip_str)
{
    uint16_t lu_len = 0;

    if(ip_str)
    {
        size_t lz_chars = 0;
        while(ip_str[lz_chars])
        {
            lz_chars++;
        }
        size_t lz_bytes = lz_chars * sizeof(uint16_t);
        lu_len          = (lz_bytes > UINT16_MAX) ? UINT16_MAX : static_cast<uint16_t>(lz_bytes);
    }

    if(sizeof(lu_len) + lu_len > iz_avail)
    {
        return 0;
    }

    memcpy(op_dst, &lu_len, sizeof(lu_len));
    if(lu_len)
    {
        memcpy(op_dst + sizeof(lu_len), ip_str, lu_len);
    }

    return sizeof(lu_len) + lu_len;
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
    struct s_p8_log_desc     *lp_desc    = nullptr;
    struct s_p8_log_item_hdr *lp_hdr     = nullptr;
    uint64_t                  lu_hash    = 0;
    size_t                    lz_avail   = 0;
    uint8_t                  *lp_dst     = nullptr;
    uint8_t                  *lp_buf_end = nullptr;
    bool                      lb_ret     = true;

    (void)ip_module;

    std::lock_guard<kit::c_spin_lock> lo_guard(mo_lock);

    if(!mp_core) [[unlikely]]
    {
        return false;
    }

    // TODO: AZH: add main buffer header where we are storing
    //  - buffer type (log, traces, metrics)
    //  - payload size - update only if function succeed

    // buffer availability check — reuse current buffer when possible
    if(mp_buffer) [[likely]]
    {
        lz_avail = mz_buf_sz - mz_offset;

        if(lz_avail < P8_LOG_MIN_BUFFER_SPACE) [[unlikely]]
        {
            mp_core->release_buffer(mp_buffer);
            mp_buffer = nullptr;
        }
    }

    if(!mp_buffer) [[unlikely]]
    {
        mp_buffer = mp_core->acquire_buffer();
        if(!mp_buffer) [[unlikely]]
        {
            return false;
        }
        mz_offset = 0;
        lz_avail  = mz_buf_sz;
    }

    // compute hash from file + line
    lu_hash    = P8_GET_LOG_HASH(ip_file, iu_line);

    auto lo_it = mo_desc_map.find(lu_hash);
    if(lo_it != mo_desc_map.end())
    {
        lp_desc = lo_it->second;
    }
    else
    {
        lp_desc = mp_core->resolve_log_desc(lu_hash, ip_file, iu_line, ip_function, ip_format);
        if(!lp_desc)
        {
            return false;
        }
        mo_desc_map[lu_hash] = lp_desc;
    }

    // verify buffer space for header + fixed-size args at minimum
    if(lz_avail < sizeof(s_p8_log_item_hdr) + lp_desc->mz_fixed_args_size) [[unlikely]]
    {
        return false;
    }

    // write item header
    {
        uint8_t *lp_base        = mp_buffer + mz_offset;
        lp_buf_end              = mp_buffer + mz_buf_sz;

        lp_hdr                  = reinterpret_cast<struct s_p8_log_item_hdr *>(lp_base);
        lp_hdr->mu_hash         = lp_desc->mu_hash;
        lp_hdr->mu_timestamp_ns = kit::get_hires_ticks();
        lp_hdr->mu_trace_id     = iu_trace_id;
        lp_hdr->mu_thread_id    = mu_thread_id;
        lp_hdr->mu_level        = static_cast<uint8_t>(ie_level);
        lp_hdr->mu_processor    = 0; // TODO: platform-specific CPU core ID
        lp_hdr->mu_attrs_count  = static_cast<uint8_t>(iz_attrs > 255 ? 255 : iz_attrs);

        lp_dst                  = lp_base + sizeof(struct s_p8_log_item_hdr);
    }

    // serialize variable arguments via pointer iteration
    {
        const s_p8_trace_arg *lp_arg     = lp_desc->ma_args;
        const s_p8_trace_arg *lp_arg_end = lp_arg + lp_desc->mz_args_count;

        for(; lp_arg < lp_arg_end; ++lp_arg)
        {
            if(lp_dst + lp_arg->mu_size > lp_buf_end) [[unlikely]]
            {
                lb_ret = false;
                goto lbl_finalize;
            }
#if defined(GTX32)
            if(4 == lp_arg->mu_size)
            {
                unsigned int lu_val = va_arg(io_args, unsigned int);
                memcpy(lp_dst, &lu_val, sizeof(lu_val));
                lp_dst += lp_arg->mu_size;
            }
            else
#endif
                if(sizeof(uint64_t) == lp_arg->mu_size)
            {
                if(P8_ARG_TYPE_DOUBLE != lp_arg->mu_type) [[likely]]
                {
                    uint64_t lu_val = va_arg(io_args, uint64_t);
                    memcpy(lp_dst, &lu_val, sizeof(lu_val));
                    lp_dst += lp_arg->mu_size;
                }
                else
                {
                    double ld_val = va_arg(io_args, double);
                    memcpy(lp_dst, &ld_val, sizeof(ld_val));
                    lp_dst += lp_arg->mu_size;
                }
            }
            else if(P8_ARG_TYPE_LDOUBLE == lp_arg->mu_type)
            {
                long double ld_val = va_arg(io_args, long double);
                memcpy(lp_dst, &ld_val, sizeof(ld_val));
                lp_dst += lp_arg->mu_size;
            }
            else
            {
                switch(lp_arg->mu_type)
                {
                case P8_ARG_TYPE_USTR8:
                case P8_ARG_TYPE_STRA:
                {
                    size_t      lz_remain  = static_cast<size_t>(lp_buf_end - lp_dst);
                    const char *lp_str     = va_arg(io_args, const char *);
                    size_t      lz_written = serialize_utf8_string(lp_dst, lz_remain, lp_str);
                    if(0 == lz_written)
                    {
                        lb_ret = false;
                        goto lbl_finalize;
                    }
                    lp_dst += lz_written;
                    break;
                }
                case P8_ARG_TYPE_USTR16:
                {
                    size_t          lz_remain  = static_cast<size_t>(lp_buf_end - lp_dst);
                    const uint16_t *lp_str     = va_arg(io_args, const uint16_t *);
                    size_t          lz_written = serialize_u16_string(lp_dst, lz_remain, lp_str);
                    if(0 == lz_written)
                    {
                        lb_ret = false;
                        goto lbl_finalize;
                    }
                    lp_dst += lz_written;
                    break;
                }
                case P8_ARG_TYPE_USTR32:
                {
                    size_t         lz_remain  = static_cast<size_t>(lp_buf_end - lp_dst);
                    const wchar_t *lp_str     = va_arg(io_args, const wchar_t *);
                    size_t         lz_written = serialize_wide_string(lp_dst, lz_remain, lp_str);
                    if(0 == lz_written)
                    {
                        lb_ret = false;
                        goto lbl_finalize;
                    }
                    lp_dst += lz_written;
                    break;
                }
                default:
                {
                    std::fprintf(stderr, "p8_log: unknown argument %d\n", lp_arg->mu_type);
                    break;
                }
                }
            }
        }
    }

    lp_hdr->mu_args_size
        = static_cast<uint16_t>(lp_dst - reinterpret_cast<uint8_t *>(lp_hdr) - sizeof(struct s_p8_log_item_hdr));

    // sync attribute cache if any attr_id is unknown to this TLS instance
    {
        bool lb_need_sync = false;
        for(size_t lz_i = 0; !lb_need_sync && ip_attrs && lz_i < iz_attrs; ++lz_i)
        {
            p8_attr_id li_id = ip_attrs[lz_i].m_id;
            if(li_id < 0 || static_cast<size_t>(li_id) >= mo_attr_cache.size()
               || !mo_attr_cache[static_cast<size_t>(li_id)])
            {
                lb_need_sync = true;
            }
        }
        if(lb_need_sync)
        {
            mp_core->sync_attr_cache(mo_attr_cache);
        }
    }

    // serialize attributes: type-aware encoding
    {
        uint8_t lu_attrs_written = 0;
        for(size_t lz_i = 0; ip_attrs && lz_i < iz_attrs; ++lz_i)
        {
            p8_attr_id li_id  = ip_attrs[lz_i].m_id;
            size_t     lz_idx = static_cast<size_t>(li_id);

            if(li_id < 0 || lz_idx >= mo_attr_cache.size() || !mo_attr_cache[lz_idx])
            {
                continue;
            }

            const s_p8_attr_desc *lp_attr_desc = mo_attr_cache[lz_idx];
            uint8_t               lu_type_tag  = static_cast<uint8_t>(lp_attr_desc->me_type);

            if(lp_attr_desc->me_type == e_p8_attr_str)
            {
                size_t lz_remain = static_cast<size_t>(lp_buf_end - lp_dst);
                size_t lz_hdr_sz = sizeof(p8_attr_id) + sizeof(uint8_t);
                if(lz_remain < lz_hdr_sz)
                {
                    break;
                }

                memcpy(lp_dst, &li_id, sizeof(p8_attr_id));
                lp_dst += sizeof(p8_attr_id);
                memcpy(lp_dst, &lu_type_tag, sizeof(uint8_t));
                lp_dst += sizeof(uint8_t);

                size_t lz_written
                    = serialize_utf8_string(lp_dst, static_cast<size_t>(lp_buf_end - lp_dst), ip_attrs[lz_i].mp_str);
                if(lz_written == 0)
                {
                    lp_dst -= lz_hdr_sz;
                    break;
                }
                lp_dst += lz_written;
            }
            else
            {
                size_t lz_needed = sizeof(p8_attr_id) + sizeof(uint8_t) + sizeof(uint64_t);
                if(lp_dst + lz_needed > lp_buf_end)
                {
                    break;
                }

                memcpy(lp_dst, &li_id, sizeof(p8_attr_id));
                lp_dst += sizeof(p8_attr_id);
                memcpy(lp_dst, &lu_type_tag, sizeof(uint8_t));
                lp_dst += sizeof(uint8_t);
                memcpy(lp_dst, &ip_attrs[lz_i].mu_u64, sizeof(uint64_t));
                lp_dst += sizeof(uint64_t);
            }

            lu_attrs_written++;
        }
        lp_hdr->mu_attrs_count = lu_attrs_written;
    }

    lp_hdr->mu_size = static_cast<uint16_t>(lp_dst - reinterpret_cast<uint8_t *>(lp_hdr));
    mz_offset       = static_cast<size_t>(lp_dst - mp_buffer);

lbl_finalize:
    if(!lb_ret)
    {
        // TODO: IF this is space problem - release buffer and make another loop with new buffer
    }

    // TODO: temporal solution - buffer management need to be updated for real arch.
    //  post-check: if remaining buffer < P8_LOG_MIN_BUFFER_SPACE, return it to pool
    if(mz_buf_sz - mz_offset < P8_LOG_MIN_BUFFER_SPACE) [[unlikely]]
    {
        mp_core->release_buffer(mp_buffer);
        mp_buffer = nullptr;
        mz_offset = 0;
    }

    return true;
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
