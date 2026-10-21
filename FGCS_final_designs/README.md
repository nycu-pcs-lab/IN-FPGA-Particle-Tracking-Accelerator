# FGCS final Graph_M D20 / Graph_L D24 HLS designs

**GitHub upload package — final source and packaged C-simulation verified.**

**Project license and weight/sample redistribution rights still need confirmation.**
Source, weight and dataset redistribution status is described in LICENSE and NOTICE.md.
No public repository, DOI, commit or tag is claimed. VERSION and checksums identify this package; this is not a Git commit or tag.

Only the final two FPGA source configurations are included. This package does not
claim to reproduce every paper figure/table: the Fig. 7–10 design-space variants,
old configurations, CPU/GPU benchmarks, Fmax sweeps, all 40 graphs and plotting
records are outside this package.

## Quick start

Extract this ZIP and use its contents as the root of your GitHub repository.
Use this version instead of the earlier release-candidate ZIP: the HLS launcher
and testbench argument forwarding have been corrected and verified.

1. Activate **Vitis HLS 2023.2** using your own installation settings.
2. From the extracted repository root, run:

```bash
python3 scripts/check_package.py
bash scripts/run_hls.sh Graph_M csim
bash scripts/run_hls.sh Graph_L csim
```

Both C simulations have passed; expect `VALID_DIFF=0`. Graph_M stores only one
graph0 sample; its testbench replays it three times in memory to satisfy the
unchanged batch pipeline. Graph_L stores one sample and processes all 33 shards
and 1,649,533 logical edge slots. **No synthesis or P&R was run in verification.**
The testbenches execute the original fixed-point top functions and compare valid
raw scores against archived final RTL outputs; this is not an FP32 accuracy rerun.

Upload the extracted repository contents, excluding `.build/` and local logs.
If using Git, initialize a local repository, add the files, create a commit, and
push to your own GitHub remote. This package does not assign an author identity,
public URL, actual public commit, Git tag, or DOI.

**Licensing remains unresolved:** `LICENSE` is a pending-status notice, not an
MIT or Apache license. Redistribution rights for the weights and TrackML-derived
samples also require confirmation. Packaging and verification do not grant rights
on behalf of the authors or institution.

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
