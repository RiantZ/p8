#include <gtest/gtest.h>
#include "list_test.hpp"

/// @brief
/// @param i_iArgC
/// @param i_pArgV
/// @return
int main(int i_iArgC, char *i_pArgV[])
{
    (void)(i_iArgC);
    (void)(i_pArgV);

    ::testing::InitGoogleTest(&i_iArgC, i_pArgV);
    return RUN_ALL_TESTS();
}