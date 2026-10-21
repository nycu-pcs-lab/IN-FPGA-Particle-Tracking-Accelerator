# FGCS final Graph_M D20 / Graph_L D24 HLS designs

**GitHub upload package — final source and packaged C-simulation verified.**

**Project license and weight/sample redistribution rights still need confirmation.**
Source, weight and dataset redistribution status is described in LICENSE and NOTICE.md.
No public repository, DOI, commit or tag is claimed. VERSION and checksums identify this package; this is not a Git commit or tag.

Only the final two FPGA source configurations are included. This package does not
claim to reproduce every paper figure/table: the Fig. 7–10 design-space variants,
old configurations, CPU/GPU benchmarks, Fmax sweeps, all 40 graphs and plotting
records are outside this package.

## 中文快速開始

本 ZIP 可直接解壓為 GitHub repository 內容；README 下方附完整英文說明。
舊 release_candidate ZIP 的啟動參數傳法已修正，請使用本版本。

1. 先啟用你自己的 **Vitis HLS 2023.2** 環境（安裝目錄由使用者決定）。
2. 在解壓後的 repository 根目錄執行：

```bash
python3 scripts/check_package.py
bash scripts/run_hls.sh Graph_M csim
bash scripts/run_hls.sh Graph_L csim
```

兩個 C simulation 已實際通過；預期會印出 `VALID_DIFF=0`。Graph_M 只保存 graph0 一張圖，testbench 在記憶體中重播三次以符合原 batch 流程，
Graph_L 範例一張，完整執行 33 shards／1,649,533 slots。**沒有跑 synthesis 或 P&R**。
採用原始固定點 top function，並與既有 final RTL 的有效邊 raw outputs 比較，非 FP32 accuracy 重測。

GitHub 上傳時將此目錄內的檔案放在 repository 根目錄；`.build/` 與本機 logs 不必上傳。
如使用 Git，可先在本機 `git init`、`git add .`、`git commit`，再依自己的 GitHub remote 推送。
本包沒有代填公開 URL／作者 Git identity，也沒有建立真正的公開 commit、tag 或 DOI。

**授權待確認：**`LICENSE` 目前是待確認聲明，不是 MIT／Apache 開源授權；權重與
TrackML 衍生範例的公開再散布權利也尚未確認。本包提供程式與驗證結果，沒有代替作者作授權決定。

## Designs and clocks

| Design | Aggregation / MAC format | E_PAR / E2_PAR / NS_PAR | Node capacity | Edge slots | Final P&R clock |
| --- | --- | --- | --- | --- | --- |
| Graph_M D20 | ap_fixed<20,12> | 8 / 12 / 2 | 9,925 | 104,328 | 8 ns / 125 MHz |
| Graph_L D24 | ap_fixed<24,16> | 8 / 8 / 4 | 43,278 | 1,649,533 | 8.8 ns / 113.636364 MHz |

Both use MERGE_PAR=4, stored node/edge formats and weights/biases ap_fixed<16,8>,
and the original final output/sigmoid interface. L retains 50,000-slot shards,
33 shards, and a 49,533-slot final shard. Architecture, pipeline and arithmetic
source are unchanged, byte-for-byte. Device: xcvu9p-flga2104-2-e.

HLS generation used **5 ns**, Vitis HLS 2023.2, vitis flow target, C++14. The supplied
HLS script preserves that target. The final routed operating clocks are 8 / 8.8 ns,
recorded separately in config.json and constraints/final_clock.xdc. A 5 ns HLS target
is not an achieved 200 MHz claim. Archived final WNS is +0.045 ns (M), +0.023 ns (L).
provenance/final_design_provenance.json connects source hashes, exported-RTL hashes
and the corresponding final implementation records. Exported RTL/DCPs are not included.

## Directory structure

```
designs/Graph_M/{src,tb,constraints,config.json}
designs/Graph_L/{src,tb,constraints,config.json}
weights/                    18 active files, 528 parameters
samples/Graph_M/             1 real graph (graph0), input/mask/expected/metadata
samples/Graph_L/             1 real graph entry, input/mask/expected/metadata
testbench/compare_outputs.h  valid-edge bit-exact checker
scripts/run_hls.sh           portable launcher for Vitis HLS 2023.2
scripts/hls.tcl              C-sim, C-synthesis, co-sim, RTL/IP export
scripts/check_package.py     static checks; no FPGA tool execution
validation/                 successful packaged M/L C-sim results and output hashes
docs/                       input format, listing response, request checklist
provenance/                 source/build lineage and checksums
```

