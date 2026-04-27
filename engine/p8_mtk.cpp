#include "p8_mtk.hpp"

static thread_local cp8_mtk go_tls_mtk;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_mtk::cp8_mtk()
    : mp_core(cp8_core::get_global_core(P8_CORE_ACQUIRE_TIMEOUT_MS))
{
    if(mp_core)
    {
        mp_core->addref();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_mtk::~cp8_mtk()
{
    if(mp_core)
    {
        mp_core->release();
        mp_core = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
h_p8_mtk_id cp8_mtk::create(const struct s_p8_mtk_base *ip_base)
{
    (void)ip_base;
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_mtk::emit(h_p8_mtk_id ih_id, double id_value)
{
    (void)ih_id;
    (void)id_value;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
h_p8_mtk_id cp8_mtk::create_query(const struct s_p8_mtk_base *ip_base,
                                  uint32_t                    iu_query_interval_ms,
                                  l_p8_mtk_query_cb           il_query,
                                  void                       *ip_user_context)
{
    (void)ip_base;
    (void)iu_query_interval_ms;
    (void)il_query;
    (void)ip_user_context;
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
h_p8_mtk_group_id cp8_mtk::create_group(const struct s_p8_mtk_base *ip_base, bool ib_multi_thread)
{
    (void)ip_base;
    (void)ib_multi_thread;
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
h_p8_mtk_group_id cp8_mtk::create_group_query(const struct s_p8_mtk_base *ip_base,
                                              uint32_t                    iu_query_interval_ms,
                                              l_p8_mtk_group_query_cb     il_query,
                                              void                       *ip_user_context)
{
    (void)ip_base;
    (void)iu_query_interval_ms;
    (void)il_query;
    (void)ip_user_context;
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_mtk::begin_group_emit(h_p8_mtk_group_id ih_group_id)
{
    (void)ih_group_id;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_mtk::emit_group(const char *ip_name, double id_value)
{
    (void)ip_name;
    (void)id_value;
    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_mtk::end_group_emit(h_p8_mtk_group_id ih_group_id)
{
    (void)ih_group_id;
    return false;
}

extern "C"
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    h_p8_mtk_id p8_mtk_create(const struct s_p8_mtk_base *ip_base)
    {
        return go_tls_mtk.create(ip_base);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_mtk_emit(h_p8_mtk_id ih_id, double id_value)
    {
        return go_tls_mtk.emit(ih_id, id_value);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    h_p8_mtk_id p8_mtk_create_query(const struct s_p8_mtk_base *ip_base,
                                    uint32_t                    iu_query_interval_ms,
                                    l_p8_mtk_query_cb           il_query,
                                    void                       *ip_user_context)
    {
        return go_tls_mtk.create_query(ip_base, iu_query_interval_ms, il_query, ip_user_context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    h_p8_mtk_group_id p8_mtk_create_group(const struct s_p8_mtk_base *ip_base, bool ib_multi_thread)
    {
        return go_tls_mtk.create_group(ip_base, ib_multi_thread);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    h_p8_mtk_group_id p8_mtk_create_group_query(const struct s_p8_mtk_base *ip_base,
                                                uint32_t                    iu_query_interval_ms,
                                                l_p8_mtk_group_query_cb     il_query,
                                                void                       *ip_user_context)
    {
        return go_tls_mtk.create_group_query(ip_base, iu_query_interval_ms, il_query, ip_user_context);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_mtk_group_emit_begin(h_p8_mtk_group_id ih_group_id)
    {
        return go_tls_mtk.begin_group_emit(ih_group_id);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_mtk_group_emit(const char *ip_name, double id_value)
    {
        return go_tls_mtk.emit_group(ip_name, id_value);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool p8_mtk_group_emit_end(h_p8_mtk_group_id ih_group_id)
    {
        return go_tls_mtk.end_group_emit(ih_group_id);
    }

} // extern "C"
