# Final package status

| Item | Result |
| --- | --- |
| Final Graph_M D20 / Graph_L D24 HLS core | Byte-identical to canonical final source and recorded manifests |
| Local include closure / vendor dependencies | Checked; AMD tool headers are external, not bundled |
| Weights | 18 active files / 528 scalars; archived text unchanged |
| Inputs / masks / expected outputs | M: graph0 only (three in-memory replays); L: graph0; checked endpoints/order/padding/lengths |
| Packaged M C-simulation | PASS: one 104,328-slot sample / 96,791 valid edges, checked over 3 replays; zero valid-bit mismatches |
| Packaged L C-simulation | PASS: 1,649,533 slots; 813,711 valid edges; zero valid-bit mismatches |
| Portable launcher | Corrected and exercised with Vitis HLS 2023.2 |
| Synthesis / new RTL co-sim / P&R | NOT RUN |
| Project/weight/data rights | Still awaiting rightsholder confirmation |
| GitHub URL / actual commit / DOI | Not created or claimed |

Only release testbench/launcher/documentation/verification files changed. No kernel
C++, precision, architecture, pipeline, FIFO, E_PAR, sharding or mathematical behavior
was modified. The former untested CLI token forwarding and double-quote TB argv
were fixed in the launcher/Tcl. All outputs are checked against existing archived
final-precision outputs, not a newly generated FP32 or ground-truth reference.
Full native logs are kept outside this package; sanitized receipts are in validation/.
