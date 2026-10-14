#ifndef SOR_SLEEP_UTIL_H
#define SOR_SLEEP_UTIL_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>

// Spi podany czas, ale budzi sie co 50 ms i konczy od razu, gdy symulacja zostaje zatrzymana.
template <class Rep, class Period>
inline void sleepWhileRunning(const std::atomic<bool>& running, std::chrono::duration<Rep, Period> d)
{
    using namespace std::chrono;
    const auto end = steady_clock::now() + duration_cast<steady_clock::duration>(d);
    while (running.load()) {
        const auto now = steady_clock::now();
        if (now >= end) return;
        std::this_thread::sleep_for(std::min<steady_clock::duration>(end - now, milliseconds(50)));
    }
}

#endif // SOR_SLEEP_UTIL_H
