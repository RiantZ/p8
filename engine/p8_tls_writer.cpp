#include "p8_tls_writer.hpp"

#include <cstring>
#include <functional>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_tls_writer::cp8_tls_writer()
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
cp8_tls_writer::~cp8_tls_writer()
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
s_p8_attrs_result cp8_tls_writer::serialize_attrs(uint8_t                    *op_dst,
                                                  const uint8_t              *ip_buf_end,
                                                  const struct s_p8_attr_val *ip_attrs,
                                                  size_t                      iz_attrs)
{
    uint8_t  lu_attrs_written = 0;
    uint8_t *lp_dst           = op_dst;

    for(size_t lz_i = 0; ip_attrs && lz_i < iz_attrs; ++lz_i)
    {
        p8_attr_id li_id  = ip_attrs[lz_i].m_id;
        size_t     lz_idx = static_cast<size_t>(li_id);

        if(li_id < 0)
        {
            // TODO: print error that attr is skipped
            continue;
        }

        if(lz_idx >= mo_attr_cache.size() || !mo_attr_cache[lz_idx])
        {
            mp_core->sync_attr_cache(mo_attr_cache);

            if(lz_idx >= mo_attr_cache.size() || !mo_attr_cache[lz_idx])
            {
                // TODO: print error that attr is skipped
                continue;
            }
        }

        const s_p8_attr_desc *lp_attr_desc = mo_attr_cache[lz_idx];

        // Type is not serialized here — it will be sent from the core with the descriptor.
        if(lp_attr_desc->me_type == e_p8_attr_str)
        {
            size_t lz_remain = static_cast<size_t>(ip_buf_end - lp_dst);
            size_t lz_hdr_sz = sizeof(p8_attr_id);
            if(lz_remain < lz_hdr_sz)
            {
                break;
            }

            memcpy(lp_dst, &li_id, sizeof(p8_attr_id));
            lp_dst += sizeof(p8_attr_id);

            size_t lz_written
                = serialize_utf8_string(lp_dst, static_cast<size_t>(ip_buf_end - lp_dst), ip_attrs[lz_i].mp_str);
            if(lz_written == 0)
            {
                lp_dst -= lz_hdr_sz;
                break;
            }
            lp_dst += lz_written;
        }
        else
        {
            size_t lz_needed = sizeof(p8_attr_id) + sizeof(uint64_t);
            if(lp_dst + lz_needed > ip_buf_end)
            {
                break;
            }

            memcpy(lp_dst, &li_id, sizeof(p8_attr_id));
            lp_dst += sizeof(p8_attr_id);
            memcpy(lp_dst, &ip_attrs[lz_i].mu_u64, sizeof(uint64_t));
            lp_dst += sizeof(uint64_t);
        }

        lu_attrs_written++;
    }

    s_p8_attrs_result lo_result;
    lo_result.mu_count = lu_attrs_written;
    lo_result.mz_bytes = static_cast<size_t>(lp_dst - op_dst);
    return lo_result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t cp8_tls_writer::serialize_utf8_string(uint8_t *op_dst, size_t iz_avail, const char *ip_str)
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
