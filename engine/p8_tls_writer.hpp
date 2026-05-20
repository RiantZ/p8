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

    cp8_core                           *mp_core   = nullptr;
    uint8_t                            *mp_buffer = nullptr;
    size_t                              mz_offset = 0;
    size_t                              mz_buf_sz = 0;
    std::vector<const s_p8_attr_desc *> mo_attr_cache;
    kit::c_lst<uint8_t *>               mo_fragments { 16 };
    uint32_t                            mu_thread_id = 0;
};
