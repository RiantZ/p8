#pragma once

#include <utility>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stddef.h>
#include <stdint.h>

namespace kit
{
/// @brief Constant iterator class for c_lst
/// @tparam t_owner The template type of the owner, typically c_lst
template <class t_owner> class c_lst_const_it
{
protected:
    template <class t_lst_value_type> friend class c_lst;

    using tp_node      = typename t_owner::tp_node;
    using t_value_type = typename t_owner::t_value_type;
    using t_pointer    = typename t_owner::t_const_pointer;
    using t_reference  = const t_value_type &;

    tp_node mp_iter;

public:
    /// @brief Default constructor
    c_lst_const_it() noexcept
        : mp_iter()
    {
    }

    /// @brief Constructs the constant iterator with a given node
    /// @param it_node Pointer to the list node
    c_lst_const_it(tp_node it_node) noexcept
        : mp_iter(it_node)
    {
    }

    /// @brief Arrow operator, provides const access to the data pointer
    /// @return Const pointer to the data in the node
    t_pointer operator->() const noexcept
    {
        return &mp_iter->mp_data;
    }

    /// @brief Dereference operator, provides const access to the data
    /// @return Const reference to the data in the node
    t_reference operator*() noexcept
    {
        return mp_iter->mp_data;
    }

    /// @brief Post-increment operator, advances to the next element
    /// @return Copy of the iterator before incrementing
    c_lst_const_it operator++(int) noexcept
    {
        c_lst_const_it lc_tmp = *this;
        mp_iter               = mp_iter->mp_next;
        return lc_tmp;
    }

    /// @brief Pre-increment operator, advances to the next element
    /// @return Reference to the iterator after incrementing
    c_lst_const_it &operator++() noexcept
    {
        mp_iter = mp_iter->mp_next;
        return *this;
    }

    /// @brief Equality comparison operator, compares node pointers
    /// @param ir_right Iterator to compare with
    /// @return True if iterators point to the same node
    bool operator==(const c_lst_const_it &ir_right) const noexcept
    {
        return mp_iter == ir_right.mp_iter;
    }

    /// @brief Inequality comparison operator, compares node pointers
    /// @param ir_right Iterator to compare with
    /// @return True if iterators point to different nodes
    bool operator!=(const c_lst_const_it &ir_right) const noexcept
    {
        return !(*this == ir_right);
    }

    /// @brief Post-decrement operator, moves to the previous element
    /// @return Copy of the iterator before decrementing
    c_lst_const_it operator--(int) noexcept
    {
        c_lst_const_it lc_tmp = *this;
        mp_iter               = mp_iter->mp_prev;
        return lc_tmp;
    }

    /// @brief Pre-decrement operator, moves to the previous element
    /// @return Reference to the iterator after decrementing
    c_lst_const_it &operator--() noexcept
    {
        mp_iter = mp_iter->mp_prev;
        return *this;
    }
};

/// @brief Iterator class for c_lst
/// @tparam t_owner The template type of the owner, typically c_lst
template <class t_owner> class c_lst_it
{
protected:
    template <class t_lst_value_type> friend class c_lst;

    using t_value_type = typename t_owner::t_value_type;
    using t_reference  = t_value_type &;
    using t_pointer    = typename t_owner::t_pointer;
    using tp_node      = typename t_owner::tp_node;

    tp_node mp_iter;

public:
    /// @brief Constructs the iterator with a given node
    /// @param it_node Pointer to the list node
    c_lst_it(tp_node it_node) noexcept
        : mp_iter(it_node)
    {
    }

    /// @brief Default constructor
    c_lst_it() noexcept
        : mp_iter()
    {
    }

    /// @brief Post-increment operator, advances to the next element
    /// @return Copy of the iterator before incrementing
    c_lst_it operator++(int) noexcept
    {
        c_lst_it lc_tmp = *this;
        mp_iter         = mp_iter->mp_next;
        return lc_tmp;
    }

    /// @brief Pre-increment operator, advances to the next element
    /// @return Reference to the iterator after incrementing
    c_lst_it &operator++() noexcept
    {
        mp_iter = mp_iter->mp_next;
        return *this;
    }

    /// @brief Post-decrement operator, moves to the previous element
    /// @return Copy of the iterator before decrementing
    c_lst_it operator--(int) noexcept
    {
        c_lst_it lc_tmp = *this;
        mp_iter         = mp_iter->mp_prev;
        return lc_tmp;
    }

    /// @brief Pre-decrement operator, moves to the previous element
    /// @return Reference to the iterator after decrementing
    c_lst_it &operator--() noexcept
    {
        mp_iter = mp_iter->mp_prev;
        return *this;
    }

    /// @brief Equality comparison operator, compares node pointers
    /// @param ir_right Iterator to compare with
    /// @return True if iterators point to the same node
    bool operator==(const c_lst_it &ir_right) const noexcept
    {
        return mp_iter == ir_right.mp_iter;
    }

    /// @brief Inequality comparison operator, compares node pointers
    /// @param ir_right Iterator to compare with
    /// @return True if iterators point to different nodes
    bool operator!=(const c_lst_it &ir_right) const noexcept
    {
        return !(*this == ir_right);
    }

    /// @brief Dereference operator, provides access to the data
    /// @return Reference to the data in the node
    t_reference operator*() noexcept
    {
        return mp_iter->mp_data;
    }

    /// @brief Arrow operator, provides access to the data pointer
    /// @return Pointer to the data in the node
    t_pointer operator->() const noexcept
    {
        return &mp_iter->mp_data;
    }
};

/// @brief A doubly-linked list container with custom memory pooling for efficient allocation
/// @tparam t_lst_value_type The type of elements stored in the list
template <typename t_lst_value_type> class c_lst
{
    template <class t_lst_it> friend class c_lst_it;

    template <class t_lst_const_it> friend class c_lst_const_it;

protected:
    /// @brief Structure representing a node in the doubly-linked list
    struct s_item
    {
        t_lst_value_type mp_data; ///< The data stored in the node
        s_item          *mp_next; ///< Pointer to the next node
        s_item          *mp_prev; ///< Pointer to the previous node
    };

private:
    /// @brief Structure representing a segment of memory pool for nodes
    struct s_pool_block
    {
        s_item       *mp_cells; ///< Array of nodes in this segment
        size_t        mz_count; ///< Number of nodes in this segment
        s_pool_block *mp_next;  ///< Pointer to the next pool segment
    };

    s_pool_block *mp_pool_head;
    s_item       *mp_pool_item_first;
    size_t        mz_pool_size;
    size_t        mz_count;
    s_item        mo_null_item;
    s_item       *mp_first;
    s_item       *mp_last;

    using t_pointer         = t_lst_value_type *;
    using t_const_pointer   = const t_lst_value_type *;
    using t_value_type      = t_lst_value_type;
    using t_reference       = t_value_type &;
    using t_const_reference = const t_value_type &;
    using tp_node           = s_item *;
    using t_iterator        = c_lst_it<c_lst<t_lst_value_type>>;
    using t_const_iterator  = c_lst_const_it<c_lst<t_lst_value_type>>;

public:
    /// @brief Default constructor for the list
    /// @param iz_pool_init_size Size of the memory pool in elements
    c_lst(size_t iz_pool_init_size = 64)
        : mz_pool_size(iz_pool_init_size)
    {
        mo_null_item.mp_next = nullptr;
        mo_null_item.mp_prev = nullptr;
        mp_first             = &mo_null_item;
        mp_last              = &mo_null_item;
        mz_count             = 0;
        mp_pool_head         = nullptr;
        mp_pool_item_first   = nullptr;
    }

    /// @brief Copy constructor
    /// @param ir_other The list to copy from
    c_lst(const c_lst &ir_other)
        : c_lst(ir_other.mz_pool_size)
    {
        for(auto it = ir_other.cbegin(); it != ir_other.cend(); ++it)
        {
            push_last(*it);
        }
    }

    /// @brief Move constructor
    /// @param ir_other The list to move from
    c_lst(c_lst &&iv_other) noexcept
    {
        mp_pool_head       = iv_other.mp_pool_head;
        mp_pool_item_first = iv_other.mp_pool_item_first;
        mz_pool_size       = iv_other.mz_pool_size;
        mz_count           = iv_other.mz_count;

        if(iv_other.mz_count == 0)
        {
            mp_first             = &mo_null_item;
            mp_last              = &mo_null_item;
            mo_null_item.mp_next = nullptr;
            mo_null_item.mp_prev = nullptr;
        }
        else
        {
            mp_first             = iv_other.mp_first;
            mp_last              = iv_other.mp_last;
            mp_first->mp_prev    = &mo_null_item;
            mp_last->mp_next     = &mo_null_item;
            mo_null_item.mp_next = mp_first;
            mo_null_item.mp_prev = mp_last;
        }

        iv_other.mp_pool_head         = nullptr;
        iv_other.mp_pool_item_first   = nullptr;
        iv_other.mz_count             = 0;
        iv_other.mp_first             = &iv_other.mo_null_item;
        iv_other.mp_last              = &iv_other.mo_null_item;
        iv_other.mo_null_item.mp_next = nullptr;
        iv_other.mo_null_item.mp_prev = nullptr;
    }

    /// @brief Constructor initializing the list with values from an initializer list
    /// @param ic_init Initializer list containing the values
    c_lst(std::initializer_list<t_lst_value_type> ic_init)
        : c_lst()
    {
        for(auto v : ic_init)
        {
            push_last(v);
        }
    }

    /// @brief Copy assignment operator
    /// @param ir_other The list to copy from
    /// @return Reference to this list
    c_lst &operator=(const c_lst &ir_other)
    {
        if(this != &ir_other)
        {
            clear();
            for(auto it = ir_other.cbegin(); it != ir_other.cend(); ++it)
            {
                push_last(*it);
            }
        }
        return *this;
    }

    /// @brief Destructor, cleans up allocated memory
    virtual ~c_lst()
    {
        s_pool_block *lp_pool_tmp = nullptr;
        while(mp_pool_head)
        {
            lp_pool_tmp  = mp_pool_head;
            mp_pool_head = mp_pool_head->mp_next;

            free_pool_block(lp_pool_tmp);
        }

        mp_pool_head       = nullptr;
        mp_pool_item_first = nullptr;
        mp_first           = &mo_null_item;
        mp_last            = &mo_null_item;
        mz_count           = 0;
    }

    /// @brief Returns a const reference to the first element
    /// @return Const reference to the first element's data
    inline t_const_reference front() const noexcept
    {
        assert(mz_count != 0 && "Empty list!");

        return mp_first->mp_data;
    }

    /// @brief Returns a reference to the first element
    /// @return Reference to the first element's data
    inline t_reference front() noexcept
    {
        assert(mz_count != 0 && "Empty list!");

        return mp_first->mp_data;
    }

    /// @brief Returns a const reference to the last element
    /// @return Const reference to the last element's data
    inline t_const_reference back() const noexcept
    {
        assert(mz_count != 0 && "Empty list!");

        return mp_last->mp_data;
    }

    /// @brief Returns a reference to the last element
    /// @return Reference to the last element's data
    inline t_reference back() noexcept
    {
        assert(mz_count != 0 && "Empty list!");

        return mp_last->mp_data;
    }

    /// @brief Returns a constant iterator to the beginning of the list
    /// @return Constant iterator pointing to the first element
    t_const_iterator cbegin() const noexcept
    {
        return t_const_iterator(mp_first);
    }

    /// @brief Returns a constant iterator to the end of the list (past the last element)
    /// @return Constant iterator pointing to the end sentinel
    t_const_iterator cend() const noexcept
    {
        return t_const_iterator(const_cast<s_item *>(&mo_null_item));
    }

    /// @brief Returns an iterator to the beginning of the list
    /// @return Iterator pointing to the first element
    t_iterator begin() noexcept
    {
        return t_iterator(mp_first);
    }

    /// @brief Returns an iterator to the end of the list (past the last element)
    /// @return Iterator pointing to the end sentinel
    t_iterator end() noexcept
    {
        return t_iterator(&mo_null_item);
    }

    /// @brief Removes and returns the first element from the list
    /// @return The data from the first element
    inline t_lst_value_type pull_first()
    {
        assert(mz_count != 0 && "Empty list!");

        t_reference lr_return = mp_first->mp_data;

        s_item *lp_cell       = mp_first;

        if(mp_first == mp_last)
        {
            mp_first             = &mo_null_item;
            mp_last              = &mo_null_item;
            mo_null_item.mp_next = nullptr;
            mo_null_item.mp_prev = nullptr;
        }
        else
        {
            mp_first             = mp_first->mp_next;
            mp_first->mp_prev    = &mo_null_item;
            mo_null_item.mp_next = mp_first;
        }

        free_item(lp_cell);
        --mz_count;
        return std::move(lr_return);
    }

    /// @brief Removes and returns the last element from the list
    /// @return The data from the last element
    inline t_lst_value_type pull_last()
    {
        assert(mz_count != 0 && "Empty list!");

        t_reference lr_return = mp_last->mp_data;

        s_item *lp_cell       = mp_last;

        if(mp_first == mp_last)
        {
            mp_first             = &mo_null_item;
            mp_last              = &mo_null_item;
            mo_null_item.mp_next = nullptr;
            mo_null_item.mp_prev = nullptr;
        }
        else
        {
            mp_last              = mp_last->mp_prev;
            mp_last->mp_next     = &mo_null_item;
            mo_null_item.mp_prev = mp_last;
        }

        free_item(lp_cell);
        --mz_count;
        return std::move(lr_return);
    }

    /// @brief Adds a new element to the end of the list using move semantics
    /// @param iv_data The data to move into the list
    inline void push_last(t_lst_value_type &&iv_data)
    {
        s_item *lp_new_cell = allocate_item();

        assert(lp_new_cell != nullptr && "Allocation failed!");

        lp_new_cell->mp_data = std::move(iv_data);

        if(mp_last != &mo_null_item)
        {
            mp_last->mp_next     = lp_new_cell;
            lp_new_cell->mp_prev = mp_last;
            lp_new_cell->mp_next = &mo_null_item;
            mo_null_item.mp_prev = lp_new_cell;
            mp_last              = lp_new_cell;
        }
        else
        {
            mp_first             = lp_new_cell;
            mp_last              = lp_new_cell;
            lp_new_cell->mp_next = &mo_null_item;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mo_null_item.mp_prev = lp_new_cell;
        }

        mz_count++;
    }

    /// @brief Adds a new element to the end of the list
    /// @param iv_data The data to add
    inline void push_last(const t_lst_value_type &ir_data)
    {
        s_item *lp_new_cell = allocate_item();

        assert(lp_new_cell != nullptr && "Allocation failed!");

        lp_new_cell->mp_data = ir_data;

        if(mp_last != &mo_null_item)
        {
            mp_last->mp_next     = lp_new_cell;
            lp_new_cell->mp_prev = mp_last;
            lp_new_cell->mp_next = &mo_null_item;
            mo_null_item.mp_prev = lp_new_cell;
            mp_last              = lp_new_cell;
        }
        else
        {
            mp_first             = lp_new_cell;
            mp_last              = lp_new_cell;
            lp_new_cell->mp_next = &mo_null_item;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mo_null_item.mp_prev = lp_new_cell;
        }

        mz_count++;
    }

    /// @brief Adds a new element to the beginning of the list
    /// @param iv_data The data to add
    inline void push_first(const t_lst_value_type &ir_data)
    {
        s_item *lp_new_cell = allocate_item();

        assert(lp_new_cell != nullptr && "Allocation failed!");

        lp_new_cell->mp_data = ir_data;

        if(mp_first != &mo_null_item)
        {
            mp_first->mp_prev    = lp_new_cell;
            lp_new_cell->mp_next = mp_first;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mp_first             = lp_new_cell;
        }
        else
        {
            mp_first             = lp_new_cell;
            mp_last              = lp_new_cell;
            lp_new_cell->mp_next = &mo_null_item;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mo_null_item.mp_prev = lp_new_cell;
        }

        mz_count++;
    }

    /// @brief Adds a new element to the beginning of the list using move semantics
    /// @param iv_data The data to move into the list
    inline void push_first(t_lst_value_type &&ir_data)
    {
        s_item *lp_new_cell = allocate_item();

        assert(lp_new_cell != nullptr && "Allocation failed!");

        lp_new_cell->mp_data = std::move(ir_data);

        if(mp_first != &mo_null_item)
        {
            mp_first->mp_prev    = lp_new_cell;
            lp_new_cell->mp_next = mp_first;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mp_first             = lp_new_cell;
        }
        else
        {
            mp_first             = lp_new_cell;
            mp_last              = lp_new_cell;
            lp_new_cell->mp_next = &mo_null_item;
            lp_new_cell->mp_prev = &mo_null_item;
            mo_null_item.mp_next = lp_new_cell;
            mo_null_item.mp_prev = lp_new_cell;
        }

        mz_count++;
    }

    /// @brief Removes all elements from the list and applies a cleanup function to each
    /// @param if_callback Lambda function to handle resource cleanup for each element's data
    template <class tlFn> inline void clear(tlFn il_callback)
    {
        while(&mo_null_item != mp_first)
        {
            s_item *lp_cell = mp_first;
            mp_first        = mp_first->mp_next;
            il_callback(lp_cell->mp_data);
            free_item(lp_cell);
        }

        mp_first                  = &mo_null_item;
        mp_last                   = &mo_null_item;
        mz_count                  = 0;

        // remove all segments, expect last one
        s_pool_block *lp_pool_tmp = nullptr;
        while((mp_pool_head) && (mp_pool_head->mp_next))
        {
            lp_pool_tmp  = mp_pool_head;
            mp_pool_head = mp_pool_head->mp_next;

            free_pool_block(lp_pool_tmp);
        }
    }

    /// @brief Removes all elements from the list
    inline void clear()
    {
        while(&mo_null_item != mp_first)
        {
            s_item *lp_cell = mp_first;
            mp_first        = mp_first->mp_next;
            free_item(lp_cell);
        }

        mp_first                  = &mo_null_item;
        mp_last                   = &mo_null_item;
        mz_count                  = 0;

        // remove all segments, expect last one
        s_pool_block *lp_pool_tmp = nullptr;
        while((mp_pool_head) && (mp_pool_head->mp_next))
        {
            lp_pool_tmp  = mp_pool_head;
            mp_pool_head = mp_pool_head->mp_next;

            free_pool_block(lp_pool_tmp);
        }
    }

    /// @brief Returns the number of elements in the list
    /// @return The size of the list
    inline size_t size()
    {
        return mz_count;
    }

    /// @brief Removes the element pointed to by the iterator and applies a cleanup function
    /// @param ir_it Iterator pointing to the element to remove
    /// @param if_callback Lambda function to handle resource cleanup for the element's data
    /// @return Iterator pointing to the next element after removal
    template <class tlFn> inline t_iterator remove(const t_iterator &ir_it, tlFn il_callback)
    {
        if(ir_it.mp_iter != &mo_null_item)
        {
            il_callback(ir_it.mp_iter->mp_data);
        }
        return remove(ir_it);
    }

    /// @brief Removes the element pointed to by the iterator
    /// @param ir_it Iterator pointing to the element to remove
    /// @return Iterator pointing to the next element after removal
    inline t_iterator remove(const t_iterator &ir_it)
    {
        if(ir_it.mp_iter == &mo_null_item)
        {
            return t_iterator(&mo_null_item);
        }

        s_item *lp_cell           = ir_it.mp_iter;
        s_item *lp_next           = lp_cell->mp_next;

        lp_cell->mp_prev->mp_next = lp_cell->mp_next;
        lp_cell->mp_next->mp_prev = lp_cell->mp_prev;

        if(lp_cell == mp_first)
        {
            mp_first = lp_cell->mp_next;
        }

        if(lp_cell == mp_last)
        {
            mp_last = lp_cell->mp_prev;
        }

        free_item(lp_cell);

        mz_count--;

        return t_iterator(lp_next);
    }

    /// @brief Inserts a new element after the specified iterator position using move semantics
    /// @param ir_it Iterator pointing to the element after which to insert
    /// @param iv_data The data to move into the list
    /// @return Iterator pointing to the newly inserted element
    inline t_iterator push_next(const t_iterator &ir_it, t_lst_value_type &&iv_data)
    {
        s_item *lp_new_cell = allocate_item();

        if((!lp_new_cell) || (ir_it.mp_iter == &mo_null_item))
        {
            return t_iterator(&mo_null_item);
        }

        s_item *lp_cur_cell = ir_it.mp_iter;

        mz_count++;
        lp_new_cell->mp_data = std::move(iv_data);

        if(lp_cur_cell)
        {
            lp_new_cell->mp_next          = lp_cur_cell->mp_next;
            lp_new_cell->mp_prev          = lp_cur_cell;
            lp_cur_cell->mp_next->mp_prev = lp_new_cell;
            lp_cur_cell->mp_next          = lp_new_cell;
        }

        if(mp_last == lp_cur_cell)
        {
            mp_last = lp_new_cell;
        }

        return t_iterator(lp_new_cell);
    }

    /// @brief Inserts a new element after the specified iterator position
    /// @param ir_it Iterator pointing to the element after which to insert
    /// @param iv_data The data to insert
    /// @return Iterator pointing to the newly inserted element
    inline t_iterator push_next(const t_iterator &ir_it, const t_lst_value_type &ir_data)
    {
        s_item *lp_new_cell = allocate_item();

        if((!lp_new_cell) || (ir_it.mp_iter == &mo_null_item))
        {
            return t_iterator(&mo_null_item);
        }

        s_item *lp_cur_cell = ir_it.mp_iter;

        mz_count++;
        lp_new_cell->mp_data = ir_data;

        if(lp_cur_cell)
        {
            lp_new_cell->mp_next          = lp_cur_cell->mp_next;
            lp_new_cell->mp_prev          = lp_cur_cell;
            lp_cur_cell->mp_next->mp_prev = lp_new_cell;
            lp_cur_cell->mp_next          = lp_new_cell;
        }

        if(mp_last == lp_cur_cell)
        {
            mp_last = lp_new_cell;
        }

        return t_iterator(lp_new_cell);
    }

private:
    /// @brief Allocates a new segment of memory for the pool
    /// @return True if allocation succeeded
    bool allocate_pool_block()
    {
        bool          lb_return = false;
        s_pool_block *lp_pool   = (s_pool_block *)malloc(sizeof(s_pool_block));

        if(lp_pool)
        {
            memset(lp_pool, 0, sizeof(s_pool_block));
            lp_pool->mz_count = mz_pool_size;
            lp_pool->mp_cells = (s_item *)malloc(sizeof(s_item) * lp_pool->mz_count);

            if(lp_pool->mp_cells)
            {
                s_item *lp_cell = nullptr;

                memset(lp_pool->mp_cells, 0, sizeof(s_item) * lp_pool->mz_count);

                lp_cell = lp_pool->mp_cells;

                for(uint32_t lu_idx = 1; lu_idx < lp_pool->mz_count; lu_idx++)
                {
                    lp_cell->mp_next = (lp_cell + 1);
                    lp_cell++;
                }

                lb_return          = true;
                lp_pool->mp_next   = mp_pool_head;
                mp_pool_head       = lp_pool;
                lp_cell->mp_next   = mp_pool_item_first;
                mp_pool_item_first = lp_pool->mp_cells;
            }
        }

        if(false == lb_return)
        {
            free_pool_block(lp_pool);
        }

        return lb_return;
    }

    /// @brief Releases memory for a pool segment
    /// @param iop_Pool Pointer to the pool segment to free
    void free_pool_block(s_pool_block *iop_pool)
    {
        if(iop_pool)
        {
            if(iop_pool->mp_cells)
            {
                free(iop_pool->mp_cells);
                iop_pool->mp_cells = nullptr;
            }
            free(iop_pool);
        }
    }

    /// @brief Allocates a new cell from the memory pool
    /// @return Pointer to the allocated cell
    s_item *allocate_item()
    {
        s_item *lp_return = nullptr;
        if(nullptr == mp_pool_item_first)
        {
            allocate_pool_block();
        }

        lp_return = mp_pool_item_first;

        if(mp_pool_item_first)
        {
            mp_pool_item_first = mp_pool_item_first->mp_next;
        }

        return lp_return;
    }

    /// @brief Returns a cell back to the memory pool
    /// @param i_pCell Pointer to the cell to free
    void free_item(s_item *ip_cell)
    {
        ip_cell->mp_prev   = nullptr;
        ip_cell->mp_next   = mp_pool_item_first;
        mp_pool_item_first = ip_cell;
    }
};
}
