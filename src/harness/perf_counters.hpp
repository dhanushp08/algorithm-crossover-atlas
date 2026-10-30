// Hardware performance counters via the Linux perf_event_open(2) syscall.
//
// Each counter is opened individually (not as a group) so that a machine
// which supports only a subset still yields the rest. When the syscall is
// unavailable (containers, WSL2, non-Linux, perf_event_paranoid too high)
// every counter reports `available() == false` and reads return -1; the
// benchmark then degrades gracefully to timing-only.
//
// Counts are scaled by time_enabled/time_running to correct for multiplexing
// when the kernel has to share a limited number of physical PMU slots.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#if defined(__linux__)
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace harness {

struct CounterSpec {
    const char* name;
    uint32_t type;
    uint64_t config;
};

// The canonical set used throughout the study. Names are the CSV column names.
inline const std::vector<CounterSpec>& default_counter_specs() {
    static const std::vector<CounterSpec> specs = {
#if defined(__linux__)
        {"cycles", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES},
        {"instructions", PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS},
        {"l1d_misses", PERF_TYPE_HW_CACHE,
         PERF_COUNT_HW_CACHE_L1D | (PERF_COUNT_HW_CACHE_OP_READ << 8) |
             (PERF_COUNT_HW_CACHE_RESULT_MISS << 16)},
        {"llc_misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES},
        {"branch_misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES},
#endif
    };
    return specs;
}

class PerfCounters {
public:
    explicit PerfCounters(const std::vector<CounterSpec>& specs = default_counter_specs())
        : specs_(specs) {
#if defined(__linux__)
        for (const auto& s : specs_) {
            struct perf_event_attr pe;
            std::memset(&pe, 0, sizeof(pe));
            pe.type = s.type;
            pe.size = sizeof(pe);
            pe.config = s.config;
            pe.disabled = 1;
            pe.exclude_kernel = 1;
            pe.exclude_hv = 1;
            pe.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
            long fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
            fds_.push_back((int)fd);
            if (fd >= 0) any_ = true;
        }
#else
        fds_.assign(specs_.size(), -1);
#endif
        values_.assign(specs_.size(), -1);
    }

    ~PerfCounters() {
#if defined(__linux__)
        for (int fd : fds_)
            if (fd >= 0) close(fd);
#endif
    }

    PerfCounters(const PerfCounters&) = delete;
    PerfCounters& operator=(const PerfCounters&) = delete;

    bool available() const { return any_; }
    bool counter_available(size_t i) const { return fds_[i] >= 0; }
    const std::vector<CounterSpec>& specs() const { return specs_; }

    void start() {
#if defined(__linux__)
        for (int fd : fds_) {
            if (fd < 0) continue;
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        }
#endif
    }

    void stop() {
#if defined(__linux__)
        for (size_t i = 0; i < fds_.size(); ++i) {
            int fd = fds_[i];
            if (fd < 0) { values_[i] = -1; continue; }
            ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            uint64_t buf[3] = {0, 0, 0};  // value, time_enabled, time_running
            ssize_t r = read(fd, buf, sizeof(buf));
            if (r != (ssize_t)sizeof(buf)) { values_[i] = -1; continue; }
            double scale = (buf[2] > 0) ? (double)buf[1] / (double)buf[2] : 1.0;
            values_[i] = (int64_t)((double)buf[0] * scale);
        }
#endif
    }

    // -1 means "not measured".
    int64_t value(size_t i) const { return values_[i]; }
    const std::vector<int64_t>& values() const { return values_; }

    std::string availability_report() const {
        std::string s;
        for (size_t i = 0; i < specs_.size(); ++i) {
            s += specs_[i].name;
            s += fds_[i] >= 0 ? ":ok " : ":unavailable ";
        }
        if (specs_.empty()) s = "(no hardware counters on this platform)";
        return s;
    }

private:
    std::vector<CounterSpec> specs_;
    std::vector<int> fds_;
    std::vector<int64_t> values_;
    bool any_ = false;
};

}  // namespace harness
