file(GLOB_RECURSE DOC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")

add_custom_target(doc SOURCES ${DOC_FILES})

set_ide_folder(doc)