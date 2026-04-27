#pragma once

#include "p8_core.hpp"

class cp8_mtk
{
public:
    cp8_mtk();
    ~cp8_mtk();

    h_p8_mtk_id       create(const struct s_p8_mtk_base *ip_base);
    bool              emit(h_p8_mtk_id ih_id, double id_value);
    h_p8_mtk_id       create_query(const struct s_p8_mtk_base *ip_base,
                                   uint32_t                    iu_query_interval_ms,
                                   l_p8_mtk_query_cb           il_query,
                                   void                       *ip_user_context);
    h_p8_mtk_group_id create_group(const struct s_p8_mtk_base *ip_base, bool ib_multi_thread);
    h_p8_mtk_group_id create_group_query(const struct s_p8_mtk_base *ip_base,
                                         uint32_t                    iu_query_interval_ms,
                                         l_p8_mtk_group_query_cb     il_query,
                                         void                       *ip_user_context);
    bool              begin_group_emit(h_p8_mtk_group_id ih_group_id);
    bool              emit_group(const char *ip_name, double id_value);
    bool              end_group_emit(h_p8_mtk_group_id ih_group_id);

private:
    cp8_core *mp_core = nullptr;
};
