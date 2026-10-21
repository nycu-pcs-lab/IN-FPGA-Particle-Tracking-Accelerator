# Sample binary format and output contract

All fields in input.bin are little-endian, with no alignment padding.

1. Header: three uint32 values: graph_count, node_count, edge_slots_per_graph.
2. For each graph in order:
   - node_count × 3 float32 node features, row-major;
   - edge_slots_per_graph × 4 float32 edge features, in HLS destination order;
   - edge_slots_per_graph × 2 uint16 endpoints, (source,destination);
   - uint32 source_count;
   - source_count uint16 source-node IDs.

Features are quantized by the TB to ap_fixed<16,8>, using the original default modes.
The graph file includes the full configured capacity, not just valid edges.
All valid endpoints are below MAX_NODE-1. Padding endpoints are the self-loop
(MAX_NODE-1,MAX_NODE-1); the padding edges form a tail after valid edges. Destination
IDs are globally nondecreasing. No new raw TrackML graph exporter is provided.

valid_mask.bin has one uint8 per edge slot, graph-major, 1=valid, 0=padding.
expected_raw.bin has one little-endian int16 per slot in the same order. It is the
archived fixed-point sigmoid output raw encoding; score = raw / 256. It is NOT
a ground-truth label and is not an FP32 baseline. Correctness checks compare raw
bits for valid edges only. Padding differences are printed but not failures.

Graph_M's sample stores only original graph0, with header graph_count=1. The
unchanged kernel's batch pipeline requires at least three entries, so the testbench
rewinds and reuses this one payload for three in-memory batch entries. It compares
each execution with the one archived graph0 expected score/mask file. Only one graph
of actual output is kept on disk (the final replay); all three replays are checked.
This is a single-sample correctness test, not three independent graphs or a latency
benchmark. The expected output is graph0 from the archived final-D20 RTL run.

Graph_L's sample contains graph0, 1,649,533 slots: 32 full 50,000-slot shards plus
a 49,533-slot shard. The TB uses existing packed interface strides. This is a fixed
33-shard schedule regardless of valid-edge count. The last packed word contains
five active lanes; unused lanes are not additional valid edges.

The aggregator preserves its unfinished destination group across shard calls.
A destination change to terminal dummy edges flushes the final valid group. The
source has no separate general unpadded final-group flush. Dummy node payload and
scores are not guaranteed equal across different batch contexts. In particular,
init resets keep_e_pre and keep_idx_pre but does not reset keep_n_pre per graph.
Do not use reserved dummy IDs for valid data or interpret padding predictions.
