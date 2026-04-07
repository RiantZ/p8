add_library(p8 STATIC
            "${P8_API}/p8_client_api.h"
            p8_core.cpp
            p8_log.cpp
            p8_trc.cpp
            p8_mtk.cpp
            )

target_include_directories(p8 PRIVATE "${P8_API}")
set_property(TARGET p8 PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(p8 PRIVATE lib_kit)

set_ide_folder(p8)
