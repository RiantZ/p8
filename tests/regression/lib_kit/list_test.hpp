#include "list.hpp"

/// @brief Test fixture for c_lst
class CLstTest : public ::testing::Test
{
protected:
    kit::c_lst<int> lst;
};

TEST_F(CLstTest, ConstructorEmpty)
{
    EXPECT_EQ(lst.size(), 0);
    EXPECT_TRUE(lst.begin() == lst.end());
}

TEST_F(CLstTest, ConstructorArgs)
{
    lst = { 10, 20, 30, 40, 50, 60, 70, 80 };
    EXPECT_EQ(lst.size(), 8);

    int l_iSum = 0;
    for(auto v : lst)
    {
        l_iSum += v;
    }

    EXPECT_EQ(l_iSum, 360);
}

class CLstTestValInt : public ::testing::Test
{
protected:
    kit::c_lst<int> lst = { 10, 20, 30, 40, 50, 60, 70, 80 };
    ;
};

TEST_F(CLstTestValInt, Iterator)
{
    int l_iSum = 0;
    for(auto v : lst)
    {
        l_iSum += v;
    }

    EXPECT_EQ(l_iSum, 360);

    auto l_cIt = lst.begin();
    EXPECT_EQ(10, *l_cIt++);
    EXPECT_EQ(20, *l_cIt++);
    EXPECT_EQ(30, *l_cIt++);
    EXPECT_EQ(40, *l_cIt++);
    EXPECT_EQ(50, *l_cIt++);
    EXPECT_EQ(60, *l_cIt++);
    EXPECT_EQ(70, *l_cIt++);
    EXPECT_EQ(80, *l_cIt++);
    EXPECT_EQ(lst.end(), l_cIt);

    l_cIt = lst.end();

    EXPECT_EQ(80, *(--l_cIt));
    EXPECT_EQ(70, *(--l_cIt));
    EXPECT_EQ(60, *(--l_cIt));
    EXPECT_EQ(50, *(--l_cIt));
    EXPECT_EQ(40, *(--l_cIt));
    EXPECT_EQ(30, *(--l_cIt));
    EXPECT_EQ(20, *(--l_cIt));
    EXPECT_EQ(10, *(--l_cIt));

    auto l_cConstIt = lst.cbegin();

    EXPECT_EQ(10, *l_cConstIt++);
    EXPECT_EQ(20, *l_cConstIt++);
    EXPECT_EQ(30, *l_cConstIt++);
    EXPECT_EQ(40, *l_cConstIt++);
    EXPECT_EQ(50, *l_cConstIt++);
    EXPECT_EQ(60, *l_cConstIt++);
    EXPECT_EQ(70, *l_cConstIt++);
    EXPECT_EQ(80, *l_cConstIt++);
    EXPECT_EQ(lst.cend(), l_cConstIt);

    l_cConstIt = lst.cend();

    EXPECT_EQ(80, *(--l_cConstIt));
    EXPECT_EQ(70, *(--l_cConstIt));
    EXPECT_EQ(60, *(--l_cConstIt));
    EXPECT_EQ(50, *(--l_cConstIt));
    EXPECT_EQ(40, *(--l_cConstIt));
    EXPECT_EQ(30, *(--l_cConstIt));
    EXPECT_EQ(20, *(--l_cConstIt));
    EXPECT_EQ(10, *(--l_cConstIt));
}

class citem
{
public:
    int val;
    citem()
        : val(0)
    {
    }
    citem(citem &&other)
        : val(other.val)
    {
        other.val = -1;
    }
    citem(const citem &other)
        : val(other.val)
    {
    }
    citem(int i)
        : val(i)
    {
    }

    citem &operator=(citem &&other) noexcept
    {
        if(this != &other)
        {
            val       = other.val;
            // Release the data pointer from the source object so that
            // the destructor does not free the memory multiple times.
            other.val = -1;
        }
        return *this;
    }

    citem &operator=(const citem &other)
    {
        val = other.val;
        return *this;
    }
};

class CLstTestValCitem : public ::testing::Test
{
protected:
    kit::c_lst<citem> lst = { { 10 }, { 20 }, { 30 }, { 40 }, { 50 }, { 60 }, { 70 }, { 80 } };
};

