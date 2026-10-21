#pragma once
#include "../src/su_defines.h"
static constexpr int ceildiv(int a,int b) { return (a+b-1)/b; }
static constexpr int ceildiv_even(int a,int b) { int n=ceildiv(a,b);return n%2?n+1:n; }
extern "C" { void su_top_model(
    
  
    const input_node       N_ch0_test[MAX_NUM_GRAPH * ceildiv_even(MAX_NODE, READ_N_PAR_PER_CH)],
    const input_edge       E_ch0_test[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    const input_idx        I_ch0_test[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    const input_src        S_ch0_test[MAX_NUM_GRAPH * ceildiv_even(SRC_NODE, READ_N_PAR_PER_CH)],
    
    const input_weight       W1_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W1_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W1_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
    const input_weight       W2_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W2_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W2_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
    const input_weight       W3_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W3_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    const input_weight       W3_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
	output_edge              O3_ch0[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E2_BLOCK_PAR)],
    
    //ap_uint<WIDTH_O> *O3_ch0
    /*/.... #node_channel = E2_BLOCK_PAR

	ap_uint<WIDTH_O3> *O3_ch1,
	ap_uint<WIDTH_O3> *O3_ch2,
	ap_uint<WIDTH_O3> *O3_ch3,
	ap_uint<WIDTH_O3> *O3_ch4,
	ap_uint<WIDTH_O3> *O3_ch5,
	ap_uint<WIDTH_O3> *O3_ch6,
	ap_uint<WIDTH_O3> *O3_ch7,
*/

//><><><<><>><><<><><><><><><><><><><>><
//const int part
    //const int* num_of_nodes,
    //const int* num_of_edges,
    //const int* num_of_des_nodes,
    const int  num_graphs,
	const ap_uint<16> src_num_sum[MAX_NUM_GRAPH]
//======================================


); }
