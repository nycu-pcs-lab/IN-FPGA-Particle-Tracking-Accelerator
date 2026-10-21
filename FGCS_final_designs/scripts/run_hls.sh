#!/usr/bin/env bash
set -euo pipefail
if (( $# < 1 || $# > 2 )); then
    echo "Usage: bash scripts/run_hls.sh Graph_M|Graph_L [csim|csynth|cosim|export|all]" >&2
    exit 2
fi
case "$1" in Graph_M|Graph_L) ;; *) echo "Unknown design" >&2; exit 2;; esac
stage="${2:-csim}"
case "$stage" in csim|csynth|cosim|export|all) ;; *) echo "Unknown stage" >&2; exit 2;; esac
command -v vitis_hls >/dev/null || { echo "Activate your Vitis HLS 2023.2 environment first." >&2; exit 127; }
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export FGCS_DESIGN="$1"
export FGCS_STAGE="$stage"
mkdir -p "$root/.build/driver/$FGCS_DESIGN"
cd "$root/.build/driver/$FGCS_DESIGN"
exec vitis_hls -f "$root/scripts/hls.tcl"
