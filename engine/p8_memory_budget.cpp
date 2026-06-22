#include "p8_memory_budget.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
cp8_memory_budget::cp8_memory_budget(size_t iz_max_bytes)
    : mz_max(iz_max_bytes)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool cp8_memory_budget::try_reserve(size_t iz_bytes)
{
    // CAS loop — succeeds when the candidate value still fits under the cap.
    size_t lz_current = mu_used.load(std::memory_order_relaxed);
    while(true)
    {
        if(lz_current > mz_max || mz_max - lz_current < iz_bytes)
        {
            return false;
        }

        if(mu_used.compare_exchange_weak(lz_current,
                                         lz_current + iz_bytes,
                                         std::memory_order_acq_rel,
                                         std::memory_order_relaxed))
        {
            return true;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cp8_memory_budget::release(size_t iz_bytes)
{
    mu_used.fetch_sub(iz_bytes, std::memory_order_acq_rel);
}
