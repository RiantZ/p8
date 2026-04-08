cmake_minimum_required(VERSION 3.20...4.0)

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

add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${P8_API} lib_kit)

target_link_libraries (${PROJECT_NAME} lib_kit gtest_main)

add_test(NAME P8_RegressionTests COMMAND P8_RegressionTests)

set_ide_folder(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)