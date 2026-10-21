# Weights and biases

The same 18 active text files are used by Graph_M and Graph_L. Values are ASCII
floating-point decimal scalars, separated by commas and whitespace. The testbench
reads layers 0, 1, 2 and converts each scalar to weight_t = ap_fixed<16,8>.
The defaults AP_TRN / AP_WRAP are unchanged. Text files preserve archived precision;
this package does not retrain, requantize on disk, or change arithmetic.

| MLP | layer weight shapes (input × output) | bias lengths | Parameters |
| --- | --- | --- | --- |
| R1 | 10×8, 8×8, 8×4 | 8, 8, 4 | 196 |
| O | 7×8, 8×8, 8×3 | 8, 8, 3 | 163 |
| R2 | 10×8, 8×8, 8×1 | 8, 8, 1 | 169 |

Total = 528 scalars. Parameter flattening order is retained exactly as read by the
unchanged HLS MLP implementation; shape notation does not imply a transposition.
The archived *_3 files duplicated active final-layer files and are omitted.
parameter_counts.json records the checked scalar counts.
Redistribution rights still require confirmation; see NOTICE.md.
