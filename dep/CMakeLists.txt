include(FetchContent)
FetchContent_Declare(
    kit
    GIT_REPOSITORY git@github.com:RiantZ/kit.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(kit)

file(CREATE_LINK
    ${kit_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kit
    SYMBOLIC COPY_ON_ERROR
)
file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/kit/doc/code_style.md
    ${PROJECT_SOURCE_DIR}/doc/code_style.md
    SYMBOLIC COPY_ON_ERROR
)
file(CREATE_LINK
    ${CMAKE_CURRENT_SOURCE_DIR}/kit/scripts/code_format.py
    ${PROJECT_SOURCE_DIR}/scripts/code_format.py
    SYMBOLIC COPY_ON_ERROR
)
