#include <gtest/gtest.h>
#include "list_test.hpp"

/// @brief
/// @param i_iArgC
/// @param i_pArgV
/// @return
int main(int ii_argc, char *ip_argv[])
{
    (void)(ii_argc);
    (void)(ip_argv);

    ::testing::InitGoogleTest(&ii_argc, ip_argv);
    return RUN_ALL_TESTS();
}