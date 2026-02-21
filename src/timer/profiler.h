#pragma once

#if HG_ENABLE_PROFILER

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <chrono>

// Data collected for each profiled function
struct FunctionProfile {
    std::string             name;
    long long               total_us   = 0;
    long long               min_us     = 0;
    long long               max_us     = 0;
    int                     call_count = 0;
    std::vector<long long>  samples;    // all individual call durations for median

    double avg_us()    const { return call_count > 0 ? (double)total_us / call_count : 0.0; }

    double median_us() const {
        if (samples.empty()) return 0.0;
        std::vector<long long> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        size_t mid = sorted.size() / 2;
        if (sorted.size() % 2 == 0) {
            return (sorted[mid - 1] + sorted[mid]) / 2.0;
        }
        return static_cast<double>(sorted[mid]);
    }
};

class Profiler {
public:
    static void record(const std::string& name, long long duration_us) {
        auto& p = _profiles[name];
        p.name = name;
        p.total_us += duration_us;
        p.call_count++;
        p.samples.push_back(duration_us);
        if (p.call_count == 1) {
            p.min_us = duration_us;
            p.max_us = duration_us;
        } else {
            p.min_us = std::min(p.min_us, duration_us);
            p.max_us = std::max(p.max_us, duration_us);
        }
    }

    static void report() {
        if (_profiles.empty()) {
            std::cout << "[PROFILER] No data recorded.\n";
            return;
        }

        // Sort by total time descending
        std::vector<FunctionProfile> sorted;
        sorted.reserve(_profiles.size());
        for (const auto& [name, profile] : _profiles) {
            sorted.push_back(profile);
        }
        std::sort(sorted.begin(), sorted.end(), [](const FunctionProfile& a, const FunctionProfile& b) {
            return a.total_us > b.total_us;
        });

        // Header
        std::cout << "\n[PROFILER] ========== Performance Report ==========\n";
        std::cout << std::left
                  << std::setw(45) << "Function"
                  << std::setw(10) << "Calls"
                  << std::setw(14) << "Total (ms)"
                  << std::setw(14) << "Avg (us)"
                  << std::setw(14) << "Median (us)"
                  << std::setw(14) << "Min (us)"
                  << std::setw(14) << "Max (us)"
                  << "\n"
                  << std::string(125, '-') << "\n";

        // Rows
        for (const auto& p : sorted) {
            std::cout << std::left
                      << std::setw(45) << p.name
                      << std::setw(10) << p.call_count
                      << std::setw(14) << std::fixed << std::setprecision(3) << p.total_us / 1000.0
                      << std::setw(14) << std::fixed << std::setprecision(1) << p.avg_us()
                      << std::setw(14) << std::fixed << std::setprecision(1) << p.median_us()
                      << std::setw(14) << p.min_us
                      << std::setw(14) << p.max_us
                      << "\n";
        }
        std::cout << "[PROFILER] ==========================================\n\n";
    }

    static void reset() { _profiles.clear(); }

    static const std::unordered_map<std::string, FunctionProfile>& getProfiles() { return _profiles; }

private:
    static std::unordered_map<std::string, FunctionProfile> _profiles;
};

inline std::unordered_map<std::string, FunctionProfile> Profiler::_profiles;

// RAII scoped timer
class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name)
        : _name(name)
        , _start(std::chrono::high_resolution_clock::now())
    {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        long long us = std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count();
        Profiler::record(_name, us);
    }

    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string _name;
    std::chrono::time_point<std::chrono::high_resolution_clock> _start;
};

// Macros
    #define HG_PROFILE_FUNCTION()      ScopedTimer _timer_func_##__LINE__(__func__)
    #define HG_PROFILE_SCOPE(name)     ScopedTimer _timer_scope_##__LINE__(name)
    #define HG_PROFILER_REPORT()       Profiler::report()
    #define HG_PROFILER_RESET()        Profiler::reset()
#else
    #define HG_PROFILE_FUNCTION()      ((void)0)
    #define HG_PROFILE_SCOPE(name)     ((void)0)
    #define HG_PROFILER_REPORT()       ((void)0)
    #define HG_PROFILER_RESET()        ((void)0)
#endif
