#ifndef __DEFINES_H_
#define __DEFINES_H_

#include "ap_int.h"
#include "ap_fixed.h"
#include <array>


constexpr int   NODE_DIM = 3;
constexpr int   EDGE_DIM = 4; 
constexpr int   EDGE_OUT_DIM = 1;
constexpr int   EDGE_BLOCK_IN_DIM = NODE_DIM * 2 + EDGE_DIM;
constexpr int   EDGE_BLOCK_OUT_DIM = EDGE_DIM;
constexpr int   EDGE_BLOCK_END_DIM = 1; //E2

constexpr int   NODE_BLOCK_IN_DIM = NODE_DIM + EDGE_DIM;
constexpr int   NODE_BLOCK_OUT_DIM = NODE_DIM;
//================================================//

//typedef ap_fixed<14,7> attr_t;
//typedef ap_fixed<14,7> attr_t;

constexpr int   data_w = 16;

//typedef ap_fixed<14,7>  attr_t;
//typedef ap_fixed<14,7>  weight_t;
typedef ap_fixed<16,8>  attr_t;
typedef ap_fixed<16,8>  weight_t;
// Sensitivity study: external formats unchanged, fractional bits = 8.
typedef ap_fixed<20,12> sens_agg_t;
typedef ap_fixed<20,12> sens_mac_t;
typedef std::array<sens_agg_t,4> sens_agg_edge;

typedef ap_uint<16>     index_t;


typedef ap_uint<16>     HBM_t;




typedef std::array<attr_t, EDGE_DIM> edge;
typedef std::array<attr_t, NODE_DIM> node;

typedef std::array<attr_t, EDGE_BLOCK_END_DIM> edge_res;

typedef std::array<index_t, 2> idx_pair;

typedef ap_uint<48> node_str; 
typedef ap_uint<64> edge_str; 
typedef ap_uint<32> edge_list_str; 
typedef attr_t DTYPE;
//================================================//
//const defined here

/*
constexpr int   MAX_NODE = 380;//739; //modify
constexpr int   MAX_EDGE = 634;//1252; //modify
constexpr int   SRC_NODE = 280;//202;
constexpr int   TOTAL_EDGE_PER_GRAPH = 634;//1252; //modify
constexpr int   MAX_NUM_GRAPH = 10;
*/
constexpr int   MAX_NODE = 9925;//739; //modify
constexpr int   MAX_EDGE = 104328;//1252; //modify
constexpr int   SRC_NODE = 4705; //2089;//202;
constexpr int   TOTAL_EDGE_PER_GRAPH = 104328;//1252; //modify
constexpr int   MAX_NUM_GRAPH = 20;


constexpr int   MLP_HIDDEN_DIM = 8;
constexpr int   MLP_LAYER = 3;

constexpr int   MAX_WEIGHT_DIM = 80; //EDGE_BLOCK_IN_DIM(10) * MLP_HIDDEN_DIM
constexpr int   MAX_BIAS_DIM = 8; //MLP_HIDDEN_DIM

constexpr int   E_BLOCK_PAR = 8;
constexpr int   N_BLOCK_PAR = 8;
constexpr int   NS_BLOCK_PAR = 2;
constexpr int   E2_BLOCK_PAR = 12;
constexpr int   MERGE_PAR = 4;

constexpr int   WIDTH_O = data_w * N_BLOCK_PAR;

constexpr int   READ_W_PAR_PER_CH = 1; //number of channel for load weight
constexpr int   READ_N_PAR_PER_CH = 2; //number per channel for load node
constexpr int   READ_E_PAR_PER_CH = E_BLOCK_PAR; //number per channel for load edge
constexpr int   READ_I_PAR_PER_CH = E_BLOCK_PAR; //number per channel for load index
constexpr int   WRITE_O_PAR_PER_CH = E2_BLOCK_PAR; //number per channel for load index

constexpr int   WIDTH_W = data_w * READ_W_PAR_PER_CH;
constexpr int   WIDTH_E = data_w * READ_E_PAR_PER_CH * EDGE_DIM;
constexpr int   WIDTH_I = data_w * READ_I_PAR_PER_CH * 2;
constexpr int   WIDTH_N = data_w * READ_N_PAR_PER_CH * NODE_DIM;

constexpr int   length_e0 = EDGE_BLOCK_IN_DIM * MLP_HIDDEN_DIM; //80 (10->8)
constexpr int   length_e1 = MLP_HIDDEN_DIM * MLP_HIDDEN_DIM; //64 (8_>8)
constexpr int   length_e2 = MLP_HIDDEN_DIM * EDGE_BLOCK_OUT_DIM; //32 (8->4)

