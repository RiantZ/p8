include(FetchContent)
FetchContent_Declare(
    kit
    GIT_REPOSITORY git@github.com:RiantZ/kit.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(kit)

configure_file(
    ${kit_SOURCE_DIR}/doc/code_style.md
    ${PROJECT_SOURCE_DIR}/doc/code_style.md
    COPYONLY
)
configure_file(
    ${kit_SOURCE_DIR}/scripts/code_format.py
    ${PROJECT_SOURCE_DIR}/scripts/code_format.py
    COPYONLY
)
