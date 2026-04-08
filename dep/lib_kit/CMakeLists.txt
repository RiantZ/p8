
#yes, cmake is standard, but CRAPPY standard ... I would like to have INTERFACE library ... but it is not visible in IDE folders
#add_library(lib_kit INTERFACE)
#target_include_directories(lib_kit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
#so I was FORCED to create normal library.

set(lib_kit ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")


file(GLOB_RECURSE SOURCE_FILES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(lib_kit STATIC ${SOURCE_FILES})

target_include_directories(lib_kit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>  # for build tree
)

set_ide_folder(lib_kit)