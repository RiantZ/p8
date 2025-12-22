add_library(p8 STATIC
            "${P8_API}/p8clientApi.h"
            p8client.cpp
            )

set_property(TARGET p8 PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(p8 PRIVATE lib_kit)

set_ide_folder(p8)