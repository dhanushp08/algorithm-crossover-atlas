#!/usr/bin/env bash
# Dump everything about the machine that the paper's "experimental setup"
# section needs. Safe to run anywhere; missing tools are simply skipped.
echo "=== date ==="; date -u +"%Y-%m-%dT%H:%M:%SZ"
echo "=== uname ==="; uname -a
echo "=== lscpu ==="; lscpu 2>/dev/null
echo "=== memory ==="; free -h 2>/dev/null
echo "=== compiler ==="; g++ --version 2>/dev/null | head -1; cmake --version 2>/dev/null | head -1
echo "=== frequency governor ==="
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "n/a"
echo "=== turbo ==="
cat /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null | sed 's/^/intel no_turbo=/' || \
cat /sys/devices/system/cpu/cpufreq/boost 2>/dev/null | sed 's/^/amd boost=/' || echo "n/a"
echo "=== perf_event_paranoid ==="; cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "n/a"
echo "=== virtualization ==="; systemd-detect-virt 2>/dev/null || grep -m1 -o hypervisor /proc/cpuinfo || echo "none detected"
echo "=== kernel cmdline ==="; cat /proc/cmdline 2>/dev/null
echo "=== load ==="; cat /proc/loadavg 2>/dev/null
