#include <gtest/gtest.h>

int main(int ii_argc, char *ip_argv[])
{
    ::testing::InitGoogleTest(&ii_argc, ip_argv);
    return RUN_ALL_TESTS();
}