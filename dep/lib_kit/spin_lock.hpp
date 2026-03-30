#pragma once

#include <atomic>
#include <mutex>
#include "plasma.hpp"

// https://rigtorp.se/spinlock/
namespace kit
{
/// @brief A recursive spin lock implementation.
///
/// This class provides a spin lock that supports recursive locking by the same thread.
/// It uses an atomic flag for synchronization and tracks the owning thread and recursion depth.
class c_spin_lock
{
    /// @brief Atomic flag indicating if the lock is held.
    std::atomic<bool> mc_flag    = { 0 };
    /// @brief ID of the thread that holds the lock.
    std::thread::id mc_thread_id = std::thread::id();
    /// @brief Recursion count for the owning thread.
    uint32_t mu_recursion        = 0;

public:
    /// @brief Default constructor.
    ///
    /// Initializes the spin lock in an unlocked state.
    c_spin_lock()
    {
    }

    c_spin_lock(const c_spin_lock &) = delete;

    /// @brief Virtual destructor.
    ///
    /// Releases any resources held by the spin lock.
    virtual ~c_spin_lock()
    {
    }

    /// @brief Acquires the lock.
    ///
    /// This function will spin until the lock is acquired. Supports recursive locking
    /// by the same thread. If the lock is already held by the current thread, it increments
    /// the recursion count instead of spinning.
    inline void lock() noexcept
    {
        for(;;)
        {
            // Optimistically assume the lock is free on the first try
            if(!mc_flag.exchange(true, std::memory_order_acquire))
            {
                mc_thread_id = std::this_thread::get_id();
                mu_recursion = 1;
                return;
            }

            // Recursive entry
            if(std::this_thread::get_id() == mc_thread_id)
            {
                mu_recursion++;
                return;
            }

            // Wait for lock to be released without generating cache misses
            while(mc_flag.load(std::memory_order_relaxed))
            {
                plasma_spin_pause();
            }
        }
    }

    /// @brief Attempts to acquire the lock without blocking.
    /// @return true if the lock was successfully acquired, false otherwise.
    ///
    /// This function performs a non-blocking attempt to acquire the lock.
    inline bool try_lock() noexcept
    {
        if(!mc_flag.load(std::memory_order_relaxed) && !mc_flag.exchange(true, std::memory_order_acquire))
        {
            mc_thread_id = std::this_thread::get_id();
            mu_recursion = 1;
            return true;
        }

        // Recursive entry
        if(std::this_thread::get_id() == mc_thread_id)
        {
            mu_recursion++;
            return true;
        }

        return false;
    }

    /// @brief Releases the lock.
    ///
    /// Decrements the recursion count. If the count reaches zero, releases the lock
    /// by setting the atomic flag to false. Asserts if called by a thread that does not own the lock.
    inline void unlock() noexcept
    {
        if(std::this_thread::get_id() != mc_thread_id)
        {
            assert("Thread ID missmatch");
        }

        if(0 == --mu_recursion)
        {
            mc_flag.store(false, std::memory_order_release);
        }

        mc_thread_id = std::thread::id();
    }
};
}