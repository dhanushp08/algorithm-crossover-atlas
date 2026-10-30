#!/usr/bin/env bash
# Put a bare-metal Linux machine into a steady state for benchmarking.
# Requires root. Reversible: run with "restore" to undo.
#
#   sudo scripts/setup_machine.sh steady    # fixed frequency, no turbo, perf enabled
#   sudo scripts/setup_machine.sh restore
#
# On WSL2 or any VM these knobs do not exist; the script says so and exits 0.
# For WSL2 the equivalent is done on the Windows side (see docs/BENCHMARKING.md).
set -u
MODE="${1:-steady}"
if [[ ! -d /sys/devices/system/cpu/cpu0/cpufreq ]]; then
  echo "[setup] no cpufreq interface (VM/WSL2?). Nothing to do here; see docs/BENCHMARKING.md."
  exit 0
fi
if [[ $EUID -ne 0 ]]; then echo "run as root"; exit 1; fi

if [[ "$MODE" == "steady" ]]; then
  # 1. performance governor on all cores
  for g in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo performance > "$g"; done
  # 2. disable turbo
  if [[ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]]; then
    echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo
  elif [[ -f /sys/devices/system/cpu/cpufreq/boost ]]; then
    echo 0 > /sys/devices/system/cpu/cpufreq/boost
  fi
  # 3. pin min = max frequency where the driver allows it
  for c in /sys/devices/system/cpu/cpu*/cpufreq; do
    if [[ -f "$c/scaling_max_freq" && -f "$c/scaling_min_freq" ]]; then
      cat "$c/scaling_max_freq" > "$c/scaling_min_freq" 2>/dev/null || true
    fi
  done
  # 4. allow user-space perf counters
  sysctl -q kernel.perf_event_paranoid=1
  # 5. drop ASLR? (no: it changes cache-set mapping run to run, but that is
  #    exactly the variance we want the repetitions to average over.)
  echo "[setup] governor=performance, turbo off, min=max freq, perf_event_paranoid=1"
  echo "[setup] current frequencies (kHz):"; grep -H . /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
elif [[ "$MODE" == "restore" ]]; then
  for g in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do echo powersave > "$g" 2>/dev/null || echo schedutil > "$g"; done
  [[ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]] && echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo
  [[ -f /sys/devices/system/cpu/cpufreq/boost ]] && echo 1 > /sys/devices/system/cpu/cpufreq/boost
  for c in /sys/devices/system/cpu/cpu*/cpufreq; do
    [[ -f "$c/cpuinfo_min_freq" ]] && cat "$c/cpuinfo_min_freq" > "$c/scaling_min_freq" 2>/dev/null
  done
  sysctl -q kernel.perf_event_paranoid=2
  echo "[setup] restored defaults"
else
  echo "usage: $0 steady|restore"; exit 2
fi
