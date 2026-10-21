# Requested HLS Source for Paper Listings (Items 12 and 32)

Purpose: add one or two short HLS source listings to paper Section 4 and prepare
a public code-availability repository. Listings must be copied **verbatim from
the final source**, not reconstructed from memory or rewritten audit excerpts.

Final designs: Graph_M D20 `<20,12>`, E_PAR=8, 125 MHz; Graph_L D24 `<24,16>`,
E_PAR=8, 8.8 ns / 113.636 MHz, with 33 edge shards.

This file translates the source request. The corresponding answers, verified
line numbers, and excerpts are in `SOURCE_CODE_RESPONSE.md`.

## 1. Required Source and Version Information

| Item | Requested information |
| --- | --- |
| Source roots | Locate `graph_l_d24/src/` and the corresponding Graph_M directory; in this package they are under `designs/Graph_L/src/` and `designs/Graph_M/src/` |
| Version | Git commit/tag, or file dates if no version control exists; identify the source corresponding to final D20/D24 results |
| Preprocessing/testbench | Paths and version identification for graph construction, destination ordering, shard splitting, and padding |
| Files | `su_top_model.cpp`, `su_defines.h`, and `su_util.cpp`; the legacy `su_host_load.h` is not requested |
| Implementation lineage | C++ alone does not prove that the source belongs to a timing-closed 8.8 ns implementation. Provide a build manifest, HLS export/Vivado Tcl, or dated/hash-identified run/report that connects source and results |
| Line-number basis | Verify whether `L:`/`M:` references in `MODEL_DATAPATH_SOURCE_CHECK.md` match this source version; otherwise identify functions by name and report current lines |

## 2. Preferred Listing 1: Graph_L Cross-Shard Aggregator Carry

Purpose: show how the unfinished destination-node sum is retained and continued
across shards without losing messages at a boundary.

Provide the **complete function and its call sites**, not only static declarations.

| Requested content | Original audit reference, requiring verification |
| --- | --- |
| Complete `aggregation_global_par_8_cl_init` | `su_top_model.cpp`, near L:1114; the checked function actually starts at L:1100 |
| Tail updating `keep_e_pre`, `keep_idx_pre`, and `keep_n_pre` | Near L:1470 |
| First-shard call, `init=true` | L:6951 |
| Subsequent-shard call, `init=false` | L:7021 |
| Definitions of `sens_agg_edge`, `index_t`, `node`, `MAX_NODE`, and `EDGE_DIM` | `su_defines.h` |

Also answer:

1. `keep_n_pre` has no explicit reset in the audited init block. What makes its
   first-shard use safe for valid data? Does this depend on an initial dummy token?
2. Is this static cross-shard carry specific to the Graph_L active path? Does
   Graph_M call a different aggregator?
3. Does correctness require destination IDs to arrive in sorted order? The
   aggregator can demonstrate that prerequisite but cannot prove input sorting.
   Provide input-side evidence from preprocessing or the testbench, including
   shard cuts inside one destination group, continuation of partial sums, the
   padding destination ID, and terminal-group handling.

## 3. Optional Listing 2: URAM Node Replicas and Read Mapping

Purpose: explain how edge lanes select among E_PAR/2 node-table replicas,
consistent with paper Section 4.4. Emphasize the **actual read index**, not storage
or partition pragmas alone; those are standard HLS facilities.

| Requested content | Original audit reference, requiring verification |
| --- | --- |
| `node_src_mp_L1` / `node_des_mp_L1` declarations and `BIND_STORAGE` / `ARRAY_PARTITION` pragmas | Graph_L near L:6810; Graph_M near M:3899 |
| EdgeBlock1 lane-to-replica read and its actual index expression | Identify the relevant `edge_block_*` function |
| Relation between array dimensions and replica count | For example, 4 in `[4][43278]` equals E_PAR/2 |

Also answer:

1. Section 4.4 states that lane i reads replica `i mod (E_PAR/2)`
   (`4-architecture.tex:105` in the manuscript request). For E_PAR=8, lanes 0/4
   share copy0, 1/5 share copy1, and so on. This differs from `i/2`, which groups
   lanes 0/1 together. Confirm the actual source expression. If the source differs,
   resolve the manuscript discrepancy before choosing the listing.
2. How does the code establish dual-port replicas shared by two lanes?

## 4. Excerpts Not Requested

- Top-level `DATAFLOW` wiring: the architecture diagram and existing listing
  already describe it.
- A standalone `PIPELINE II=1`: standard HLS usage does not illustrate the design
  contribution by itself.
- `BIND_OP` for the opt_rsrc LUT/DSP exchange: a brief textual explanation suffices.
- The previously removed Section 7.2 `merge_step` excerpt, unless there is a clear
  reason it is more informative than the replica-read listing.

## 5. Requested Excerpt Format

For each candidate, provide:

1. Filename, function name, and exact start/end lines in the actual file.
2. Verbatim source, preserving comments, indentation, and whitespace.
3. Suggested omission ranges, with exact line numbers; do not present edited
   excerpts as complete unmodified functions.
4. Answers to the related questions above.

## 6. Code-Availability Information Requiring Confirmation

| Question | Purpose |
| --- | --- |
| Public GitHub URL and approved license, such as MIT or Apache-2.0 | Populate the code-availability section accurately |
| A Zenodo DOI for a fixed release | Cite an actual immutable version |
| Release scope: HLS, M/L configurations, co-sim TBs, pseudo-graph scripts, `fgcs_in_benchmark`, Fmax/P&R Tcl | State exactly what is public and what is omitted |
| Permission to redistribute TrackML-derived graph inputs | If inputs cannot be released, describe the available generation scripts and execution limitations honestly |
| Planned public-release date | Describe an unpublished release as planned; do not invent a repository URL |

Unknown items must remain explicitly unknown or pending. Do not choose a project
license, fabricate an approved redistribution scope, or claim publication on
behalf of the authors.

## 7. Subsequent Workflow

1. Supply the requested source and evidence.
2. Present candidate excerpts and verified lines for review without editing the paper.
3. After author approval, update `revision/4-architecture.tex`, add the
   code-availability section, update Items 12/32 and `revision/revision_record.md`,
   and rebuild the manuscript.
4. Marking a status `[O]` requires an explicit author instruction.