TEST_F(CLstTestValCitem, Iterator)
{
    int l_iSum = 0;
    for(auto v : lst)
    {
        l_iSum += v.val;
    }

    EXPECT_EQ(l_iSum, 360);

    auto l_cIt = lst.begin();
    EXPECT_EQ(10, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(20, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(30, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(40, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(50, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(60, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(70, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(80, l_cIt->val);
    l_cIt++;
    EXPECT_EQ(lst.end(), l_cIt);

    l_cIt = --(lst.end());

    EXPECT_EQ(80, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(70, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(60, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(50, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(40, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(30, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(20, l_cIt->val);
    l_cIt--;
    EXPECT_EQ(10, l_cIt->val);
    l_cIt--;

    auto l_cConstIt = lst.cbegin();

    EXPECT_EQ(10, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(20, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(30, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(40, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(50, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(60, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(70, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(80, l_cConstIt->val);
    l_cConstIt++;
    EXPECT_EQ(lst.cend(), l_cConstIt);

    l_cConstIt = --(lst.cend());

    EXPECT_EQ(80, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(70, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(60, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(50, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(40, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(30, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(20, l_cConstIt->val);
    l_cConstIt--;
    EXPECT_EQ(10, l_cConstIt->val);
    l_cConstIt--;
}

TEST_F(CLstTestValCitem, PushFirstMove)
{
    citem item(100);
    lst.push_first(std::move(item));
    EXPECT_EQ(lst.size(), 9); // original 8 + 1
    EXPECT_EQ(lst.front().val, 100);
    EXPECT_EQ(item.val, -1);  // moved from
}

TEST_F(CLstTest, PullFirst)
{
    lst.push_last(1);
    lst.push_last(2);
    EXPECT_EQ(lst.pull_first(), 1);
    EXPECT_EQ(lst.size(), 1);
    EXPECT_EQ(lst.front(), 2);
    EXPECT_EQ(lst.back(), 2);
}

TEST_F(CLstTest, PullLast)
{
    lst.push_last(1);
    lst.push_last(2);
    EXPECT_EQ(lst.pull_last(), 2);
    EXPECT_EQ(lst.size(), 1);
    EXPECT_EQ(lst.front(), 1);
    EXPECT_EQ(lst.back(), 1);
}

TEST_F(CLstTest, Iterators)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(3);

    auto it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_TRUE(it == lst.end());
}

TEST_F(CLstTest, Clear)
{
    lst.push_last(1);
    lst.push_last(2);
    EXPECT_EQ(lst.size(), 2);
    lst.clear();
    EXPECT_EQ(lst.size(), 0);
    EXPECT_TRUE(lst.begin() == lst.end());
}

TEST_F(CLstTest, ClearWithCleanup)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(3);
    EXPECT_EQ(lst.size(), 3);

    std::vector<int> cleanedValues;
    lst.clear([&](int &val) { cleanedValues.push_back(val); });

    EXPECT_EQ(lst.size(), 0);
    EXPECT_TRUE(lst.begin() == lst.end());
    // Check that cleanup was called for all elements
    EXPECT_EQ(cleanedValues.size(), 3);
    EXPECT_EQ(cleanedValues[0], 1);
    EXPECT_EQ(cleanedValues[1], 2);
    EXPECT_EQ(cleanedValues[2], 3);
}

TEST_F(CLstTest, ClearWithCleanupEmpty)
{
    bool cleanupCalled = false;
    lst.clear(
        [&](int &val)
        {
            cleanupCalled = true;
            (void)(val);
        });
    EXPECT_FALSE(cleanupCalled);
    EXPECT_EQ(lst.size(), 0);
    EXPECT_TRUE(lst.begin() == lst.end());
}

TEST_F(CLstTestValCitem, ClearWithCleanupMove)
{
    // Test with citem class that tracks moves
    std::vector<int> originalValues;
    for(auto &item : lst)
    {
        originalValues.push_back(item.val);
    }
    EXPECT_EQ(originalValues.size(), 8); // Should have 8 elements

    std::vector<int> cleanedValues;
    lst.clear([&](citem &item) { cleanedValues.push_back(item.val); });

    EXPECT_EQ(lst.size(), 0);
    EXPECT_TRUE(lst.begin() == lst.end());
    // Check that cleanup was called for all elements
    EXPECT_EQ(cleanedValues.size(), 8);
    // Values should be in order: 10, 20, 30, 40, 50, 60, 70, 80
    EXPECT_EQ(cleanedValues[0], 10);
    EXPECT_EQ(cleanedValues[1], 20);
    EXPECT_EQ(cleanedValues[2], 30);
    EXPECT_EQ(cleanedValues[3], 40);
    EXPECT_EQ(cleanedValues[4], 50);
    EXPECT_EQ(cleanedValues[5], 60);
    EXPECT_EQ(cleanedValues[6], 70);
    EXPECT_EQ(cleanedValues[7], 80);
}

TEST_F(CLstTest, InitializerList)
{
    kit::c_lst<int> lst2 = { 1, 2, 3 };
    EXPECT_EQ(lst2.size(), 3);
    auto it = lst2.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
}

TEST_F(CLstTest, PushFirstEmpty)
{
    lst.push_first(42);
    EXPECT_EQ(lst.size(), 1);
    EXPECT_EQ(lst.front(), 42);
    EXPECT_EQ(lst.back(), 42);
}

TEST_F(CLstTest, PushFirstNonEmpty)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_first(0);
    EXPECT_EQ(lst.size(), 3);
    EXPECT_EQ(lst.front(), 0);
    EXPECT_EQ(lst.back(), 2);
    auto it = lst.begin();
    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST_F(CLstTest, PushFirstMultiple)
{
    lst.push_first(3);
    lst.push_first(2);
    lst.push_first(1);
    EXPECT_EQ(lst.size(), 3);
    EXPECT_EQ(lst.front(), 1);
    EXPECT_EQ(lst.back(), 3);
    auto it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
}

TEST_F(CLstTest, PushNextBegin)
{
    lst.push_last(1);
    lst.push_last(3);
    auto it    = lst.begin();
    auto newIt = lst.push_next(it, 2);
    EXPECT_EQ(lst.size(), 3);
    EXPECT_EQ(*newIt, 2);
    it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
}

TEST_F(CLstTest, PushNextMiddle)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(4);
    auto it = lst.begin();
    ++it; // points to 2
    auto newIt = lst.push_next(it, 3);
    EXPECT_EQ(lst.size(), 4);
    EXPECT_EQ(*newIt, 3);
    it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 4);
}

TEST_F(CLstTest, PushNextEnd)
{
    lst.push_last(1);
    lst.push_last(2);
    auto it    = lst.end();
    auto newIt = lst.push_next(it, 3);
    EXPECT_EQ(newIt, lst.end()); // Should return end() for invalid iterator
    EXPECT_EQ(lst.size(), 2);    // Size should not change
}

TEST_F(CLstTest, PushNextEmptyList)
{
    auto it    = lst.begin();
    auto newIt = lst.push_next(it, 1);
    EXPECT_EQ(newIt, lst.end()); // Should return end() for empty list
    EXPECT_EQ(lst.size(), 0);    // Size should not change
}

TEST_F(CLstTestValCitem, PushNextMove)
{
    citem item(50);
    auto  it   = lst.begin();
    // insert after first element (10)
    auto newIt = lst.push_next(it, std::move(item));
    EXPECT_EQ(lst.size(), 9); // original 8 + 1
    EXPECT_EQ((*newIt).val, 50);
    EXPECT_EQ(item.val, -1);  // moved from
    // Check order: 10, 50, 20, 30, 40, 50, 60, 70, 80
    it = lst.begin();
    EXPECT_EQ(it->val, 10);
    ++it;
    EXPECT_EQ(it->val, 50);
    ++it;
    EXPECT_EQ(it->val, 20);
}

TEST_F(CLstTest, RemoveBegin)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(3);
    auto it     = lst.begin();
    auto nextIt = lst.remove(it);
    EXPECT_EQ(lst.size(), 2);
    EXPECT_EQ(*nextIt, 2);
    it = lst.begin();
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 3);
}

TEST_F(CLstTest, RemoveMiddle)
{
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(3);
    lst.push_last(4);
    auto it = lst.begin();
    ++it; // points to 2
    auto nextIt = lst.remove(it);
    EXPECT_EQ(lst.size(), 3);
    EXPECT_EQ(*nextIt, 3);
    it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 4);
}

TEST_F(CLstTest, RemoveEnd)
{
    lst.push_last(1);
    lst.push_last(2);
    auto it     = lst.end();
    auto nextIt = lst.remove(it);
    EXPECT_EQ(nextIt, lst.end()); // Should return end() for invalid iterator
    EXPECT_EQ(lst.size(), 2);     // Size should not change
}

TEST_F(CLstTest, RemoveLastElement)
{
    lst.push_last(42);
    auto it     = lst.begin();
    auto nextIt = lst.remove(it);
    EXPECT_EQ(lst.size(), 0);
    EXPECT_EQ(nextIt, lst.end());
    EXPECT_TRUE(lst.begin() == lst.end());
}

TEST_F(CLstTest, RemoveSingleElement)
{
    lst.push_last(1);
    EXPECT_EQ(lst.size(), 1);
    auto it     = lst.begin();
    auto nextIt = lst.remove(it);
    EXPECT_EQ(lst.size(), 0);
    EXPECT_EQ(nextIt, lst.end());
}

TEST_F(CLstTest, RemoveWithCleanup)
{
    bool cleanupCalled = false;
    lst.push_last(1);
    lst.push_last(2);
    lst.push_last(3);
    auto it = lst.begin();
    ++it; // points to 2
    auto nextIt = lst.remove(it,
                             [&](int &val)
                             {
                                 cleanupCalled = true;
                                 EXPECT_EQ(val, 2);
                             });
    EXPECT_TRUE(cleanupCalled);
    EXPECT_EQ(lst.size(), 2);
    EXPECT_EQ(*nextIt, 3);
    it = lst.begin();
    EXPECT_EQ(*it, 1);
    ++it;
    EXPECT_EQ(*it, 3);
}
