cmake_minimum_required(VERSION 3.20)

project(P8_RegressionTests)

include(FetchContent)

#see: https://github.com/google/googletest/releases/
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)

# For Windows: Prevent overriding the parent project's compiler/linker on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(${P8_API})
include_directories(lib_kit)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries (${PROJECT_NAME} lib_kit gtest_main)

add_test(NAME P8_RegressionTests COMMAND P8_RegressionTests)

set_ide_folder(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)