## Prerequisites

Use Linux, Vitis HLS 2023.2 and Vivado 2023.2/XSim with access to the target device
and required licenses. Source the settings scripts from your own installation;
no workstation installation paths or vendor license/header files are bundled.
Python 3 standard library is sufficient for the packaging checker. The HLS tools
provide ap_int.h, ap_fixed.h, hls_math.h and hls_stream.h; they are external dependencies.
Use a writable extraction directory. HLS simulations can consume substantial memory.

## Static integrity check

From the extracted package root:

```bash
python3 scripts/check_package.py
```

This checks SHA-256, local include closure, weight counts, sample dimensions,
endpoints/order/padding, expected lengths and scalar score range. It does not run
inference or establish C/RTL equivalence.

## HLS commands

The two `csim` commands below were executed successfully on the packaged samples
on 2026-10-21 using Vitis HLS 2023.2. Synthesis/co-simulation/export commands are
provided as instructions and were not executed in this verification. Run from the
package root after activating Vitis HLS 2023.2:

```bash
bash scripts/run_hls.sh Graph_M csim
bash scripts/run_hls.sh Graph_L csim
bash scripts/run_hls.sh Graph_M csynth
bash scripts/run_hls.sh Graph_L csynth
bash scripts/run_hls.sh Graph_M cosim
bash scripts/run_hls.sh Graph_L cosim
bash scripts/run_hls.sh Graph_M export
bash scripts/run_hls.sh Graph_L export
```

The launcher passes design/stage via environment variables because Vitis HLS 2023.2
does not forward trailing command-line tokens as Tcl argc/argv.
Each invocation creates/resets only its design project under .build/. `cosim`
first runs csynth, then Verilog C/RTL co-sim using XSim. `export` first runs csynth,
then exports RTL/IP. Optional stage `all` runs C-sim, csynth, co-sim and export.
Build reports are in .build/Graph_M/solution1/syn/report/ (and Graph_L respectively),
co-sim reports in solution1/sim/report/, and actual_raw.bin / exported_ip.zip in
.build/outputs/Graph_M or Graph_L. No implementation/place/route is invoked.

The TB reads packaged input and common weights, calls the actual unchanged top,
writes every configured score, then compares valid raw output bits. Expected
success prints VALID_DIFF=0; padding differences are reported separately and do
not fail. Expected scores were copied from existing final-precision RTL results,
not computed anew. Both portable testbenches have now passed compilation and
execution with VALID_DIFF=0. See validation/csim_summary.json. This sample check
does not establish correctness on every possible input or repeat the full accuracy study.

Graph_M stores only one graph0 input/mask/expected output. Its testbench replays
that single payload three times in memory to satisfy the unchanged minimum batch
pipeline, checks all three executions, and retains one graph of output on disk.
These are three executions of one sample, not three different dataset graphs.
Graph_L stores and executes one graph using all 33 shards. Neither sample test is
the paper's complete latency batch or a performance rerun.

## Scope and interpretation

- FPGA latency reported in the paper is RTL co-simulation cycles converted using
  the corresponding timing-closed clock, not measured board latency. It excludes
  host/XRT/PCIe overhead; do not use the 5 ns HLS simulation target as the final clock.
- Power is a Vivado vectorless post-route estimate (Medium confidence), not measured
  board power or a workload-covered activity estimate. No power measurement is run here.
- Reproducing resources/timing/power requires Vivado, appropriate licenses and full
  implementation constraints/settings. Only final clock XDCs and build lineage are
  supplied here; complete original P&R projects/physical XDCs are not part of this
  source package. Exact P&R reproduction and run-to-run variation are not established.
- L executes a fixed maximum edge/shard envelope; valid edges do not reduce it.
  Node-state tables must fit the compiled on-chip capacity. There is no node tiling,
  external-memory node cache or DDR node spilling in these final sources.
- Carry correctness requires destination-sorted input and reserved dummy tail.
  See docs/INPUT_FORMAT.md and docs/SOURCE_CODE_RESPONSE.md.
- Sample inputs and weights are included for local review, but redistribution rights
  are unresolved. This package must not be described as already public or open-source licensed.
  If either must be withheld, revise runnable-sample claims before public release.

## Code snippets and availability statement

See docs/SOURCE_CODE_RESPONSE.md for exact public-tree line numbers, complete
Graph_L aggregator and call sites, actual modulo read mapping and known unknowns.
No manuscript, tracking table or response letter was modified. A code-availability
statement needs an actual approved license, repository/version URL and confirmed
sample/weight distribution scope before it can claim availability.
