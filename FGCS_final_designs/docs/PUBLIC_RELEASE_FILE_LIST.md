# Required Files for Releasing the Final Graph_M and Graph_L Designs

Scope: buildable source code for the **final** Graph_M design (D20 `<20,12>`,
E_PAR=8, 125 MHz) and Graph_L design (D24 `<24,16>`, E_PAR=8,
8.8 ns / 113.636 MHz, 33 edge shards).
Purpose: address Item 12 (code/artifact availability) and support the source
listings requested for Item 32.

**Scope not promised:** this repository does not claim to reproduce every paper
result. Figures 7-10 and parts of the resource tables cover several historical
design-space configurations (`5-evaluation.tex:65` preserves aggregate figure
values), which cannot all be rerun using only the final two sources. CPU/GPU
benchmarks, Fmax sweeps, the complete graph collections, and plotting data may be
provided later in a separate experimental reproduction package; they are not
prerequisites for this final-design source package.

## 1. Required Contents

| # | Item | Contents | Confirmation needed |
| --- | --- | --- | --- |
| 1 | Complete Graph_M source | `su_top_model.cpp` and all actual local header/C++ dependencies, including `su_defines.h`, `su_util.cpp`, and their includes; not only the top file | Run HLS C-synthesis or C simulation in a clean directory to check dependency completeness |
| 2 | Complete Graph_L source | The equivalent dependency closure for `graph_l_d24/src/` | Same check as Graph_M |
| 3 | Configuration | Capacity constants, E_PAR, E2_PAR, NS_PAR, shard size, fixed-point formats, and clock targets; Graph_L uses 50,000 edges/shard and 33 shards | Check consistency with Tables 2 and 9 |
| 4 | Weights | 528 parameters, shared by both designs or supplied in their respective quantized forms, with a format description | Confirm redistribution rights; disclose any withheld weights |
| 5 | At least one legal input and expected output per design | One destination-sorted, padded Graph_M graph and one Graph_L graph, with expected outputs | Confirm redistribution rights for TrackML-derived inputs |
| 6 | Testbenches and build instructions | Executable C-sim/co-sim testbenches and Vitis HLS Tcl with `csynth`/`cosim` commands and arguments | Exercise the README procedure in a clean environment |
| 7 | Version and tool information | Fixed commit/tag where available, Vitis HLS/Vivado versions, exact XCVU9P part, and clock periods | Link the source to the successful 8.8 ns run using a build manifest, export record, hashes, or dated run information |
| 8 | README | Directory structure, build/testbench instructions, expected outputs, and limitations | See Section 3 |
| 9 | LICENSE | Approved terms such as MIT or Apache-2.0 | Confirm the authors/institution may choose the license; uploading to GitHub alone is not a project reuse license |

This file is the translated requirements checklist, not a declaration that every
administrative or publication requirement is complete. Current implementation
and licensing status are recorded in `PACKAGING_STATUS.md`, `README.md`,
`NOTICE.md`, and `validation/csim_summary.json`. Only C simulation was run for
package verification; synthesis and P&R were not run.

## 2. Correspondence with Paper Listings (Item 32)

The two listings selected for the paper must exist **verbatim** in the released
source. Line numbers must be checked against the released files.

| Excerpt | Original audit references; recheck in the released source |
| --- | --- |
| Graph_L aggregator carry: `aggregation_global_par_8_cl_init`, carry commit, first-shard and subsequent-shard calls | `su_top_model.cpp` L:1114, L:1470, L:6951, L:7021 |
| Node-table URAM replicas and lane-to-replica mapping, `i mod (E_PAR/2)` | Graph_L near L:6810; Graph_M near M:3899 |

See `SOURCE_CODE_REQUEST.md` for the detailed request and
`SOURCE_CODE_RESPONSE.md` for checked locations. The complete L aggregator starts
at line 1100; line 1114 is the first static carry declaration.

## 3. Limitations the README Must State

- Only the final Graph_M/Graph_L designs are included. Historical design-space
  configurations from Figures 7-10, old versions, CPU/GPU benchmarks, and Fmax
  sweeps are outside this repository.
- FPGA latency is derived from RTL co-simulation cycles and the timing-closed
  operating clock; it is not measured board latency. Power is a Vivado vectorless
  estimate with Medium confidence, not measured board power.
- If weights or test graphs are withheld, identify which procedures cannot be
  rerun. Do not describe an incomplete package as fully executable.
- Resource, timing, and power reproduction needs Vivado and appropriate licenses.
  State tool versions and constraints without promising identical P&R results;
  run-to-run variability has not been evaluated.

## 4. Cleanup Before Public Release

- Remove private absolute paths, account names, and machine names.
- Exclude temporary files, DCPs, large logs, and unused historical sources. Mark any
  retained historical file that does not correspond to the reported final design.
- Do not include the historical Graph_L 1.086 ms result as a final-design result.
- Exclude earlier papers' PDFs, private rebuttal drafts, and internal
  `experiment_data/` reports.
- Check source comments for unpublished internal information.

## 5. Code-Availability Statement

After an actual repository is published, describe precisely what it contains:
final Graph_M/Graph_L HLS sources, configurations, supplied inputs/weights,
testbenches, and build instructions. Also identify omissions, including other
design-space configurations, CPU/GPU scripts, original experimental records,
and any inputs withheld for licensing reasons. Cite a real fixed version
(commit/tag or Zenodo DOI) and the approved license.

Before publication, describe the planned release without inventing a URL or
claiming that the repository is already available.

## 6. Subsequent Manuscript Workflow

1. Assemble these artifacts and answer `SOURCE_CODE_REQUEST.md`.
2. Present candidate listings and exact line numbers for author review without
   editing the manuscript.
3. After author approval, add the code-availability section and source listings,
   update Items 12/32 and `revision/revision_record.md`, and rebuild the paper.
4. Marking an item `[O]` requires an explicit author instruction.
