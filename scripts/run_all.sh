#!/usr/bin/env bash
# Run the full experiment suite (or a quick smoke subset) for all four pairs.
#
#   scripts/run_all.sh [smoke|quick|full] [results-dir] [core-to-pin]
#
# smoke  : ~1 minute, verifies the pipeline end to end
# quick  : ~10-20 minutes, coarse sweeps, 3 reps  (good for a first look)
# refine : ~10 minutes, fine grid around the crossovers only, 5 reps
# patch  : ~1 hour, re-measures the large-input cells (see note in the case)
# full   : hours, the sweeps used in the paper, 7 reps
#
# Output: <results-dir>/<pair>.csv, <results-dir>/<pair>.log (stderr incl.
# [cell] lines), <results-dir>/sysinfo.txt, <results-dir>/build_flags.txt
set -euo pipefail
PROFILE="${1:-quick}"
OUT="${2:-results/$(date +%Y%m%d-%H%M%S)-$PROFILE}"
PIN="${3:-}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build}"   # override to benchmark a differently-configured build
mkdir -p "$OUT"

PINARG=()
if [[ -n "$PIN" ]]; then PINARG=(--pin "$PIN"); fi

case "$PROFILE" in
  smoke)
    REPS=2; WARM=1; MAXS=5
    SORT_SIZES="64:1000000:4";     SORT_DISTS="uniform64,uniform16";          SORT_PARAMS="8"
    MM_SIZES="32,64,128,256";      MM_DISTS="uniform";                        MM_PARAMS="32,64"
    BI_SIZES="8:65536:4";          BI_DISTS="uniform";                        BI_PARAMS="16,32"
    DJ_SIZES="1000:1000000:10";    DJ_DISTS="random,grid";                    DJ_PARAMS="4,32"
    ;;
  quick)
    REPS=3; WARM=1; MAXS=20
    SORT_SIZES="16:100000000:4";   SORT_DISTS="uniform64,uniform32,uniform16,nearly_sorted,few_unique"; SORT_PARAMS="8,11"
    MM_SIZES="16,32,64,128,256,512,1024,2048"; MM_DISTS="uniform";            MM_PARAMS="16,32,64,128"
    BI_SIZES="8:1048576:2";        BI_DISTS="uniform";                        BI_PARAMS="8,16,32,64"
    DJ_SIZES="100:10000000:4";     DJ_DISTS="random,grid,dense";              DJ_PARAMS="4,16,64"
    ;;
  full)
    # ~4 h on a 2024 laptop core. Factor-2 grids, 5 timed reps after 2 warm-ups.
    REPS=5; WARM=2; MAXS=90
    SORT_SIZES="16:100000000:2";   SORT_DISTS="uniform64,uniform32,uniform16,uniform8,nearly_sorted,reverse,few_unique,organ_pipe"; SORT_PARAMS="8,11,16"
    MM_SIZES="16,24,32,48,64,96,128,192,256,384,512,768,1024,1536,2048,3072,4096"; MM_DISTS="uniform,integer"; MM_PARAMS="8,16,32,64,128,256"
    BI_SIZES="8:1048576:1.5";      BI_DISTS="uniform,ones";                   BI_PARAMS="8,16,32,64,128"
    DJ_SIZES="100:10000000:2";     DJ_DISTS="random,grid,dense";              DJ_PARAMS="2,4,16,64,256"
    ;;
  refine)
    # Dense sweep of the crossover neighbourhoods only (factor-1.19 grid,
    # 5 reps). Pool with the quick runs or analyse alone.
    REPS=5; WARM=1; MAXS=20
    SORT_SIZES="512:16384:1.19";   SORT_DISTS="uniform64,uniform32,uniform16,few_unique"; SORT_PARAMS="8,11"
    MM_SIZES="16:512:1.19";        MM_DISTS="uniform";                        MM_PARAMS="16,32,64,128"
    BI_SIZES="16:512:1.19";        BI_DISTS="uniform";                        BI_PARAMS="8,16,32,64"
    DJ_SIZES="400:8192:1.19";      DJ_DISTS="dense";                          DJ_PARAMS="4"
    ;;
  patch)
    # Re-measures the cells of the full+refine runs in which the input copy
    # was a sizeable fraction of the raw time (harness v1 over-subtracted the
    # copy there). Analyse with --supersede so these replace the old cells.
    REPS=5; WARM=2; MAXS=90
    SORT_SIZES="2097152:100000000:2"; SORT_DISTS="uniform64,uniform32,uniform16,uniform8,nearly_sorted,reverse,few_unique,organ_pipe"; SORT_PARAMS="8,11,16"
    MM_SIZES="96:256:1.19";        MM_DISTS="uniform,integer";                MM_PARAMS="8,16,32,64,128,256"
    BI_SIZES="8:32:1.19";          BI_DISTS="uniform,ones";                   BI_PARAMS="8,16,32,64,128"
    DJ_SIZES="100:8192:1.19";      DJ_DISTS="dense";                          DJ_PARAMS="4"
    DJ_EXTRA_SIZES="1048576:10000000:2"; DJ_EXTRA_DISTS="random,grid";       DJ_EXTRA_PARAMS="2,4,16,64,256"
    ;;
  *) echo "unknown profile $PROFILE"; exit 2;;
esac

echo "[run_all] profile=$PROFILE out=$OUT pin=${PIN:-none}"
"$ROOT/scripts/collect_sysinfo.sh" > "$OUT/sysinfo.txt" 2>/dev/null || true
cp "$BIN/build_flags.txt" "$OUT/" 2>/dev/null || true
echo "profile=$PROFILE reps=$REPS warmup=$WARM max_seconds=$MAXS" > "$OUT/profile.txt"

run() {
  local pair=$1; shift
  echo "[run_all] $pair: $*"
  "$BIN/bench_$pair" "$@" --reps "$REPS" --warmup "$WARM" --max-seconds "$MAXS" "${PINARG[@]}" \
      --verbose --out "$OUT/$pair.csv" 2> "$OUT/$pair.log"
  grep -c . "$OUT/$pair.csv" | sed "s/^/[run_all] $pair rows: /"
}

run sorting  --sizes "$SORT_SIZES" --dists "$SORT_DISTS" --params "$SORT_PARAMS"
run matmul   --sizes "$MM_SIZES"   --dists "$MM_DISTS"   --params "$MM_PARAMS"
run bigint   --sizes "$BI_SIZES"   --dists "$BI_DISTS"   --params "$BI_PARAMS"
run dijkstra --sizes "$DJ_SIZES"   --dists "$DJ_DISTS"   --params "$DJ_PARAMS"
if [[ -n "${DJ_EXTRA_SIZES:-}" ]]; then
  echo "[run_all] dijkstra (extra): --sizes $DJ_EXTRA_SIZES --dists $DJ_EXTRA_DISTS"
  "$BIN/bench_dijkstra" --sizes "$DJ_EXTRA_SIZES" --dists "$DJ_EXTRA_DISTS" --params "$DJ_EXTRA_PARAMS" \
      --reps "$REPS" --warmup "$WARM" --max-seconds "$MAXS" "${PINARG[@]}" --verbose \
      --out "$OUT/dijkstra_extra.csv" 2> "$OUT/dijkstra_extra.log"
fi

echo "[run_all] done -> $OUT"
