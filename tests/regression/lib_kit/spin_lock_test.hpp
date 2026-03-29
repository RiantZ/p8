#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <synchapi.h>
#endif

#include <semaphore>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

#include "spin_lock.hpp"

#if defined(__linux__)
    #include <pthread.h>
#endif

using namespace std::chrono_literals;

// TEST(BaseTest, Test1)
//{
//
//     SRWLOCK SRWLock;
//     InitializeSRWLock(&SRWLock);
//
//     int32_t li_val = 0;
//     auto    begin  = std::chrono::high_resolution_clock::now();
//
//     for(int i = 0; i < 1'000'000; i++)
//     {
//         AcquireSRWLockExclusive(&SRWLock);
//         li_val += 1;
//         ReleaseSRWLockExclusive(&SRWLock);
//     }
//
//     auto end = std::chrono::high_resolution_clock::now();
//     std::cout << "NUL TIME: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() << "ns"
//               << std::endl;
// }

kit::c_spin_lock        gc_spinlock_t2;
std::mutex              gc_mutex_t2;
std::counting_semaphore gc_sem_t2 { 1 };

float get_miter_per_second(uint64_t                                              iu_iterations,
                           const std::chrono::high_resolution_clock::time_point &ir_begin,
                           const std::chrono::high_resolution_clock::time_point &ir_end)
{
    uint64_t lu_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ir_end - ir_begin).count();
    if(lu_time_ns)
    {
        return (float)(iu_iterations * 1'000'000'000 / lu_time_ns) / 1'000'000.0f;
    }
    else
    {
        return 0.0f;
    }
}

#pragma optimize("", off)
int32_t Sum_Million()
{
    int32_t li_val = 0;
    for(int i = 0; i < 1'000'000; i++)
    {
        li_val += 1;
    }
    return li_val;
}
#pragma optimize("", on)

TEST(SpinLockTests, Performance)
{
    std::cout << "Different lock mechanism perfromance tests" << std::endl;

    int32_t li_val = 0;

    auto begin     = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 1'000'000; i++)
    {
        gc_mutex_t2.lock();
        li_val++;
        gc_mutex_t2.unlock();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Mtx per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;

    begin = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < 1'000'000; i++)
    {
        gc_spinlock_t2.lock();
        li_val++;
        gc_spinlock_t2.unlock();
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Spl per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;

    begin = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 1'000'000; i++)
    {
        gc_sem_t2.acquire();
        li_val++;
        gc_sem_t2.release();
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Sem per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE lh_Mutex = CreateMutex(NULL, FALSE, NULL);

    begin           = std::chrono::high_resolution_clock::now();

    for(int i = 0; i < 1'000'000; i++)
    {
        WaitForSingleObject(lh_Mutex, INFINITE);
        li_val++;
        ReleaseMutex(lh_Mutex);
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "WND per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;

    CloseHandle(lh_Mutex);
#elif defined(__linux__)
    pthread_mutex_t i_Lock;

    pthread_mutexattr_t l_sAttr;
    memset(&l_sAttr, 0, sizeof(pthread_mutexattr_t));
    pthread_mutexattr_init(&l_sAttr);
    pthread_mutexattr_settype(&l_sAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&i_Lock, &l_sAttr);
    pthread_mutexattr_destroy(&l_sAttr);

    begin = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < 1'000'000; i++)
    {
        pthread_mutex_lock(&i_Lock);
        li_val++;
        pthread_mutex_unlock(&i_Lock);
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "LNX per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;

    pthread_mutex_destroy(&i_Lock);
#endif

    begin = std::chrono::high_resolution_clock::now();
    Sum_Million();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "NUL per sec.: " << get_miter_per_second(1'000'000, begin, end) << "M" << std::endl;
}

TEST(SpinLockTests, Concurency)
{
    int              li_test3 = 0;
    kit::c_spin_lock lc_test3;

    std::vector<std::thread> lc_threads;
    uint32_t                 lu_iterations  = 100'000;
    uint32_t                 lu_cores_count = std::thread::hardware_concurrency();

    for(size_t zI = 0; zI < lu_cores_count; zI++)
    {
        lc_threads.push_back(std::thread(
            [&]
            {
                for(uint32_t i = 0; i < lu_iterations; i++)
                {
                    lc_test3.lock();
                    li_test3++;
                    lc_test3.unlock();
                }
            }));
    }

    for(auto &t : lc_threads)
    {
        t.join();
    };

    EXPECT_EQ(li_test3, lu_iterations * lu_cores_count);
}
