include(FetchContent)
FetchContent_Declare(
    kit
    GIT_REPOSITORY git@github.com:RiantZ/kit.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(kit)

# JSON for Modern C++ - used by engine/p8_config.cpp to parse the p8 config file.
# Pinned to a stable release tag; bump deliberately, not to "master".
set(JSON_BuildTests       OFF CACHE INTERNAL "")
set(JSON_Install          OFF CACHE INTERNAL "")
set(JSON_MultipleHeaders  OFF CACHE INTERNAL "")
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

if(NOT IS_SYMLINK "${CMAKE_CURRENT_SOURCE_DIR}/kit")
    file(CREATE_LINK
        ${kit_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/kit
        SYMBOLIC COPY_ON_ERROR
    )
endif()
if(NOT IS_SYMLINK "${PROJECT_SOURCE_DIR}/doc/code_style.md")
    file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/kit/doc/code_style.md
        ${PROJECT_SOURCE_DIR}/doc/code_style.md
        SYMBOLIC COPY_ON_ERROR
    )
endif()
if(NOT IS_SYMLINK "${PROJECT_SOURCE_DIR}/scripts/code_format.py")
    file(CREATE_LINK
        ${CMAKE_CURRENT_SOURCE_DIR}/kit/scripts/code_format.py
        ${PROJECT_SOURCE_DIR}/scripts/code_format.py
        SYMBOLIC COPY_ON_ERROR
    )
endif()
