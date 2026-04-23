add_library(p8 STATIC
            "${P8_API}/p8_client_api.h"
            "${P8_API}/p8_config.h"
            p8_core.hpp
            p8_core.cpp
            p8_log.hpp
            p8_log.cpp
            p8_mtk.hpp
            p8_mtk.cpp
            p8_trc.hpp
            p8_trc.cpp
            p8_xxh.hpp
            p8_config.cpp
            )

target_include_directories(p8 PRIVATE "${P8_API}")
set_property(TARGET p8 PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(p8 PRIVATE kit::kit nlohmann_json::nlohmann_json xxhash::xxhash)

set_ide_folder(p8)
