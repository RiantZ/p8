#include "list.hpp"

/// @brief Test fixture for c_lst
class c_lst_test : public ::testing::Test
{
protected:
    kit::c_lst<int> mc_lst;
};

TEST_F(c_lst_test, ConstructorEmpty)
{
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

TEST_F(c_lst_test, ConstructorArgs)
{
    mc_lst = { 10, 20, 30, 40, 50, 60, 70, 80 };
    EXPECT_EQ(mc_lst.size(), 8);

    int li_sum = 0;
    for(auto v : mc_lst)
    {
        li_sum += v;
    }

    EXPECT_EQ(li_sum, 360);
}

class c_lst_test_val_int : public ::testing::Test
{
protected:
    kit::c_lst<int> mc_lst = { 10, 20, 30, 40, 50, 60, 70, 80 };
};

TEST_F(c_lst_test_val_int, Iterator)
{
    int li_sum = 0;
    for(auto v : mc_lst)
    {
        li_sum += v;
    }

    EXPECT_EQ(li_sum, 360);

    auto lc_it = mc_lst.begin();
    EXPECT_EQ(10, *lc_it++);
    EXPECT_EQ(20, *lc_it++);
    EXPECT_EQ(30, *lc_it++);
    EXPECT_EQ(40, *lc_it++);
    EXPECT_EQ(50, *lc_it++);
    EXPECT_EQ(60, *lc_it++);
    EXPECT_EQ(70, *lc_it++);
    EXPECT_EQ(80, *lc_it++);
    EXPECT_EQ(mc_lst.end(), lc_it);

    lc_it = mc_lst.end();

    EXPECT_EQ(80, *(--lc_it));
    EXPECT_EQ(70, *(--lc_it));
    EXPECT_EQ(60, *(--lc_it));
    EXPECT_EQ(50, *(--lc_it));
    EXPECT_EQ(40, *(--lc_it));
    EXPECT_EQ(30, *(--lc_it));
    EXPECT_EQ(20, *(--lc_it));
    EXPECT_EQ(10, *(--lc_it));

    auto lc_const_it = mc_lst.cbegin();

    EXPECT_EQ(10, *lc_const_it++);
    EXPECT_EQ(20, *lc_const_it++);
    EXPECT_EQ(30, *lc_const_it++);
    EXPECT_EQ(40, *lc_const_it++);
    EXPECT_EQ(50, *lc_const_it++);
    EXPECT_EQ(60, *lc_const_it++);
    EXPECT_EQ(70, *lc_const_it++);
    EXPECT_EQ(80, *lc_const_it++);
    EXPECT_EQ(mc_lst.cend(), lc_const_it);

    lc_const_it = mc_lst.cend();

    EXPECT_EQ(80, *(--lc_const_it));
    EXPECT_EQ(70, *(--lc_const_it));
    EXPECT_EQ(60, *(--lc_const_it));
    EXPECT_EQ(50, *(--lc_const_it));
    EXPECT_EQ(40, *(--lc_const_it));
    EXPECT_EQ(30, *(--lc_const_it));
    EXPECT_EQ(20, *(--lc_const_it));
    EXPECT_EQ(10, *(--lc_const_it));
}

class c_item
{
public:
    int mi_val;
    c_item()
        : mi_val(0)
    {
    }
    c_item(c_item &&iv_other)
        : mi_val(iv_other.mi_val)
    {
        iv_other.mi_val = -1;
    }
    c_item(const c_item &ir_other)
        : mi_val(ir_other.mi_val)
    {
    }
    c_item(int ii_val)
        : mi_val(ii_val)
    {
    }

    c_item &operator=(c_item &&iv_other) noexcept
    {
        if(this != &iv_other)
        {
            mi_val          = iv_other.mi_val;
            // Release the data pointer from the source object so that
            // the destructor does not free the memory multiple times.
            iv_other.mi_val = -1;
        }
        return *this;
    }

    c_item &operator=(const c_item &ir_other)
    {
        mi_val = ir_other.mi_val;
        return *this;
    }
};

class c_lst_test_val_citem : public ::testing::Test
{
protected:
    kit::c_lst<c_item> mc_lst = { { 10 }, { 20 }, { 30 }, { 40 }, { 50 }, { 60 }, { 70 }, { 80 } };
};

TEST_F(c_lst_test_val_citem, Iterator)
{
    int li_sum = 0;
    for(auto v : mc_lst)
    {
        li_sum += v.mi_val;
    }

    EXPECT_EQ(li_sum, 360);

    auto lc_it = mc_lst.begin();
    EXPECT_EQ(10, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(20, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(30, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(40, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(50, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(60, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(70, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(80, lc_it->mi_val);
    lc_it++;
    EXPECT_EQ(mc_lst.end(), lc_it);

    lc_it = --(mc_lst.end());

    EXPECT_EQ(80, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(70, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(60, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(50, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(40, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(30, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(20, lc_it->mi_val);
    lc_it--;
    EXPECT_EQ(10, lc_it->mi_val);
    lc_it--;

    auto lc_const_it = mc_lst.cbegin();

    EXPECT_EQ(10, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(20, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(30, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(40, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(50, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(60, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(70, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(80, lc_const_it->mi_val);
    lc_const_it++;
    EXPECT_EQ(mc_lst.cend(), lc_const_it);

    lc_const_it = --(mc_lst.cend());

    EXPECT_EQ(80, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(70, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(60, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(50, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(40, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(30, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(20, lc_const_it->mi_val);
    lc_const_it--;
    EXPECT_EQ(10, lc_const_it->mi_val);
    lc_const_it--;
}

TEST_F(c_lst_test_val_citem, PushFirstMove)
{
    c_item lo_item(100);
    mc_lst.push_first(std::move(lo_item));
    EXPECT_EQ(mc_lst.size(), 9);   // original 8 + 1
    EXPECT_EQ(mc_lst.front().mi_val, 100);
    EXPECT_EQ(lo_item.mi_val, -1); // moved from
}

TEST_F(c_lst_test, PullFirst)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    EXPECT_EQ(mc_lst.pull_first(), 1);
    EXPECT_EQ(mc_lst.size(), 1);
    EXPECT_EQ(mc_lst.front(), 2);
    EXPECT_EQ(mc_lst.back(), 2);
}

TEST_F(c_lst_test, PullLast)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    EXPECT_EQ(mc_lst.pull_last(), 2);
    EXPECT_EQ(mc_lst.size(), 1);
    EXPECT_EQ(mc_lst.front(), 1);
    EXPECT_EQ(mc_lst.back(), 1);
}

TEST_F(c_lst_test, Iterators)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);

    auto lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
    ++lc_it;
    EXPECT_TRUE(lc_it == mc_lst.end());
}

TEST_F(c_lst_test, Clear)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    EXPECT_EQ(mc_lst.size(), 2);
    mc_lst.clear();
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

TEST_F(c_lst_test, ClearWithCleanup)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);
    EXPECT_EQ(mc_lst.size(), 3);

    std::vector<int> lc_cleaned_values;
    mc_lst.clear([&](int &val) { lc_cleaned_values.push_back(val); });

    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
    // Check that cleanup was called for all elements
    EXPECT_EQ(lc_cleaned_values.size(), 3);
    EXPECT_EQ(lc_cleaned_values[0], 1);
    EXPECT_EQ(lc_cleaned_values[1], 2);
    EXPECT_EQ(lc_cleaned_values[2], 3);
}

TEST_F(c_lst_test, ClearWithCleanupEmpty)
{
    bool lb_cleanup_called = false;
    mc_lst.clear(
        [&](int &val)
        {
            lb_cleanup_called = true;
            (void)(val);
        });
    EXPECT_FALSE(lb_cleanup_called);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

TEST_F(c_lst_test_val_citem, ClearWithCleanupMove)
{
    // Test with c_item class that tracks moves
    std::vector<int> lc_original_values;
    for(auto &ro_item : mc_lst)
    {
        lc_original_values.push_back(ro_item.mi_val);
    }
    EXPECT_EQ(lc_original_values.size(), 8); // Should have 8 elements

    std::vector<int> lc_cleaned_values;
    mc_lst.clear([&](c_item &ro_item) { lc_cleaned_values.push_back(ro_item.mi_val); });

    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
    // Check that cleanup was called for all elements
    EXPECT_EQ(lc_cleaned_values.size(), 8);
    // Values should be in order: 10, 20, 30, 40, 50, 60, 70, 80
    EXPECT_EQ(lc_cleaned_values[0], 10);
    EXPECT_EQ(lc_cleaned_values[1], 20);
    EXPECT_EQ(lc_cleaned_values[2], 30);
    EXPECT_EQ(lc_cleaned_values[3], 40);
    EXPECT_EQ(lc_cleaned_values[4], 50);
    EXPECT_EQ(lc_cleaned_values[5], 60);
    EXPECT_EQ(lc_cleaned_values[6], 70);
    EXPECT_EQ(lc_cleaned_values[7], 80);
}

TEST_F(c_lst_test, InitializerList)
{
    kit::c_lst<int> lc_lst2 = { 1, 2, 3 };
    EXPECT_EQ(lc_lst2.size(), 3);
    auto lc_it = lc_lst2.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
}

TEST_F(c_lst_test, PushFirstEmpty)
{
    mc_lst.push_first(42);
    EXPECT_EQ(mc_lst.size(), 1);
    EXPECT_EQ(mc_lst.front(), 42);
    EXPECT_EQ(mc_lst.back(), 42);
}

TEST_F(c_lst_test, PushFirstNonEmpty)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_first(0);
    EXPECT_EQ(mc_lst.size(), 3);
    EXPECT_EQ(mc_lst.front(), 0);
    EXPECT_EQ(mc_lst.back(), 2);
    auto lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 0);
    ++lc_it;
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
}

TEST_F(c_lst_test, PushFirstMultiple)
{
    mc_lst.push_first(3);
    mc_lst.push_first(2);
    mc_lst.push_first(1);
    EXPECT_EQ(mc_lst.size(), 3);
    EXPECT_EQ(mc_lst.front(), 1);
    EXPECT_EQ(mc_lst.back(), 3);
    auto lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
}

TEST_F(c_lst_test, PushNextBegin)
{
    mc_lst.push_last(1);
    mc_lst.push_last(3);
    auto lc_it     = mc_lst.begin();
    auto lc_new_it = mc_lst.push_next(lc_it, 2);
    EXPECT_EQ(mc_lst.size(), 3);
    EXPECT_EQ(*lc_new_it, 2);
    lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
}

TEST_F(c_lst_test, PushNextMiddle)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(4);
    auto lc_it = mc_lst.begin();
    ++lc_it; // points to 2
    auto lc_new_it = mc_lst.push_next(lc_it, 3);
    EXPECT_EQ(mc_lst.size(), 4);
    EXPECT_EQ(*lc_new_it, 3);
    lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
    ++lc_it;
    EXPECT_EQ(*lc_it, 4);
}

TEST_F(c_lst_test, PushNextEnd)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    auto lc_it     = mc_lst.end();
    auto lc_new_it = mc_lst.push_next(lc_it, 3);
    EXPECT_EQ(lc_new_it, mc_lst.end()); // Should return end() for invalid iterator
    EXPECT_EQ(mc_lst.size(), 2);        // Size should not change
}

TEST_F(c_lst_test, PushNextEmptyList)
{
    auto lc_it     = mc_lst.begin();
    auto lc_new_it = mc_lst.push_next(lc_it, 1);
    EXPECT_EQ(lc_new_it, mc_lst.end()); // Should return end() for empty list
    EXPECT_EQ(mc_lst.size(), 0);        // Size should not change
}

TEST_F(c_lst_test_val_citem, PushNextMove)
{
    c_item lo_item(50);
    auto   lc_it   = mc_lst.begin();
    // insert after first element (10)
    auto lc_new_it = mc_lst.push_next(lc_it, std::move(lo_item));
    EXPECT_EQ(mc_lst.size(), 9);   // original 8 + 1
    EXPECT_EQ((*lc_new_it).mi_val, 50);
    EXPECT_EQ(lo_item.mi_val, -1); // moved from
    // Check order: 10, 50, 20, 30, 40, 50, 60, 70, 80
    lc_it = mc_lst.begin();
    EXPECT_EQ(lc_it->mi_val, 10);
    ++lc_it;
    EXPECT_EQ(lc_it->mi_val, 50);
    ++lc_it;
    EXPECT_EQ(lc_it->mi_val, 20);
}

TEST_F(c_lst_test, RemoveBegin)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);
    auto lc_it      = mc_lst.begin();
    auto lc_next_it = mc_lst.remove(lc_it);
    EXPECT_EQ(mc_lst.size(), 2);
    EXPECT_EQ(*lc_next_it, 2);
    lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
}

TEST_F(c_lst_test, RemoveMiddle)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);
    mc_lst.push_last(4);
    auto lc_it = mc_lst.begin();
    ++lc_it; // points to 2
    auto lc_next_it = mc_lst.remove(lc_it);
    EXPECT_EQ(mc_lst.size(), 3);
    EXPECT_EQ(*lc_next_it, 3);
    lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
    ++lc_it;
    EXPECT_EQ(*lc_it, 4);
}

TEST_F(c_lst_test, RemoveEnd)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    auto lc_it      = mc_lst.end();
    auto lc_next_it = mc_lst.remove(lc_it);
    EXPECT_EQ(lc_next_it, mc_lst.end()); // Should return end() for invalid iterator
    EXPECT_EQ(mc_lst.size(), 2);         // Size should not change
}

TEST_F(c_lst_test, RemoveLastElement)
{
    mc_lst.push_last(42);
    auto lc_it      = mc_lst.begin();
    auto lc_next_it = mc_lst.remove(lc_it);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_EQ(lc_next_it, mc_lst.end());
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

TEST_F(c_lst_test, RemoveSingleElement)
{
    mc_lst.push_last(1);
    EXPECT_EQ(mc_lst.size(), 1);
    auto lc_it      = mc_lst.begin();
    auto lc_next_it = mc_lst.remove(lc_it);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_EQ(lc_next_it, mc_lst.end());
}

TEST_F(c_lst_test, RemoveWithCleanup)
{
    bool lb_cleanup_called = false;
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);
    auto lc_it = mc_lst.begin();
    ++lc_it; // points to 2
    auto lc_next_it = mc_lst.remove(lc_it,
                                    [&](int &val)
                                    {
                                        lb_cleanup_called = true;
                                        EXPECT_EQ(val, 2);
                                    });
    EXPECT_TRUE(lb_cleanup_called);
    EXPECT_EQ(mc_lst.size(), 2);
    EXPECT_EQ(*lc_next_it, 3);
    lc_it = mc_lst.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
}

// --- Copy constructor ---

TEST_F(c_lst_test, CopyConstructor)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);

    kit::c_lst<int> lc_copy(mc_lst);

    EXPECT_EQ(lc_copy.size(), 3);

    auto lc_it = lc_copy.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);

    // Verify independence: modifying original does not affect copy
    mc_lst.push_last(4);
    EXPECT_EQ(mc_lst.size(), 4);
    EXPECT_EQ(lc_copy.size(), 3);
}

TEST_F(c_lst_test, CopyConstructorEmpty)
{
    kit::c_lst<int> lc_copy(mc_lst);
    EXPECT_EQ(lc_copy.size(), 0);
    EXPECT_TRUE(lc_copy.begin() == lc_copy.end());
}

// --- Move constructor ---

TEST_F(c_lst_test, MoveConstructor)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);
    mc_lst.push_last(3);

    kit::c_lst<int> lc_moved(std::move(mc_lst));

    EXPECT_EQ(lc_moved.size(), 3);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());

    auto lc_it = lc_moved.begin();
    EXPECT_EQ(*lc_it, 1);
    ++lc_it;
    EXPECT_EQ(*lc_it, 2);
    ++lc_it;
    EXPECT_EQ(*lc_it, 3);
    ++lc_it;
    EXPECT_TRUE(lc_it == lc_moved.end());
}

TEST_F(c_lst_test, MoveConstructorEmpty)
{
    kit::c_lst<int> lc_moved(std::move(mc_lst));
    EXPECT_EQ(lc_moved.size(), 0);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(lc_moved.begin() == lc_moved.end());
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

// --- push_last move overload with tracked type ---

TEST_F(c_lst_test_val_citem, PushLastMove)
{
    c_item lo_item(100);
    mc_lst.push_last(std::move(lo_item));
    EXPECT_EQ(mc_lst.size(), 9);   // original 8 + 1
    EXPECT_EQ(mc_lst.back().mi_val, 100);
    EXPECT_EQ(lo_item.mi_val, -1); // moved from
}

// --- pull_first / pull_last on single-element list ---

TEST_F(c_lst_test, PullFirstSingleElement)
{
    mc_lst.push_last(42);
    EXPECT_EQ(mc_lst.pull_first(), 42);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

TEST_F(c_lst_test, PullLastSingleElement)
{
    mc_lst.push_last(42);
    EXPECT_EQ(mc_lst.pull_last(), 42);
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());
}

// --- const front() and back() ---

TEST_F(c_lst_test, FrontBackConst)
{
    mc_lst.push_last(10);
    mc_lst.push_last(20);
    mc_lst.push_last(30);

    const kit::c_lst<int> &lrc_lst = mc_lst;
    EXPECT_EQ(lrc_lst.front(), 10);
    EXPECT_EQ(lrc_lst.back(), 30);
}

// --- Pool growth (adding more elements than initial pool size) ---

TEST_F(c_lst_test, PoolGrowth)
{
    // Default pool size is 64; add 200 elements to force multiple reallocations
    const int li_count = 200;
    for(int i = 0; i < li_count; ++i)
    {
        mc_lst.push_last(i);
    }

    EXPECT_EQ(mc_lst.size(), static_cast<size_t>(li_count));

    int li_expected = 0;
    for(auto v : mc_lst)
    {
        EXPECT_EQ(v, li_expected);
        ++li_expected;
    }

    // Verify clear after growth works correctly
    mc_lst.clear();
    EXPECT_EQ(mc_lst.size(), 0);
    EXPECT_TRUE(mc_lst.begin() == mc_lst.end());

    // Verify list is usable after clear
    mc_lst.push_last(999);
    EXPECT_EQ(mc_lst.size(), 1);
    EXPECT_EQ(mc_lst.front(), 999);
}

// --- remove(it, callback) with end() iterator ---

TEST_F(c_lst_test, RemoveWithCleanupEndIterator)
{
    mc_lst.push_last(1);
    mc_lst.push_last(2);

    bool lb_cleanup_called = false;
    auto lc_it             = mc_lst.end();
    auto lc_next_it        = mc_lst.remove(lc_it, [&](int &) { lb_cleanup_called = true; });

    EXPECT_FALSE(lb_cleanup_called);
    EXPECT_EQ(lc_next_it, mc_lst.end());
    EXPECT_EQ(mc_lst.size(), 2);
}
