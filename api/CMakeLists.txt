set(P8_API ${PROJECT_SOURCE_DIR}/api CACHE INTERNAL "")

file(GLOB_RECURSE DOC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")

add_custom_target(api SOURCES ${DOC_FILES})

set_ide_folder(api)