constexpr int   length_e22 = MLP_HIDDEN_DIM * EDGE_BLOCK_END_DIM; //8 (8->1)

constexpr int   length_b0 = NODE_BLOCK_IN_DIM * MLP_HIDDEN_DIM; //56
constexpr int   length_b1 = MLP_HIDDEN_DIM * MLP_HIDDEN_DIM; //64
constexpr int   length_b2 = MLP_HIDDEN_DIM * NODE_BLOCK_OUT_DIM; //24

//constexpr int   E2_BLOCK_PAR = 1;
//constexpr int   A_BLOCK_PAR = 1;

//assume weight < 16 bits 
//able to load 512/16 = 32 value at 1 HBM element (32 = 8*4)

//assume attr_t <= 16 bits
//able to load 32 nodes at 1 HBM channel

//compute initial edge input come from A.)HBM ==> max total PE parallel (E_PAR * GROUP) = 32 
//                                     B.)preload_to_BRAM 
//                                     C.)preload_to_stream? 


constexpr int   READ_W_EB_PAR = 1; //number of channel for load weight = number of PE execution?
constexpr int   READ_W_NB_PAR = 1; //number of channel for load weight


typedef struct alignas(READ_E_PAR_PER_CH  * EDGE_DIM * data_w/8){
    attr_t e_attr[READ_E_PAR_PER_CH  * EDGE_DIM]; //edge_dim * E_PAR
}input_edge;

typedef struct alignas(READ_N_PAR_PER_CH * EDGE_DIM * data_w/8){
    attr_t n_attr[READ_N_PAR_PER_CH * EDGE_DIM ]; //edge_dim * E_PAR
    //attr_t dummy; //for 2^n bits per channel
}input_node;

typedef struct alignas(READ_I_PAR_PER_CH * 2 * 16/8){
    index_t i_attr[READ_I_PAR_PER_CH * 2]; //edge_dim * E_PAR
}input_idx;

typedef struct alignas(READ_N_PAR_PER_CH * 1 * 16/8){
    index_t s_attr[READ_N_PAR_PER_CH]; //edge_dim * E_PAR
}input_src;

typedef struct alignas(READ_W_PAR_PER_CH * data_w/8){
    weight_t w_attr[READ_W_PAR_PER_CH]; 
}input_weight;

typedef struct alignas(16 * data_w/8){
    attr_t o_attr[16]; //edge_dim * E_PAR
}output_edge;


// BRAM for intermediate storage
//FM_TYPE messages_ping[EDGE_PARALLEL][ceildiv(MAX_NODE, EDGE_PARALLEL)][EMB_DIM];
//FM_TYPE messages_pong[EDGE_PARALLEL][ceildiv(MAX_NODE, EDGE_PARALLEL)][EMB_DIM];

/*
extern attr_t node_ping_in[MAX_NODE][NODE_DIM];
extern attr_t node_pong_in[MAX_NODE][NODE_DIM];
//attr_t node_ping_out[MAX_NODE][NODE_DIM];
//attr_t node_pong_out[MAX_NODE][NODE_DIM];

//attr_t edge_ping_in[MAX_EDGE][EDGE_DIM];
//attr_t edge_pong_in[MAX_EDGE][EDGE_DIM];
extern attr_t edge_ping_out[MAX_EDGE];
extern attr_t edge_pong_out[MAX_EDGE];

extern index_t edge_list_ping[MAX_EDGE][2];
extern index_t edge_list_pong[MAX_EDGE][2];
*/
//edgeblock1
/*
extern weight_t buf_w0[E_BLOCK_PAR][MLP_LAYER][MAX_WEIGHT_DIM];
extern weight_t buf_b0[E_BLOCK_PAR][MLP_LAYER][MAX_BIAS_DIM];

//nodeblock
extern weight_t buf_w1[N_BLOCK_PAR][MLP_LAYER][MAX_WEIGHT_DIM];
extern weight_t buf_b1[N_BLOCK_PAR][MLP_LAYER][MAX_BIAS_DIM];

//edgeblock2
extern weight_t buf_w2[E2_BLOCK_PAR][MLP_LAYER][MAX_WEIGHT_DIM];
extern weight_t buf_b2[E2_BLOCK_PAR][MLP_LAYER][MAX_BIAS_DIM];
*/



//parallel * n ==> weight_width need duplicated for n for each PE -- shiyu(suit for small model)
//parallel * n ==> total weight seperate into n part for n PE compute parallel -- JunXuan(save BRAM)

#endif
