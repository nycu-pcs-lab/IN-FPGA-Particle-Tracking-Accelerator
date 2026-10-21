# Verified package C-simulation

Both packaged designs were built and executed using the packaged source, input,
weights, testbench and launcher in Vitis HLS 2023.2 (SW build 4023990).
Graph_M: 1 stored graph0; 104,328 slots / 96,791 valid edges. The TB replays
it three times in memory (312,984 internal slots), checking VALID_DIFF=0 each time.
Only one sample input, mask and expected output is distributed.
Graph_L: 1 graph, 1,649,533 output slots, 813,711 valid edges, VALID_DIFF=0.
Valid-edge comparisons are raw-bit exact against archived final-precision RTL outputs.
See csim_summary.json for output hashes and padding-difference counts.

The top/defines/util source files were not changed. The launcher was corrected
for Vitis HLS 2023.2 environment-based argument passing and Tcl list-based TB argv.
This replaces the old untested trailing-CLI-argument invocation.

No csynth, native Vivado synthesis, RTL co-simulation, P&R, latency experiment or
power measurement was executed in this verification. Clock metadata records the
prior final routed operating points. A passing sample is not proof for arbitrary
inputs or a rerun of all 40 accuracy graphs.

Native full logs and generated projects remain in the local verification workspace;
they are excluded from this package to remove machine/account paths. Their hashes
are recorded in csim_summary.json. Successful receipt lines are provided below.
