#include "p8_client_api.h"

h_p8_mtk_id p8_mtk_create(const s_metric_base *)
{
    return -1;
}

bool p8_mtk_emit(h_p8_mtk_id, double)
{
    return false;
}

h_p8_mtk_id p8_mtk_create_query(const s_metric_base *, uint32_t, l_p8_mtk_query_cb, void *)
{
    return -1;
}

h_p8_mtk_group_id p8_mtk_create_group(const s_metric_base *, bool)
{
    return -1;
}

h_p8_mtk_id p8_mtk_group_add(h_p8_mtk_group_id, const char *)
{
    return -1;
}

bool p8_mtk_group_del(h_p8_mtk_id)
{
    return false;
}

h_p8_mtk_group_id p8_mtk_create_group_query(const s_metric_base *, uint32_t, l_p8_mtk_group_query_cb, void *)
{
    return -1;
}

bool p8_mtk_group_emit_begin(h_p8_mtk_group_id)
{
    return false;
}

bool p8_mtk_group_emit(h_p8_mtk_id, double)
{
    return false;
}

bool p8_mtk_group_emit_end(h_p8_mtk_group_id)
{
    return false;
}
