#pragma once

#include "p8_core.hpp"

#include <vector>

struct s_p8_attrs_result
{
    uint8_t mu_count;
    size_t  mz_bytes;
};

class cp8_tls_writer
{
public:
    cp8_tls_writer();
    ~cp8_tls_writer();

    cp8_tls_writer(const cp8_tls_writer &)            = delete;
    cp8_tls_writer &operator=(const cp8_tls_writer &) = delete;
    cp8_tls_writer(cp8_tls_writer &&)                 = delete;
    cp8_tls_writer &operator=(cp8_tls_writer &&)      = delete;

protected:
    s_p8_attrs_result serialize_attrs(uint8_t                    *op_dst,
                                      const uint8_t              *ip_buf_end,
                                      const struct s_p8_attr_val *ip_attrs,
                                      size_t                      iz_attrs);

    bool flush_and_acquire_fragment(uint64_t iu_timestamp);

    static size_t serialize_utf8_string(uint8_t *op_dst, size_t iz_avail, const char *ip_str);

    bool copy_fragmented(uint8_t   *&io_dst,
                         uint8_t   *&io_buf_end,
                         const void *ip_src,
                         size_t      iz_len,
                         uint64_t    iu_timestamp,
                         size_t     &oz_written);

    cp8_core                           *mp_core     = nullptr;
    uint8_t                            *mp_buffer   = nullptr;
    size_t                              mz_buf_used = 0;
    size_t                              mz_buf_max  = 0;
    std::vector<const s_p8_attr_desc *> mo_attr_cache;
    kit::c_lst<uint8_t *>               mo_fragments { 16 };
    uint32_t                            mu_thread_id = 0;
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define P8_ENSURE_SPACE(dst, buf_end, needed, timestamp)                                                              \
    do                                                                                                                \
    {                                                                                                                 \
        if((dst) + (needed) > (buf_end)) [[unlikely]]                                                                 \
        {                                                                                                             \
            mz_buf_used = static_cast<size_t>((dst) - mp_buffer);                                                     \
            if(!flush_and_acquire_fragment((timestamp)))                                                              \
            {                                                                                                         \
                goto lbl_discard;                                                                                     \
            }                                                                                                         \
            (dst)     = mp_buffer + mz_buf_used;                                                                      \
            (buf_end) = mp_buffer + mz_buf_max;                                                                       \
        }                                                                                                             \
    } while(0)
// NOLINTEND(cppcoreguidelines-macro-usage)
