#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <array>
#include "hls_math.h"
#include "hls_stream.h"
#include "ap_int.h"
#include "ap_fixed.h"


#include "su_defines.h"
#include "su_util.cpp"

//#define MAX_FIFO_DEPTH
#define MIN_FIFO_DEPTH

using std::cout; 
using std::endl;  
using std::array;

void write_back_v2(
	attr_t                  edge_ping_out[MAX_EDGE], //edge_block2 outout modify? //final_output
	output_edge            *O_ch0,
    int num_of_edge
){
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR) ;
    
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1 
        for(int pe = 0 ; pe < E_BLOCK_PAR; pe ++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                O_ch0[iters].o_attr[pe] = edge_ping_out[iters * E_BLOCK_PAR + pe];
            }
        }
    }
}

void node_block_end(
    hls::stream<node>       node_in[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_in[N_BLOCK_PAR],
    attr_t               node_update[MAX_NODE][NODE_DIM],
    int num_of_des_node,
    int num_of_src_node
)
{
    int num_iters_src = ceildiv(num_of_src_node, N_BLOCK_PAR);
    for(int iters = 0; iters < num_iters_src; iters ++){
        for(int pe = 0 ; pe < N_BLOCK_PAR; pe ++){
            if(iters * N_BLOCK_PAR + pe < num_of_src_node){
                //cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
                node n;
                index_t idx;
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    if(dim == 0){
                        node_in[pe] >> n;
                        idx_des_in[pe] >> idx;
                    }
                    node_update[idx][dim] = n[dim];      
                }
            }
        }
    }
    
    int num_iters = ceildiv(num_of_des_node, N_BLOCK_PAR);
    for(int iters = 0; iters < num_iters; iters ++){
        // cout<<"nbe iters des: "<< iters<<endl;
        for(int pe = 0 ; pe < N_BLOCK_PAR; pe ++){
            if(iters * N_BLOCK_PAR + pe < num_of_des_node){
                //cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
                node n;
                index_t idx;
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    if(dim == 0){
                        node_in[pe] >> n;
                        idx_des_in[pe] >> idx;
                    }
                    node_update[idx][dim] = n[dim];      
                }
            }
        }
    }
}

node_str merge_step(
    node_str node_update_tmp0,
    node_str node_update_tmp1,
    node_str node_update_tmp2,
    node_str node_update_tmp3,
    node_str node_update_tmp4,
    node_str node_update_tmp5
    
){
#pragma HLS INLINE OFF
#pragma HLS latency min=8
    node_str tmp_add[4]; 
    
    tmp_add[0] = node_update_tmp0 | node_update_tmp1;
    tmp_add[1] = node_update_tmp2 | node_update_tmp3;
    tmp_add[2] = node_update_tmp4 | node_update_tmp5;
    tmp_add[3] = tmp_add[0] | tmp_add[1] | tmp_add[2];
    
    return tmp_add[3];
}

void node_block_end_par_8_join_end_SLG(
	#ifdef __SYNTHESIS__
    node_str  node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str  node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
    node_str 	node_src_mp_L2[E2_BLOCK_PAR/2][MAX_NODE],
    node_str 	node_des_mp_L2[E2_BLOCK_PAR/2][MAX_NODE],
    #else
    node_str  (**node_update_tmp1),
	node_str  (**node_update_tmp2),
    
    node_str  (**node_src_mp_L2),
	node_str  (**node_des_mp_L2),
    #endif
	
    int num_of_edge,
    int num_of_src_node
)
{
//#pragma HLS INLINE

    
    for(int iters = 0; iters < MAX_NODE; iters++){
#pragma HLS PIPELINE II=1
#pragma HLS UNROLL factor = MERGE_PAR * 2
        //cout<<"nbep8je iters:"<<iters<<endl;

		/*
        tmp_add = 
        node_update_tmp1[0][iters] | node_update_tmp1[1][iters] | node_update_tmp1[2][iters] | node_update_tmp1[3][iters] | node_update_tmp1[4][iters] | node_update_tmp1[5][iters] |
        node_update_tmp2[0][iters] | node_update_tmp2[1][iters] | node_update_tmp2[2][iters] | node_update_tmp2[3][iters] | node_update_tmp2[4][iters] | node_update_tmp2[5][iters];
        */
        node_str tmp_add, tmp_add_ff;
		
        tmp_add = merge_step(
            node_update_tmp1[0][iters], node_update_tmp1[1][iters],
            node_update_tmp2[0][iters], node_update_tmp2[1][iters],
            node_update_tmp2[2][iters], node_update_tmp2[3][iters]
        );

        tmp_add_ff = tmp_add;
        
        for(int pe = 0; pe < E2_BLOCK_PAR/2; pe++){
            node_src_mp_L2[pe][iters] = tmp_add_ff;
			node_des_mp_L2[pe][iters] = tmp_add_ff;
        }
    
    } 
}
void node_block_end_par_8_join_end(
	#ifdef __SYNTHESIS__
    node_str  node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str  node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
    #else
    // node_str  (*node_update_tmp1)[MAX_NODE],
	// node_str  (*node_update_tmp2)[MAX_NODE],
    node_str  (**node_update_tmp1),
	node_str  (**node_update_tmp2),
    #endif
    //attr_t	node_update[MAX_NODE][NODE_DIM],
	node_str 	node_src_mp_L2[E2_BLOCK_PAR/2][MAX_NODE],
    node_str 	node_des_mp_L2[E2_BLOCK_PAR/2][MAX_NODE],
	
    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
)
{
#pragma HLS INLINE

    node_str tmp_add;
    for(int iters = 0; iters < MAX_NODE; iters++){
#pragma HLS PIPELINE II=1
#pragma HLS latency min=2720
#pragma HLS UNROLL factor = MERGE_PAR * 2
        //cout<<"nbep8je iters:"<<iters<<endl;

		/*
        tmp_add = 
        node_update_tmp1[0][iters] | node_update_tmp1[1][iters] | node_update_tmp1[2][iters] | node_update_tmp1[3][iters] | node_update_tmp1[4][iters] | node_update_tmp1[5][iters] |
        node_update_tmp2[0][iters] | node_update_tmp2[1][iters] | node_update_tmp2[2][iters] | node_update_tmp2[3][iters] | node_update_tmp2[4][iters] | node_update_tmp2[5][iters];
        */
		tmp_add = 
        node_update_tmp1[0][iters] | 
        node_update_tmp2[0][iters] | node_update_tmp2[1][iters] | node_update_tmp2[2][iters] | node_update_tmp2[3][iters];// | node_update_tmp2[4][iters] | node_update_tmp2[5][iters] | node_update_tmp2[6][iters] | node_update_tmp2[7][iters];
        
        for(int pe = 0; pe < E2_BLOCK_PAR/2; pe++){
            node_src_mp_L2[pe][iters] = tmp_add;
			node_des_mp_L2[pe][iters] = tmp_add;
        }
    
    } 
}
void node_block_end_par_4_join_end(
	
    attr_t  node_update_tmp1[N_BLOCK_PAR][MAX_NODE][NODE_DIM],
	attr_t  node_update_tmp2[N_BLOCK_PAR][MAX_NODE][NODE_DIM],
    
    //attr_t	node_update[MAX_NODE][NODE_DIM],
	attr_t 	node_src_mp_L2[E_BLOCK_PAR][MAX_NODE][NODE_DIM],
    attr_t 	node_des_mp_L2[E_BLOCK_PAR][MAX_NODE][NODE_DIM],
	
    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
)
{
//#pragma HLS INLINE

    node tmp_add;
    for(int iters = 0; iters < MAX_NODE; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS UNROLL factor = 6*2
    
         for(int dim = 0; dim < NODE_DIM; dim ++){
            
            for(int pe = 0; pe < E_BLOCK_PAR; pe++){
				
				if(pe == 0){
					tmp_add[dim] = (node_update_tmp1[0][iters][dim] | node_update_tmp1[1][iters][dim]
                         | node_update_tmp1[2][iters][dim] | node_update_tmp1[3][iters][dim])
						 + (node_update_tmp2[0][iters][dim] | node_update_tmp2[1][iters][dim]
                         | node_update_tmp2[2][iters][dim] | node_update_tmp2[3][iters][dim]);
				}
				node_src_mp_L2[pe][iters][dim] = tmp_add[dim];      
				node_des_mp_L2[pe][iters][dim] = tmp_add[dim];   
			}
            //node_update[iters][dim] = tmp_add[dim];      
        }
    } 
}
void node_block_end_par_4_join_init(
    hls::stream<node>       node_out1[NS_BLOCK_PAR], //input-streaming message
    hls::stream<index_t> idx_des_out1[NS_BLOCK_PAR],
	
	hls::stream<node>       node_out2[N_BLOCK_PAR], //input-streaming message
    hls::stream<index_t> idx_des_out2[N_BLOCK_PAR],
	#ifdef __SYNTHESIS__
    node_str  node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str  node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
    #else
    // node_str  (*node_update_tmp1)[MAX_NODE],
	// node_str  (*node_update_tmp2)[MAX_NODE],
    node_str  (**node_update_tmp1),
	node_str  (**node_update_tmp2),
    #endif

    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node,
    bool init
)
{
#pragma HLS INLINE

    int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR);
    
    
    if(init){
        for(int iters = 0; iters < num_iters_src; iters ++){
    #pragma HLS PIPELINE II=1
    #pragma HLS loop_tripcount max = SRC_NODE / NS_BLOCK_PAR + 1  
    #pragma HLS DEPENDENCE variable=node_update_tmp1 intra WAW false
    //#pragma HLS DEPENDENCE variable=node_update_tmp1 inter WAW false
            for(int pe = 0 ; pe < NS_BLOCK_PAR; pe ++){


                if(iters * NS_BLOCK_PAR + pe < num_of_src_node){
                    //cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
                    node n;
                    index_t idx;
                    node_str tmp;
                    
                    
                    for(int dim = 0; dim < NODE_DIM; dim ++){
                        if(dim == 0){
                            node_out1[pe] >> n;
                            idx_des_out1[pe] >> idx;
                        }
                        tmp(data_w*(dim+1)-1, data_w*dim) = DTYPE_to_uint16<DTYPE>(n[dim]); 
                    }
                    node_update_tmp1[pe%(NS_BLOCK_PAR/2)][idx] = tmp; 
                }
            }
        }
    }
	int num_iters = ceildiv(num_of_edge, N_BLOCK_PAR);
	for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / N_BLOCK_PAR + 1 
#pragma HLS DEPENDENCE variable=node_update_tmp2 intra WAW false
//#pragma HLS DEPENDENCE variable=node_update_tmp2 inter WAW false
		//cout<<"nbe iters des: "<< iters<<endl;
		for(int pe = 0 ; pe < N_BLOCK_PAR; pe ++){

			if(iters * N_BLOCK_PAR + pe < num_of_edge){
				//cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
				node n;
				index_t idx;
				node_str tmp;
				for(int dim = 0; dim < NODE_DIM; dim ++){
                    if(dim == 0){
                        node_out2[pe] >> n;
                        idx_des_out2[pe] >> idx;
                        //cout<<"c: "<<idx<<endl;
                    }
                    tmp(data_w*(dim+1)-1, data_w*dim) = DTYPE_to_uint16<DTYPE>(n[dim]);
                }
                node_update_tmp2[pe%(N_BLOCK_PAR/2)][idx] = tmp;      
			}
		}
	}
}
void node_block_end_par_4_join(
    hls::stream<node>       node_out1[NS_BLOCK_PAR], //input-streaming message
    hls::stream<index_t> idx_des_out1[NS_BLOCK_PAR],
	
	hls::stream<node>       node_out2[N_BLOCK_PAR], //input-streaming message
    hls::stream<index_t> idx_des_out2[N_BLOCK_PAR],
	#ifdef __SYNTHESIS__
    node_str  node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str  node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
    #else
    // node_str  (*node_update_tmp1)[MAX_NODE],
	// node_str  (*node_update_tmp2)[MAX_NODE],
    node_str  (**node_update_tmp1),
	node_str  (**node_update_tmp2),
    #endif

    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
)
{
#pragma HLS INLINE

    int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR);
    
    for(int iters = 0; iters < num_iters_src; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = SRC_NODE / NS_BLOCK_PAR + 1  
#pragma HLS DEPENDENCE variable=node_update_tmp1 intra WAW false
//#pragma HLS DEPENDENCE variable=node_update_tmp1 inter WAW false
        for(int pe = 0 ; pe < NS_BLOCK_PAR; pe ++){


            if(iters * NS_BLOCK_PAR + pe < num_of_src_node){
                //cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
                node n;
                index_t idx;
                node_str tmp;
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    if(dim == 0){
                        node_out1[pe] >> n;
                        idx_des_out1[pe] >> idx;
                    }
                    tmp(data_w*(dim+1)-1, data_w*dim) = DTYPE_to_uint16<DTYPE>(n[dim]); 
                         
                }
                node_update_tmp1[0][idx] = tmp; 
            }
        }
    }
    
    int num_iters = ceildiv(num_of_edge, N_BLOCK_PAR);
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / N_BLOCK_PAR + 1 
#pragma HLS DEPENDENCE variable=node_update_tmp2 intra WAW false
//#pragma HLS DEPENDENCE variable=node_update_tmp2 inter WAW false
        //cout<<"nbe iters des: "<< iters<<endl;
        for(int pe = 0 ; pe < N_BLOCK_PAR; pe ++){

            if(iters * N_BLOCK_PAR + pe < num_of_edge){
                //cout<<"nbe "<<iters * N_BLOCK_PAR + pe<<endl;;
                node n;
                index_t idx;
                node_str tmp;
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    if(dim == 0){
                        node_out2[pe] >> n;
                        idx_des_out2[pe] >> idx;
                        //cout<<"c: "<<idx<<endl;
                    }
                    tmp(data_w*(dim+1)-1, data_w*dim) = DTYPE_to_uint16<DTYPE>(n[dim]);
                }
                node_update_tmp2[pe%(N_BLOCK_PAR/2)][idx] = tmp;      
            }
        }
    }
}
void node_block_par_4_not_join_init(
    hls::stream<node>       node_in[N_BLOCK_PAR], //input-streaming message
    hls::stream<sens_agg_edge>       edge_in[N_BLOCK_PAR],
    hls::stream<index_t> idx_des_in[N_BLOCK_PAR],
    
    hls::stream<node>       node_fork_in[NS_BLOCK_PAR], //input-streaming message
    hls::stream<edge>       edge_fork_in[NS_BLOCK_PAR],
    hls::stream<index_t> idx_des_fork_in[NS_BLOCK_PAR],
    
    hls::stream<node>       node_out1[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out1[N_BLOCK_PAR],
	
	hls::stream<node>       node_out2[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out2[N_BLOCK_PAR],
   
    //nodeblock
    //weight_t buf_w1[][MLP_LAYER][MAX_WEIGHT_DIM],
    //weight_t buf_b1[][MLP_LAYER][MAX_BIAS_DIM],
	
	weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node,
    bool init
){

#pragma HLS stable variable=node_in
#pragma HLS stable variable=edge_in
#pragma HLS stable variable=idx_des_in
 
#pragma HLS INLINE

	int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR) ;
	int num_iters = ceildiv(num_of_edge, N_BLOCK_PAR) ;
	
    
    if(init){
        for(int iters = 0; iters < num_iters_src; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = ceildiv(SRC_NODE, NS_BLOCK_PAR)
            for(int pe = 0; pe < NS_BLOCK_PAR; pe++){
                if (iters * NS_BLOCK_PAR + pe < num_of_src_node){
                    
                    array<sens_agg_t, NODE_BLOCK_IN_DIM> node_block_in;
                    node node_block_out, node_block_out_ff;	
                    index_t idx;
                    node n;
                    sens_agg_edge e;
                    
                    node_fork_in[pe] >> n;
                    sens_read_edge(edge_fork_in[pe],e);
                    idx_des_fork_in[pe]  >> idx;
                  
                    for(int dim = 0; dim < NODE_DIM; dim ++){
                        
                        node_block_in[dim] = n[dim];
                    }
                    for(int dim = 0; dim < EDGE_DIM; dim ++){
                        
                        node_block_in[NODE_DIM+dim] = e[dim];
                    }
                    //cout<<"* "<<iters * NS_BLOCK_PAR + pe;
                    
                    sens_MLP_inputwide_dsp<sens_agg_t, attr_t, weight_t, NODE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, NODE_BLOCK_OUT_DIM, MLP_LAYER>(
                        node_block_in, 
                        node_block_out,
                        //buf_w1[pe],
                        //buf_b1[pe]
                        buf_w1,
                        buf_b1
                    ); //weight);
                    
                    
                    if(idx==949){
                        cout<<"==========="<<endl;
                        cout<<"node #1"<<iters<<endl;
                        cout<<"node in: ";
                        print_fixed(node_block_in,8);
                        
                        cout<<"node out: ";
                        print_fixed(node_block_out, 8);
                    }
                   
                    node_block_out_ff = node_block_out;
                    node_out1[pe] << node_block_out_ff;
                    idx_des_out1[pe] << idx;
                }
            }
        }
    }    
	for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = ceildiv(MAX_EDGE, N_BLOCK_PAR)  
		for(int pe = 0; pe < N_BLOCK_PAR; pe++){
			if (iters * N_BLOCK_PAR + pe < num_of_edge){
			 
				array<sens_agg_t, NODE_BLOCK_IN_DIM> node_block_in;
				node node_block_out, node_block_out_ff;	
				index_t idx;
				node n;
				sens_agg_edge e;
				//cout <<"nb "<< iters * N_BLOCK_PAR + pe << endl;
				
				node_in[pe] >> n;
				sens_read_edge(edge_in[pe],e);
				idx_des_in[pe]  >> idx;
			

				for(int dim = 0; dim < NODE_DIM; dim ++){
					
					node_block_in[dim] = n[dim];
				}
				for(int dim = 0; dim < EDGE_DIM; dim ++){
					
					node_block_in[NODE_DIM+dim] = e[dim];
				}
				//cout<<"aa"<<endl;
		  
				sens_MLP_inputwide_dsp<sens_agg_t, attr_t, weight_t, NODE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, NODE_BLOCK_OUT_DIM, MLP_LAYER>(
					node_block_in, 
					node_block_out,
					//buf_w1[pe],
					//buf_b1[pe]
					buf_w1,
					buf_b1
				); //weight);
				
			
			if(idx==1){
				cout<<"==========="<<endl;
				cout<<"node #1"<<iters<<endl;
				cout<<"node in: ";
				print_fixed(node_block_in,8);
				
				cout<<"node out: ";
				print_fixed(node_block_out, 8);
			}
			/*
			if(idx >= MAX_NODE||idx <0){
				cout<<"error!!!!"<<idx<<endl;
				cout<<iters<<endl;
			}
			*/
                node_block_out_ff = node_block_out;
				node_out2[pe] << node_block_out_ff;
				idx_des_out2[pe] << idx;
			}  
		}
	}
}
void node_block_par_4_not_join(
    hls::stream<node>       node_in[N_BLOCK_PAR], //input-streaming message
    hls::stream<sens_agg_edge>       edge_in[N_BLOCK_PAR],
    hls::stream<index_t> idx_des_in[N_BLOCK_PAR],
    
    hls::stream<node>       node_fork_in[NS_BLOCK_PAR], //input-streaming message
    hls::stream<edge>       edge_fork_in[NS_BLOCK_PAR],
    hls::stream<index_t> idx_des_fork_in[NS_BLOCK_PAR],
    
    hls::stream<node>       node_out1[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out1[N_BLOCK_PAR],
	
	hls::stream<node>       node_out2[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out2[N_BLOCK_PAR],
   
    //nodeblock
    //weight_t buf_w1[][MLP_LAYER][MAX_WEIGHT_DIM],
    //weight_t buf_b1[][MLP_LAYER][MAX_BIAS_DIM],
	
	weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
){

#pragma HLS stable variable=node_in
#pragma HLS stable variable=edge_in
#pragma HLS stable variable=idx_des_in
 
#pragma HLS INLINE

	int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR) ;
	int num_iters = ceildiv(num_of_edge, N_BLOCK_PAR) ;
	
    for(int iters = 0; iters < num_iters_src; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = ceildiv(SRC_NODE, NS_BLOCK_PAR)
        for(int pe = 0; pe < NS_BLOCK_PAR; pe++){
            if (iters * NS_BLOCK_PAR + pe < num_of_src_node){
                
                array<sens_agg_t, NODE_BLOCK_IN_DIM> node_block_in;
                node node_block_out;	
                index_t idx;
                node n;
                sens_agg_edge e;
                
                node_fork_in[pe] >> n;
                sens_read_edge(edge_fork_in[pe],e);
                idx_des_fork_in[pe]  >> idx;
              
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    node_block_in[dim] = n[dim];
                }
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    
                    node_block_in[NODE_DIM+dim] = e[dim];
                }
                //cout<<"* "<<iters * NS_BLOCK_PAR + pe;
                
                sens_MLP_inputwide_dsp<sens_agg_t, attr_t, weight_t, NODE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, NODE_BLOCK_OUT_DIM, MLP_LAYER>(
                    node_block_in, 
                    node_block_out,
                    //buf_w1[pe],
                    //buf_b1[pe]
                    buf_w1,
                    buf_b1
                ); //weight);
                
                /*
                if(idx==1){
                    cout<<"==========="<<endl;
                    cout<<"node #1"<<iters<<endl;
                    cout<<"node in: ";
                    print_fixed(node_block_in,8);
                    
                    cout<<"node out: ";
                    print_fixed(node_block_out, 8);
                }
                if(idx >= MAX_NODE||idx <0){
                    cout<<"error!!!!"<<idx<<endl;
                    cout<<iters<<endl;
                }
                */

                node_out1[pe] << node_block_out;
                idx_des_out1[pe] << idx;
            }
        }
    }
   
	
	for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = ceildiv(MAX_EDGE, N_BLOCK_PAR)  
        for(int pe = 0; pe < N_BLOCK_PAR; pe++){
            if (iters * N_BLOCK_PAR + pe < num_of_edge){
             
                array<sens_agg_t, NODE_BLOCK_IN_DIM> node_block_in;
				node node_block_out;	
				index_t idx;
				node n;
				sens_agg_edge e;
				//cout <<"nb "<< iters * N_BLOCK_PAR + pe << endl;
                
                node_in[pe] >> n;
                sens_read_edge(edge_in[pe],e);
                idx_des_in[pe]  >> idx;
            

                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    node_block_in[dim] = n[dim];
                }
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    
                    node_block_in[NODE_DIM+dim] = e[dim];
                }
                //cout<<"aa"<<endl;
          
				sens_MLP_inputwide_dsp<sens_agg_t, attr_t, weight_t, NODE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, NODE_BLOCK_OUT_DIM, MLP_LAYER>(
					node_block_in, 
					node_block_out,
					//buf_w1[pe],
					//buf_b1[pe]
					buf_w1,
					buf_b1
				); //weight);
				
            /*
			if(idx==1){
				cout<<"==========="<<endl;
				cout<<"node #1"<<iters<<endl;
				cout<<"node in: ";
				print_fixed(node_block_in,8);
				
				cout<<"node out: ";
				print_fixed(node_block_out, 8);
			}
			if(idx >= MAX_NODE||idx <0){
				cout<<"error!!!!"<<idx<<endl;
				cout<<iters<<endl;
			}
		    */
			
				node_out2[pe] << node_block_out;
                idx_des_out2[pe] << idx;
			}  
        }
    }
}
/*
void node_block_par_4_join(
    hls::stream<node>       node_in[N_BLOCK_PAR], //input-streaming message
    hls::stream<edge>       edge_in[N_BLOCK_PAR],
    hls::stream<index_t> idx_des_in[N_BLOCK_PAR],
    
    hls::stream<node>       node_fork_in[N_BLOCK_PAR], //input-streaming message
    hls::stream<edge>       edge_fork_in[N_BLOCK_PAR],
    hls::stream<index_t> idx_des_fork_in[N_BLOCK_PAR],
    
    hls::stream<node>       node_out1[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out1[N_BLOCK_PAR],
	
	hls::stream<node>       node_out2[N_BLOCK_PAR], //output-streaming message
    hls::stream<index_t> idx_des_out2[N_BLOCK_PAR],
   
    //nodeblock
    //weight_t buf_w1[][MLP_LAYER][MAX_WEIGHT_DIM],
    //weight_t buf_b1[][MLP_LAYER][MAX_BIAS_DIM],
	
	weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
){

#pragma HLS stable variable=node_in
#pragma HLS stable variable=edge_in
#pragma HLS stable variable=idx_des_in
 
#pragma HLS INLINE

	int num_iters_src = ceildiv(num_of_src_node, N_BLOCK_PAR) ;
	int num_iters = ceildiv(num_of_edge, N_BLOCK_PAR) ;
	
	index_t idx;
	node n;
	edge e;
	array<attr_t, NODE_BLOCK_IN_DIM> node_block_in;
	node node_block_out;
                
	
    for(int iters = 0; iters < num_iters_src + num_iters; iters ++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = SRC_NODE / N_BLOCK_PAR + MAX_EDGE/N_BLOCK_PAR + 1 
        for(int pe = 0; pe < N_BLOCK_PAR; pe++){
            if (iters < num_iters_src && iters * N_BLOCK_PAR + pe < num_of_src_node){
               
                
                node_fork_in[pe] >> n;
                edge_fork_in[pe] >> e;
                idx_des_fork_in[pe]  >> idx;
              

                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    node_block_in[dim] = n[dim];
                }
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    
                    node_block_in[NODE_DIM+dim] = e[dim];
                }
                //cout<<"* "<<iters * N_BLOCK_PAR + pe;
                
            }
			else if (iters >= num_iters_src && (iters - num_iters_src) * N_BLOCK_PAR + pe < num_of_edge){
             
                //cout <<"nb "<< iters * N_BLOCK_PAR + pe << endl;
                
                node_in[pe] >> n;
                edge_in[pe] >> e;
                idx_des_in[pe]  >> idx;
            

                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    node_block_in[dim] = n[dim];
                }
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    
                    node_block_in[NODE_DIM+dim] = e[dim];
                }
                //cout<<"aa"<<endl;
            }
            else{
                
                for(int dim = 0; dim < NODE_DIM + EDGE_DIM; dim ++){
                    node_block_in[dim] = 0;
                }
                idx = MAX_NODE-1;
            }
			
			MLP<attr_t, weight_t, NODE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, NODE_BLOCK_OUT_DIM, MLP_LAYER>(
				node_block_in, 
				node_block_out,
				//buf_w1[pe],
				//buf_b1[pe]
				buf_w1,
				buf_b1
			); //weight);
			
           
			
			if (iters < num_iters_src && iters * N_BLOCK_PAR + pe < num_of_src_node){
				node_out1[pe] << node_block_out;
                idx_des_out1[pe] << idx;
			}
			else if (iters >= num_iters_src && (iters - num_iters_src) * N_BLOCK_PAR + pe < num_of_edge){
				node_out2[pe] << node_block_out;
                idx_des_out2[pe] << idx;
			}  
        }
    }

}
*/
void node_block_fork_init(
    
    #ifdef __SYNTHESIS__
	node_str node_padding[NS_BLOCK_PAR/2][MAX_NODE],
	#else
	node_str **node_padding,	
	#endif
    index_t *src_list,
    
    hls::stream<node>   node_fork_out[NS_BLOCK_PAR], //output-streaming message
    hls::stream<edge>   edge_fork_out[NS_BLOCK_PAR],
    hls::stream<index_t> idx_fork_out[NS_BLOCK_PAR],
    
    int num_of_src_node,
    bool init
){
//#pragma HLS INLINE 
    
    int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR) ;
    
    node node_des;
    edge edge_des;
    index_t idx_des;
    index_t idx_s;
    
    if(init){
        for(int iters = 0; iters < num_iters_src; iters ++){
    #pragma HLS PIPELINE II=1
    #pragma HLS loop_tripcount max = SRC_NODE / NS_BLOCK_PAR + 1 
            for(int pe = 0; pe < NS_BLOCK_PAR; pe++){
                
                idx_s = iters * NS_BLOCK_PAR + pe;
                if(idx_s < (num_of_src_node)){
                    
                    idx_des = src_list[idx_s];
                    
                    node_str tmp;
                    tmp = node_padding[pe % (NS_BLOCK_PAR/2)][idx_des];
                    
                    
                    for(int dim = 0; dim < NODE_DIM; dim++){
                        
                        node_des[dim] = uint16_to_DTYPE<DTYPE>(tmp(data_w*(dim+1)-1, data_w*(dim)));
                    }
                    for(int dim = 0; dim < EDGE_DIM; dim++){
                        edge_des[dim] = (attr_t)(0);
                    }
                    
                    if(idx_s == 780){
                        cout<<"$ "<<idx_s<<endl;
                        print_fixed(node_des, 8);
                    }
                    
                    node_fork_out[pe] << node_des;
                    edge_fork_out[pe] << edge_des;
                    idx_fork_out[pe]  << idx_des;
                }
            }
        }
    }
}
void node_block_fork(
    
    node_str node_padding[MAX_NODE],
    index_t *src_list,
    
    hls::stream<node>   node_fork_out[NS_BLOCK_PAR], //output-streaming message
    hls::stream<edge>   edge_fork_out[NS_BLOCK_PAR],
    hls::stream<index_t> idx_fork_out[NS_BLOCK_PAR],
    
    int num_of_src_node
){
//#pragma HLS INLINE 
    
    int num_iters_src = ceildiv(num_of_src_node, NS_BLOCK_PAR) ;
    
    node node_des;
    edge edge_des;
    index_t idx_des;
    index_t idx_s;
    
    for(int iters = 0; iters < num_iters_src; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = SRC_NODE / NS_BLOCK_PAR + 1 
        for(int pe = 0; pe < NS_BLOCK_PAR; pe++){
            
            idx_s = iters * NS_BLOCK_PAR + pe;
            if(idx_s < (num_of_src_node)){
                
                idx_des = src_list[idx_s];
                
                for(int dim = 0; dim < NODE_DIM; dim++){
                    node_des[dim] = uint16_to_DTYPE<DTYPE>(node_padding[idx_des](data_w*(dim+1)-1, data_w*(dim)));
                }
                for(int dim = 0; dim < EDGE_DIM; dim++){
                    edge_des[dim] = (attr_t)(0);
                }
				
				if(idx_s == 780){
					cout<<"$ "<<idx_s<<endl;
					print_fixed(node_des, 8);
				}
                
                node_fork_out[pe] << node_des;
                edge_fork_out[pe] << edge_des;
                idx_fork_out[pe]  << idx_des;
            }
        }
    }
}
void adder_down(
    edge    &ein_1,
    edge    &ein_2,
    index_t &idx_in_1,
    index_t &idx_in_2,
    node    &des_in_1,
    node    &des_in_2,
    
    edge    &eout_1,
    edge    &eout_2,
    index_t &idx_out_1,
    index_t &idx_out_2,
    node    &des_out_1,
    node    &des_out_2,
    bool    &sel
    
){
    edge e;
    for(int dim=0; dim<EDGE_DIM; dim++){
        e[dim] = ein_1[dim] + ein_2[dim];
    }
    
    if(idx_in_1 == idx_in_2){
        eout_1 = e;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = (index_t)(MAX_NODE);
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 1;
    }
    else{
        eout_1 = ein_1;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 0;
    }
}
void adder_up(
    edge    &ein_1,
    edge    &ein_2,
    index_t &idx_in_1,
    index_t &idx_in_2,
    node    &des_in_1,
    node    &des_in_2,
    
    edge    &eout_1,
    edge    &eout_2,
    index_t &idx_out_1,
    index_t &idx_out_2,
    node    &des_out_1,
    node    &des_out_2,
    bool    &sel
    
){
    edge e;
    for(int dim=0; dim<EDGE_DIM; dim++){
        e[dim] = ein_1[dim] + ein_2[dim];
    }
    
    if(idx_in_1 == idx_in_2){
        eout_1 = ein_1;
        eout_2 = e;
        idx_out_1 = (index_t)(MAX_NODE);
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 1;
    }
    else{
        eout_1 = ein_1;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 0;
    }
}
void sens_adder_down(
    sens_agg_edge    &ein_1,
    sens_agg_edge    &ein_2,
    index_t &idx_in_1,
    index_t &idx_in_2,
    node    &des_in_1,
    node    &des_in_2,
    
    sens_agg_edge    &eout_1,
    sens_agg_edge    &eout_2,
    index_t &idx_out_1,
    index_t &idx_out_2,
    node    &des_out_1,
    node    &des_out_2,
    bool    &sel
    
){
    sens_agg_edge e;
    for(int dim=0; dim<EDGE_DIM; dim++){
        e[dim] = ein_1[dim] + ein_2[dim];
    }
    
    if(idx_in_1 == idx_in_2){
        eout_1 = e;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = (index_t)(MAX_NODE);
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 1;
    }
    else{
        eout_1 = ein_1;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 0;
    }
}
void sens_adder_up(
    sens_agg_edge    &ein_1,
    sens_agg_edge    &ein_2,
    index_t &idx_in_1,
    index_t &idx_in_2,
    node    &des_in_1,
    node    &des_in_2,
    
    sens_agg_edge    &eout_1,
    sens_agg_edge    &eout_2,
    index_t &idx_out_1,
    index_t &idx_out_2,
    node    &des_out_1,
    node    &des_out_2,
    bool    &sel
    
){
    sens_agg_edge e;
    for(int dim=0; dim<EDGE_DIM; dim++){
        e[dim] = ein_1[dim] + ein_2[dim];
    }
    
    if(idx_in_1 == idx_in_2){
        eout_1 = ein_1;
        eout_2 = e;
        idx_out_1 = (index_t)(MAX_NODE);
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 1;
    }
    else{
        eout_1 = ein_1;
        eout_2 = ein_2;
        idx_out_1 = idx_in_1;
        idx_out_2 = idx_in_2;
        des_out_1 = des_in_1;
        des_out_2 = des_in_2;
        sel = 0;
    }
}
void aggregation_global_par_8_cl_init(
    hls::stream<edge>       edge_local_in[8],   //input-streaming message
    hls::stream<index_t>    idx_local_in[8],
    hls::stream<node>       node_des_in[8],
    
    hls::stream<node>       node_des_out[8], //output-streaming message
    hls::stream<sens_agg_edge>       edge_local_out[8],
    hls::stream<index_t>    idx_local_out[8],
    int num_of_edge,
    bool init
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 8);
    
    static sens_agg_edge        keep_e_pre;
    static index_t     keep_idx_pre;
    static node        keep_n_pre;
    
    if(init){
        for(int dim =0; dim < EDGE_DIM; dim++){
            keep_e_pre[dim] = 0;
        }
        keep_idx_pre = (index_t)(MAX_NODE-1);
    }
    
    //cout<<"aggregation start idx "<<keep_idx_pre<<", keep_e_pre "<<endl;
    //print_fixed(keep_e_pre, 8);
    
    sens_agg_edge        keep_e;
    index_t     keep_idx;
    node        keep_n;
    /*
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
	*/
    
    index_t     idx_in[8];
    sens_agg_edge        edge_in[8];
    node        node_des[8];
    
    sens_agg_edge        zero = {0,0,0,0};
    index_t     save_last_idx;
    node        save_last_node;
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        //cout<<"iters "<< iters << endl;
        
        for(int pe = 0; pe < 8; pe++){

            
            if(iters * 8 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                sens_read_edge(edge_local_in[pe],edge_in[pe]);
                node_des_in[pe] >> node_des[pe];
                
                save_last_idx = idx_in[pe];
                save_last_node = node_des[pe];
            }
            else{
                idx_in[pe] =  save_last_idx;
                edge_in[pe] = zero;
                node_des[pe] = save_last_node;
                
            }
            
            /*
            if(iters * 8 + pe == num_of_edge-1){
               
            }
            */
            if(idx_in[pe] == 949){
                //cout<<"idx 1 agg_in"<<endl;
                //print_fixed(edge_in[pe], 8);
            }
            
			
        }
           
        sens_agg_edge    e_out_1[2][3];
        sens_agg_edge    e_out_2[2][3];
        index_t i_out_1[2][3];
        index_t i_out_2[2][3];
        node    n_out_1[2][3];
        node    n_out_2[2][3];
        bool    sel[2][3];
        
        sens_agg_edge    e_out_can[2][4];
        index_t i_out_can[2][4];
        node    n_out_can[2][4];
        
        sens_agg_edge    e_out[2][5];
        index_t i_out[2][5];
        node    n_out[2][5];
        
        sens_agg_edge    e_out_s2[9];
        index_t i_out_s2[9];
        node    n_out_s2[9];
        
        sens_agg_edge    e_out_s3[9];
        index_t i_out_s3[9];
        node    n_out_s3[9];
        
        //sens_agg_edge    tmp[2];
        
        int j[2];
        int r_i[2];
        
        int case_i[2];
        int case_j[2];
        
        for(int add_pe=0; add_pe<2; add_pe++){
        //#pragma HLS UNROLL
            sens_adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            sens_adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            sens_adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
            
            case_i[add_pe] = sel[add_pe][2] + sel[add_pe][1] + sel[add_pe][0];
            switch(case_i[add_pe]){
			case 0: 
				case_j[add_pe]=4;
				break;
			case 1: 
				case_j[add_pe]=3;
				break;
			case 2: 
				case_j[add_pe]=2;
				break;
			case 3: 
				case_j[add_pe]=1;
				break;
            }  
            
            j[add_pe] = 0;
            for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
                if(i_out_can[add_pe][i]!=MAX_NODE){
                    e_out[add_pe][j[add_pe]] = e_out_can[add_pe][i];
                    n_out[add_pe][j[add_pe]] = n_out_can[add_pe][i];
                    i_out[add_pe][j[add_pe]] = i_out_can[add_pe][i];
                    j[add_pe]++;
                }
            }
        } 
		
		
		
		//cout<<"j vs. casej "<<j[0]<<" "<<j[1]<<" "<<case_j[0]<<" "<<case_j[1]<<endl;
		
		//cout<<"i_out[0]"<<" ";
        /*
		for(int i=0; i< j[0]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[0][i]<<" ";
        }
		cout<<endl;
		//cout<<"i_out[1]"<<" ";
		for(int i=0; i< j[1]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[1][i]<<" ";
        }
		cout<<endl;
        for(int i=0; i< j[2]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[2][i]<<" ";
        }
		cout<<endl;
		*/
		bool add_pe_1_bias;
		bool add_pe_2_bias;
		
        //===========================================================================//
        
		sens_agg_edge tmp_merge_01;
		for(int dim=0; dim<4; dim++){
			tmp_merge_01[dim] = e_out[0][0][dim] + keep_e_pre[dim];
		}
		
		sens_agg_edge tmp_merge_12;
		for(int dim=0; dim<4; dim++){
			tmp_merge_12[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim];
		}
        
		
		sens_agg_edge tmp_merge_012;
		for(int dim=0; dim<4; dim++){
			tmp_merge_012[dim] = e_out[0][0][dim] + e_out[1][0][dim] + keep_e_pre[dim];
		}
        
		add_pe_1_bias = (keep_idx_pre          == i_out[0][0]) ? 1: 0;
		add_pe_2_bias = (i_out[0][case_j[0]-1] == i_out[1][0]) ? 1: 0;

        
        
        if(case_j[1]==1){
            if(case_j[0]==1){
                if(add_pe_1_bias && add_pe_2_bias){
                    keep_e   = tmp_merge_012;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else if(add_pe_2_bias){
                    keep_e   = tmp_merge_12;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else{
                    keep_e   = e_out[1][0];
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }
            }else{
                if(add_pe_2_bias){
                    keep_e   = tmp_merge_12;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else{
                    keep_e   = e_out[1][0];
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }
            }
           
        }else{
            keep_e   = e_out[1][case_j[1]-1];
            keep_idx = i_out[1][case_j[1]-1];
            keep_n   = n_out[1][case_j[1]-1];
        }
  
		
		//cout<<"check"<<case_j[0]<<" "<<case_j[1]<<" "<<add_pe_1_bias<<" "<<add_pe_2_bias<<endl;
		//bool merge=(add_pe_1_bias == 1 && add_pe_2_bias == 1 && case_j[0] == 1);
		//bool merge_top = (add_pe_1_bias == 1 && case_j[0] == 1);
        

        bool merge_012  =  (add_pe_1_bias && add_pe_2_bias && (case_j[0]==1));
        bool merge_01   =  (add_pe_1_bias);
        bool merge_12   =  (add_pe_2_bias);
        
		
		if(merge_012){
			e_out_s2[0] = tmp_merge_012;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPII "<<0<<" "<<i_out[0][0]<<endl;
	
		}else if(merge_01){
			e_out_s2[0] = tmp_merge_01;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPIII "<<0<<" "<<i_out[0][0]<<endl;
	
		}
		else{
			e_out_s2[0] = keep_e_pre;
			n_out_s2[0] = keep_n_pre;
			i_out_s2[0] = keep_idx_pre;
			//cout<<"PPIV "<<0<<" "<<keep_idx_pre<<endl;
		}
		
        if(add_pe_1_bias){
            for(int i=0; i<4; i++){
                if(i>=1 && i<case_j[0]-1){
                    //cout<<"AAI "<<i<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i] = e_out[0][i];
                    n_out_s2[i] = n_out[0][i];
                    i_out_s2[i] = i_out[0][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[0]-1){
                    //cout<<"AAII "<<i+1<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i+1] = e_out[0][i];
                    n_out_s2[i+1] = n_out[0][i];
                    i_out_s2[i+1] = i_out[0][i];
                }
            }
        }
        /*
		for(int i=0; i<4; i++){
			
            if(i>=add_pe_1_bias && i<case_j[0]-1){
				//cout<<"AAI"<<i+(!add_pe_1_bias)<<" "<<i_out[0][i]<<endl;
				e_out_s2[i+(!add_pe_1_bias)] = e_out[0][i];
                n_out_s2[i+(!add_pe_1_bias)] = n_out[0][i];
                i_out_s2[i+(!add_pe_1_bias)] = i_out[0][i];
			}
		}
        */
		
        int casej_s2_t1 = case_j[0]+(!add_pe_1_bias);
		
      
		if(merge_12 && !(merge_012 || (merge_01 && case_j[0]==1))){
			//cout<<"BBII "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_12;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if (!(merge_012 || (merge_01 && case_j[0]==1))){
            //cout<<"BBIII "<<casej_s2_t1-1<<" "<<i_out[0][case_j[0]-1]<<endl;
            e_out_s2[casej_s2_t1-1] = e_out[0][case_j[0]-1];
			n_out_s2[casej_s2_t1-1] = n_out[0][case_j[0]-1];
			i_out_s2[casej_s2_t1-1] = i_out[0][case_j[0]-1];
        }
        
        if(add_pe_2_bias){
            for(int i=1; i<4; i++){
                if(i<case_j[1]){
                    //cout<<"CCI "<<casej_s2_t1+i-(1)<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i-(1)] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i-(1)] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i-(1)] = i_out[1][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[1]){
                    //cout<<"CCII "<<casej_s2_t1+i<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i] = i_out[1][i];
                }
            }
        }
       
        //======================================================================================//
		
        //==================================================================================//
        
		int casej_s3= 1+case_j[0]+case_j[1]-(add_pe_1_bias)-(add_pe_2_bias);
        
       
        
		//cout<<"final "<<casej_s3-1<<endl;
		//cout<<"keepi "<<keep_idx<<endl;
        //print_fixed(keep_e, 8);
		
		
		
		//cout<<"i_out_s2"<<" ";
		//for(int i=0; i< casej_s3; i++){ //adder_pe 2 tmp save to e_out[2]
        //    cout<<i_out_s2[i]<<" ";
        //}
		//cout<<endl;
		
        keep_e_pre = keep_e;
        keep_idx_pre = keep_idx;
        keep_n_pre = keep_n;
		
        for(int out_pe = 0; out_pe < 8; out_pe ++){
#pragma HLS UNROLL
            if(iters * 8 + out_pe < num_of_edge){
                
               //cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 >= casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"index "<< index_t(MAX_NODE-1) << endl;
					
                }
                else if(out_pe + 1 < casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"index "<<  i_out_s2[out_pe] << endl;
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
                    
					if(i_out_s2[out_pe] == 949){
						cout<<"BBB: idx 1 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
                    
                    
                }
            }
        } 
    }
}
void aggregation_global_par_8_cl(
    hls::stream<edge>       edge_local_in[8],   //input-streaming message
    hls::stream<index_t>    idx_local_in[8],
    hls::stream<node>       node_des_in[8],
    
    hls::stream<node>       node_des_out[8], //output-streaming message
    hls::stream<sens_agg_edge>       edge_local_out[8],
    hls::stream<index_t>    idx_local_out[8],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 8);
    
    sens_agg_edge        keep_e_pre = {0,0,0,0};
    index_t     keep_idx_pre = (index_t)(MAX_NODE-1);
    node        keep_n_pre;
    
    
    sens_agg_edge        keep_e;
    index_t     keep_idx;
    node        keep_n;
    /*
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
	*/
    
    index_t     idx_in[8];
    sens_agg_edge        edge_in[8];
    node        node_des[8];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        cout<<"iters "<< iters << endl;
        
        for(int pe = 0; pe < 8; pe++){

            if(iters * 8 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                sens_read_edge(edge_local_in[pe],edge_in[pe]);
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE);
            }
            
            
            if(idx_in[pe] == 11){
                cout<<"idx 1 agg_in"<<endl;
                print_fixed(edge_in[pe], 8);
            }
            
			
        }
           
        sens_agg_edge    e_out_1[2][3];
        sens_agg_edge    e_out_2[2][3];
        index_t i_out_1[2][3];
        index_t i_out_2[2][3];
        node    n_out_1[2][3];
        node    n_out_2[2][3];
        bool    sel[2][3];
        
        sens_agg_edge    e_out_can[2][4];
        index_t i_out_can[2][4];
        node    n_out_can[2][4];
        
        sens_agg_edge    e_out[2][5];
        index_t i_out[2][5];
        node    n_out[2][5];
        
        sens_agg_edge    e_out_s2[9];
        index_t i_out_s2[9];
        node    n_out_s2[9];
        
        sens_agg_edge    e_out_s3[9];
        index_t i_out_s3[9];
        node    n_out_s3[9];
        
        //sens_agg_edge    tmp[2];
        
        int j[2];
        int r_i[2];
        
        int case_i[2];
        int case_j[2];
        
        for(int add_pe=0; add_pe<2; add_pe++){
        //#pragma HLS UNROLL
            sens_adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            sens_adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            sens_adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
            
            case_i[add_pe] = sel[add_pe][2] + sel[add_pe][1] + sel[add_pe][0];
            switch(case_i[add_pe]){
			case 0: 
				case_j[add_pe]=4;
				break;
			case 1: 
				case_j[add_pe]=3;
				break;
			case 2: 
				case_j[add_pe]=2;
				break;
			case 3: 
				case_j[add_pe]=1;
				break;
            }  
            
            j[add_pe] = 0;
            for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
                if(i_out_can[add_pe][i]!=MAX_NODE){
                    e_out[add_pe][j[add_pe]] = e_out_can[add_pe][i];
                    n_out[add_pe][j[add_pe]] = n_out_can[add_pe][i];
                    i_out[add_pe][j[add_pe]] = i_out_can[add_pe][i];
                    j[add_pe]++;
                }
            }
        } 
		
		
		
		//cout<<"j vs. casej "<<j[0]<<" "<<j[1]<<" "<<case_j[0]<<" "<<case_j[1]<<endl;
		
		//cout<<"i_out[0]"<<" ";
        /*
		for(int i=0; i< j[0]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[0][i]<<" ";
        }
		cout<<endl;
		//cout<<"i_out[1]"<<" ";
		for(int i=0; i< j[1]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[1][i]<<" ";
        }
		cout<<endl;
        for(int i=0; i< j[2]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[2][i]<<" ";
        }
		cout<<endl;
		*/
		bool add_pe_1_bias;
		bool add_pe_2_bias;
		
        //===========================================================================//
        
		sens_agg_edge tmp_merge_01;
		for(int dim=0; dim<4; dim++){
			tmp_merge_01[dim] = e_out[0][0][dim] + keep_e_pre[dim];
		}
		
		sens_agg_edge tmp_merge_12;
		for(int dim=0; dim<4; dim++){
			tmp_merge_12[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim];
		}
        
		
		sens_agg_edge tmp_merge_012;
		for(int dim=0; dim<4; dim++){
			tmp_merge_012[dim] = e_out[0][0][dim] + e_out[1][0][dim] + keep_e_pre[dim];
		}
        
		add_pe_1_bias = (keep_idx_pre          == i_out[0][0]) ? 1: 0;
		add_pe_2_bias = (i_out[0][case_j[0]-1] == i_out[1][0]) ? 1: 0;

        
        
        if(case_j[1]==1){
            if(case_j[0]==1){
                if(add_pe_1_bias && add_pe_2_bias){
                    keep_e   = tmp_merge_012;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else if(add_pe_2_bias){
                    keep_e   = tmp_merge_12;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else{
                    keep_e   = e_out[1][0];
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }
            }else{
                if(add_pe_2_bias){
                    keep_e   = tmp_merge_12;
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }else{
                    keep_e   = e_out[1][0];
                    keep_idx = i_out[1][0];
                    keep_n   = n_out[1][0];
                }
            }
           
        }else{
            keep_e   = e_out[1][case_j[1]-1];
            keep_idx = i_out[1][case_j[1]-1];
            keep_n   = n_out[1][case_j[1]-1];
        }
  
		
		//cout<<"check"<<case_j[0]<<" "<<case_j[1]<<" "<<add_pe_1_bias<<" "<<add_pe_2_bias<<endl;
		//bool merge=(add_pe_1_bias == 1 && add_pe_2_bias == 1 && case_j[0] == 1);
		//bool merge_top = (add_pe_1_bias == 1 && case_j[0] == 1);
        

        bool merge_012  =  (add_pe_1_bias && add_pe_2_bias && (case_j[0]==1));
        bool merge_01   =  (add_pe_1_bias);
        bool merge_12   =  (add_pe_2_bias);
        
		
		if(merge_012){
			e_out_s2[0] = tmp_merge_012;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPII "<<0<<" "<<i_out[0][0]<<endl;
	
		}else if(merge_01){
			e_out_s2[0] = tmp_merge_01;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPIII "<<0<<" "<<i_out[0][0]<<endl;
	
		}
		else{
			e_out_s2[0] = keep_e_pre;
			n_out_s2[0] = keep_n_pre;
			i_out_s2[0] = keep_idx_pre;
			cout<<"PPIV "<<0<<" "<<keep_idx_pre<<endl;
		}
		
        if(add_pe_1_bias){
            for(int i=0; i<4; i++){
                if(i>=1 && i<case_j[0]-1){
                    cout<<"AAI "<<i<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i] = e_out[0][i];
                    n_out_s2[i] = n_out[0][i];
                    i_out_s2[i] = i_out[0][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[0]-1){
                    cout<<"AAII "<<i+1<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i+1] = e_out[0][i];
                    n_out_s2[i+1] = n_out[0][i];
                    i_out_s2[i+1] = i_out[0][i];
                }
            }
        }
        /*
		for(int i=0; i<4; i++){
			
            if(i>=add_pe_1_bias && i<case_j[0]-1){
				//cout<<"AAI"<<i+(!add_pe_1_bias)<<" "<<i_out[0][i]<<endl;
				e_out_s2[i+(!add_pe_1_bias)] = e_out[0][i];
                n_out_s2[i+(!add_pe_1_bias)] = n_out[0][i];
                i_out_s2[i+(!add_pe_1_bias)] = i_out[0][i];
			}
		}
        */
		
        int casej_s2_t1 = case_j[0]+(!add_pe_1_bias);
		
      
		if(merge_12 && !(merge_012 || (merge_01 && case_j[0]==1))){
			cout<<"BBII "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_12;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if (!(merge_012 || (merge_01 && case_j[0]==1))){
            cout<<"BBIII "<<casej_s2_t1-1<<" "<<i_out[0][case_j[0]-1]<<endl;
            e_out_s2[casej_s2_t1-1] = e_out[0][case_j[0]-1];
			n_out_s2[casej_s2_t1-1] = n_out[0][case_j[0]-1];
			i_out_s2[casej_s2_t1-1] = i_out[0][case_j[0]-1];
        }
        
        if(add_pe_2_bias){
            for(int i=1; i<4; i++){
                if(i<case_j[1]){
                    cout<<"CCI "<<casej_s2_t1+i-(1)<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i-(1)] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i-(1)] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i-(1)] = i_out[1][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[1]){
                    cout<<"CCII "<<casej_s2_t1+i<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i] = i_out[1][i];
                }
            }
        }
       
        //======================================================================================//
		
        //==================================================================================//
        
		int casej_s3= 1+case_j[0]+case_j[1]-(add_pe_1_bias)-(add_pe_2_bias);
        
       
        
		cout<<"final "<<casej_s3-1<<endl;
		cout<<"keepi "<<keep_idx<<endl;
        print_fixed(keep_e, 8);
		
		
		
		cout<<"i_out_s2"<<" ";
		for(int i=0; i< casej_s3; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out_s2[i]<<" ";
        }
		cout<<endl;
		
        keep_e_pre = keep_e;
        keep_idx_pre = keep_idx;
        keep_n_pre = keep_n;
		
        for(int out_pe = 0; out_pe < 8; out_pe ++){
#pragma HLS UNROLL
            if(iters * 8 + out_pe < num_of_edge){
                
               //cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 >= casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"index "<< index_t(MAX_NODE-1) << endl;
					
                }
                else if(out_pe + 1 < casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"index "<<  i_out_s2[out_pe] << endl;
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
                    
					if(i_out_s2[out_pe] == 11){
						cout<<"BBB: idx 11 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
                    
                    
                }
            }
        } 
    }
}
void aggregation_global_par_16(
    hls::stream<edge>       edge_local_in[16],   //input-streaming message
    hls::stream<index_t>    idx_local_in[16],
    hls::stream<node>       node_des_in[16],
    
    hls::stream<node>       node_des_out[16], //output-streaming message
    hls::stream<edge>       edge_local_out[16],
    hls::stream<index_t>    idx_local_out[16],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 16);
    
    edge        keep_e_pre = {0,0,0,0};
    index_t     keep_idx_pre = (index_t)(MAX_NODE-1);
    node        keep_n_pre;
    
    edge        keep_e;
    index_t     keep_idx;
    node        keep_n;
    /*
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
	*/
    
    index_t     idx_in[16];
    edge        edge_in[16];
    node        node_des[16];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        //cout<<"iters "<< iters << endl;
        
        for(int pe = 0; pe < 16; pe++){

            if(iters * 16 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE);
            }
            
            /*
            if(idx_in[pe] == 70){
                cout<<"idx 70 agg_in"<<endl;
                print_fixed(edge_in[pe], 8);
            }
            */
            
			
        }
           
        edge    e_out_1[4][3];
        edge    e_out_2[4][3];
        index_t i_out_1[4][3];
        index_t i_out_2[4][3];
        node    n_out_1[4][3];
        node    n_out_2[4][3];
        bool    sel[4][3];
        
        edge    e_out_can[4][4];
        index_t i_out_can[4][4];
        node    n_out_can[4][4];
        
        edge    e_out[4][5];
        index_t i_out[4][5];
        node    n_out[4][5];
        
        edge    e_out_s2[17];
        index_t i_out_s2[17];
        node    n_out_s2[17];
        
        edge    e_out_s3[17];
        index_t i_out_s3[17];
        node    n_out_s3[17];
        
        //edge    tmp[2];
        
        int j[4];
        
        ap_uint<3> case_i[4];
        ap_uint<3> case_j[4];
        
        for(int add_pe=0; add_pe<4; add_pe++){
        //#pragma HLS UNROLL
            adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
            
            case_i[add_pe] = sel[add_pe][2] + sel[add_pe][1] + sel[add_pe][0];
            switch(case_i[add_pe]){
			case 0: 
				case_j[add_pe]=4;
				break;
			case 1: 
				case_j[add_pe]=3;
				break;
			case 2: 
				case_j[add_pe]=2;
				break;
			case 3: 
				case_j[add_pe]=1;
				break;
            }  
            
            j[add_pe] = 0;
            for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
                if(i_out_can[add_pe][i]!=MAX_NODE){
                    e_out[add_pe][j[add_pe]] = e_out_can[add_pe][i];
                    n_out[add_pe][j[add_pe]] = n_out_can[add_pe][i];
                    i_out[add_pe][j[add_pe]] = i_out_can[add_pe][i];
                    j[add_pe]++;
                }
            }
        } 
		
		
		
		//cout<<"j vs. casej "<<j[0]<<" "<<j[1]<<" "<<case_j[0]<<" "<<case_j[1]<<endl;
		
		//cout<<"i_out[0]"<<" ";
        /*
		for(int i=0; i< j[0]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[0][i]<<" ";
        }
		cout<<endl;
		//cout<<"i_out[1]"<<" ";
		for(int i=0; i< j[1]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[1][i]<<" ";
        }
		cout<<endl;
        for(int i=0; i< j[2]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[2][i]<<" ";
        }
		cout<<endl;
		*/
		bool add_pe_1_bias;
		bool add_pe_2_bias;
        bool add_pe_3_bias;
        bool add_pe_4_bias;
		
        //===========================================================================//
        
		edge tmp_merge_01;
		for(int dim=0; dim<4; dim++){
			tmp_merge_01[dim] = e_out[0][0][dim] + keep_e_pre[dim];
		}
		
		edge tmp_merge_12;
		for(int dim=0; dim<4; dim++){
			tmp_merge_12[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim];
		}
        
        edge tmp_merge_23;
		for(int dim=0; dim<4; dim++){
			tmp_merge_23[dim] = e_out[1][case_j[1]-1][dim] + e_out[2][0][dim];
		}
        
        edge tmp_merge_34;
		for(int dim=0; dim<4; dim++){
			tmp_merge_34[dim] = e_out[2][case_j[2]-1][dim] + e_out[3][0][dim];
		}
		
		edge tmp_merge_012;
		for(int dim=0; dim<4; dim++){
			tmp_merge_012[dim] = e_out[0][0][dim] + e_out[1][0][dim] + keep_e_pre[dim];
		}
        
        edge tmp_merge_123;
		for(int dim=0; dim<4; dim++){
			tmp_merge_123[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim] + e_out[2][0][dim];
		}
        
        edge tmp_merge_234;
		for(int dim=0; dim<4; dim++){
			tmp_merge_234[dim] = e_out[1][case_j[1]-1][dim] + e_out[2][0][dim] + e_out[3][0][dim];
		}
        
        edge tmp_merge_0123;
		for(int dim=0; dim<4; dim++){
			tmp_merge_0123[dim] = e_out[0][0][dim] + e_out[1][0][dim] + e_out[2][0][dim] + keep_e_pre[dim];
		}
        
        edge tmp_merge_1234;
		for(int dim=0; dim<4; dim++){
			tmp_merge_1234[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim] + e_out[2][0][dim] + e_out[3][0][dim];
		}
        
        edge tmp_merge_01234;
		for(int dim=0; dim<4; dim++){
			tmp_merge_01234[dim] = keep_e_pre[dim] + e_out[0][0][dim] + e_out[1][0][dim] + e_out[2][0][dim] + e_out[3][0][dim];
		}
		
		add_pe_1_bias = (keep_idx_pre          == i_out[0][0]) ? 1: 0;
		add_pe_2_bias = (i_out[0][case_j[0]-1] == i_out[1][0]) ? 1: 0;
        add_pe_3_bias = (i_out[1][case_j[1]-1] == i_out[2][0]) ? 1: 0;
        add_pe_4_bias = (i_out[2][case_j[2]-1] == i_out[3][0]) ? 1: 0;
        
        if(case_j[3]==1){
            if(case_j[2]==1){
                if(case_j[1]==1){
                    if(case_j[0]==1){
                        if(add_pe_1_bias && add_pe_2_bias && add_pe_3_bias && add_pe_4_bias){
                            keep_e   = tmp_merge_01234;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else if(add_pe_2_bias && add_pe_3_bias && add_pe_4_bias){
                            keep_e   = tmp_merge_1234;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else if(add_pe_3_bias && add_pe_4_bias){
                            keep_e   = tmp_merge_234;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else if(add_pe_4_bias){
                            keep_e   = tmp_merge_34;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else{
                            keep_e   = e_out[3][0];
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }
                    }else{
                        if(add_pe_2_bias && add_pe_3_bias && add_pe_4_bias){
                            keep_e   = tmp_merge_1234;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else if(add_pe_3_bias && add_pe_4_bias){
                            keep_e   = tmp_merge_234;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else if(add_pe_4_bias){
                            keep_e   = tmp_merge_34;
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }else{
                            keep_e   = e_out[3][0];
                            keep_idx = i_out[3][0];
                            keep_n   = n_out[3][0];
                        }
                    }
                }else{
                   if(add_pe_3_bias && add_pe_4_bias){
                        keep_e   = tmp_merge_234;
                        keep_idx = i_out[3][0];
                        keep_n   = n_out[3][0];
                    }
                    else if(add_pe_4_bias){
                        keep_e   = tmp_merge_34;
                        keep_idx = i_out[3][0];
                        keep_n   = n_out[3][0];
                    }else{
                        keep_e   = e_out[3][0];
                        keep_idx = i_out[3][0];
                        keep_n   = n_out[3][0];
                    }
                }
            }else{
               if(add_pe_4_bias){
                    keep_e   = tmp_merge_34;
                    keep_idx = i_out[3][0];
                    keep_n   = n_out[3][0];
               }else{
                    keep_e   = e_out[3][0];
                    keep_idx = i_out[3][0];
                    keep_n   = n_out[3][0];
               }
            }
        }else{
            keep_e   = e_out[3][case_j[3]-1];
            keep_idx = i_out[3][case_j[3]-1];
            keep_n   = n_out[3][case_j[3]-1];
        }
		
		//cout<<"check"<<case_j[0]<<" "<<case_j[1]<<" "<<add_pe_1_bias<<" "<<add_pe_2_bias<<endl;
		//bool merge=(add_pe_1_bias == 1 && add_pe_2_bias == 1 && case_j[0] == 1);
		//bool merge_top = (add_pe_1_bias == 1 && case_j[0] == 1);
        
        bool merge_01234 =  (add_pe_1_bias && add_pe_2_bias && add_pe_3_bias && add_pe_4_bias && (case_j[0]==1) && (case_j[1]==1) && (case_j[2]==1));
        
        
        bool merge_0123 =  (add_pe_1_bias && add_pe_2_bias && add_pe_3_bias && (case_j[0]==1) && (case_j[1]==1));
        bool merge_012  =  (add_pe_1_bias && add_pe_2_bias && (case_j[0]==1));
        bool merge_01   =  (add_pe_1_bias);
        
        bool merge_1234 =  (add_pe_2_bias && add_pe_3_bias && add_pe_4_bias && (case_j[1]==1) && (case_j[2]==1));
        bool merge_123  =  (add_pe_2_bias && add_pe_3_bias && (case_j[1]==1));
        bool merge_12   =  (add_pe_2_bias);
        
        bool merge_234  =  (add_pe_3_bias && add_pe_4_bias && (case_j[2]==1));
        bool merge_23   =  (add_pe_3_bias);
        
        bool merge_34   =  (add_pe_4_bias);
		
		if(merge_01234){
            e_out_s2[0] = tmp_merge_01234;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPJ "<<0<<" "<<i_out[0][0]<<endl;
        }
        else if(merge_0123)
		{
			e_out_s2[0] = tmp_merge_0123;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPI "<<0<<" "<<i_out[0][0]<<endl;
			
		}else if(merge_012){
			e_out_s2[0] = tmp_merge_012;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPII "<<0<<" "<<i_out[0][0]<<endl;
	
		}else if(merge_01){
			e_out_s2[0] = tmp_merge_01;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			//cout<<"PPIII "<<0<<" "<<i_out[0][0]<<endl;
	
		}
		else{
			e_out_s2[0] = keep_e_pre;
			n_out_s2[0] = keep_n_pre;
			i_out_s2[0] = keep_idx_pre;
			//cout<<"PPIV "<<0<<" "<<keep_idx_pre<<endl;
		}
		
		/*
        if(add_pe_1_bias){
            for(int i=1; i<4; i++){
                if(i+1<case_j[0]){
                    //cout<<"AAI "<<i<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i] = e_out[0][i];
                    n_out_s2[i] = n_out[0][i];
                    i_out_s2[i] = i_out[0][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i+1<case_j[0]){
                    //cout<<"AAII "<<i+1<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i+1] = e_out[0][i];
                    n_out_s2[i+1] = n_out[0][i];
                    i_out_s2[i+1] = i_out[0][i];
                }
            }
        }
		*/
		
		switch(add_pe_1_bias){
			case 1:
				for(int i=1; i<4; i++){
					if(i+1<case_j[0]){
						//cout<<"AAI "<<i<<" "<<i_out[0][i]<<endl;
						e_out_s2[i] = e_out[0][i];
						n_out_s2[i] = n_out[0][i];
						i_out_s2[i] = i_out[0][i];
					}
				}
			break;
			case 0:
				for(int i=0; i<4; i++){
					if(i+1<case_j[0]){
						//cout<<"AAII "<<i+1<<" "<<i_out[0][i]<<endl;
						e_out_s2[i+1] = e_out[0][i];
						n_out_s2[i+1] = n_out[0][i];
						i_out_s2[i+1] = i_out[0][i];
					}
				}
			break;	
		}
        /*
		for(int i=0; i<4; i++){
			
            if(i>=add_pe_1_bias && i<case_j[0]-1){
				//cout<<"AAI"<<i+(!add_pe_1_bias)<<" "<<i_out[0][i]<<endl;
				e_out_s2[i+(!add_pe_1_bias)] = e_out[0][i];
                n_out_s2[i+(!add_pe_1_bias)] = n_out[0][i];
                i_out_s2[i+(!add_pe_1_bias)] = i_out[0][i];
			}
		}
        */
		
        ap_uint<5> casej_s2_t1 = case_j[0]+(!add_pe_1_bias);
		
        //wrap 12
        //cout<<"BB con "<<merge_0123<<" "<<merge_0123<<" "<<merge_012<<" "<<(!(merge_0123 || merge_012))<<endl;
		if(merge_1234 && !(merge_01234 || merge_0123 || merge_012 || (merge_01 && case_j[0]==1))){
            //cout<<"BBJ "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_1234;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
        }
        else if(merge_123 && !(merge_01234 || merge_0123 || merge_012 || (merge_01 && case_j[0]==1))){
			//cout<<"BBI "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_123;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if(merge_12 && !(merge_01234 || merge_0123 || merge_012|| (merge_01 && case_j[0]==1))){
			//cout<<"BBII "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_12;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if (!(merge_01234 || merge_0123 || merge_012|| (merge_01 && case_j[0]==1))){
            //cout<<"BBIII "<<casej_s2_t1-1<<" "<<i_out[0][case_j[0]-1]<<endl;
            e_out_s2[casej_s2_t1-1] = e_out[0][case_j[0]-1];
			n_out_s2[casej_s2_t1-1] = n_out[0][case_j[0]-1];
			i_out_s2[casej_s2_t1-1] = i_out[0][case_j[0]-1];
        }
        
		/*
        if(add_pe_2_bias){
            for(int i=1; i<4; i++){
                if(i+1<case_j[1]){
                    //cout<<"CCI "<<casej_s2_t1+i-(1)<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i-(1)] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i-(1)] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i-(1)] = i_out[1][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i+1<case_j[1]){
                    //cout<<"CCII "<<casej_s2_t1+i<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i] = i_out[1][i];
                }
            }
        }
		*/
		switch(add_pe_2_bias){
			case 1:
				for(int i=1; i<4; i++){
					if(i+1<case_j[1]){
						//cout<<"CCI "<<casej_s2_t1+i-(1)<<" "<<i_out[1][i]<<endl;
						e_out_s2[casej_s2_t1+i-(1)] = e_out[1][i];
						n_out_s2[casej_s2_t1+i-(1)] = n_out[1][i];
						i_out_s2[casej_s2_t1+i-(1)] = i_out[1][i];
					}
				}
			break;
			case 0:
				for(int i=0; i<4; i++){
					if(i+1<case_j[1]){
						//cout<<"CCII "<<casej_s2_t1+i<<" "<<i_out[1][i]<<endl;
						e_out_s2[casej_s2_t1+i] = e_out[1][i];
						n_out_s2[casej_s2_t1+i] = n_out[1][i];
						i_out_s2[casej_s2_t1+i] = i_out[1][i];
					}
				}
			break;	
		}
        /*
		for(int i=0; i<4; i++){
			if(i>=add_pe_2_bias && i<case_j[1]-1){
				//cout<<"CCI"<<case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)<<" "<<i_out[1][i]<<endl;
				e_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = e_out[1][i];
                n_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = n_out[1][i];
                i_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = i_out[1][i];
			}
		}
        */
        
        ap_uint<5> casej_s2_t2 = case_j[1]+case_j[0]+1-(add_pe_1_bias)-(add_pe_2_bias);
		
        //wrap 23
        if(merge_234 && !(merge_01234 || merge_1234 || merge_0123  || merge_123 || (((merge_12 || merge_012) && case_j[1]==1)))){
            //cout<<"DDJ "<<casej_s2_t2-1<<" "<<i_out[2][0]<<endl;
			e_out_s2[casej_s2_t2-1] = tmp_merge_234;
			n_out_s2[casej_s2_t2-1] = n_out[2][0];
			i_out_s2[casej_s2_t2-1] = i_out[2][0];
        }
        else if(merge_23 && !(merge_01234 || merge_1234 || merge_0123  || merge_123 || (((merge_12 || merge_012) && case_j[1]==1)))){
			//cout<<"DDI "<<casej_s2_t2-1<<" "<<i_out[2][0]<<endl;
			e_out_s2[casej_s2_t2-1] = tmp_merge_23;
			n_out_s2[casej_s2_t2-1] = n_out[2][0];
			i_out_s2[casej_s2_t2-1] = i_out[2][0];
		}else if (!(merge_01234 || merge_1234 || merge_0123 || merge_123 || (((merge_12 || merge_012) && case_j[1]==1)))){
            //cout<<"DDII "<<casej_s2_t2-1<<" "<<i_out[1][case_j[1]-1]<<endl;
            e_out_s2[casej_s2_t2-1] = e_out[1][case_j[1]-1];
			n_out_s2[casej_s2_t2-1] = n_out[1][case_j[1]-1];
			i_out_s2[casej_s2_t2-1] = i_out[1][case_j[1]-1];
        }
        
		/*
        if(add_pe_3_bias){
            for(int i=1; i<4; i++){
                if(i+1<case_j[2]){
                    //cout<<"EEI "<<casej_s2_t2+i-(1)<<" "<<i_out[2][i]<<endl;
                    e_out_s2[casej_s2_t2+i-(1)] = e_out[2][i];
                    n_out_s2[casej_s2_t2+i-(1)] = n_out[2][i];
                    i_out_s2[casej_s2_t2+i-(1)] = i_out[2][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i+1<case_j[2]){
                    //cout<<"EEII "<<casej_s2_t2+i<<" "<<i_out[2][i]<<endl;
                    e_out_s2[casej_s2_t2+i] = e_out[2][i];
                    n_out_s2[casej_s2_t2+i] = n_out[2][i];
                    i_out_s2[casej_s2_t2+i] = i_out[2][i];
                }
            }
        }
		*/
		
		switch(add_pe_3_bias){
			case 1:
				for(int i=1; i<4; i++){
					if(i+1<case_j[2]){
						//cout<<"EEI "<<casej_s2_t2+i-(1)<<" "<<i_out[2][i]<<endl;
						e_out_s2[casej_s2_t2+i-(1)] = e_out[2][i];
						n_out_s2[casej_s2_t2+i-(1)] = n_out[2][i];
						i_out_s2[casej_s2_t2+i-(1)] = i_out[2][i];
					}
				}
			break;
			case 0:
				for(int i=0; i<4; i++){
					if(i+1<case_j[2]){
						//cout<<"EEII "<<casej_s2_t2+i<<" "<<i_out[2][i]<<endl;
						e_out_s2[casej_s2_t2+i] = e_out[2][i];
						n_out_s2[casej_s2_t2+i] = n_out[2][i];
						i_out_s2[casej_s2_t2+i] = i_out[2][i];
					}
				}
			break;	
		}
        
        ap_uint<5> casej_s2_t3 = case_j[2]+case_j[1]+case_j[0]+1-(add_pe_1_bias)-(add_pe_2_bias)-(add_pe_3_bias);
		
        //wrap 34
        if(merge_34 && !(merge_01234 || merge_1234 || merge_234 || (((merge_23 || merge_123 || merge_0123) && case_j[2]==1)))){
			//cout<<"DDI "<<casej_s2_t3-1<<" "<<i_out[3][0]<<endl;
			e_out_s2[casej_s2_t3-1] = tmp_merge_34;
			n_out_s2[casej_s2_t3-1] = n_out[3][0];
			i_out_s2[casej_s2_t3-1] = i_out[3][0];
		}else if (!(merge_01234 || merge_1234 || merge_234 || (((merge_23 || merge_123 || merge_0123) && case_j[2]==1)))){
            //cout<<"DDII "<<casej_s2_t3-1<<" "<<i_out[2][case_j[2]-1]<<endl;
            e_out_s2[casej_s2_t3-1] = e_out[2][case_j[2]-1];
			n_out_s2[casej_s2_t3-1] = n_out[2][case_j[2]-1];
			i_out_s2[casej_s2_t3-1] = i_out[2][case_j[2]-1];
        }
        
		/*
        if(add_pe_4_bias){
            for(int i=1; i<4; i++){
                if(i<case_j[3]){
                    //cout<<"EEI "<<casej_s2_t3+i-(1)<<" "<<i_out[3][i]<<endl;
                    e_out_s2[casej_s2_t3+i-(1)] = e_out[3][i];
                    n_out_s2[casej_s2_t3+i-(1)] = n_out[3][i];
                    i_out_s2[casej_s2_t3+i-(1)] = i_out[3][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[3]){
                    //cout<<"EEII "<<casej_s2_t3+i<<" "<<i_out[3][i]<<endl;
                    e_out_s2[casej_s2_t3+i] = e_out[3][i];
                    n_out_s2[casej_s2_t3+i] = n_out[3][i];
                    i_out_s2[casej_s2_t3+i] = i_out[3][i];
                }
            }
        }
		*/
		
		switch(add_pe_4_bias){
			case 1:
				for(int i=1; i<4; i++){
					if(i<case_j[3]){
						//cout<<"EEI "<<casej_s2_t3+i-(1)<<" "<<i_out[3][i]<<endl;
						e_out_s2[casej_s2_t3+i-(1)] = e_out[3][i];
						n_out_s2[casej_s2_t3+i-(1)] = n_out[3][i];
						i_out_s2[casej_s2_t3+i-(1)] = i_out[3][i];
					}
				}
			break;
			case 0:
				for(int i=0; i<4; i++){
					if(i<case_j[3]){
						//cout<<"EEII "<<casej_s2_t3+i<<" "<<i_out[3][i]<<endl;
						e_out_s2[casej_s2_t3+i] = e_out[3][i];
						n_out_s2[casej_s2_t3+i] = n_out[3][i];
						i_out_s2[casej_s2_t3+i] = i_out[3][i];
					}
				}
			break;	
		}
		
        /*
        for(int i=0; i<4; i++){
            if(i>=add_pe_3_bias && i<case_j[2]){
                e_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = e_out[2][i];
                n_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = n_out[2][i];
                i_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = i_out[2][i];
            }
        }
        */
        
        //======================================================================================//
		
        //==================================================================================//
        
		ap_uint<5> casej_s3= 1+case_j[0]+case_j[1]+case_j[2]+case_j[3]-(add_pe_1_bias)-(add_pe_2_bias)-(add_pe_3_bias)-(add_pe_4_bias);
        
       
        
		/*
		cout<<"final "<<casej_s3-1<<endl;
		cout<<"keepi "<<keep_idx<<endl;
        print_fixed(keep_e, 8);
		
		
		
		cout<<"i_out_s2"<<" ";
		for(int i=0; i< casej_s3; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out_s2[i]<<" ";
        }
		cout<<endl;
		*/
        keep_e_pre = keep_e;
        keep_idx_pre = keep_idx;
        keep_n_pre = keep_n;
		
        for(int out_pe = 0; out_pe < 16; out_pe ++){
#pragma HLS UNROLL
            if(iters * 16 + out_pe < num_of_edge){
                
               //cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 >= casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"index "<< index_t(MAX_NODE-1) << endl;
					
                }
                else if(out_pe + 1 < casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"index "<<  i_out_s2[out_pe] << endl;
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
                    /*
					if(i_out_s2[out_pe] == 70){
						cout<<"BBB: idx 70 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
					*/
                    
                    
                }
            }
        } 
    }
}
void aggregation_global_par_12(
    hls::stream<edge>       edge_local_in[12],   //input-streaming message
    hls::stream<index_t>    idx_local_in[12],
    hls::stream<node>       node_des_in[12],
    
    hls::stream<node>       node_des_out[12], //output-streaming message
    hls::stream<edge>       edge_local_out[12],
    hls::stream<index_t>    idx_local_out[12],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 12);
    
    edge        keep_e_pre = {0,0,0};
    index_t     keep_idx_pre = (index_t)(MAX_NODE-1);
    node        keep_n_pre;
    
    edge        keep_e;
    index_t     keep_idx;
    node        keep_n;
    /*
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
	*/
    
    index_t     idx_in[12];
    edge        edge_in[12];
    node        node_des[12];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        cout<<"iters "<< iters << endl;
        
        for(int pe = 0; pe < 12; pe++){

            if(iters * 12 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE);
            }
            
            
            if(idx_in[pe] == 11){
                cout<<"idx 1 agg_in"<<endl;
                print_fixed(edge_in[pe], 8);
            }
            
			
        }
           
        edge    e_out_1[3][3];
        edge    e_out_2[3][3];
        index_t i_out_1[3][3];
        index_t i_out_2[3][3];
        node    n_out_1[3][3];
        node    n_out_2[3][3];
        bool    sel[3][3];
        
        edge    e_out_can[3][4];
        index_t i_out_can[3][4];
        node    n_out_can[3][4];
        
        edge    e_out[3][5];
        index_t i_out[3][5];
        node    n_out[3][5];
        
        edge    e_out_s2[13];
        index_t i_out_s2[13];
        node    n_out_s2[13];
        
        edge    e_out_s3[13];
        index_t i_out_s3[13];
        node    n_out_s3[13];
        
        //edge    tmp[2];
        
        int j[3];
        int r_i[3];
        
        int case_i[3];
        int case_j[3];
        
        for(int add_pe=0; add_pe<3; add_pe++){
        //#pragma HLS UNROLL
            adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
            
            case_i[add_pe] = sel[add_pe][2] + sel[add_pe][1] + sel[add_pe][0];
            switch(case_i[add_pe]){
			case 0: 
				case_j[add_pe]=4;
				break;
			case 1: 
				case_j[add_pe]=3;
				break;
			case 2: 
				case_j[add_pe]=2;
				break;
			case 3: 
				case_j[add_pe]=1;
				break;
            }  
            
            j[add_pe] = 0;
            for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
                if(i_out_can[add_pe][i]!=MAX_NODE){
                    e_out[add_pe][j[add_pe]] = e_out_can[add_pe][i];
                    n_out[add_pe][j[add_pe]] = n_out_can[add_pe][i];
                    i_out[add_pe][j[add_pe]] = i_out_can[add_pe][i];
                    j[add_pe]++;
                }
            }
        } 
		
		
		
		//cout<<"j vs. casej "<<j[0]<<" "<<j[1]<<" "<<case_j[0]<<" "<<case_j[1]<<endl;
		
		//cout<<"i_out[0]"<<" ";
        /*
		for(int i=0; i< j[0]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[0][i]<<" ";
        }
		cout<<endl;
		//cout<<"i_out[1]"<<" ";
		for(int i=0; i< j[1]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[1][i]<<" ";
        }
		cout<<endl;
        for(int i=0; i< j[2]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[2][i]<<" ";
        }
		cout<<endl;
		*/
		bool add_pe_1_bias;
		bool add_pe_2_bias;
        bool add_pe_3_bias;
		
        //===========================================================================//
        
		edge tmp_merge_01;
		for(int dim=0; dim<4; dim++){
			tmp_merge_01[dim] = e_out[0][0][dim] + keep_e_pre[dim];
		}
		
		edge tmp_merge_12;
		for(int dim=0; dim<4; dim++){
			tmp_merge_12[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim];
		}
        
        edge tmp_merge_23;
		for(int dim=0; dim<4; dim++){
			tmp_merge_23[dim] = e_out[1][case_j[1]-1][dim] + e_out[2][0][dim];
		}
		
		edge tmp_merge_012;
		for(int dim=0; dim<4; dim++){
			tmp_merge_012[dim] = e_out[0][0][dim] + e_out[1][0][dim] + keep_e_pre[dim];
		}
        
        edge tmp_merge_123;
		for(int dim=0; dim<4; dim++){
			tmp_merge_123[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim] + e_out[2][0][dim];
		}
        
        edge tmp_merge_0123;
		for(int dim=0; dim<4; dim++){
			tmp_merge_0123[dim] = e_out[0][0][dim] + e_out[1][0][dim] + e_out[2][0][dim] + keep_e_pre[dim];
		}
		
		add_pe_1_bias = (keep_idx_pre          == i_out[0][0]) ? 1: 0;
		add_pe_2_bias = (i_out[0][case_j[0]-1] == i_out[1][0]) ? 1: 0;
        add_pe_3_bias = (i_out[1][case_j[1]-1] == i_out[2][0]) ? 1: 0;
        
        if(case_j[2]==1){
            if(case_j[1]==1){
                if(case_j[0]==1){
                    if(add_pe_1_bias && add_pe_2_bias && add_pe_3_bias){
                        keep_e   = tmp_merge_0123;
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }else if(add_pe_2_bias && add_pe_3_bias){
                        keep_e   = tmp_merge_123;
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }else if(add_pe_3_bias){
                        keep_e   = tmp_merge_23;
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }else{
                        keep_e   = e_out[2][0];
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }
                }else{
                    if(add_pe_2_bias && add_pe_3_bias){
                        keep_e   = tmp_merge_123;
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }
                    else if(add_pe_3_bias){
                        keep_e   = tmp_merge_23;
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }else{
                        keep_e   = e_out[2][0];
                        keep_idx = i_out[2][0];
                        keep_n   = n_out[2][0];
                    }
                }
               
            }else{
               if(add_pe_3_bias){
                    keep_e   = tmp_merge_23;
                    keep_idx = i_out[2][0];
                    keep_n   = n_out[2][0];
               }else{
                    keep_e   = e_out[2][0];
                    keep_idx = i_out[2][0];
                    keep_n   = n_out[2][0];
               }
            }
            
        }else{
            keep_e   = e_out[2][case_j[2]-1];
            keep_idx = i_out[2][case_j[2]-1];
            keep_n   = n_out[2][case_j[2]-1];
        }
		
		//cout<<"check"<<case_j[0]<<" "<<case_j[1]<<" "<<add_pe_1_bias<<" "<<add_pe_2_bias<<endl;
		//bool merge=(add_pe_1_bias == 1 && add_pe_2_bias == 1 && case_j[0] == 1);
		//bool merge_top = (add_pe_1_bias == 1 && case_j[0] == 1);
        
        bool merge_0123 =  (add_pe_1_bias && add_pe_2_bias && add_pe_3_bias && (case_j[0]==1) && (case_j[1]==1));
        bool merge_012  =  (add_pe_1_bias && add_pe_2_bias && (case_j[0]==1));
        bool merge_01   =  (add_pe_1_bias);
        
        bool merge_123  =  (add_pe_2_bias && add_pe_3_bias && (case_j[1]==1));
        bool merge_12   =  (add_pe_2_bias);
        
        bool merge_23   =  (add_pe_3_bias);
		
		if(merge_0123)
		{
			e_out_s2[0] = tmp_merge_0123;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPI "<<0<<" "<<i_out[0][0]<<endl;
			
		}else if(merge_012){
			e_out_s2[0] = tmp_merge_012;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPII "<<0<<" "<<i_out[0][0]<<endl;
	
		}else if(merge_01){
			e_out_s2[0] = tmp_merge_01;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPIII "<<0<<" "<<i_out[0][0]<<endl;
	
		}
		else{
			e_out_s2[0] = keep_e_pre;
			n_out_s2[0] = keep_n_pre;
			i_out_s2[0] = keep_idx_pre;
			cout<<"PPIV "<<0<<" "<<keep_idx_pre<<endl;
		}
		
        if(add_pe_1_bias){
            for(int i=0; i<4; i++){
                if(i>=1 && i<case_j[0]-1){
                    cout<<"AAI "<<i<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i] = e_out[0][i];
                    n_out_s2[i] = n_out[0][i];
                    i_out_s2[i] = i_out[0][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[0]-1){
                    cout<<"AAII "<<i+1<<" "<<i_out[0][i]<<endl;
                    e_out_s2[i+1] = e_out[0][i];
                    n_out_s2[i+1] = n_out[0][i];
                    i_out_s2[i+1] = i_out[0][i];
                }
            }
        }
        /*
		for(int i=0; i<4; i++){
			
            if(i>=add_pe_1_bias && i<case_j[0]-1){
				//cout<<"AAI"<<i+(!add_pe_1_bias)<<" "<<i_out[0][i]<<endl;
				e_out_s2[i+(!add_pe_1_bias)] = e_out[0][i];
                n_out_s2[i+(!add_pe_1_bias)] = n_out[0][i];
                i_out_s2[i+(!add_pe_1_bias)] = i_out[0][i];
			}
		}
        */
		
        int casej_s2_t1 = case_j[0]+(!add_pe_1_bias);
		
        cout<<"BB con "<<merge_0123<<" "<<merge_0123<<" "<<merge_012<<" "<<(!(merge_0123 || merge_012))<<endl;
		if(merge_123 && !(merge_0123 || merge_012 || (merge_01 && case_j[0]==1))){
			cout<<"BBI "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_123;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if(merge_12 && !(merge_0123 || merge_012|| (merge_01 && case_j[0]==1))){
			cout<<"BBII "<<casej_s2_t1-1<<" "<<i_out[1][0]<<endl;
			e_out_s2[casej_s2_t1-1] = tmp_merge_12;
			n_out_s2[casej_s2_t1-1] = n_out[1][0];
			i_out_s2[casej_s2_t1-1] = i_out[1][0];
		}else if (!(merge_0123 || merge_012|| (merge_01 && case_j[0]==1))){
            cout<<"BBIII "<<casej_s2_t1-1<<" "<<i_out[0][case_j[0]-1]<<endl;
            e_out_s2[casej_s2_t1-1] = e_out[0][case_j[0]-1];
			n_out_s2[casej_s2_t1-1] = n_out[0][case_j[0]-1];
			i_out_s2[casej_s2_t1-1] = i_out[0][case_j[0]-1];
        }
        
        if(add_pe_2_bias){
            for(int i=1; i<4; i++){
                if(i<case_j[1]-1){
                    cout<<"CCI "<<casej_s2_t1+i-(1)<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i-(1)] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i-(1)] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i-(1)] = i_out[1][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[1]-1){
                    cout<<"CCII "<<casej_s2_t1+i<<" "<<i_out[1][i]<<endl;
                    e_out_s2[casej_s2_t1+i] = e_out[1][i];
                    n_out_s2[casej_s2_t1+i] = n_out[1][i];
                    i_out_s2[casej_s2_t1+i] = i_out[1][i];
                }
            }
        }
        /*
		for(int i=0; i<4; i++){
			if(i>=add_pe_2_bias && i<case_j[1]-1){
				//cout<<"CCI"<<case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)<<" "<<i_out[1][i]<<endl;
				e_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = e_out[1][i];
                n_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = n_out[1][i];
                i_out_s2[casej_s2_t1+i-(add_pe_2_bias)] = i_out[1][i];
			}
		}
        */
        
        int casej_s2_t2 = case_j[1]+case_j[0]+1-(add_pe_1_bias)-(add_pe_2_bias);
		
        if(merge_23 && !(merge_0123  || merge_123 || (((merge_12 || merge_012) && case_j[1]==1)))){
			cout<<"DDI "<<casej_s2_t2-1<<" "<<i_out[2][0]<<endl;
			e_out_s2[casej_s2_t2-1] = tmp_merge_23;
			n_out_s2[casej_s2_t2-1] = n_out[2][0];
			i_out_s2[casej_s2_t2-1] = i_out[2][0];
		}else if (!(merge_0123 || merge_123 || (((merge_12 || merge_012) && case_j[1]==1)))){
            cout<<"DDII "<<casej_s2_t2-1<<" "<<i_out[1][case_j[1]-1]<<endl;
            e_out_s2[casej_s2_t2-1] = e_out[1][case_j[1]-1];
			n_out_s2[casej_s2_t2-1] = n_out[1][case_j[1]-1];
			i_out_s2[casej_s2_t2-1] = i_out[1][case_j[1]-1];
        }
        
        if(add_pe_3_bias){
            for(int i=1; i<4; i++){
                if(i<case_j[2]){
                    cout<<"EEI "<<casej_s2_t2+i-(1)<<" "<<i_out[2][i]<<endl;
                    e_out_s2[casej_s2_t2+i-(1)] = e_out[2][i];
                    n_out_s2[casej_s2_t2+i-(1)] = n_out[2][i];
                    i_out_s2[casej_s2_t2+i-(1)] = i_out[2][i];
                }
            }
        }else{
            for(int i=0; i<4; i++){
                if(i<case_j[2]){
                    cout<<"EEII "<<casej_s2_t2+i<<" "<<i_out[2][i]<<endl;
                    e_out_s2[casej_s2_t2+i] = e_out[2][i];
                    n_out_s2[casej_s2_t2+i] = n_out[2][i];
                    i_out_s2[casej_s2_t2+i] = i_out[2][i];
                }
            }
        }
        /*
        for(int i=0; i<4; i++){
            if(i>=add_pe_3_bias && i<case_j[2]){
                e_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = e_out[2][i];
                n_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = n_out[2][i];
                i_out_s2[casej_s2_t2 + i -(add_pe_3_bias)] = i_out[2][i];
            }
        }
        */
        
        //======================================================================================//
		
        //==================================================================================//
        
		int casej_s3= 1+case_j[0]+case_j[1]+case_j[2]-(add_pe_1_bias)-(add_pe_2_bias)-(add_pe_3_bias);
        
       
        
		cout<<"final "<<casej_s3-1<<endl;
		cout<<"keepi "<<keep_idx<<endl;
        print_fixed(keep_e, 8);
		
		
		
		cout<<"i_out_s2"<<" ";
		for(int i=0; i< casej_s3; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out_s2[i]<<" ";
        }
		cout<<endl;
		
        keep_e_pre = keep_e;
        keep_idx_pre = keep_idx;
        keep_n_pre = keep_n;
		
        for(int out_pe = 0; out_pe < 12; out_pe ++){
#pragma HLS UNROLL
            if(iters * 12 + out_pe < num_of_edge){
                
               //cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 >= casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"index "<< index_t(MAX_NODE-1) << endl;
					
                }
                else if(out_pe + 1 < casej_s3){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"index "<<  i_out_s2[out_pe] << endl;
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
                    
					if(i_out_s2[out_pe] == 11){
						cout<<"BBB: idx 11 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
                    
                    
                }
            }
        } 
    }
}
void aggregation_global_par_8_time_v2(
    hls::stream<edge>       edge_local_in[8],   //input-streaming message
    hls::stream<index_t>    idx_local_in[8],
    hls::stream<node>       node_des_in[8],
    
    hls::stream<node>       node_des_out[8], //output-streaming message
    hls::stream<edge>       edge_local_out[8],
    hls::stream<index_t>    idx_local_out[8],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 8);
    
    edge        keep_e = {0,0,0};
    index_t     keep_idx = (index_t)(MAX_NODE-1);
    node        keep_n;
    /*
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
	*/
    
    index_t     idx_in[8];
    edge        edge_in[8];
    node        node_des[8];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        //cout<<"iters "<< iters << endl;
        
        for(int pe = 0; pe < 8; pe++){

            if(iters * 8 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE);
            }
            
            /*
            if(idx_in[pe] == 1){
                cout<<"idx 1 agg_in"<<endl;
                print_fixed(edge_in[pe], 8);
            }
            */
			
        }
           
        edge    e_out_1[2][3];
        edge    e_out_2[2][3];
        index_t i_out_1[2][3];
        index_t i_out_2[2][3];
        node    n_out_1[2][3];
        node    n_out_2[2][3];
        bool    sel[2][3];
        
        edge    e_out_can[2][4];
        index_t i_out_can[2][4];
        node    n_out_can[2][4];
        
        edge    e_out[2][5];
        index_t i_out[2][5];
        node    n_out[2][5];
        
        edge    e_out_s2[9];
        index_t i_out_s2[9];
        node    n_out_s2[9];
        
        //edge    tmp[2];
        
        int j[2];
        int r_i[2];
        
        int case_i[2];
        int case_j[2];
        
        for(int add_pe=0; add_pe<2; add_pe++){
        //#pragma HLS UNROLL
            adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
        } 
		
		case_i[0] = sel[0][2] + sel[0][1] + sel[0][0];
		case_i[1] = sel[1][2] + sel[1][1] + sel[1][0];
            
		switch(case_i[0]){
			case 0: 
				case_j[0]=4;
				break;
			case 1: 
				case_j[0]=3;
				break;
			case 2: 
				case_j[0]=2;
				break;
			case 3: 
				case_j[0]=1;
				break;
		}  
		switch(case_i[1]){
			case 0: 
				case_j[1]=4;
				break;
			case 1: 
				case_j[1]=3;
				break;
			case 2: 
				case_j[1]=2;
				break;
			case 3: 
				case_j[1]=1;
				break;
		}
		
		j[0] = 0;
		for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
            if(i_out_can[0][i]!=MAX_NODE){
                e_out[0][j[0]] = e_out_can[0][i];
                n_out[0][j[0]] = n_out_can[0][i];
                i_out[0][j[0]] = i_out_can[0][i];
                j[0]++;
            }
        }
		j[1] = 0;
        for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
            if(i_out_can[1][i]!=MAX_NODE){
                e_out[1][j[1]] = e_out_can[1][i];
                n_out[1][j[1]] = n_out_can[1][i];
                i_out[1][j[1]] = i_out_can[1][i];
                j[1]++;
            }
        }
		
		//cout<<"j vs. casej "<<j[0]<<" "<<j[1]<<" "<<case_j[0]<<" "<<case_j[1]<<endl;
		
		//cout<<"i_out[0]"<<" ";
		for(int i=0; i< j[0]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[0][i]<<" ";
        }
		cout<<endl;
		//cout<<"i_out[1]"<<" ";
		for(int i=0; i< j[1]; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out[1][i]<<" ";
        }
		cout<<endl;
		
		bool add_pe_1_bias;
		bool add_pe_2_bias;
		
		edge tmp;
		for(int dim=0; dim<4; dim++){
			tmp[dim] = e_out[0][0][dim] + keep_e[dim];
		}
		
		edge tmp_2;
		for(int dim=0; dim<4; dim++){
			tmp_2[dim] = e_out[0][case_j[0]-1][dim] + e_out[1][0][dim];
		}
		
		edge tmp_merge;
		for(int dim=0; dim<4; dim++){
			tmp_merge[dim] = e_out[0][0][dim] + e_out[1][0][dim] + keep_e[dim];
		}
		
		add_pe_1_bias = (keep_idx == i_out[0][0]) ? 1: 0;
		add_pe_2_bias = (i_out[0][case_j[0]-1] == i_out[1][0]) ? 1: 0;
		
		//cout<<"check"<<case_j[0]<<" "<<case_j[1]<<" "<<add_pe_1_bias<<" "<<add_pe_2_bias<<endl;
		bool merge=(add_pe_1_bias == 1 && add_pe_2_bias == 1 && case_j[0] == 1);
		bool merge_top = (add_pe_1_bias == 1 && case_j[0] == 1);
		
		if(merge)
		{
			e_out_s2[0] = tmp_merge;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPI"<<0<<" "<<i_out[0][0]<<endl;
			
		}else if(add_pe_1_bias == 1){
			e_out_s2[0] = tmp;
			n_out_s2[0] = n_out[0][0];
			i_out_s2[0] = i_out[0][0];
			cout<<"PPII"<<0<<" "<<i_out[0][0]<<endl;
			//merge = true;
		}
		else{
			e_out_s2[0] = keep_e;
			n_out_s2[0] = keep_n;
			i_out_s2[0] = keep_idx;
			cout<<"PPIII"<<0<<" "<<keep_idx<<endl;
		}
		
		for(int i=0; i<4; i++){
			if(i>=add_pe_1_bias && i<case_j[0]-1){
				cout<<"AAI"<<i+(!add_pe_1_bias)<<" "<<i_out[0][i]<<endl;
				e_out_s2[i+(!add_pe_1_bias)] = e_out[0][i];
                n_out_s2[i+(!add_pe_1_bias)] = n_out[0][i];
                i_out_s2[i+(!add_pe_1_bias)] = i_out[0][i];
			}
		}
		
		
		if(add_pe_2_bias == 1 && merge_top==false && merge == false){
			cout<<"BBI"<<case_j[0]-1+(!add_pe_1_bias)<<" "<<i_out[1][0]<<endl;
			e_out_s2[case_j[0]-1+(!add_pe_1_bias)] = tmp_2;
			n_out_s2[case_j[0]-1+(!add_pe_1_bias)] = n_out[1][0];
			i_out_s2[case_j[0]-1+(!add_pe_1_bias)] = i_out[1][0];
		}else if(add_pe_2_bias == 0 && merge_top==false && merge == false){
			cout<<"BBII"<<case_j[0]-1+(!add_pe_1_bias)<<" "<<i_out[0][case_j[0]-1]<<endl;
			e_out_s2[case_j[0]-1+(!add_pe_1_bias)] = e_out[0][case_j[0]-1];
			n_out_s2[case_j[0]-1+(!add_pe_1_bias)] = n_out[0][case_j[0]-1];
			i_out_s2[case_j[0]-1+(!add_pe_1_bias)] = i_out[0][case_j[0]-1];
		}
		
		for(int i=0; i<4; i++){
			if(i>=add_pe_2_bias && i<case_j[1]){
				cout<<"CCI"<<case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)<<" "<<i_out[1][i]<<endl;
				e_out_s2[case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)] = e_out[1][i];
                n_out_s2[case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)] = n_out[1][i];
                i_out_s2[case_j[0]+(!add_pe_1_bias)+i-(add_pe_2_bias)] = i_out[1][i];
			}
		}
		
		
		
		keep_e   = e_out_s2[case_j[1]+case_j[0]+(!add_pe_1_bias)-(add_pe_2_bias)-1];
		keep_idx = i_out_s2[case_j[1]+case_j[0]+(!add_pe_1_bias)-(add_pe_2_bias)-1];
		keep_n   = n_out_s2[case_j[1]+case_j[0]+(!add_pe_1_bias)-(add_pe_2_bias)-1];
		
		cout<<"final"<<case_j[1]+case_j[0]+(!add_pe_1_bias)-(add_pe_2_bias)-1<<endl;
		cout<<"keepi"<<keep_idx<<endl;
		
		int casej_s2= case_j[1]+case_j[0]+(!add_pe_1_bias)-(add_pe_2_bias);
		
		cout<<"i_out_s2"<<" ";
		for(int i=0; i< casej_s2; i++){ //adder_pe 2 tmp save to e_out[2]
            cout<<i_out_s2[i]<<" ";
        }
		cout<<endl;
		
		
        for(int out_pe = 0; out_pe < 8; out_pe ++){
#pragma HLS UNROLL
            if(iters * 8 + out_pe < num_of_edge){
                
               //cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 >= casej_s2){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"index "<< index_t(MAX_NODE-1) << endl;
					
                }
                else if(out_pe + 1 < casej_s2){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"index "<<  i_out_s2[out_pe] << endl;
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
					if(i_out_s2[out_pe] == 1){
						cout<<"BBB: idx 1 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
                }
            }
        } 
    }
}
void aggregation_global_par_8_time(
    hls::stream<edge>       edge_local_in[8],   //input-streaming message
    hls::stream<index_t>    idx_local_in[8],
    hls::stream<node>       node_des_in[8],
    
    hls::stream<node>       node_des_out[8], //output-streaming message
    hls::stream<edge>       edge_local_out[8],
    hls::stream<index_t>    idx_local_out[8],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 8);
    
    edge        keep_e;
    index_t     keep_idx = (index_t)(MAX_NODE-1);
    node        keep_n;
    
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
    
    index_t     idx_in[8];
    edge        edge_in[8];
    node        node_des[8];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
        //cout<<"iters "<< iters << endl;
        bool out_pre = (iters != 0 ? false : true);
        for(int pe = 0; pe < 8; pe++){

            if(iters * 8 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE-1);
            }
            
            if(idx_in[pe] == 173){
                cout<<"idx 173 agg_in"<<endl;
                print_fixed(edge_in[pe], 8);
            }
			
        }
           
        edge    e_out_1[2][3];
        edge    e_out_2[2][3];
        index_t i_out_1[2][3];
        index_t i_out_2[2][3];
        node    n_out_1[2][3];
        node    n_out_2[2][3];
        bool    sel[2][3];
        
        edge    e_out_can[2][4];
        index_t i_out_can[2][4];
        node    n_out_can[2][4];
        
        edge    e_out[2][5];
        index_t i_out[2][5];
        node    n_out[2][5];
        
        edge    e_out_s2[9];
        index_t i_out_s2[9];
        node    n_out_s2[9];
        
        edge    tmp[2];
        
        int j[2];
        int r_i[2];
        
        int case_i[2];
        int case_j[2];
        
        for(int add_pe=0; add_pe<2; add_pe++){
        //#pragma HLS UNROLL
            adder_up(
                edge_in[4*add_pe + 0], edge_in[4*add_pe + 1], idx_in[4*add_pe + 0], idx_in[4*add_pe + 1], node_des[4*add_pe + 0], node_des[4*add_pe + 1], e_out_1[add_pe][0], e_out_2[add_pe][0],i_out_1[add_pe][0], i_out_2[add_pe][0], n_out_1[add_pe][0], n_out_2[add_pe][0],sel[add_pe][0]
            );
            
            adder_down(
                edge_in[4*add_pe + 2], edge_in[4*add_pe + 3], idx_in[4*add_pe + 2], idx_in[4*add_pe + 3], node_des[4*add_pe + 2], node_des[4*add_pe +3], e_out_1[add_pe][1], e_out_2[add_pe][1],i_out_1[add_pe][1], i_out_2[add_pe][1], n_out_1[add_pe][1], n_out_2[add_pe][1],sel[add_pe][1]            
            ); 
            
            adder_down(
                e_out_2[add_pe][0], e_out_1[add_pe][1], i_out_2[add_pe][0], i_out_1[add_pe][1], n_out_2[add_pe][0], n_out_1[add_pe][1], e_out_1[add_pe][2], e_out_2[add_pe][2], i_out_1[add_pe][2], i_out_2[add_pe][2], n_out_1[add_pe][2], n_out_2[add_pe][2],sel[add_pe][2]
            );
            
            e_out_can[add_pe][0] = e_out_1[add_pe][0];
            e_out_can[add_pe][1] = e_out_1[add_pe][2];
            e_out_can[add_pe][2] = e_out_2[add_pe][2];
            e_out_can[add_pe][3] = e_out_2[add_pe][1];
            
            i_out_can[add_pe][0] = i_out_1[add_pe][0];
            i_out_can[add_pe][1] = i_out_1[add_pe][2];
            i_out_can[add_pe][2] = i_out_2[add_pe][2];
            i_out_can[add_pe][3] = i_out_2[add_pe][1];
            
            n_out_can[add_pe][0] = n_out_1[add_pe][0];
            n_out_can[add_pe][1] = n_out_1[add_pe][2];
            n_out_can[add_pe][2] = n_out_2[add_pe][2];
            n_out_can[add_pe][3] = n_out_2[add_pe][1];
            
        } 
        
        for(int i=0; i<2; i++){
            for(int dim = 0; dim < EDGE_DIM; dim ++){
                tmp[i][dim] = e_out_can[0][i][dim] + keep_e[dim];
            }
        }
        
        if(sel[0][0] == false){
            if(keep_idx == i_out_can[0][0]){
                e_out_s2[0] = tmp[0];
                n_out_s2[0] = n_out_can[0][0];
                i_out_s2[0] = i_out_can[0][0];
                j[0]=1;
                r_i[0] = 1;
            }
            else{
                e_out_s2[0] = keep_e;
                n_out_s2[0] = keep_n;
                i_out_s2[0] = keep_idx;
                
                e_out_s2[1] = e_out_can[0][0];
                n_out_s2[1] = n_out_can[0][0];
                i_out_s2[1] = i_out_can[0][0];
                j[0]=2;
                r_i[0] = 1;
            }
        }else{
            if(keep_idx == i_out_can[0][1]){
                e_out_s2[0] = tmp[1];
                n_out_s2[0] = n_out_can[0][1];
                i_out_s2[0] = i_out_can[0][1];
                j[0]=1;
                r_i[0] = 2;
            }
            else{
                e_out_s2[0] = keep_e;
                n_out_s2[0] = keep_n;
                i_out_s2[0] = keep_idx;
                
                e_out_s2[1] = e_out_can[0][1];
                n_out_s2[1] = n_out_can[0][1];
                i_out_s2[1] = i_out_can[0][1];
                j[0]=2;
                r_i[0] = 2;
            }
        }
        
        case_j[0] = j[0];
        case_j[1] = 1;
        
        for(int add_pe=0; add_pe<2; add_pe++){
            
            case_i[add_pe] = sel[add_pe][2] + sel[add_pe][1] + sel[add_pe][0];
            
            switch(case_i[add_pe]){
                case 0: 
                    case_j[add_pe]+=3;
                    break;
                case 1: 
                    case_j[add_pe]+=2;
                    break;
                case 2: 
                    case_j[add_pe]+=1;
                    break;
                case 3: 
                    case_j[add_pe]+=0;
                    break;
            }  
        }    
        
        
        
        j[1] = 0;
        for(int i=0; i< 4; i++){ //adder_pe 2 tmp save to e_out[2]
            if(i_out_can[1][i]!=MAX_NODE){
                e_out[1][j[1]] = e_out_can[1][i];
                n_out[1][j[1]] = n_out_can[1][i];
                i_out[1][j[1]] = i_out_can[1][i];
                j[1]++;
            }
        }
        
        for(int i=1; i< 4; i++){ //adder_pe 1 save to e_out_s2
            if(i>=r_i[0] && i_out_can[0][i]!=MAX_NODE){
                e_out_s2[j[0]] = e_out_can[0][i];
                n_out_s2[j[0]] = n_out_can[0][i];
                i_out_s2[j[0]] = i_out_can[0][i];
                j[0]++;
            }
        }
        
        int add_pe_2_bias;
        
        if(i_out_s2[case_j[0]-1] == i_out[1][0]){
            
            for(int dim = 0; dim < EDGE_DIM; dim++){
                e_out_s2[case_j[0]-1][dim] += e_out[1][0][dim]; 
            }
            
            add_pe_2_bias = 1; 
        }
        else{
            
            add_pe_2_bias = 0;
        }
        
        int casej_s2=case_j[0] + case_j[1] - add_pe_2_bias;
       
        
        for(int add_pe_2 = 0; add_pe_2 < 4; add_pe_2++){
            
            if(add_pe_2 >= add_pe_2_bias && add_pe_2 < case_j[1]){
                
                n_out_s2[case_j[0] + add_pe_2 - add_pe_2_bias] = n_out[1][add_pe_2];
                e_out_s2[case_j[0] + add_pe_2 - add_pe_2_bias] = e_out[1][add_pe_2];
                i_out_s2[case_j[0] + add_pe_2 - add_pe_2_bias] = i_out[1][add_pe_2];
                //casej_s2 ++;
            }
        }
        
		
		if(case_j[1] - add_pe_2_bias <=0){
			keep_e   = e_out_s2[case_j[0]-1];
			keep_idx = i_out[1][0];
			keep_n   = n_out[1][0];
		}
		else{
			keep_e   = e_out[1][case_j[1]-1];
			keep_idx = i_out[1][case_j[1]-1];
			keep_n   = n_out[1][case_j[1]-1];
		}
		
		/*
		keep_e   = e_out_s2[casej_s2-1];
		keep_idx = i_out_s2[casej_s2-1];
		keep_n   = n_out_s2[casej_s2-1];
		*/
		
		/*
		if(keep_idx == 27){
			cout<<"agg keep 27"<<endl;
			print_fixed(keep_e, 8);
		}
        */
        
        for(int out_pe = 0; out_pe < 8; out_pe ++){
#pragma HLS UNROLL
            if(iters * 8 + out_pe < num_of_edge){
                
               // cout<<"agg_iters "<< iters * 8 + out_pe << endl;
                
                
                if(out_pe + 1 == casej_s2){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"agg_idx "<< index_t(MAX_NODE-1) << endl;
					/*
					if(i_out_s2[out_pe] == 27){
						cout<<"AAA: idx 27 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
					*/
					/*
					keep_e   = e_out_s2[out_pe];
					keep_idx = i_out_s2[out_pe];
					keep_n   = n_out_s2[out_pe];
					*/
                }
                else if(out_pe + 1 < casej_s2){
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << i_out_s2[out_pe];
					
					//cout<<"agg_idx "<< i_out_s2[out_pe] << endl; 
					//cout<<"agg_edo ";
					//print_fixed(e_out_s2[out_pe], 8);
					//cout<<"----------"<<endl;
					
					if(i_out_s2[out_pe] == 173){
						cout<<"BBB: idx 173 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
					
					
					
                }
                else{
                    node_des_out[out_pe]   << n_out_s2[out_pe];
                    edge_local_out[out_pe] << e_out_s2[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					/*
					if(i_out_s2[out_pe] == 27){
						cout<<"CCC: idx 27 agg_out"<<endl;
						print_fixed(e_out_s2[out_pe], 8);
					}
					*/
					//cout<<"agg_idx "<< index_t(MAX_NODE-1) << endl;
                }
				
				
			
            }
        }
        
    }
}
void aggregation_global_par_4_time(
    hls::stream<edge>       edge_local_in[4],   //input-streaming message
    hls::stream<index_t>    idx_local_in[4],
    hls::stream<node>       node_des_in[4],
    
    hls::stream<node>       node_des_out[4], //output-streaming message
    hls::stream<edge>       edge_local_out[4],
    hls::stream<index_t>    idx_local_out[4],
    int num_of_edge
){
//#pragma HLS INLINE
    int num_iters = ceildiv(num_of_edge, 4);
    
    edge        keep_e;
    index_t     keep_idx = (index_t)(MAX_NODE-1);
    node        keep_n;
    
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
    
    index_t     idx_in[4];
    edge        edge_in[4];
    node        node_des[4];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
       // cout<<"iters "<< iters << endl;
        bool out_pre = (iters != 0 ? false : true);
        for(int pe = 0; pe < E_BLOCK_PAR; pe++){

            if(iters * 4 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE-1);
            }
        }
           
        edge    e_out_1[3];
        edge    e_out_2[3];
        index_t i_out_1[3];
        index_t i_out_2[3];
        node    n_out_1[3];
        node    n_out_2[3];
        bool    sel[3];
        
        edge    e_out_can[4];
        index_t i_out_can[4];
        node    n_out_can[4];
        
        edge    e_out[5];
        index_t i_out[5];
        node    n_out[5];
        
        
        adder_up(
            edge_in[0], edge_in[1], idx_in[0], idx_in[1], node_des[0], node_des[1],
            e_out_1[0], e_out_2[0], i_out_1[0], i_out_2[0], n_out_1[0], n_out_2[0],
            sel[0]
        );
        
        adder_down(
            edge_in[2], edge_in[3], idx_in[2], idx_in[3], node_des[2], node_des[3],
            e_out_1[1], e_out_2[1], i_out_1[1], i_out_2[1], n_out_1[1], n_out_2[1],
            sel[1]            
        );
        
        adder_down(
            e_out_2[0], e_out_1[1], i_out_2[0], i_out_1[1], n_out_2[0], n_out_1[1],
            e_out_1[2], e_out_2[2], i_out_1[2], i_out_2[2], n_out_1[2], n_out_2[2],
            sel[2]
        );
        
        e_out_can[0] = e_out_1[0];
        e_out_can[1] = e_out_1[2];
        e_out_can[2] = e_out_2[2];
        e_out_can[3] = e_out_2[1];
        
        i_out_can[0] = i_out_1[0];
        i_out_can[1] = i_out_1[2];
        i_out_can[2] = i_out_2[2];
        i_out_can[3] = i_out_2[1];
        
        n_out_can[0] = n_out_1[0];
        n_out_can[1] = n_out_1[2];
        n_out_can[2] = n_out_2[2];
        n_out_can[3] = n_out_2[1];
        
        edge tmp[4];
        
        for(int i=0; i<4; i++){
            for(int dim = 0; dim < EDGE_DIM; dim ++){
                tmp[i][dim] = e_out_can[i][dim] + keep_e[dim];
            }
        }
        
        
        int j;
        int r_i;
     
        int case_i = (sel[2] << 2) + (sel[1] << 1) + sel[0];
        
        if(sel[0] == false){
            if(keep_idx == i_out_can[0]){
                e_out[0] = tmp[0];
                n_out[0] = n_out_can[0];
                i_out[0] = i_out_can[0];
                j=1;
                r_i = 1;
            }
            else{
                e_out[0] = keep_e;
                n_out[0] = keep_n;
                i_out[0] = keep_idx;
                
                e_out[1] = e_out_can[0];
                n_out[1] = n_out_can[0];
                i_out[1] = i_out_can[0];
                j=2;
                r_i = 1;
            }
        }else{
            if(keep_idx == i_out_can[1]){
                e_out[0] = tmp[1];
                n_out[0] = n_out_can[1];
                i_out[0] = i_out_can[1];
                j=1;
                r_i = 2;
            }
            else{
                e_out[0] = keep_e;
                n_out[0] = keep_n;
                i_out[0] = keep_idx;
                
                e_out[1] = e_out_can[1];
                n_out[1] = n_out_can[1];
                i_out[1] = i_out_can[1];
                j=2;
                r_i = 2;
            }
        }
        int case_j = j;
        
        for(int i=1; i< 4; i++){
            if(i>=r_i && i_out_can[i]!=0){
                e_out[j] = e_out_can[i];
                n_out[j] = n_out_can[i];
                i_out[j] = i_out_can[i];
                j++;
            }
        }
        
        
        switch(case_i){
            case 0: 
                case_j+=3;
                break;
            case 1: 
                case_j+=2;
                break;
            case 2: 
                case_j+=2;
                break;
            case 3: 
                case_j+=1;
                break;
            case 4: 
                case_j+=2;
                break;
            case 5: 
                case_j+=1;
                break;
            case 6: 
                case_j+=1;
                break;
            case 7: 
                case_j+=0;
                break;
                
        }
        
        
        for(int out_pe = 0; out_pe < 4; out_pe ++){
#pragma HLS UNROLL
            if(iters * 4 + out_pe < num_of_edge){
                
                //cout<<"agg_iters "<< iters * 4 + out_pe << endl;
                
                
                if(out_pe + 1 == case_j){
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"agg_idx "<< index_t(MAX_NODE-1) << endl;
					
                    
                    keep_e   = e_out[out_pe];
                    keep_idx = i_out[out_pe];
                    keep_n   = n_out[out_pe];
                }
                else if(out_pe + 1 < case_j){
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << i_out[out_pe];
					
					//cout<<"agg_idx "<< i_out[out_pe] << endl;
					//cout<<"agg_edo ";
					//print_fixed(e_out[out_pe], 8);
					//cout<<"----------"<<endl;
                }
                else{
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
					
					//cout<<"agg_idx "<< index_t(MAX_NODE-1) << endl;
                }
            }
        }
        
    }
}
void aggregation_global_par_4(
    hls::stream<edge>       edge_local_in[4],   //input-streaming message
    hls::stream<index_t>    idx_local_in[4],
    hls::stream<node>       node_des_in[4],
    
    hls::stream<node>       node_des_out[4], //output-streaming message
    hls::stream<edge>       edge_local_out[4],
    hls::stream<index_t>    idx_local_out[4],
    int num_of_edge
){
    int num_iters = ceildiv(num_of_edge, 4) ;
    
    edge        keep_e;
    index_t     keep_idx = (index_t)(MAX_NODE-1);
    node        keep_n;
    
    for(int dim = 0; dim < EDGE_DIM; dim++){
    //#pragma HLS UNROLL
        keep_e[dim] = 0;
    }
    
    index_t     idx_in[4];
    edge        edge_in[4];
    node        node_des[4];
    
    //static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1
        //cout<<"iters "<< iters << endl;
        bool out_pre = (iters != 0 ? false : true);
        for(int pe = 0; pe < E_BLOCK_PAR; pe++){

            if(iters * 4 + pe < num_of_edge){
        //#pragma HLS UNROLL
                idx_local_in[pe] >> idx_in[pe];
                edge_local_in[pe] >> edge_in[pe];
                node_des_in[pe] >> node_des[pe];
            }
            else{
                idx_in[pe] =  (index_t)(MAX_NODE-1);
            }
        }
           
        edge    e_out_1[3];
        edge    e_out_2[3];
        index_t i_out_1[3];
        index_t i_out_2[3];
        node    n_out_1[3];
        node    n_out_2[3];
        bool    sel[3];
        
        edge    e_out_can[4];
        index_t i_out_can[4];
        node    n_out_can[4];
        
        edge    e_out[5];
        index_t i_out[5];
        node    n_out[5];
        
        
        adder_up(
            edge_in[0], edge_in[1], idx_in[0], idx_in[1], node_des[0], node_des[1],
            e_out_1[0], e_out_2[0], i_out_1[0], i_out_2[0], n_out_1[0], n_out_2[0],
            sel[0]            
        );
        
        adder_down(
            edge_in[2], edge_in[3], idx_in[2], idx_in[3], node_des[2], node_des[3],
            e_out_1[1], e_out_2[1], i_out_1[1], i_out_2[1], n_out_1[1], n_out_2[1],
            sel[1]
        );
        
        adder_down(
            e_out_2[0], e_out_1[1], i_out_2[0], i_out_1[1], n_out_2[0], n_out_1[1],
            e_out_1[2], e_out_2[2], i_out_1[2], i_out_2[2], n_out_1[2], n_out_2[2],
            sel[2]
        );
        
        e_out_can[0] = e_out_1[0];
        e_out_can[1] = e_out_1[2];
        e_out_can[2] = e_out_2[2];
        e_out_can[3] = e_out_2[1];
        
        i_out_can[0] = i_out_1[0];
        i_out_can[1] = i_out_1[2];
        i_out_can[2] = i_out_2[2];
        i_out_can[3] = i_out_2[1];
        
        n_out_can[0] = n_out_1[0];
        n_out_can[1] = n_out_1[2];
        n_out_can[2] = n_out_2[2];
        n_out_can[3] = n_out_2[1];
        
        edge tmp[4];
        
        for(int i=0; i<4; i++){
            for(int dim = 0; dim < EDGE_DIM; dim ++){
                tmp[i][dim] = e_out_can[i][dim] + keep_e[dim];
            }
        }
        
        
        int j=0;
        
        for(int i=0; i<4; i++){
            if(out_pre == false && i_out_can[i]!=0){
                if(keep_idx == i_out_can[i]){
                    /*
                    for(int dim = 0; dim < EDGE_DIM; dim++){
                        e_out[j][dim] = e_out_can[i][dim] + keep_e[dim];
                    }
                    */
                    e_out[j] = tmp[i];
                    n_out[j] = n_out_can[i];
                    i_out[j] = i_out_can[i];
                    j++;
                    out_pre = true;
                }
                else{
                    e_out[j] = keep_e;
                    n_out[j] = keep_n;
                    i_out[j] = keep_idx;
                    
                    e_out[j+1] = e_out_can[i];
                    n_out[j+1] = n_out_can[i];
                    i_out[j+1] = i_out_can[i];
                    
                    j+=2;
                    out_pre = true;
                }
                
            }
            else if(out_pre == true && i_out_can[i]!=0){
                e_out[j] = e_out_can[i];
                n_out[j] = n_out_can[i];
                i_out[j] = i_out_can[i];
                j++;
            }
        }
        
        for(int out_pe = 0; out_pe < 4; out_pe ++){
#pragma HLS UNROLL
            if(iters * 4 + out_pe < num_of_edge){
                
                //cout<<"agg_iters "<< iters * 4 + out_pe << endl;
                //cout<<"agg_idx "<< i_out[out_pe] << endl;
                
                if(out_pe < j-1){
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << i_out[out_pe];
                }
                else if(out_pe == j-1){
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
                    
                    keep_e   = e_out[out_pe];
                    keep_idx = i_out[out_pe];
                    keep_n   = n_out[out_pe];
                }
                else{
                    node_des_out[out_pe]   << n_out[out_pe];
                    edge_local_out[out_pe] << e_out[out_pe];
                    idx_local_out[out_pe]  << index_t(MAX_NODE-1); //dummy!!!
                }
            }
        }
        
    }
}

//to-do: don't need to send idx, reduce to binary stream, if output: 1, if keep: 0 
void aggregation_global(
    hls::stream<edge>   edge_local_in[E_BLOCK_PAR],   //input-streaming message
    hls::stream<index_t> idx_local_in[E_BLOCK_PAR],
    hls::stream<node>     node_des_in[E_BLOCK_PAR],
    
    hls::stream<node>    &node_des_out, //output-streaming message
    hls::stream<edge>    &edge_local_out,
    hls::stream<index_t> &idx_local_out,
    //hls::stream<node> node_aggr_out[E_BLOCK_PAR]
    //int num_of_des_node;
    int num_of_edge
){
//#pragma HLS INLINE off
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR);
    
    edge      keep_e;
    node      keep_n;
    index_t keep_idx;
    
    static int count = 0;
    
    for(int iters = 0 ; iters < num_iters; iters++){

        for(int pe = 0; pe < E_BLOCK_PAR; pe++){
#pragma HLS PIPELINE II=1           
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                index_t idx_in;
                edge edge_in;
                node node_des;
               
                idx_local_in[pe] >> idx_in;
                edge_local_in[pe] >> edge_in;
                node_des_in[pe] >> node_des;
                
                /*
                if(iters * E_BLOCK_PAR + pe < 8){
                    cout << "==========="<<endl;
                    //cout << "agg_in:"<<iters * E_BLOCK_PAR + pe << endl;
                    cout << "agg_idx "<<  idx_in<< endl;
                    cout << "keep_idx "<<  keep_idx<< endl;
                    print_fixed(edge_in, 12);
                }
                */
                
                if((!(iters == 0 && pe == 0)) && keep_idx != idx_in){
                    edge_local_out << keep_e;
                    node_des_out << keep_n;
                    idx_local_out << keep_idx;
                    
                    /*
                    cout<<"qqq -------------"<<keep_idx<<endl;
                    //cout<<count++<<endl;
                    print_fixed(keep_e, 12);
                    print_fixed(keep_n, 12);
                    cout<<"qqq -------------"<<endl;
                    */
                }
                
                if(iters == 0 && pe == 0){
                    keep_e = edge_in;
                   //cout << "dim_c" <<dim<< endl;
                }
                else if((!(iters == 0 && pe == 0)) && (keep_idx == idx_in)){
                    for(int dim = 0; dim < EDGE_DIM; dim++){
                        keep_e[dim] += edge_in[dim];
                    }
                    //cout << "dim_d" <<dim<< endl;
                }
                else{
                  keep_e = edge_in;
                    //cout << "dim_e" <<dim<< endl;
                }
                keep_idx = idx_in;
                keep_n = node_des;
                
                
                if(iters * E_BLOCK_PAR + pe == num_of_edge-1){
                    edge_local_out << keep_e;
                    node_des_out << keep_n;
                    idx_local_out << keep_idx;
                    //cout<<"qqq "<<keep_idx<<endl;
                    //cout<<count++<<endl;
                }
                
                if(iters * E_BLOCK_PAR + pe < 8){
                    print_fixed(keep_e, 12);
                }
                
                /*
                for(int dim = 0; dim < EDGE_DIM; dim++){
                    if(dim == 0){
                        //cout << "dim_a0" <<dim<< endl;
                        //edge input
                        idx_local_in[pe] >> idx_in;
                        edge_local_in[pe] >> edge_in;
                        node_des_in[pe] >> node_des;
                        
                        
                        if((idx_in == 4) || (idx_in == 5)){
                            cout<<"==========="<<endl;
                            cout<<"agg_in: des "<<idx_in<<endl;
                            print_fixed(edge_in, 12);
                            print_fixed(node_des, 12);
                        }
                        
                        
                        //cout<<"des_ag:"<<idx_in<<endl;
                        
                        //cout << "dim_a1" <<dim<< endl;
                        //edge output
                        if(iters!=0){
                            //cout << "dim_a2" <<dim<< endl;
                            if(keep_idx != idx_in){
                                //count++;
                                
                                
                                if((idx_in == 4) || (idx_in == 5)){
                                    cout<<"==========="<<endl;
                                    cout<<"agg_out: des "<<keep_idx<<endl;
                                    print_fixed(keep_e, 12);
                                    print_fixed(keep_n, 12);
                                }
                                
                                //cout<<"pppp "<<count <<endl;
                                //cout<<"des "<<keep_idx<<endl;
                                //print_fixed(keep_e, 12);
                                
                                edge_local_out << keep_e;
                                node_des_out << keep_n;
                                idx_local_out << keep_idx;
                            }
                        }
                        //update keep_idx
                        //keep_idx = idx_in;
                        //cout << "dim_a3" <<dim<< endl;
                    }


                    if(iters == 0 && pe == 0){
                        keep_e[dim] = edge_in[dim];
                       //cout << "dim_c" <<dim<< endl;
                    }
                    else if(keep_idx == idx_in){
                        keep_e[dim] += edge_in[dim];
                        //cout << "dim_d" <<dim<< endl;
                    }
                    else{
                      keep_e[dim] = edge_in[dim];
                        //cout << "dim_e" <<dim<< endl;
                    }
                    keep_idx = idx_in;
                    keep_n = node_des;
                    
                    //final_output
                    if(dim == EDGE_DIM-1){
                        //cout << "dim_b" <<dim<< endl;
                        //cout << "iters" <<iters<<" "<< num_iters-1 << endl;
                        if(iters * E_BLOCK_PAR + pe == num_of_edge-1){
                            //count++;
                            //cout<<"end_ebf: "<< count <<endl;
                            //cout<<"des "<<keep_idx<<endl;
                            edge_local_out << keep_e;
                            node_des_out << keep_n;
                            idx_local_out << keep_idx;
                        }
                    }
                }
                */
            }
        }
    }
}


void edge_block_end(
    hls::stream<edge_res>    edge_in[E_BLOCK_PAR], //input-streaming message
    attr_t               edge_update[MAX_EDGE], //output BRAM
    int num_of_edge    
)
{
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR);
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
        for(int pe = 0 ; pe < E_BLOCK_PAR; pe ++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                //cout<< "ebn2 "<< iters * E_BLOCK_PAR + pe << endl;
                edge_res e;
                edge_in[pe] >> e;
                edge_update[iters * E_BLOCK_PAR + pe] = e[0]; 

            }
            
        }
    }
}
void edge_block_L2_SLG(
    
    #ifdef __SYNTHESIS__
    //output
    node_str          node_src[E2_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des[E2_BLOCK_PAR/2][MAX_NODE],
    #else
    node_str          **node_src,
    node_str          **node_des,
    #endif
    
    // node_str  **node_src, //input - BRAM
    // node_str  **node_des, //input - BRAM
    
    hls::stream<edge>      edge_att_in[E2_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_idx_in[E2_BLOCK_PAR], 
    
    
    hls::stream<edge_res>    edge_embedding_out_sig[E2_BLOCK_PAR], //output-streaming message
    
	weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_edge

){
    //collect input data
    int num_iters = ceildiv(num_of_edge, E2_BLOCK_PAR);
    
    //cout<<num_iters<< " "<< num_of_edge<<endl;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E2_BLOCK_PAR + 1 
        for(int pe = 0; pe < E2_BLOCK_PAR; pe++){
        //unroll to E_BLOCK_PAR 
            //idx_pair idx;
            //cout<<"??"<<endl;
            //cout<<"E2 iters "<<iters<<endl;
            if(iters * E2_BLOCK_PAR + pe < num_of_edge){
                
                //cout << "======="<<endl;
                //cout<<"eb2 "<< iters * E_BLOCK_PAR + pe<<endl;
                
                
                edge e;
                index_t s;
                index_t d;
                
                idx_pair idx;
                
                array<attr_t, EDGE_BLOCK_IN_DIM> edge_block_in;
                edge_res edge_block_out;
                edge_res edge_block_out_sig, edge_block_out_sig_ff;
                //cout <<"eb2_a00"<<endl;
                edge_att_in[pe] >> e;
                edge_idx_in[pe] >> idx;
                
                node_str src_tmp;
                node_str dst_tmp;
                
                s = idx[0];
                d = idx[1];
				
				
                src_tmp = node_src[pe%(E2_BLOCK_PAR/2)][s];
                dst_tmp = node_des[pe%(E2_BLOCK_PAR/2)][d];
                
                //attr_t edge_block_in[EDGE_BLOCK_IN_DIM];
                //attr_t edge_block_out[EDGE_BLOCK_END_DIM];
                //cout <<"eb2_a0"<<endl;
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    
                    //edge_block_in[dim] = edge_att_in[iters * E2_BLOCK_PAR + pe][dim];
                    edge_block_in[NODE_DIM * 2 + dim] = e[dim];
                    
                }
               
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    edge_block_in[NODE_DIM+dim] = uint16_to_DTYPE<DTYPE>(src_tmp(data_w*(dim+1)-1, data_w*dim));
                    edge_block_in[dim] = uint16_to_DTYPE<DTYPE>(dst_tmp(data_w*(dim+1)-1, data_w*dim));
                    //nd[pe][dim] = node_des[pe][d][dim];
                }
                
                //cout <<"eb2_b"<<endl;
               
                MLP_dsp<attr_t, weight_t, EDGE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, EDGE_BLOCK_END_DIM, MLP_LAYER>(
                    edge_block_in, 
                    edge_block_out,
					buf_w2,
                    buf_b2 					
                ); 
                
                
                
                Sigmoid_end<attr_t, EDGE_BLOCK_END_DIM ,ap_fixed<18,4>,1024>(
                    edge_block_out, 
                    edge_block_out_sig
                );
                
                
                if(s==948 && d==949){
                    cout<<"==========="<<endl;
                    cout<<"edge2 #"<<iters * E_BLOCK_PAR + pe<<endl;
                    cout<<"edge2 in: ";
                    print_fixed(edge_block_in,8);
                    
                    cout<<"edge2 out: ";
                    print_fixed(edge_block_out, 8);
                    cout<<"edge2 sig idx: ";
                    print_fixed(edge_block_out_sig, 8);
                }
                
                
                /*
                cout <<"========"<<endl;
                attr_t result = edge_block_out_sig[0];
                attr_t before = edge_block_out[0];
                cout << DTYPE_to_uint16<attr_t>(before) << endl;
                cout << DTYPE_to_uint16<attr_t>(result) << endl;
                */
                
                edge_block_out_sig_ff = edge_block_out_sig;
                
                edge_embedding_out_sig[pe] << edge_block_out_sig_ff;
                
    
                //cout <<"eb2_obs "<< iters * E_BLOCK_PAR + pe <<": " <<  edge_block_out[0] << endl;
                //cout <<"eb2_out "<< iters * E_BLOCK_PAR + pe <<": " <<  edge_block_out_sig[0] << endl;
            }
        }
    }
}

void edge_block_L2(
    
    node_str  node_src[E2_BLOCK_PAR/2][MAX_NODE], //input - BRAM
    node_str  node_des[E2_BLOCK_PAR/2][MAX_NODE], //input - BRAM
    
    // node_str  **node_src, //input - BRAM
    // node_str  **node_des, //input - BRAM
    
    hls::stream<edge>      edge_att_in[E2_BLOCK_PAR], //input-streaming message

    #ifdef __SYNTHESIS__
    index_t   edge_idx_in[MAX_EDGE][2],
    #else
    // index_t   (*edge_idx_in)[2],
    index_t   (**edge_idx_in),
    #endif
    
    hls::stream<edge_res>    edge_embedding_out_sig[E2_BLOCK_PAR], //output-streaming message
    
	weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_edge

){
    //collect input data
    int num_iters = ceildiv(num_of_edge, E2_BLOCK_PAR);
    
    //cout<<num_iters<< " "<< num_of_edge<<endl;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E2_BLOCK_PAR + 1 
        for(int pe = 0; pe < E2_BLOCK_PAR; pe++){
        //unroll to E_BLOCK_PAR 
            //idx_pair idx;
            //cout<<"??"<<endl;
            //cout<<"E2 iters "<<iters<<endl;
            if(iters * E2_BLOCK_PAR + pe < num_of_edge){
                
                //cout << "======="<<endl;
                //cout<<"eb2 "<< iters * E_BLOCK_PAR + pe<<endl;
                
                
                edge e;
                index_t s;
                index_t d;
                
                array<attr_t, EDGE_BLOCK_IN_DIM> edge_block_in;
                edge_res edge_block_out;
                edge_res edge_block_out_sig;
                //cout <<"eb2_a00"<<endl;
                edge_att_in[pe] >> e;

                //attr_t edge_block_in[EDGE_BLOCK_IN_DIM];
                //attr_t edge_block_out[EDGE_BLOCK_END_DIM];
                //cout <<"eb2_a0"<<endl;
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    if(dim == 0){
                        s = edge_idx_in[iters * E2_BLOCK_PAR + pe][0];
                        //cout <<"eb2_a1"<<endl;
                    }
                    if(dim == 1){
                        d = edge_idx_in[iters * E2_BLOCK_PAR + pe][1];
                        //cout <<"eb2_a2"<<endl;
                    }
                    //edge_block_in[dim] = edge_att_in[iters * E2_BLOCK_PAR + pe][dim];
                    edge_block_in[NODE_DIM * 2 + dim] = e[dim];
                    
                }
                
                node_str src_tmp;
                node_str dst_tmp;
                
                src_tmp = node_src[pe%(E2_BLOCK_PAR/2)][s];
                dst_tmp = node_des[pe%(E2_BLOCK_PAR/2)][d];
               
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    edge_block_in[NODE_DIM+dim] = uint16_to_DTYPE<DTYPE>(src_tmp(data_w*(dim+1)-1, data_w*dim));
                    edge_block_in[dim] = uint16_to_DTYPE<DTYPE>(dst_tmp(data_w*(dim+1)-1, data_w*dim));
                    //nd[pe][dim] = node_des[pe][d][dim];
                }
                
                //cout <<"eb2_b"<<endl;
               
                MLP_dsp<attr_t, weight_t, EDGE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, EDGE_BLOCK_END_DIM, MLP_LAYER>(
                    edge_block_in, 
                    edge_block_out,
					buf_w2,
                    buf_b2 					
                ); 
                
                
                
                Sigmoid_end<attr_t, EDGE_BLOCK_END_DIM ,ap_fixed<18,4>,1024>(
                    edge_block_out, 
                    edge_block_out_sig
                );
                
                /*
                if(s==0 && d==1){
                    cout<<"==========="<<endl;
                    cout<<"edge2 #"<<iters * E_BLOCK_PAR + pe<<endl;
                    cout<<"edge2 in: ";
                    print_fixed(edge_block_in,12);
                    
                    cout<<"edge2 out: ";
                    print_fixed(edge_block_out, 12);
                    cout<<"edge2 sig idx: ";
                    print_fixed(edge_block_out_sig, 12);
                }
                */
                
                /*
                cout <<"========"<<endl;
                attr_t result = edge_block_out_sig[0];
                attr_t before = edge_block_out[0];
                cout << DTYPE_to_uint16<attr_t>(before) << endl;
                cout << DTYPE_to_uint16<attr_t>(result) << endl;
                */
                
                edge_embedding_out_sig[pe] << edge_block_out_sig;
                
    
                //cout <<"eb2_obs "<< iters * E_BLOCK_PAR + pe <<": " <<  edge_block_out[0] << endl;
                //cout <<"eb2_out "<< iters * E_BLOCK_PAR + pe <<": " <<  edge_block_out_sig[0] << endl;
            }
        }
    }
}


void edge_block(
    
    #ifdef __SYNTHESIS__
    node_str  node_src[E_BLOCK_PAR/2][MAX_NODE], //input - BRAM
    node_str  node_des[E_BLOCK_PAR/2][MAX_NODE], //input - BRAM
    #else
    // node_str  (*node_src)[MAX_NODE], //input - BRAM
    // node_str  (*node_des)[MAX_NODE], //input - BRAM
    node_str  (**node_src), //input - BRAM
    node_str  (**node_des), //input - BRAM
    #endif
    hls::stream<edge>      edge_att_in[E_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_idx_in[E_BLOCK_PAR], //input-streaming message
    

    hls::stream<edge>      edge_embedding_out[E_BLOCK_PAR], //output-streaming message
    hls::stream<edge>      edge_embedding_out_copy[E_BLOCK_PAR], //output-streaming message
    hls::stream<node>            node_des_out[E_BLOCK_PAR],
    hls::stream<index_t>        idx_des_out[E_BLOCK_PAR],
	
    #ifdef __SYNTHESIS__
	index_t edge_list[MAX_EDGE][2], //output BRAM
    #else
    // index_t (*edge_list)[2], //output BRAM
    index_t (**edge_list), //output BRAM
    #endif
 
	weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_edge

){
    /*
    cout<<"load node src 11"<<endl;
    for(int dim=0; dim<3; dim++){
        cout<< uint16_to_DTYPE<DTYPE>(node_src[4][11](data_w * (dim+1)-1, data_w *dim))<<" ";
    }
    cout<<endl;
    cout<<"load node des 11"<<endl;
    for(int dim=0; dim<3; dim++){
        cout<< uint16_to_DTYPE<attr_t>(node_des[4][11](data_w * (dim+1)-1, data_w *dim)) <<" ";
    }
    cout<<endl;
    */
    //collect input data
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR) ;

	cout<<"hihi9"<<endl;
	
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
#pragma HLS DEPENDENCE variable=edge_list intra WAW false 
#pragma HLS DEPENDENCE variable=edge_list inter WAW false 
        for(int pe = 0; pe < E_BLOCK_PAR; pe++){
        //unroll to E_BLOCK_PAR 
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                
                
            
                idx_pair idx;
                edge e;
                index_t s;
                index_t d;
                node    nd;
                
                //attr_t edge_block_in[EDGE_BLOCK_IN_DIM];
                //attr_t edge_block_out[EDGE_BLOCK_OUT_DIM];
                std::array<attr_t, EDGE_BLOCK_IN_DIM> edge_block_in;
                edge edge_block_out, edge_block_out_ff;
                
                edge_att_in[pe] >> e;
                edge_idx_in[pe] >> idx; //(s,d)
                
                s = idx[0];
                d = idx[1];
                
                
                //cout<<"iters "<<iters * E_BLOCK_PAR + pe <<" "<<s<<" "<<d<<endl;
                //print_fixed(e, 8);
                
                node_str src_tmp;
                node_str dst_tmp;
                
                src_tmp = node_src[pe%(E_BLOCK_PAR/2)][s];
                dst_tmp = node_des[pe%(E_BLOCK_PAR/2)][d];
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    edge_block_in[NODE_DIM+dim](data_w-1, 0) = uint16_to_DTYPE<attr_t>(src_tmp(data_w * (dim+1)-1, data_w *dim));
                    edge_block_in[dim](data_w-1, 0) = uint16_to_DTYPE<attr_t>(dst_tmp(data_w * (dim+1)-1, data_w *dim));
                    
                    /*
                    if(iters * E_BLOCK_PAR + pe == 19){
                        int item_int = edge_block_in[dim] *  int(pow(2, 8));
                        float item_f = item_int / pow(2, 8);   
                        cout<<item_f<<" ";
                        cout<<"!! "<<" "<<uint16_to_DTYPE<attr_t>(node_des[4][11](data_w * (dim+1)-1, data_w *dim))<<endl;;
                    }
                    */
                    
                    nd[dim](data_w-1, 0) = uint16_to_DTYPE<DTYPE>(dst_tmp(data_w * (dim+1)-1, data_w *dim));
                }
                for(int dim = 0; dim < EDGE_DIM; dim ++){
                    edge_block_in[2*NODE_DIM + dim] = e[dim];
                }
				for(int dim = 0 ; dim < 2; dim++){
					edge_list[iters * E_BLOCK_PAR + pe][dim] = idx[dim];
				}
               
               
                MLP_dsp<attr_t, weight_t, EDGE_BLOCK_IN_DIM, MLP_HIDDEN_DIM, EDGE_BLOCK_OUT_DIM, MLP_LAYER>(
                    edge_block_in, 
                    edge_block_out,
					buf_w0,
                    buf_b0
                ); 
                
                edge_block_out_ff = edge_block_out;
            
                edge_embedding_out[pe] << edge_block_out_ff;
                edge_embedding_out_copy[pe] << edge_block_out_ff;
                node_des_out[pe] << nd;
                idx_des_out[pe] << d;
				
				
                
                if( s==948 && d==949){
                    cout<<"========"<<endl;
                    cout<<"iters: "<<iters * E_BLOCK_PAR + pe << endl;
                    //cout<<"num: " << DTYPE_to_float<attr_t>(e[0]) << " " << DTYPE_to_float<attr_t>(e[1]) << endl;
                    cout<< s <<" to "<<d<<endl;
                    cout<<"e_before: ";
                    print_fixed(e, 8);
                    cout<<"e_before_in: ";
                    print_fixed(edge_block_in, 8);
                    cout<<"e_after: ";
                    print_fixed(edge_block_out, 8);
                    cout<<"node_des ";
                    print_fixed(nd, 8);
                    cout<<"node_des_id ";
                    cout << d<< endl;
                }
                
                
                /*
                attr_t e0 = e[0];
                attr_t e1 = e[1];
                int e0_int = e0 << 7;
                int e1_int = e1 << 7;
                float e0_float = e0_int / (128.0);
                float e1_float = e1_int / (128.0);
                cout<<"num: " << e0_float<< " " << e1_float << endl;
                cout<<"des_eb: "<<d<<endl;
                */
                
            }
        }
    }
}

void load_weight_v3(

    
	const input_weight *W1_ch0,
	const input_weight *W1_ch1,
	const input_weight *W1_ch2,
    
	const input_weight *W2_ch0,
    const input_weight *W2_ch1,
    const input_weight *W2_ch2,
    
    const input_weight *W3_ch0,
    const input_weight *W3_ch1,
    const input_weight *W3_ch2,
    weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    //edgeblock2
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM]
){
    
    
    int base = ceildiv(MAX_WEIGHT_DIM + MAX_BIAS_DIM, READ_W_PAR_PER_CH);
    
    //cout<<"load_w_a"<<endl;
    
    for(int iters = 0; iters < base + 2; iters++){ //for even burst 214-234
#pragma HLS PIPELINE II=1        
        input_weight w10 = W1_ch0[iters]; 
		input_weight w11 = W1_ch1[iters]; 
		input_weight w12 = W1_ch2[iters]; 
		input_weight w20 = W2_ch0[iters]; 
		input_weight w21 = W2_ch1[iters]; 
		input_weight w22 = W2_ch2[iters];
        input_weight w30 = W3_ch0[iters]; 
		input_weight w31 = W3_ch1[iters]; 
		input_weight w32 = W3_ch2[iters]; 
        
        //cout<< W10(16,0) << endl;
        //cout<<"load_w_b"<<endl;
        
        for(int nn = 0; nn < READ_W_PAR_PER_CH ; nn++){
#pragma HLS UNROLL
            
			if(iters * READ_W_PAR_PER_CH + nn < length_e0){
				buf_w0[0][iters * READ_W_PAR_PER_CH + nn] = w10.w_attr[nn]; //edgeblock1, W0
				buf_w2[0][iters * READ_W_PAR_PER_CH + nn] = w30.w_attr[nn]; //edgeblock2, W0
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_e0 + MLP_HIDDEN_DIM){
				buf_b0[0][iters * READ_W_PAR_PER_CH + nn - length_e0] = w10.w_attr[nn]; //edgeblock1, b0
				buf_b2[0][iters * READ_W_PAR_PER_CH + nn - length_e0] = w30.w_attr[nn]; //edgeblock2, b0
			}
			
			if(iters * READ_W_PAR_PER_CH + nn < length_e1){
				buf_w0[1][iters * READ_W_PAR_PER_CH + nn] = w11.w_attr[nn]; //edgeblock1, W1
				buf_w2[1][iters * READ_W_PAR_PER_CH + nn] = w31.w_attr[nn]; //edgeblock2, W1
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_e1 + MLP_HIDDEN_DIM){
				buf_b0[1][iters * READ_W_PAR_PER_CH + nn - length_e1] = w11.w_attr[nn]; //edgeblock1, b1
				buf_b2[1][iters * READ_W_PAR_PER_CH + nn - length_e1] = w31.w_attr[nn]; //edgeblock2, b1
			}
			
			if(iters * READ_W_PAR_PER_CH + nn < length_e2){
				buf_w0[2][iters * READ_W_PAR_PER_CH + nn] = w12.w_attr[nn]; //edgeblock1, W2
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_e2 + EDGE_BLOCK_OUT_DIM){
				buf_b0[2][iters * READ_W_PAR_PER_CH + nn - length_e2] = w12.w_attr[nn];  //edgeblock1, b2
			}
			
			if(iters * READ_W_PAR_PER_CH + nn < length_e22){
				buf_w2[2][iters * READ_W_PAR_PER_CH + nn] = w32.w_attr[nn];  //edgeblock2, W2
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_e22 + EDGE_BLOCK_END_DIM){
				buf_b2[2][iters * READ_W_PAR_PER_CH + nn - length_e22] = w32.w_attr[nn];  //edgeblock2, b2
			}
                
                //cout<<"load_w_c"<<endl;
            
            
           
			if(iters * READ_W_PAR_PER_CH + nn < length_b0){
				buf_w1[0][iters * READ_W_PAR_PER_CH + nn] = w20.w_attr[nn];  //nodeblock1, W0
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_b0 + MLP_HIDDEN_DIM){
				buf_b1[0][iters * READ_W_PAR_PER_CH + nn - length_b0] = w20.w_attr[nn]; //nodeblock1, b0
			}
			
			if(iters * READ_W_PAR_PER_CH + nn < length_b1){
				buf_w1[1][iters * READ_W_PAR_PER_CH + nn] = w21.w_attr[nn]; //nodeblock1, W1
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_b1 + MLP_HIDDEN_DIM){
				buf_b1[1][iters * READ_W_PAR_PER_CH + nn - length_b1] = w21.w_attr[nn]; //nodeblock1, b1
			}
			
			if(iters * READ_W_PAR_PER_CH + nn < length_b2){
				buf_w1[2][iters * READ_W_PAR_PER_CH + nn] = w22.w_attr[nn]; //nodeblock1, W2
			}
			else if(iters * READ_W_PAR_PER_CH + nn < length_b2 + NODE_BLOCK_OUT_DIM){
				buf_b1[2][iters * READ_W_PAR_PER_CH + nn - length_b2] = w22.w_attr[nn]; //nodeblock1, b2
			}
                //cout<<"load_w_d"<<endl;
        }
    }
}
void load_graph_src(
    const input_src  *S_ch0_test,     
    // index_t           src_list[SRC_NODE],
    index_t           *src_list,
    
    int num_of_src_node
){
    int num_iters = ceildiv_even(SRC_NODE, READ_N_PAR_PER_CH);
    //cout<<"load_graph_src "<<num_iters<<endl;
	
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = ceildiv_even(SRC_NODE, READ_N_PAR_PER_CH) 
        input_src ST0 = S_ch0_test[iters];
 #pragma HLS array_partition variable=ST0 dim=1 complete       
        for(int nn = 0; nn < READ_N_PAR_PER_CH; nn++){
           if(iters * READ_N_PAR_PER_CH + nn < num_of_src_node){
               src_list[iters * READ_N_PAR_PER_CH + nn] = ST0.s_attr[nn];
           }
        }
    }
    
    
}
    
void reset_node_tmp(
    #ifdef __SYNTHESIS__
    node_str          node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str          node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    #else
    // node_str          (*node_update_tmp1)[MAX_NODE],
	// node_str          (*node_update_tmp2)[MAX_NODE],
    
    node_str          (**node_update_tmp1),
	node_str          (**node_update_tmp2),
    #endif
    int num_of_node
){
    node_str zero = 0;
    for(int n=0;n<MAX_NODE;n++){
#pragma HLS PIPELINE II=1 
#pragma HLS UNROLL factor = MERGE_PAR * 2
        for(int pe = 0; pe < NS_BLOCK_PAR/2; pe++){
#pragma HLS UNROLL
            node_update_tmp1[pe][n] = zero;
        }
    }
	
	for(int n=0;n<MAX_NODE;n++){
#pragma HLS PIPELINE II=1 
#pragma HLS UNROLL factor = MERGE_PAR * 2
        for(int pe = 0; pe < N_BLOCK_PAR/2; pe++){
#pragma HLS UNROLL
            node_update_tmp2[pe][n] = zero;
        }
    }

}
void load_graph_edge_v2(
    //ap_uint<WIDTH_E> *E_ch0,  //edge*n //HBM
    //ap_uint<WIDTH_I> *I_ch0, //edge*2 //HBM
    
    const input_edge0 *E_ch0_test,
    //const input_edge1 *E_ch1_test,
    const input_idx  *I_ch0_test,
    
    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair> edge_list_str[E_BLOCK_PAR],

    //index_t edge_list[MAX_EDGE][2], //internal BRAM
    int num_of_edge
){
//#pragma HLS INLINE off
//#pragma HLS PIPELINE II=1
    int num_iters = ceildiv(MAX_EDGE, E_BLOCK_PAR) ;
    //cout<<"load_graph_edge_v2 "<<num_iters<<endl;
	
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1 
#pragma HLS loop_tripcount max = ceildiv_even(MAX_EDGE, E_BLOCK_PAR)
        //cout<<"load_edge_iters"<<iters<<endl;
        //ap_uint<WIDTH_E> E0 = E_ch0[iters];
        //ap_uint<WIDTH_I> I0 = I_ch0[iters];
        
        input_edge0 ET0 = E_ch0_test[iters];
        //input_edge1 ET1 = E_ch1_test[iters];
        input_idx  IT0 = I_ch0_test[iters];
        //cout<<"load_edge_iters"<<iters<<endl;
        
        for(int pe = 0; pe < E_BLOCK_PAR; pe++){
#pragma HLS UNROLL               
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                    
                edge e;
                idx_pair idx;
                
                //cout<<"=============="<<endl;
                //cout<<"addr: "<< &(E_ch0_test[iters]) <<endl;
                //print_fixed()
                
                for(int dim = 0; dim < EDGE_DIM; dim++){
#pragma HLS UNROLL  
                    // if(pe<READ_E_PAR_PER_CH0){
                        // e[dim] = ET0.e_attr[pe * EDGE_DIM + dim];
                    // }else{
                        // e[dim] = ET1.e_attr[(pe-READ_E_PAR_PER_CH0) * EDGE_DIM + dim];
                    // }
                    
                    e[dim] = ET0.e_attr[pe * EDGE_DIM + dim];
                    
                    if(dim < 2){
                        idx[dim] = IT0.i_attr[pe * 2 + dim];
                    }
                }
                
                // cout<<"load edge"<<endl;
                // print_fixed(e,12);
                // cout<<idx[0]<<" "<<idx[1]<<endl;
                
                edge_in[pe] << e;
                edge_list_str[pe] << idx;
                
            }
        }
        //cout<<endl;
    }
}
void load_graph_node_L1(

    const input_node  *N_ch0_test,
    #ifdef __SYNTHESIS__
    node_str          node_src_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_in_IN[NS_BLOCK_PAR/2][MAX_NODE],
    #else
	// node_str          (*node_src_mp_L1)[MAX_NODE],
    // node_str          (*node_des_mp_L1)[MAX_NODE],
    node_str          (**node_src_mp_L1),
    node_str          (**node_des_mp_L1),
    node_str          (**node_in_IN),                              
    #endif
    // node_str          node_in_IN[MAX_NODE],
    
    int num_of_node
){

//#pragma HLS array_partition variable=N_ch0_test dim=2 complete
#pragma HLS aggregate variable=N_ch0_test
    
    int num_iters = ceildiv_even(MAX_NODE, READ_N_PAR_PER_CH);
	
	//cout<<"load_graph_node_L1 "<<endl;
	//cout<<N_ch0_test[0].n_attr[0]<<endl;
    
    for(int iters = 0 ; iters < num_iters; iters++){
#pragma HLS PIPELINE II=1    

        input_node NT0 = N_ch0_test[iters];
#pragma HLS array_partition variable=NT0 dim=1 complete       
        for(int nn = 0; nn < READ_N_PAR_PER_CH; nn++){
//#pragma HLS PIPELINE II=1    
#pragma HLS UNROLL
            node_str tmp1;
			
			if(iters * READ_N_PAR_PER_CH + nn < MAX_NODE){
				for(int dim=0; dim<NODE_DIM; dim++){
					tmp1(data_w * (dim+1) -1, data_w * dim)= DTYPE_to_uint16<DTYPE>(NT0.n_attr[nn * NODE_DIM + dim]);
				}

				for(int pe=0; pe<NS_BLOCK_PAR/2; pe++){
                    node_in_IN[pe][iters * READ_N_PAR_PER_CH + nn] = tmp1;
                }

				for(int pe = 0; pe < E_BLOCK_PAR/2; pe++){
		#pragma HLS UNROLL
					node_src_mp_L1[pe][iters * READ_N_PAR_PER_CH + nn] = tmp1;
					node_des_mp_L1[pe][iters * READ_N_PAR_PER_CH + nn] = tmp1;
				}
				
				if(iters * READ_N_PAR_PER_CH + nn == MAX_NODE-1)
					cout<<iters<<"lgn "<<node_des_mp_L1[3][481]<<endl;
			}
        }
		
		//
    }
	/*
	cout<<"load node 1"<<endl;
	node nd;
	for(int dim=0;dim<3;dim++){
	nd[dim](data_w-1, 0) = uint16_to_DTYPE<DTYPE>(node_des_mp_L1[3][1](data_w * (dim+1)-1, data_w *dim));
    }
	print_fixed(nd, 8);
	*/
	/*
	ap_uint<48> ttt = (ap_uint<48>)node_des_mp_L1[4][11];
	cout<<"test1: "<<ttt<<endl;
	ap_uint<64> ttt_64 = (ap_uint<64>)node_des_mp_L1[4][11];
	cout<<"test1: "<<ttt_64<<endl;
	
	cout<<"test1: "<<node_des_mp_L1[4][11]<<endl;
	
    ap_uint<16> t1 = (ttt)(15, 0);
	ap_uint<16> t2 = (node_des_mp_L1[4][11])(31, 16);
	ap_uint<16> t3 = (node_des_mp_L1[4][11])(47, 32);
	
	attr_t nt1 = uint16_to_DTYPE<attr_t>(t1);
	attr_t nt2 = uint16_to_DTYPE<attr_t>(t2);
	attr_t nt3 = uint16_to_DTYPE<attr_t>(t3);
	
	cout<<"test: "<<nt1<<" "<<nt2<<" "<<nt3<<endl;
	
	for(int pe = 0; pe<4; pe++){
		cout<<(node_src_mp_L1[pe])<<" ";
	}
	cout<<endl;
	for(int pe = 0; pe<3; pe++){
		cout<<(node_src_mp_L1[pe+1]) - (node_src_mp_L1[pe])<<" ";
	}
	cout<<endl;
	for(int i=0; i<12; i++){
		cout<<&(node_src_mp_L1[0][i])<<" ";
	}
	cout<<endl;
	for(int i=0; i<12; i++){
		cout<<&(node_src_mp_L1[1][i])<<" ";
	}
	cout<<endl;
	
    cout<<node_in_IN[11](47, 0)<<" "<<node_src_mp_L1[0][11](47, 0)<<" "
                <<node_des_mp_L1[0][11](47, 0)<<" "<<node_src_mp_L1[1][11](47, 0)<<" "<<node_src_mp_L1[2][11](47, 0)<<" "<<(ap_uint<48>)node_src_mp_L1[3][11](47, 0)<<" "<<endl;
    */
	/*
    cout<< node_in_IN[11]<<endl;
    cout<<"load node in 11"<<endl;
	for(int b=0; b<16; b++){
		cout<< node_in_IN[11][(15-b)];
	}
	
    for(int dim=0; dim<3; dim++){
		
        cout<< (ap_uint<16>)(node_in_IN[11](data_w * (dim+1)-1, data_w *dim))<<" ";
    }
    cout<<endl;
    cout<<"load node des 11"<<endl;
    for(int dim=0; dim<3; dim++){
        cout<< (node_des_mp_1d[4 * MAX_NODE + 11](data_w * (dim+1)-1, data_w *dim)) <<" ";
    }
    cout<<endl; 
	*/
    
}
void write_back_ping_pong(
	hls::stream<edge_res>  edge_in[E2_BLOCK_PAR], //edge_block2 outout modify? //final_output
	output_edge            *O_ch0,

    int num_of_edge
){
//#pragma HLS DATAFLOW
    int num_iters = ceildiv_even(MAX_EDGE, E2_BLOCK_PAR);
    //cout<<"write_back_ping_pong "<<num_iters<<endl;
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = ceildiv_even(MAX_EDGE, E2_BLOCK_PAR) 
        output_edge out;
        for(int pe = 0 ; pe < E2_BLOCK_PAR; pe ++){
            if(iters * E2_BLOCK_PAR + pe < num_of_edge){
                edge_res   e;
                edge_in[pe] >> e;
                out.o_attr[pe] = e[0];
                
            }
			else{
				out.o_attr[pe] = 0;
			}
            
            
        }
        O_ch0[iters] = out;
    }
}
void stream_adaptor_8_12(
    hls::stream<edge>     edge_embedding_adapt[12], //output-streaming message   
    hls::stream<edge>     edge_embedding_in[8],
	int num_of_edge
){
    
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR);
    ap_uint<7> offset = 0;

	edge edge_0 = {0,0,0,0};
    edge tmp[E_BLOCK_PAR];
	edge keep[E2_BLOCK_PAR];
	edge save[E2_BLOCK_PAR];
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
//cout<<"stream_adaptor iters "<<iters<<endl;
        
		for(int pe=0; pe<E_BLOCK_PAR; pe++){
#pragma HLS UNROLL			
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp[pe];
			}
        }
		
		ap_uint<3> mod = iters % 3;
		switch(mod){
			case 0: 
				for(int i=0; i<8; i++){
					keep[i] = tmp[i];
				}
				break;
			case 1: 
				for(int i=0; i<4; i++){
					save[8+i] = tmp[i];
				}
				for(int i=0; i<8; i++){
					save[i] = keep[i];
				}
				for(int i=0; i<4; i++){
					keep[i] = tmp[4+i];
				}
				for(int c=0; c<12; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
			case 2: 
				for(int i=0; i<8; i++){
					save[4+i] = tmp[i];
				}
				for(int i=0; i<4; i++){
					save[i] = keep[i];
				}
				for(int c=0; c<12; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
			
        }  
		/*
		ap_uint<7> offset_add = offset + E_BLOCK_PAR;
        for(int pe=0; pe<E_BLOCK_PAR; pe++){
#pragma HLS UNROLL			
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp[pe];
			}
        }
		if(offset_add >= E2_BLOCK_PAR){
			for(int c=0; c<E2_BLOCK_PAR; c++){
				if(c<offset){
					save[c] = keep[c];
				}
			}
			for(int c=0; c<E_BLOCK_PAR; c++){
				if(c>=E2_BLOCK_PAR-offset){
					keep[c-(E2_BLOCK_PAR-offset)] = tmp[c];
				}else{
					save[offset + c] = tmp[c];
				}
			}
			for(int c=0; c<E2_BLOCK_PAR; c++){
				edge_embedding_adapt[c] << save[c];
			}
			offset = offset_add-E2_BLOCK_PAR;
		}else{
			for(int c=0; c<E_BLOCK_PAR; c++){
				keep[offset+c] = tmp[c];
			}
			offset = offset_add;
		}
    }*/
	}
	for(int pe2 = 0; pe2<E2_BLOCK_PAR; pe2++){
#pragma HLS UNROLL
		if(pe2<(num_of_edge % E2_BLOCK_PAR))
			edge_embedding_adapt[pe2] << keep[pe2];
	}
	
}
void stream_adaptor_16_20(
    hls::stream<edge>     edge_embedding_adapt[20], //output-streaming message   
    hls::stream<edge>     edge_embedding_in[16],
	int num_of_edge
){
    
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR);
    ap_uint<7> offset = 0;

	edge edge_0 = {0,0,0,0};
    edge tmp[E_BLOCK_PAR];
	edge keep[E2_BLOCK_PAR];
	edge save[E2_BLOCK_PAR];
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
//cout<<"stream_adaptor iters "<<iters<<endl;
        
		for(int pe=0; pe<E_BLOCK_PAR; pe++){
#pragma HLS UNROLL			
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp[pe];
			}
        }
		
		ap_uint<3> mod = iters % 5;
		switch(mod){
			case 0: 
				for(int i=0; i<16; i++){
					keep[i] = tmp[i];
				}
				break;
			case 1: 
				for(int i=0; i<4; i++){
					save[16+i] = tmp[i];
				}
				for(int i=0; i<16; i++){
					save[i] = keep[i];
				}
				for(int i=0; i<12; i++){
					keep[i] = tmp[4+i];
				}
				for(int c=0; c<20; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
			case 2: 
				for(int i=0; i<8; i++){
					save[12+i] = tmp[i];
				}
				for(int i=0; i<12; i++){
					save[i] = keep[i];
				}
				for(int i=0; i<8; i++){
					keep[i] = tmp[8+i];
				}
				for(int c=0; c<20; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
			case 3: 
				for(int i=0; i<12; i++){
					save[8+i] = tmp[i];
				}
				for(int i=0; i<8; i++){
					save[i] = keep[i];
				}
				for(int i=0; i<4; i++){
					keep[i] = tmp[12+i];
				}
				for(int c=0; c<20; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
			case 4: 
				for(int i=0; i<16; i++){
					save[4+i] = tmp[i];
				}
				for(int i=0; i<4; i++){
					save[i] = keep[i];
				}
				for(int i=0; i<0; i++){
					keep[i] = tmp[16+i];
				}
				for(int c=0; c<20; c++){
					edge_embedding_adapt[c] << save[c];
				}
				break;
        }  
		/*
		ap_uint<7> offset_add = offset + E_BLOCK_PAR;
        for(int pe=0; pe<E_BLOCK_PAR; pe++){
#pragma HLS UNROLL			
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp[pe];
			}
        }
		if(offset_add >= E2_BLOCK_PAR){
			for(int c=0; c<E2_BLOCK_PAR; c++){
				if(c<offset){
					save[c] = keep[c];
				}
			}
			for(int c=0; c<E_BLOCK_PAR; c++){
				if(c>=E2_BLOCK_PAR-offset){
					keep[c-(E2_BLOCK_PAR-offset)] = tmp[c];
				}else{
					save[offset + c] = tmp[c];
				}
			}
			for(int c=0; c<E2_BLOCK_PAR; c++){
				edge_embedding_adapt[c] << save[c];
			}
			offset = offset_add-E2_BLOCK_PAR;
		}else{
			for(int c=0; c<E_BLOCK_PAR; c++){
				keep[offset+c] = tmp[c];
			}
			offset = offset_add;
		}
    }*/
	}
	for(int pe2 = 0; pe2<E2_BLOCK_PAR; pe2++){
#pragma HLS UNROLL
		if(pe2<(num_of_edge % E2_BLOCK_PAR))
			edge_embedding_adapt[pe2] << keep[pe2];
	}
	
}
void stream_adaptor(
    hls::stream<edge>     edge_embedding_adapt[E2_BLOCK_PAR], //output-streaming message   
    hls::stream<edge>     edge_embedding_in[E_BLOCK_PAR],
	int num_of_edge
){
    
    int num_iters = ceildiv(num_of_edge, E_BLOCK_PAR);
    ap_uint<7> offset = 0;

	edge tmp_collect[E2_BLOCK_PAR];
#pragma HLS ARRAY_PARTITION variable=tmp_collect dim=1 complete
	edge  edge_0 = {0,0,0,0};
    edge tmp[E_BLOCK_PAR];
	edge keep[E2_BLOCK_PAR];
	edge save[E2_BLOCK_PAR];
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_EDGE / E_BLOCK_PAR + 1
#pragma HLS DEPENDENCE variable=tmp_collect intra WAW false 
cout<<"stream_adaptor iters "<<iters<<endl;
        
		ap_uint<7> offset_add = offset + E_BLOCK_PAR;
        for(int pe=0; pe<E_BLOCK_PAR; pe++){
#pragma HLS UNROLL			
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp[pe];
			}
        }
		if(offset_add >= E2_BLOCK_PAR){
			for(int c=0; c<E2_BLOCK_PAR; c++){
				if(c<offset){
					save[c] = keep[c];
				}
			}
			for(int c=0; c<E_BLOCK_PAR; c++){
				if(c>=E2_BLOCK_PAR-offset){
					keep[c-(E2_BLOCK_PAR-offset)] = tmp[c];
				}else{
					save[offset + c] = tmp[c];
				}
			}
			for(int c=0; c<E2_BLOCK_PAR; c++){
				edge_embedding_adapt[c] << save[c];
			}
			offset = offset_add-E2_BLOCK_PAR;
		}else{
			for(int c=0; c<E_BLOCK_PAR; c++){
				keep[offset+c] = tmp[c];
			}
			offset = offset_add;
		}
		/*
        for(int pe=0; pe<E_BLOCK_PAR; pe++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge_embedding_in[pe] >> tmp_collect[(offset+pe)%(2*E2_BLOCK_PAR)];
				//tmp_collect[offset+pe] = tmp[pe];
            }else{
				tmp_collect[(offset+pe)%(2*E2_BLOCK_PAR)] = edge_0;
			}
        }
		ap_uint<5> offset_tmp = offset+E_BLOCK_PAR;
		if(offset_tmp >= E2_BLOCK_PAR){
			for(int pe2 = 0; pe2<E2_BLOCK_PAR; pe2++){
				edge_embedding_adapt[pe2] << tmp_collect[pe2];

			}

			offset = offset_tmp%E2_BLOCK_PAR;
		}else{
			offset = offset_tmp;
		}
		*/
    }
	for(int pe2 = 0; pe2<E2_BLOCK_PAR; pe2++){
#pragma HLS UNROLL
		if(pe2<(num_of_edge % E2_BLOCK_PAR))
			edge_embedding_adapt[pe2] << keep[pe2];
	}
}

void message_passing_L2_ping_pong(
    
    hls::stream<edge_res>     edge_embedding_out[E2_BLOCK_PAR], //output-streaming message   
    hls::stream<edge>         edge_embedding_in[E_BLOCK_PAR],
    
    #ifdef __SYNTHESIS__
    index_t                   edge_list[MAX_EDGE][2],
    node_str      node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE], //input BRAM
	node_str      node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    #else
    index_t       (**edge_list),
    node_str      (**node_update_tmp1), //input BRAM
	node_str      (**node_update_tmp2),    
    #endif
    
	
    // index_t       (*edge_list)[2],
    
    // node_str      (*node_update_tmp1)[MAX_NODE], //input BRAM
	// node_str      (*node_update_tmp2)[MAX_NODE],
    
    
    
	/*
    //edgeblock2
    weight_t buf_w2[][MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[][MLP_LAYER][MAX_BIAS_DIM],
    */
	//edgeblock2
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
	
    int num_of_edge,
    int num_of_node,
    int num_of_des_node,
	int num_of_src_node
){
#pragma HLS DATAFLOW
//#pragma HLS INLINE off    
    
    cout<<num_of_src_node<<endl;
    cout<<num_of_edge<<endl;
    cout<<"why???"<<endl;
    node_str node_src_mp_L2[E2_BLOCK_PAR/2][MAX_NODE];
    node_str node_des_mp_L2[E2_BLOCK_PAR/2][MAX_NODE];
    //cout<<"hihi14"<<endl;

#pragma HLS ARRAY_PARTITION variable=node_src_mp_L2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L2 cyclic factor=MERGE_PAR dim=2


#pragma HLS ARRAY_PARTITION variable=node_des_mp_L2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L2 cyclic factor=MERGE_PAR dim=2

#pragma HLS BIND_STORAGE variable=node_src_mp_L2 type=ram_2p impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_des_mp_L2 type=ram_2p impl=URAM latency=3

hls::stream<edge>  edge_embedding_adapt_in[E2_BLOCK_PAR];
#pragma HLS STREAM variable=edge_embedding_adapt_in depth=1100//ceildiv(MAX_EDGE, E2_BLOCK_PAR)+4 //!!!!!!!!!!

    node_block_end_par_8_join_end(
		
		node_update_tmp1,
		node_update_tmp2,
		
		//node_out_L1,
		node_src_mp_L2,
		node_des_mp_L2,
		
		num_of_edge,
		num_of_des_node,
		num_of_src_node
	);
    cout<<"node_block_end_par_8_join_end done"<<endl;
    stream_adaptor_8_12(
        edge_embedding_adapt_in,
        edge_embedding_in,
		num_of_edge
    );
    cout<<"stream_adaptor done"<<endl;
	
    edge_block_L2(
        node_src_mp_L2,        //input - BRAM
        node_des_mp_L2,        //input - BRAM
        edge_embedding_adapt_in,
        edge_list, 
        
        edge_embedding_out,
        
        buf_w2,
        buf_b2,
        
        num_of_edge
    );
    //cout<<"edge_block_L2 done"<<endl;
}

void message_passing_L1_ping_pong(

    #ifdef __SYNTHESIS__
    index_t       edge_list_IN[MAX_EDGE][2], //output BRAM
    node_str      node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE], //output BRAM
	node_str      node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    
    hls::stream<edge>      edge_embedding_copy[E_BLOCK_PAR], //output-streaming message
    hls::stream<edge>      edge_in[E_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_list[E_BLOCK_PAR], //input-streaming message
    
    node_str      node_src_mp[E_BLOCK_PAR/2][MAX_NODE],
    node_str      node_des_mp[E_BLOCK_PAR/2][MAX_NODE],
    node_str      node_padding[MAX_NODE],
    
    #else
    // index_t       (*edge_list_IN)[2], //output BRAM
    // node_str      (*node_update_tmp1)[MAX_NODE], //output BRAM
	// node_str      (*node_update_tmp2)[MAX_NODE],
    
    index_t       (**edge_list_IN), //output BRAM
    node_str      (**node_update_tmp1), //output BRAM
	node_str      (**node_update_tmp2),
   

    hls::stream<edge>      edge_embedding_copy[E_BLOCK_PAR], //output-streaming message
    hls::stream<edge>      edge_in[E_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_list[E_BLOCK_PAR], //input-streaming message
    
    // node_str      (*node_src_mp)[MAX_NODE],
    // node_str      (*node_des_mp)[MAX_NODE],
    
    node_str      (**node_src_mp),
    node_str      (**node_des_mp),
    
    node_str      *node_padding,

    #endif
    
    
   
    index_t       *src_list,
	
	/*
    //edgeblock
    weight_t    buf_w0[][MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b0[][MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t    buf_w1[][MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b1[][MLP_LAYER][MAX_BIAS_DIM],
	*/
	 //edgeblock
    weight_t    buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t    buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b1[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_edge,
    int num_of_node,
    int num_of_des_node,
    int num_of_src_node
){

//#pragma HLS INLINE off   
#pragma HLS DATAFLOW
//><><><<><>><><<><><><><><><><><><><>><
//================================================
//edgeblock (many PE inside)
// |-load subgraph data to node_s, node_r, edge_sr
// |-execute MLP
//input: list of load data 
//================================================

    hls::stream<edge>      edge_embedding[E_BLOCK_PAR];
    hls::stream<node>      node_des[E_BLOCK_PAR];
    hls::stream<index_t>   idx_des[E_BLOCK_PAR];

    hls::stream<sens_agg_edge>      edge_agg_par_4[E_BLOCK_PAR];
    hls::stream<node>      node_des_agg_par_4[E_BLOCK_PAR];
    hls::stream<index_t>   idx_des_agg_par_4[E_BLOCK_PAR];
    
    hls::stream<node>      node_des_n[N_BLOCK_PAR]; //output-streaming message
    hls::stream<edge>      edge_des_n[N_BLOCK_PAR];
    hls::stream<index_t>   idx_des_n[N_BLOCK_PAR];
	
	hls::stream<node>   	node_fork_n[NS_BLOCK_PAR];
    hls::stream<edge>   	edge_fork_n[NS_BLOCK_PAR];
    hls::stream<index_t> 	idx_fork_n[NS_BLOCK_PAR];
    
    hls::stream<node>      node_out1[N_BLOCK_PAR]; //output-streaming message
    hls::stream<index_t>   idx_des_out1[N_BLOCK_PAR];
	hls::stream<node>      node_out2[N_BLOCK_PAR]; //output-streaming message
    hls::stream<index_t>   idx_des_out2[N_BLOCK_PAR];


#ifdef MIN_FIFO_DEPTH

#pragma HLS STREAM variable=edge_embedding		depth=2
#pragma HLS STREAM variable=node_des       		depth=2
#pragma HLS STREAM variable=idx_des        		depth=2

#pragma HLS STREAM variable=edge_agg_par_4       depth=3//48 (SRC_NODE / N_BLOCK_PAR)//need depth or not??     co-sim:52
#pragma HLS STREAM variable=node_des_agg_par_4   depth=3//(SRC_NODE / N_BLOCK_PAR)//need depth or not??     
#pragma HLS STREAM variable=idx_des_agg_par_4    depth=3//(SRC_NODE / N_BLOCK_PAR)//need depth or not??     


#pragma HLS STREAM variable=node_fork_n          depth=3
#pragma HLS STREAM variable=edge_fork_n          depth=3
#pragma HLS STREAM variable=idx_fork_n           depth=3

#pragma HLS STREAM variable=node_out1    		 depth=2
#pragma HLS STREAM variable=idx_des_out1     	 depth=2

#pragma HLS STREAM variable=node_out2            depth=2
#pragma HLS STREAM variable=idx_des_out2         depth=2

   

#else

#pragma HLS STREAM variable=edge_embedding  depth=MAX_EDGE
#pragma HLS STREAM variable=node_des        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des         depth=MAX_EDGE

#pragma HLS STREAM variable=edge_agg_par_4        depth=MAX_EDGE
#pragma HLS STREAM variable=node_des_agg_par_4    depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_agg_par_4     depth=MAX_EDGE   


#pragma HLS STREAM variable=node_fork_n      depth=MAX_NODE
#pragma HLS STREAM variable=edge_fork_n      depth=MAX_EDGE
#pragma HLS STREAM variable=idx_fork_n       depth=MAX_EDGE

#pragma HLS STREAM variable=node_out1        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_out1     depth=MAX_EDGE

#pragma HLS STREAM variable=node_out2        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_out2     depth=MAX_EDGE  
  

#endif 
    cout<<"hihi8"<<endl;
    
    edge_block(
        
        node_src_mp,        //input - BRAM
        node_des_mp,        //input - BRAM
        edge_in,
        edge_list, 
        
        edge_embedding,     //output-streaming message
        edge_embedding_copy,
        node_des,
        idx_des,
		
		edge_list_IN,
        
        buf_w0,
        buf_b0,
        
        num_of_edge
     );
	//cout<<"edge_block done"<<endl;
    
  
	node_block_fork(
        node_padding,
        src_list,
        
        node_fork_n,      //output-streaming message
        edge_fork_n,
        idx_fork_n,
        
        num_of_src_node
    );
    //cout<<"node_block_fork done"<<endl;
//><><><<><>><><<><><><><><><><><><><>><
//================================================
//aggregation (many PE inside)
// original (critcal path II = #EDGE)
//================================================
   
    //cout<<"hihi9"<<endl;
    aggregation_global_par_8_cl(
        edge_embedding,     //input-streaming message
        idx_des,
        node_des,
    
        node_des_agg_par_4, //output-streaming message
        edge_agg_par_4,
        idx_des_agg_par_4,
        
        num_of_edge
    );
    //cout<<"aggregation_global_par_8_time done"<<endl;
    //cout<<"hihi10"<<endl; 
   
    //inline!!
	node_block_par_4_not_join(
        node_des_agg_par_4, //input-streaming message
        edge_agg_par_4,
        idx_des_agg_par_4,
        
        node_fork_n,      //input-streaming message
        edge_fork_n,
        idx_fork_n,
   
        node_out1, //output-streaming message
		idx_des_out1,
	
		node_out2, //output-streaming message
		idx_des_out2,
        
        buf_w1,
        buf_b1,
        
        num_of_edge,
        num_of_des_node,
        num_of_src_node
    );	
    //cout<<"node_block_par_4_join done"<<endl;
    //inline!!
    node_block_end_par_4_join(
		node_out1, //input-streaming message
		idx_des_out1,

		node_out2, //input-streaming message
		idx_des_out2,

		node_update_tmp1,
		node_update_tmp2,
		
		num_of_edge,
		num_of_des_node,
		num_of_src_node
	);
    //cout<<"node_block_end_par_4_join done"<<endl;
}
/*
void pre_load_ping_pong(

    //input
    input_node *N_ch0_test,
    input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    input_idx  *I_ch0_test,
    input_src  *S_ch0_test,
    
    
    #ifdef __SYNTHESIS__
    //output
    node_str          node_src_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_in_IN[MAX_NODE],
    index_t           src_list[SRC_NODE],
    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    node_str          node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str          node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    
    #else
    node_str          (**node_src_mp_L1),
    node_str          (**node_des_mp_L1),
    // node_str          (*node_src_mp_L1)[MAX_NODE],
    // node_str          (*node_des_mp_L1)[MAX_NODE],
    node_str          *node_in_IN,
    index_t           *src_list,

    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    
    node_str          (**node_update_tmp1),
	node_str          (**node_update_tmp2),
    // node_str          (*node_update_tmp1)[MAX_NODE],
	// node_str          (*node_update_tmp2)[MAX_NODE],
    
    #endif
    
    //parameter
    int num_of_node,
    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
    
){
#pragma HLS DATAFLOW

    load_graph_node_L1(
        
        N_ch0_test,        
        node_src_mp_L1, 
        node_des_mp_L1,
        node_in_IN,
        num_of_node
    );
    cout<<"load_graph_node_L1 done"<<endl;
    
    load_graph_edge_v2(
        
        E_ch0_test,
        //E_ch1_test,
        I_ch0_test,
        
        edge_in, 
        edge_in_idx,
        //edge_list_IN,
        num_of_edge
    );
    cout<<"load_graph_edge_v2 done"<<endl;
    
    load_graph_src(
        S_ch0_test,        
        src_list,
        num_of_src_node
    );
    cout<<"load_graph_src done"<<endl;
    reset_node_tmp(
        node_update_tmp1,
        node_update_tmp2,
        num_of_node
    );
    cout<<"reset_node_tmp done"<<endl;
}
*/


void message_passing_L2_ping_pong_SLG(
    
    hls::stream<edge_res>     edge_embedding_out[E2_BLOCK_PAR], //output-streaming message   
    hls::stream<edge>         edge_embedding_in[E_BLOCK_PAR],
    hls::stream<idx_pair>     edge_list[E_BLOCK_PAR],

    #ifdef __SYNTHESIS__
    //output
    node_str          node_src_mp_L2[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des_mp_L2[E_BLOCK_PAR/2][MAX_NODE],
    #else
    node_str          **node_src_mp_L2,
    node_str          **node_des_mp_L2,
    #endif
	
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
	
    int num_of_edge,
    int num_of_node
){
#pragma HLS DATAFLOW
//#pragma HLS INLINE off    

    edge_block_L2_SLG(
        node_src_mp_L2,        //input - BRAM
        node_des_mp_L2,        //input - BRAM
        edge_embedding_in,
        edge_list, 
        
        edge_embedding_out,
        
        buf_w2,
        buf_b2,
        
        num_of_edge
    );
    cout<<"edge_block_L2 done"<<endl;
}
void load_MPL2_ping_pong(

    //input
    input_edge0 *E_ch0_M1_out,
    //input_edge1 *E_ch1_M1_out,
    input_idx  *I_ch0_M1_out,
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
   
    //parameter
    int num_of_edge
){
#pragma HLS DATAFLOW

    load_graph_edge_v2(
        
        E_ch0_M1_out,
        //E_ch1_M1_out,
        I_ch0_M1_out,
        
        edge_in, 
        edge_in_idx,
        num_of_edge
    );
    cout<<"load_graph_edge_v2 done"<<endl;
}
void pre_load_MPL2_ping_pong(

    //input
    input_edge0 *E_ch0_M1_out,
    //input_edge1 *E_ch1_M1_out,
    input_idx  *I_ch0_M1_out,
    
    #ifdef __SYNTHESIS__
    //output
    node_str          node_src_mp_L2[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des_mp_L2[E_BLOCK_PAR/2][MAX_NODE],
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    node_str          node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str          node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    
    #else
    node_str          **node_src_mp_L2,
    node_str          **node_des_mp_L2,
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    node_str          (**node_update_tmp1),
	node_str          (**node_update_tmp2),
    #endif
    
    //parameter
    int num_of_node,
    int num_of_edge,
	int num_of_src_node
    
){
#pragma HLS DATAFLOW

    load_graph_edge_v2(
        
        E_ch0_M1_out,
        //E_ch1_M1_out,
        I_ch0_M1_out,
        
        edge_in, 
        edge_in_idx,
        num_of_edge
    );
    cout<<"load_graph_edge_v2 done"<<endl;
    
    node_block_end_par_8_join_end_SLG(
		
		node_update_tmp1,
		node_update_tmp2,
		
		//node_out_L1,
		node_src_mp_L2,
		node_des_mp_L2,
		
		num_of_edge,
		num_of_src_node
	);
    cout<<"node_block_end_par_8_join_end done"<<endl;
}
void store_MPL1_ping_pong(
    
    hls::stream<edge>         edge_in[E_BLOCK_PAR],
    input_edge0 *E_ch0_M1_out,
    //input_edge1 *E_ch1_M1_out,
    input_idx  *I_ch0_M1_out,
    
    #ifdef __SYNTHESIS__
    index_t                   edge_list[MAX_EDGE][2],
    #else
    index_t       (**edge_list),
    #endif
    
    int num_of_edge
){
#pragma HLS DATAFLOW
    int num_iters = ceildiv_even(MAX_EDGE, E_BLOCK_PAR);
    cout<<"write_back_ping_pong "<<num_iters<<endl;
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = ceildiv_even(MAX_EDGE, E_BLOCK_PAR) 
        input_edge0 out_e;
        input_idx   out_i;
        for(int pe = 0 ; pe < E_BLOCK_PAR; pe ++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge   e;
                edge_in[pe] >> e;
                for(int dim =0; dim<EDGE_DIM; dim++){
#pragma HLS UNROLL
                    out_e.e_attr[pe*EDGE_DIM + dim] = e[dim];
                }
                for(int dim =0; dim<2; dim++){
#pragma HLS UNROLL
                    out_i.i_attr[pe*2 + dim] = edge_list[iters * E_BLOCK_PAR + pe][dim];
                }
            }
        }
        E_ch0_M1_out[iters] = out_e;
        I_ch0_M1_out[iters] = out_i;
    }
}
void store_MPL1_ping_pong_edge(
	hls::stream<edge>  edge_in[E_BLOCK_PAR], //edge_block2 outout modify? //final_output
	input_edge0        *E_ch0_M1_out,

    int num_of_edge
){
//#pragma HLS DATAFLOW
    int num_iters = ceildiv_even(MAX_EDGE, E_BLOCK_PAR);
    //cout<<"write_back_ping_pong "<<num_iters<<endl;
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = ceildiv_even(MAX_EDGE, E_BLOCK_PAR) 
        input_edge0 out_e;
        for(int pe = 0 ; pe < E_BLOCK_PAR; pe ++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                edge   e;
                edge_in[pe] >> e;
				if(pe == 0){
					for(int dim =0; dim<EDGE_DIM; dim++){
						E_ch0_M1_out[iters].e_attr[0+ dim] = e[dim];
						//out_e.e_attr[pe*EDGE_DIM + dim] = e[dim];
					}
				}
				//E_ch0_M1_out[iters].e_attr[0] = e;
            }
        }
        
    } 
}
void store_MPL1_ping_pong_list(
    
    input_idx  *I_ch0_M1_out,
    
    #ifdef __SYNTHESIS__
    index_t                   edge_list[MAX_EDGE][2],
    #else
    index_t       (**edge_list),
    #endif
    
    int num_of_edge
){
    int num_iters = ceildiv_even(MAX_EDGE, E_BLOCK_PAR);
    cout<<"write_back_ping_pong_list "<<num_iters<<endl;
    for(int iters = 0; iters < num_iters; iters ++){
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = ceildiv_even(MAX_EDGE, E_BLOCK_PAR) 
        input_idx   out_i;
        for(int pe = 0 ; pe < E_BLOCK_PAR; pe ++){
            if(iters * E_BLOCK_PAR + pe < num_of_edge){
                for(int dim =0; dim<2; dim++){
                    out_i.i_attr[pe*2 + dim] = edge_list[iters * E_BLOCK_PAR + pe][dim];
                }
            }
        }
        I_ch0_M1_out[iters] = out_i;
    }
}

void message_passing_L1_ping_pong_SLG(

    #ifdef __SYNTHESIS__
    index_t       edge_list_IN[MAX_EDGE][2], //output BRAM
    node_str      node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE], //output BRAM
	node_str      node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    
    hls::stream<edge>      edge_embedding_copy[E_BLOCK_PAR], //output-streaming message
    hls::stream<edge>      edge_in[E_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_list[E_BLOCK_PAR], //input-streaming message
    
    node_str      node_src_mp[E_BLOCK_PAR/2][MAX_NODE],
    node_str      node_des_mp[E_BLOCK_PAR/2][MAX_NODE],
    node_str      node_padding[NS_BLOCK_PAR/2][MAX_NODE],
    
    #else
    // index_t       (*edge_list_IN)[2], //output BRAM
    // node_str      (*node_update_tmp1)[MAX_NODE], //output BRAM
	// node_str      (*node_update_tmp2)[MAX_NODE],
    
    index_t       (**edge_list_IN), //output BRAM
    node_str      (**node_update_tmp1), //output BRAM
	node_str      (**node_update_tmp2),
   

    hls::stream<edge>      edge_embedding_copy[E_BLOCK_PAR], //output-streaming message
    hls::stream<edge>      edge_in[E_BLOCK_PAR], //input-streaming message
    hls::stream<idx_pair>  edge_list[E_BLOCK_PAR], //input-streaming message
    
    // node_str      (*node_src_mp)[MAX_NODE],
    // node_str      (*node_des_mp)[MAX_NODE],
    
    node_str      (**node_src_mp),
    node_str      (**node_des_mp),
    
    node_str      **node_padding,

    #endif
   
    index_t       *src_list,
	 //edgeblock
    weight_t    buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t    buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t    buf_b1[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_edge,
    int num_of_node,
    int num_of_des_node,
    int num_of_src_node,
    bool init
){

//#pragma HLS INLINE off   
#pragma HLS DATAFLOW
//><><><<><>><><<><><><><><><><><><><>><
//================================================
//edgeblock (many PE inside)
// |-load subgraph data to node_s, node_r, edge_sr
// |-execute MLP
//input: list of load data 
//================================================

    hls::stream<edge>      edge_embedding[E_BLOCK_PAR];
    hls::stream<node>      node_des[E_BLOCK_PAR];
    hls::stream<index_t>   idx_des[E_BLOCK_PAR];

    hls::stream<sens_agg_edge>      edge_agg_par_4[E_BLOCK_PAR];
    hls::stream<node>      node_des_agg_par_4[E_BLOCK_PAR];
    hls::stream<index_t>   idx_des_agg_par_4[E_BLOCK_PAR];
    
    hls::stream<node>      node_des_n[N_BLOCK_PAR]; //output-streaming message
    hls::stream<edge>      edge_des_n[N_BLOCK_PAR];
    hls::stream<index_t>   idx_des_n[N_BLOCK_PAR];
	
	hls::stream<node>   	node_fork_n[NS_BLOCK_PAR];
    hls::stream<edge>   	edge_fork_n[NS_BLOCK_PAR];
    hls::stream<index_t> 	idx_fork_n[NS_BLOCK_PAR];
    
    hls::stream<node>      node_out1[N_BLOCK_PAR]; //output-streaming message
    hls::stream<index_t>   idx_des_out1[N_BLOCK_PAR];
	hls::stream<node>      node_out2[N_BLOCK_PAR]; //output-streaming message
    hls::stream<index_t>   idx_des_out2[N_BLOCK_PAR];


#ifdef MIN_FIFO_DEPTH

#pragma HLS STREAM variable=edge_embedding		depth=2
#pragma HLS STREAM variable=node_des       		depth=2
#pragma HLS STREAM variable=idx_des        		depth=2

#pragma HLS STREAM variable=edge_agg_par_4       depth=3//48 (SRC_NODE / N_BLOCK_PAR)//need depth or not??     co-sim:52
#pragma HLS STREAM variable=node_des_agg_par_4   depth=3//(SRC_NODE / N_BLOCK_PAR)//need depth or not??     
#pragma HLS STREAM variable=idx_des_agg_par_4    depth=3//(SRC_NODE / N_BLOCK_PAR)//need depth or not??     


#pragma HLS STREAM variable=node_fork_n          depth=3
#pragma HLS STREAM variable=edge_fork_n          depth=3
#pragma HLS STREAM variable=idx_fork_n           depth=3

#pragma HLS STREAM variable=node_out1    		 depth=2
#pragma HLS STREAM variable=idx_des_out1     	 depth=2

#pragma HLS STREAM variable=node_out2            depth=2
#pragma HLS STREAM variable=idx_des_out2         depth=2

   

#else

#pragma HLS STREAM variable=edge_embedding  depth=MAX_EDGE
#pragma HLS STREAM variable=node_des        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des         depth=MAX_EDGE

#pragma HLS STREAM variable=edge_agg_par_4        depth=MAX_EDGE
#pragma HLS STREAM variable=node_des_agg_par_4    depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_agg_par_4     depth=MAX_EDGE   


#pragma HLS STREAM variable=node_fork_n      depth=MAX_NODE
#pragma HLS STREAM variable=edge_fork_n      depth=MAX_EDGE
#pragma HLS STREAM variable=idx_fork_n       depth=MAX_EDGE

#pragma HLS STREAM variable=node_out1        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_out1     depth=MAX_EDGE

#pragma HLS STREAM variable=node_out2        depth=MAX_NODE
#pragma HLS STREAM variable=idx_des_out2     depth=MAX_EDGE  
  

#endif 
    cout<<"hihi8"<<endl;
    
    edge_block(
        
        node_src_mp,        //input - BRAM
        node_des_mp,        //input - BRAM
        edge_in,
        edge_list, 
        
        edge_embedding,     //output-streaming message
        edge_embedding_copy,
        node_des,
        idx_des,
		
		edge_list_IN,
        
        buf_w0,
        buf_b0,
        
        num_of_edge
     );
	cout<<"edge_block done"<<endl;
    
    
	node_block_fork_init(
        node_padding,
        src_list,
        
        node_fork_n,      //output-streaming message
        edge_fork_n,
        idx_fork_n,
        
        num_of_src_node,
        init
    );
    
    cout<<"node_block_fork done"<<endl;
//><><><<><>><><<><><><><><><><><><><>><
//================================================
//aggregation (many PE inside)
// original (critcal path II = #EDGE)
//================================================
   
    //cout<<"hihi9"<<endl;
    aggregation_global_par_8_cl_init(
        edge_embedding,     //input-streaming message
        idx_des,
        node_des,
    
        node_des_agg_par_4, //output-streaming message
        edge_agg_par_4,
        idx_des_agg_par_4,
        
        num_of_edge,
        init
    );
    cout<<"aggregation_global_par_8_cl_init done"<<endl;
    //cout<<"hihi10"<<endl; 
   
    //inline!!
	node_block_par_4_not_join_init(
        node_des_agg_par_4, //input-streaming message
        edge_agg_par_4,
        idx_des_agg_par_4,
        
        node_fork_n,      //input-streaming message
        edge_fork_n,
        idx_fork_n,
   
        node_out1, //output-streaming message
		idx_des_out1,
	
		node_out2, //output-streaming message
		idx_des_out2,
        
        buf_w1,
        buf_b1,
        
        num_of_edge,
        num_of_des_node,
        num_of_src_node,
        init
    );	
    cout<<"node_block_par_4_not_join_init done"<<endl;
    //inline!!
    node_block_end_par_4_join_init(
		node_out1, //input-streaming message
		idx_des_out1,

		node_out2, //input-streaming message
		idx_des_out2,

		node_update_tmp1,
		node_update_tmp2,
		
		num_of_edge,
		num_of_des_node,
		num_of_src_node,
        init
	);
    cout<<"node_block_end_par_4_join_init done"<<endl;
}

void load_MPL1_ping_pong(

    //input
    const input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    const input_idx  *I_ch0_test,
    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    
    //parameter
    int num_of_edge
){
#pragma HLS DATAFLOW

    load_graph_edge_v2(
        
        E_ch0_test,
        //E_ch1_test,
        I_ch0_test,
        
        edge_in, 
        edge_in_idx,
        //edge_list_IN,
        num_of_edge
    );
    cout<<"load_graph_edge_v2 done"<<endl;

}
void pre_load_MPL1_ping_pong(

    //input
    const input_node *N_ch0_test,
    const input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    const input_idx  *I_ch0_test,
    const input_src  *S_ch0_test,
    
    
    #ifdef __SYNTHESIS__
    //output
    node_str          node_src_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_des_mp_L1[E_BLOCK_PAR/2][MAX_NODE],
    node_str          node_in_IN[NS_BLOCK_PAR/2][MAX_NODE],
    index_t           src_list[SRC_NODE],
    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    node_str          node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str          node_update_tmp2[N_BLOCK_PAR/2][MAX_NODE],
    
    #else
    node_str          (**node_src_mp_L1),
    node_str          (**node_des_mp_L1),
    // node_str          (*node_src_mp_L1)[MAX_NODE],
    // node_str          (*node_des_mp_L1)[MAX_NODE],
    node_str          (**node_in_IN),
    index_t           *src_list,

    
    hls::stream<edge>           edge_in[E_BLOCK_PAR],
    hls::stream<idx_pair>       edge_in_idx[E_BLOCK_PAR],
    
    node_str          (**node_update_tmp1),
	node_str          (**node_update_tmp2),
    // node_str          (*node_update_tmp1)[MAX_NODE],
	// node_str          (*node_update_tmp2)[MAX_NODE],
    
    #endif
    
    //parameter
    int num_of_node,
    int num_of_edge,
    int num_of_des_node,
    int num_of_src_node
    
){
#pragma HLS DATAFLOW

    load_graph_node_L1(
        
        N_ch0_test,        
        node_src_mp_L1, 
        node_des_mp_L1,
        node_in_IN,
        num_of_node
    );
    cout<<"load_graph_node_L1 done"<<endl;
    
    load_graph_edge_v2(
        
        E_ch0_test,
        //E_ch1_test,
        I_ch0_test,
        
        edge_in, 
        edge_in_idx,
        //edge_list_IN,
        num_of_edge
    );
    cout<<"load_graph_edge_v2 done"<<endl;
    
    load_graph_src(
        S_ch0_test,        
        src_list,
        num_of_src_node
    );
    cout<<"load_graph_src done"<<endl;
    reset_node_tmp(
        node_update_tmp1,
        node_update_tmp2,
        num_of_node
    );
    cout<<"reset_node_tmp done"<<endl;
}

void MPL2_loop(

	output_edge      *O3_ch0,
    input_edge0      *E_ch0_M1_out,
    //input_edge1      *E_ch1_M1_out,
    input_idx        *I_ch0_M1_out,
	
	#ifdef __SYNTHESIS__
	node_str	node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str    node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
	#else
	node_str	**node_update_tmp1,
    node_str 	**node_update_tmp2,
	#endif
	
	weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_node,
    int *num_of_edges,
    int num_of_graph,
	int num_of_src_node
){
#ifndef __SYNTHESIS__
    int processed_edges = 0;
#endif

	
	#ifdef __SYNTHESIS__
	node_str          node_src_mp_L2[E2_BLOCK_PAR/2][MAX_NODE];
    node_str          node_des_mp_L2[E2_BLOCK_PAR/2][MAX_NODE];

	#else
	node_str **node_src_mp_L2 = (node_str **)malloc(E2_BLOCK_PAR/2*sizeof(node_str*));
    node_str **node_des_mp_L2 = (node_str **)malloc(E2_BLOCK_PAR/2*sizeof(node_str*));
	for (int i = 0; i < E2_BLOCK_PAR/2; i++) {
		node_src_mp_L2[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
		node_des_mp_L2[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
	} 
	#endif

#pragma HLS BIND_STORAGE variable=node_src_mp_L2 type=RAM_T2P impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_des_mp_L2 type=RAM_T2P impl=URAM latency=3

#pragma HLS ARRAY_PARTITION variable=node_src_mp_L2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L2 dim=2 cyclic factor = MERGE_PAR
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L2 dim=2 cyclic factor = MERGE_PAR
	
		
    hls::stream<edge>           edge_in_ping[E_BLOCK_PAR];
	hls::stream<edge>           edge_in_pong[E_BLOCK_PAR];
	
    hls::stream<idx_pair>       edge_in_idx_ping[E_BLOCK_PAR];
	hls::stream<idx_pair>       edge_in_idx_pong[E_BLOCK_PAR];
	
    hls::stream<edge_res>   edge_embedding_L2_ping[E2_BLOCK_PAR];
    hls::stream<edge_res>   edge_embedding_L2_pong[E2_BLOCK_PAR];

#pragma HLS BIND_STORAGE variable=edge_in_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_pong type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_idx_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_idx_pong type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L2_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L2_pong type=fifo impl=BRAM

#ifdef MIN_FIFO_DEPTH   

#pragma HLS STREAM variable=edge_in_ping             depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4 
#pragma HLS STREAM variable=edge_in_pong             depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4 
#pragma HLS STREAM variable=edge_in_idx_ping         depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4
#pragma HLS STREAM variable=edge_in_idx_pong         depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4  
#pragma HLS STREAM variable=edge_embedding_L2_ping   depth=ceildiv(MAX_EDGE, E2_BLOCK_PAR) + 4   
#pragma HLS STREAM variable=edge_embedding_L2_pong   depth=ceildiv(MAX_EDGE, E2_BLOCK_PAR) + 4  

#else
    
#pragma HLS STREAM variable=edge_in_ping             depth=MAX_NODE
#pragma HLS STREAM variable=edge_in_pong             depth=MAX_NODE     
#pragma HLS STREAM variable=edge_in_idx_ping         depth=MAX_NODE
#pragma HLS STREAM variable=edge_in_idx_pong         depth=MAX_NODE         
#pragma HLS STREAM variable=edge_embedding_L2_ping   depth=MAX_EDGE
#pragma HLS STREAM variable=edge_embedding_L2_pong   depth=MAX_EDGE

#endif   

	
	//10
    pre_load_MPL2_ping_pong(
        &E_ch0_M1_out[0],
        // &E_ch1_M1_out[0],
        &I_ch0_M1_out[0],
        
        node_src_mp_L2,
        node_des_mp_L2,
        
        edge_in_ping,
        edge_in_idx_ping,
        node_update_tmp1,
        node_update_tmp2,
        
        num_of_node,
        num_of_edges[0],
		num_of_src_node
    );
	
    //11
	message_passing_L2_ping_pong_SLG(
    
        edge_embedding_L2_ping, //output-streaming message   
        edge_in_ping,
        edge_in_idx_ping, //!!!BRAM from preload
        
        node_src_mp_L2, //input BRAM
        node_des_mp_L2,

        //edgeblock2
        buf_w2,
        buf_b2,
            
        num_of_edges[0],
        num_of_node
    ); 
    
    //12
    load_MPL2_ping_pong(

        //input
        &E_ch0_M1_out[ceildiv(MAX_EDGE, E_BLOCK_PAR)],
        //&E_ch1_M1_out[ceildiv(MAX_EDGE, E_BLOCK_PAR)],
        &I_ch0_M1_out[ceildiv(MAX_EDGE, E_BLOCK_PAR)],
        
        //output
        edge_in_pong,
        edge_in_idx_pong,
        
        //parameter
        num_of_edges[1]
    );
    
    int edge_in_offset_L2 = 2*ceildiv(MAX_EDGE, E_BLOCK_PAR);
    int edge_out_offset_L2 = 0;
    
    for(int shard = 0; shard < num_of_graph-2; shard++){
        
        if(shard % 2 == 0){
        
            //13
            write_back_ping_pong(
                edge_embedding_L2_ping,
                &O3_ch0[edge_out_offset_L2],
                num_of_edges[shard]
            );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[shard];
#endif
            //14
            cout<<"shard "<<shard<<endl;
            message_passing_L2_ping_pong_SLG(
                edge_embedding_L2_pong, //output-streaming message   
                edge_in_pong,
                edge_in_idx_pong, //!!!BRAM from preload
                
                node_src_mp_L2, //input BRAM
                node_des_mp_L2,

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[shard+1],
                num_of_node
            ); 
            
            //15
            load_MPL2_ping_pong(
                //input
                &E_ch0_M1_out[edge_in_offset_L2],
                //&E_ch1_M1_out[edge_in_offset_L2],
                &I_ch0_M1_out[edge_in_offset_L2],
                
                //output
                edge_in_ping,
                edge_in_idx_ping,
                
                //parameter
                num_of_edges[shard+2]
            );
        }else{
        
            //13
            write_back_ping_pong(
                edge_embedding_L2_pong,
                &O3_ch0[edge_out_offset_L2],
                num_of_edges[shard]
            );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[shard];
#endif
            //14
            cout<<"shard "<<shard<<endl;
            message_passing_L2_ping_pong_SLG(
                edge_embedding_L2_ping, //output-streaming message   
                edge_in_ping,
                edge_in_idx_ping, //!!!BRAM from preload
                
                node_src_mp_L2, //input BRAM
                node_des_mp_L2,

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[shard+1],
                num_of_node
            ); 
            
            //15
            load_MPL2_ping_pong(
                //input
                &E_ch0_M1_out[edge_in_offset_L2],
                //&E_ch1_M1_out[edge_in_offset],
                &I_ch0_M1_out[edge_in_offset_L2],
                
                //output
                edge_in_pong,
                edge_in_idx_pong,
                
                //parameter
                num_of_edges[shard+2]
            );
        }
        
        edge_in_offset_L2 += ceildiv(MAX_EDGE, E_BLOCK_PAR);
        edge_out_offset_L2 += ceildiv(MAX_EDGE, E2_BLOCK_PAR);
    }
    
    if(num_of_graph % 2 ==0){
        //16
        write_back_ping_pong(
            edge_embedding_L2_ping,
            &O3_ch0[edge_out_offset_L2],
            num_of_edges[num_of_graph-2]
        );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[num_of_graph-2];
#endif
        
        edge_out_offset_L2 += ceildiv(MAX_EDGE, E2_BLOCK_PAR);
        
        //17
        message_passing_L2_ping_pong_SLG(
        
            edge_embedding_L2_pong, //output-streaming message   
            edge_in_pong,
            edge_in_idx_pong, //!!!BRAM from preload
            
            node_src_mp_L2, //input BRAM
            node_des_mp_L2,

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node
		);
        //18
        write_back_ping_pong(
            edge_embedding_L2_pong,
            &O3_ch0[edge_out_offset_L2],
            num_of_edges[num_of_graph-1]
        );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[num_of_graph-1];
#endif
    }else{
        //16
        write_back_ping_pong(
            edge_embedding_L2_pong,
            &O3_ch0[edge_out_offset_L2],
            num_of_edges[num_of_graph-2]
        );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[num_of_graph-2];
#endif
        
        edge_out_offset_L2 += ceildiv(MAX_EDGE, E2_BLOCK_PAR);
        
        //17
        message_passing_L2_ping_pong_SLG(
        
            edge_embedding_L2_ping, //output-streaming message   
            edge_in_ping,
            edge_in_idx_ping, //!!!BRAM from preload
            
            node_src_mp_L2, //input BRAM
            node_des_mp_L2,

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node
        ); 
        //18
        write_back_ping_pong(
            edge_embedding_L2_ping,
            &O3_ch0[edge_out_offset_L2],
            num_of_edges[num_of_graph-1]
        );
#ifndef __SYNTHESIS__
        processed_edges += num_of_edges[num_of_graph-1];
#endif
    }
#ifndef __SYNTHESIS__
    assert(processed_edges == TOTAL_EDGE_PER_GRAPH);
    assert(processed_edges == 1649533);
    assert(num_of_edges[num_of_graph - 1] == 49533);
    cout << "GRAPH_L_PROCESSED_EDGES=" << processed_edges
         << " LAST_SHARD_EDGES=" << num_of_edges[num_of_graph - 1] << endl;
#endif
}



void MPL1_loop(
    
    const input_node  *N_ch0_test,
    const input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    const input_idx   *I_ch0_test,
    const input_src   *S_ch0_test,

    input_edge0      *E_ch0_M1_out,
    input_idx        *I_ch0_M1_out,
    
    #ifdef __SYNTHESIS__
    node_str          node_update_tmp1[NS_BLOCK_PAR/2][MAX_NODE],
	node_str          node_update_tmp2[N_BLOCK_PAR /2][MAX_NODE],
    #else
    node_str **node_update_tmp1,
    node_str **node_update_tmp2,
	#endif
 
    weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_node,
    int *num_of_edges,
    int num_of_des_node,
    int num_of_src_node,
    int num_of_graph
   
){
	#ifdef __SYNTHESIS__
    node_str          node_src_mp_L1[E_BLOCK_PAR/2][MAX_NODE];
    node_str          node_des_mp_L1[E_BLOCK_PAR/2][MAX_NODE];
	node_str          node_in_IN[NS_BLOCK_PAR/2][MAX_NODE];
	
    index_t           edge_list_IN_ping[MAX_EDGE][2];
    index_t           edge_list_IN_pong[MAX_EDGE][2];
	
	index_t           src_list[SRC_NODE];

    #else
    node_str **node_src_mp_L1 = (node_str **)malloc(E_BLOCK_PAR/2*sizeof(node_str*));
    node_str **node_des_mp_L1 = (node_str **)malloc(E_BLOCK_PAR/2*sizeof(node_str*));
	for (int i = 0; i < E_BLOCK_PAR/2; i++) {
		node_src_mp_L1[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
		node_des_mp_L1[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
	}
	
	node_str **node_in_IN = (node_str **)malloc(NS_BLOCK_PAR/2*sizeof(node_str));
    for (int i = 0; i < NS_BLOCK_PAR/2; i++) {
        node_in_IN[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
    }
	
    index_t **edge_list_IN_ping = (index_t **)malloc(MAX_EDGE*sizeof(index_t*));
    for (int i = 0; i < MAX_EDGE; i++) {
        edge_list_IN_ping[i] = (index_t *)malloc(2*sizeof(index_t));
    }
    index_t **edge_list_IN_pong = (index_t **)malloc(MAX_EDGE*sizeof(index_t*));
    for (int i = 0; i < MAX_EDGE; i++) {
        edge_list_IN_pong[i] = (index_t *)malloc(2*sizeof(index_t));
    }
	
	index_t *src_list = (index_t *)malloc(SRC_NODE*sizeof(index_t));
    #endif

#pragma HLS BIND_STORAGE variable=node_src_mp_L1 type=RAM_2P impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_des_mp_L1 type=RAM_2P impl=URAM latency=3
//modify
#pragma HLS BIND_STORAGE variable=node_in_IN type=RAM_T2P impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=edge_list_IN_ping type=RAM_T2P impl=BRAM 
#pragma HLS BIND_STORAGE variable=edge_list_IN_pong type=RAM_T2P impl=BRAM

 
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 dim=2 cyclic factor = READ_N_PAR_PER_CH

#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 dim=2 cyclic factor = READ_N_PAR_PER_CH

#pragma HLS ARRAY_PARTITION variable=node_in_IN dim=1 complete
#pragma HLS ARRAY_PARTITION variable=node_in_IN dim=2 cyclic factor = READ_N_PAR_PER_CH //check!!!


#pragma HLS ARRAY_PARTITION variable=edge_list_IN_ping dim=1 cyclic factor = 8 // lcd(E, E2) //READ_N_PAR_PER_CH //check!!!
#pragma HLS ARRAY_PARTITION variable=edge_list_IN_ping dim=2 complete

#pragma HLS ARRAY_PARTITION variable=edge_list_IN_pong dim=1 cyclic factor = 8 // lcd(E, E2) //READ_N_PAR_PER_CH //check!!!
#pragma HLS ARRAY_PARTITION variable=edge_list_IN_pong dim=2 complete

#pragma HLS ARRAY_PARTITION variable=src_list dim=1 cyclic factor = READ_N_PAR_PER_CH//check!!!


 
    hls::stream<edge>           edge_in_ping[E_BLOCK_PAR];
    hls::stream<idx_pair>       edge_in_idx_ping[E_BLOCK_PAR];
    
    hls::stream<edge>       edge_embedding_L1_ping[E_BLOCK_PAR];

    
    hls::stream<edge>           edge_in_pong[E_BLOCK_PAR];
    hls::stream<idx_pair>       edge_in_idx_pong[E_BLOCK_PAR];
    
    hls::stream<edge>       edge_embedding_L1_pong[E_BLOCK_PAR];


#pragma HLS BIND_STORAGE variable=edge_in_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_pong type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_idx_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_idx_pong type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L1_ping type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L1_pong type=fifo impl=BRAM

#ifdef MIN_FIFO_DEPTH   

#pragma HLS STREAM variable=edge_in_ping             depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4 
#pragma HLS STREAM variable=edge_in_pong             depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4 
#pragma HLS STREAM variable=edge_in_idx_ping         depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4
#pragma HLS STREAM variable=edge_in_idx_pong         depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4
#pragma HLS STREAM variable=edge_embedding_L1_ping   depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4   
#pragma HLS STREAM variable=edge_embedding_L1_pong   depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4  

#else
    
#pragma HLS STREAM variable=edge_in_ping             depth=MAX_NODE    
#pragma HLS STREAM variable=edge_in_pong             depth=MAX_NODE 
#pragma HLS STREAM variable=edge_in_idx_ping         depth=MAX_NODE   
#pragma HLS STREAM variable=edge_in_idx_pong         depth=MAX_NODE
#pragma HLS STREAM variable=edge_embedding_L1_ping   depth=MAX_EDGE    
#pragma HLS STREAM variable=edge_embedding_L1_pong   depth=MAX_EDGE 

#endif   
 
	
//#pragma HLS inline off
    
    cout<<"hihi1"<<endl;
    int edge_in_offset_L1;
    int edge_out_offset_L1;

    //1
    pre_load_MPL1_ping_pong(

        //input
        &N_ch0_test[0],
        &E_ch0_test[0],
        //&E_ch1_test[0],
        &I_ch0_test[0],
        &S_ch0_test[0],
        
        //output
        node_src_mp_L1,
        node_des_mp_L1,
        
        node_in_IN,
        src_list,
        
        
        edge_in_ping,
        edge_in_idx_ping,
        
        node_update_tmp1,
        node_update_tmp2,
        
        
        //parameter
        num_of_node,
        num_of_edges[0],
        num_of_des_node,
        num_of_src_node
    );
    //2
	message_passing_L1_ping_pong_SLG(
        
		edge_list_IN_ping,
		
		node_update_tmp1, //output BRAM
        node_update_tmp2,

        edge_embedding_L1_ping, //output-streaming message
        
        edge_in_ping, //input-streaming message
        edge_in_idx_ping, //input-streaming message
        
        
        node_src_mp_L1,
        node_des_mp_L1,
        
        node_in_IN,
        src_list,
        
        //edgeblock
        buf_w0,
        buf_b0,

        //nodeblock
        buf_w1,
        buf_b1,

        num_of_edges[0],
        num_of_node,
        num_of_des_node,
        num_of_src_node,
        true
    );
    cout<<"end 2"<<endl;
    //cout<<ceildiv(MAX_EDGE, E_BLOCK_PAR)<<endl;
    //3
    load_MPL1_ping_pong(
 
        //input
        &E_ch0_test[ceildiv(MAX_EDGE, E_BLOCK_PAR)],
        //&E_ch1_test[0],
        &I_ch0_test[ceildiv(MAX_EDGE, E_BLOCK_PAR)],
        
        //output          
        edge_in_pong,
        edge_in_idx_pong,
        //parameter
        num_of_edges[1]
    );
    
    edge_in_offset_L1  = 2*ceildiv(MAX_EDGE, E_BLOCK_PAR);
    edge_out_offset_L1 = 0;
	int edge_in_offset_factor = ceildiv(MAX_EDGE, E_BLOCK_PAR);
	int edge_out_offset_factor = ceildiv(MAX_EDGE, E2_BLOCK_PAR);
    
    for(int shard = 0; shard < num_of_graph-2; shard++){
        

        if(shard % 2 == 0){

            //4
            store_MPL1_ping_pong(
                edge_embedding_L1_ping,
                &E_ch0_M1_out[edge_out_offset_L1],
                //&E_ch1_M1_out[edge_out_offset],
                &I_ch0_M1_out[edge_out_offset_L1],
                edge_list_IN_ping,
                num_of_edges[shard]
            );
           //5
            message_passing_L1_ping_pong_SLG(
                
                edge_list_IN_pong,
                
                node_update_tmp1, //output BRAM
                node_update_tmp2,

                edge_embedding_L1_pong, //output-streaming message
                
                edge_in_pong, //input-streaming message
                edge_in_idx_pong, //input-streaming message
                
                
                node_src_mp_L1,
                node_des_mp_L1,
                
                node_in_IN,
                src_list,
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[shard+1],
                num_of_node,
                num_of_des_node,
                num_of_src_node,
                false
            ); 
            //6
            load_MPL1_ping_pong(

                //input
                &E_ch0_test[edge_in_offset_L1],
                //&E_ch1_test[edge_in_offset_L1],
                &I_ch0_test[edge_in_offset_L1],
                
                //output          
                edge_in_ping,
                edge_in_idx_ping,
                //parameter
                num_of_edges[shard+2]
            );
            
        }else{
            
            //4
            store_MPL1_ping_pong(
                edge_embedding_L1_pong,
                &E_ch0_M1_out[edge_out_offset_L1],
                //&E_ch1_M1_out[edge_out_offset],
                &I_ch0_M1_out[edge_out_offset_L1],
                edge_list_IN_pong,
                num_of_edges[shard]
            );
            
            //5
            message_passing_L1_ping_pong_SLG(
                
                edge_list_IN_ping,
                
                node_update_tmp1, //output BRAM
                node_update_tmp2,

                edge_embedding_L1_ping, //output-streaming message
                
                edge_in_ping, //input-streaming message
                edge_in_idx_ping, //input-streaming message
                
                
                node_src_mp_L1,
                node_des_mp_L1,
                
                node_in_IN,
                src_list,
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[shard+1],
                num_of_node,
                num_of_des_node,
                num_of_src_node,
                false
            ); 
            
            //6
            load_MPL1_ping_pong(

                //input
                &E_ch0_test[edge_in_offset_L1],
                //&E_ch1_test[edge_in_offset_L1],
                &I_ch0_test[edge_in_offset_L1],
                
                //output          
                edge_in_pong,
                edge_in_idx_pong,
                //parameter
                num_of_edges[shard+2]
            );
           
           
        }
        edge_in_offset_L1 += edge_in_offset_factor;
        edge_out_offset_L1 += edge_out_offset_factor;   
    }

    
    if(num_of_graph % 2 == 0){
        
        //7
        store_MPL1_ping_pong(
            edge_embedding_L1_ping,
            &E_ch0_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            //&E_ch1_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            &I_ch0_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            edge_list_IN_ping,
            num_of_edges[num_of_graph-2]
        );
        
       // edge_out_offset_L1 += edge_out_offset_factor;
        
        //8
        message_passing_L1_ping_pong_SLG(
            
            edge_list_IN_pong,
            
            node_update_tmp1, //output BRAM
            node_update_tmp2,

            edge_embedding_L1_pong, //output-streaming message
            
            edge_in_pong, //input-streaming message
            edge_in_idx_pong, //input-streaming message
            
            
            node_src_mp_L1,
            node_des_mp_L1,
            
            node_in_IN,
            src_list,
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_node,
            false
        ); 
        
        //9
        store_MPL1_ping_pong(
            edge_embedding_L1_pong,
            &E_ch0_M1_out[(num_of_graph-1) * edge_out_offset_factor],
            //&E_ch1_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            &I_ch0_M1_out[(num_of_graph-1) * edge_out_offset_factor],
            edge_list_IN_pong,
            num_of_edges[num_of_graph-1]
        );
    }else{
        
        store_MPL1_ping_pong(
            edge_embedding_L1_pong,
            &E_ch0_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            //&E_ch1_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            &I_ch0_M1_out[(num_of_graph-2) * edge_out_offset_factor],
            edge_list_IN_pong,
            num_of_edges[num_of_graph-2]
        );
        
        //edge_out_offset_L1 += edge_out_offset_factor;
        
        //8
        message_passing_L1_ping_pong_SLG(
            
            edge_list_IN_ping,
            
            node_update_tmp1, //output BRAM
            node_update_tmp2,

            edge_embedding_L1_ping, //output-streaming message
            
            edge_in_ping, //input-streaming message
            edge_in_idx_ping, //input-streaming message
            
            
            node_src_mp_L1,
            node_des_mp_L1,
            
            node_in_IN,
            src_list,
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_node,
            false
        );
       
        store_MPL1_ping_pong(
            edge_embedding_L1_ping,
            &E_ch0_M1_out[(num_of_graph-1) * edge_out_offset_factor],
            //&E_ch1_M1_out[(num_of_graph-1) * edge_out_offset_factor],
            &I_ch0_M1_out[(num_of_graph-1) * edge_out_offset_factor],
            edge_list_IN_ping,
            num_of_edges[num_of_graph-1]
        );
    }
    
}
void interaction_network_pingpong_1G(

    const input_node  *N_ch0_test,
    const input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    const input_idx   *I_ch0_test,
    const input_src   *S_ch0_test,
    
    output_edge       *O3_ch0,
    
    input_edge0      *E_ch0_M1_ping,
    //input_edge1      *E_ch1_M1_ping,
    input_idx        *I_ch0_M1_ping,
	
	input_edge0      *E_ch0_M1_pong,
    //input_edge1     *E_ch1_M1_pong,
    input_idx        *I_ch0_M1_pong,
    
    
    
	weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    //edgeblock2
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_node,
    int *num_of_edges,
    int num_of_des_node,
    int *num_of_src_nodes,
    int num_of_shards,
    int num_graphs


){
    
    
    #ifdef __SYNTHESIS__
    node_str          node_update_tmp1[2][NS_BLOCK_PAR/2][MAX_NODE];
	node_str          node_update_tmp2[2][N_BLOCK_PAR /2][MAX_NODE];
    #else
    node_str ***node_update_tmp1 = (node_str ***)malloc(2*sizeof(node_str**));
    node_str ***node_update_tmp2 = (node_str ***)malloc(2*sizeof(node_str**));
 
    for (int i = 0; i < 2; i++) {
        node_update_tmp1[i] = (node_str **)malloc(NS_BLOCK_PAR/2*sizeof(node_str*));
        node_update_tmp2[i] = (node_str **)malloc(N_BLOCK_PAR /2*sizeof(node_str*));
        for (int j = 0; j < NS_BLOCK_PAR/2; j++) {
            node_update_tmp1[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
        }
		for (int j = 0; j < N_BLOCK_PAR/2; j++) {
            node_update_tmp2[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
        }
		
    }
    #endif
        
    
#pragma HLS BIND_STORAGE variable=node_update_tmp1 type=RAM_T2P impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_update_tmp2 type=RAM_T2P impl=URAM latency=3

#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 dim=3 cyclic factor = MERGE_PAR


#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 dim=3 cyclic factor = MERGE_PAR

	int edge_offset = ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv(MAX_EDGE, E_BLOCK_PAR);
	int node_offset = ceildiv(MAX_NODE, READ_N_PAR_PER_CH);
	int src_offset  = ceildiv(SRC_NODE, READ_N_PAR_PER_CH);
	int out_offset  = ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv(MAX_EDGE, E2_BLOCK_PAR);

    MPL1_loop(
		&N_ch0_test[0],
		&E_ch0_test[0],
		//&E_ch1_test,
		&I_ch0_test[0],
		&S_ch0_test[0],

		E_ch0_M1_ping,
		I_ch0_M1_ping,
		
		node_update_tmp1[0],
		node_update_tmp2[0],
		
		buf_w0,
		buf_b0,

		//nodeblock
		buf_w1,
		buf_b1,
		
		num_of_node,
		num_of_edges,
		num_of_des_node,
		num_of_src_nodes[0],
		num_of_shards
	);

//=============================================================//
    
	
	
	for(int g=0; g< num_graphs-1; g++){
		
		if(g%2 == 0){
			
			MPL2_loop(
				&O3_ch0[g*out_offset],
				E_ch0_M1_ping,
				//E_ch1_M1_ping,
				I_ch0_M1_ping,
				
				node_update_tmp1[0],
				node_update_tmp2[0],
				
				buf_w2,
				buf_b2,
				
				num_of_node,
				num_of_edges,
				num_of_shards,
				num_of_src_nodes[g]
			);
			
			 MPL1_loop(
				&N_ch0_test[(g+1)*node_offset],
				&E_ch0_test[(g+1)*edge_offset],
				//&E_ch1_test,
				&I_ch0_test[(g+1)*edge_offset],
				&S_ch0_test[(g+1)*src_offset],

				E_ch0_M1_pong,
				I_ch0_M1_pong,
				
				node_update_tmp1[1],
				node_update_tmp2[1],
				
				buf_w0,
				buf_b0,

				//nodeblock
				buf_w1,
				buf_b1,
				
				num_of_node,
				num_of_edges,
				num_of_des_node,
				num_of_src_nodes[g+1],
				num_of_shards
			);
		}else{
			MPL2_loop(
				&O3_ch0[g*out_offset],
				E_ch0_M1_pong,
				//E_ch1_M1_pong,
				I_ch0_M1_pong,
				
				node_update_tmp1[1],
				node_update_tmp2[1],
				
				buf_w2,
				buf_b2,
				
				num_of_node,
				num_of_edges,
				num_of_shards,
				num_of_src_nodes[g]
			);
			
			MPL1_loop(
				&N_ch0_test[(g+1)*node_offset],
				&E_ch0_test[(g+1)*edge_offset],
				//&E_ch1_test,
				&I_ch0_test[(g+1)*edge_offset],
				&S_ch0_test[(g+1)*src_offset],

				E_ch0_M1_ping,
				I_ch0_M1_ping,
				
				node_update_tmp1[0],
				node_update_tmp2[0],
				
				buf_w0,
				buf_b0,

				//nodeblock
				buf_w1,
				buf_b1,
				
				num_of_node,
				num_of_edges,
				num_of_des_node,
				num_of_src_nodes[g+1],
				num_of_shards
			);
		}
	}
	
	if(num_graphs % 2 == 0){
		MPL2_loop(
			&O3_ch0[(num_graphs-1)*out_offset],
			E_ch0_M1_pong,
			//&E_ch1_M1_pong,
			I_ch0_M1_pong,
			
			node_update_tmp1[1],
			node_update_tmp2[1],
			
			buf_w2,
			buf_b2,
			
			num_of_node,
			num_of_edges,
			num_of_shards,
			num_of_src_nodes[(num_graphs-1)]
		);
	}else{
		MPL2_loop(
			&O3_ch0[(num_graphs-1)*out_offset],
			E_ch0_M1_ping,
			//&E_ch1_M1_ping,
			I_ch0_M1_ping,
			
			node_update_tmp1[0],
			node_update_tmp2[0],
			
			buf_w2,
			buf_b2,
			
			num_of_node,
			num_of_edges,
			num_of_shards,
			num_of_src_nodes[(num_graphs-1)]
		);
	}
}


/*
void interaction_network_pingpong(

    input_node  *N_ch0_test,
    input_edge0 *E_ch0_test,
    //input_edge1 *E_ch1_test,
    input_idx   *I_ch0_test,
    input_src   *S_ch0_test,
    
    output_edge       *O3_ch0,
    
    input_edge0      *E_ch0_M1_out,
    //input_edge1      *E_ch1_M1_out[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    input_idx        *I_ch0_M1_out,
    
   
	weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM],

    //nodeblock
    weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM],

    //edgeblock2
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM],
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM],
    
    int num_of_node,
    int *num_of_edges,
    int num_of_des_node,
    int *num_of_src_nodes,
    int num_of_graph


){
   
    #ifdef __SYNTHESIS__
    node_str          node_update_tmp1[4][NS_BLOCK_PAR/2][MAX_NODE];
	node_str          node_update_tmp2[4][N_BLOCK_PAR /2][MAX_NODE];
    
    node_str          node_src_mp_L1[2][E_BLOCK_PAR/2][MAX_NODE];
    node_str          node_des_mp_L1[2][E_BLOCK_PAR/2][MAX_NODE];
  
    node_str          node_in_IN[2][MAX_NODE];
    index_t           edge_list_IN[2][MAX_EDGE][2];
    
    index_t           src_list[2][SRC_NODE];
    #else
    node_str ***node_update_tmp1 = (node_str ***)malloc(4*sizeof(node_str**));
    node_str ***node_update_tmp2 = (node_str ***)malloc(4*sizeof(node_str**));
    node_str ***node_src_mp_L1 = (node_str ***)malloc(2*sizeof(node_str**));
    node_str ***node_des_mp_L1 = (node_str ***)malloc(2*sizeof(node_str**));
    index_t  ***edge_list_IN = (index_t ***)malloc(2*sizeof(index_t**));
    
    for (int i = 0; i < 4; i++) {
        node_update_tmp1[i] = (node_str **)malloc(NS_BLOCK_PAR/2*sizeof(node_str*));
        node_update_tmp2[i] = (node_str **)malloc(N_BLOCK_PAR /2*sizeof(node_str*));
        for (int j = 0; j < NS_BLOCK_PAR/2; j++) {
            node_update_tmp1[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
        }
		for (int j = 0; j < N_BLOCK_PAR/2; j++) {
            node_update_tmp2[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
        }
		
    }
    for (int i = 0; i < 2; i++) {
        node_src_mp_L1[i] = (node_str **)malloc(E_BLOCK_PAR/2*sizeof(node_str*));
        node_des_mp_L1[i] = (node_str **)malloc(E_BLOCK_PAR/2*sizeof(node_str*));
        edge_list_IN[i] = (index_t **)malloc(MAX_EDGE*sizeof(index_t*));
        for (int j = 0; j < E_BLOCK_PAR/2; j++) {
            node_src_mp_L1[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
            node_des_mp_L1[i][j] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
        }
        for (int j = 0; j < MAX_EDGE; j++) {
            edge_list_IN[i][j] = (index_t *)malloc(2*sizeof(index_t));
        }
    }
    
    node_str **node_in_IN = (node_str **)malloc(2*sizeof(node_str*));
    for (int i = 0; i < 2; i++) {
        node_in_IN[i] = (node_str *)malloc(MAX_NODE*sizeof(node_str));
    }
    index_t **src_list = (index_t **)malloc(2*sizeof(index_t*));
    for (int i = 0; i < 2; i++) {
        src_list[i] = (index_t *)malloc(SRC_NODE*sizeof(index_t));
    }
    #endif
        
    
#pragma HLS BIND_STORAGE variable=node_update_tmp1 type=RAM_T2P impl=URAM
#pragma HLS BIND_STORAGE variable=node_update_tmp2 type=RAM_T2P impl=URAM
#pragma HLS BIND_STORAGE variable=node_src_mp_L1 type=RAM_2P impl=URAM
#pragma HLS BIND_STORAGE variable=node_des_mp_L1 type=RAM_2P impl=URAM
//modify
#pragma HLS BIND_STORAGE variable=node_in_IN type=RAM_T2P impl=URAM
#pragma HLS BIND_STORAGE variable=edge_list_IN type=RAM_T2P impl=BRAM


#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_update_tmp1 dim=3 cyclic factor = MERGE_PAR


#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_update_tmp2 dim=3 cyclic factor = MERGE_PAR


#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 dim=3 cyclic factor = READ_N_PAR_PER_CH


#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 dim=3 cyclic factor = READ_N_PAR_PER_CH


#pragma HLS ARRAY_PARTITION variable=node_in_IN dim=1 complete
#pragma HLS ARRAY_PARTITION variable=node_in_IN dim=2 cyclic factor = READ_N_PAR_PER_CH //check!!!


#pragma HLS ARRAY_PARTITION variable=edge_list_IN dim=1 complete
#pragma HLS ARRAY_PARTITION variable=edge_list_IN dim=2 cyclic factor = 24 // lcd(E, E2) //READ_N_PAR_PER_CH //check!!!
#pragma HLS ARRAY_PARTITION variable=edge_list_IN dim=3 complete

#pragma HLS ARRAY_PARTITION variable=src_list dim=1 complete
#pragma HLS ARRAY_PARTITION variable=src_list dim=2 cyclic factor = READ_N_PAR_PER_CH//check!!!

 
    hls::stream<edge>           edge_in[2][E_BLOCK_PAR];
    hls::stream<idx_pair>       edge_in_idx[2][E_BLOCK_PAR];
    
    hls::stream<edge>       edge_embedding_L1[2][E_BLOCK_PAR];
    hls::stream<edge_res>   edge_embedding_L2[2][E2_BLOCK_PAR];

#pragma HLS BIND_STORAGE variable=edge_in type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_in_idx type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L1 type=fifo impl=BRAM
#pragma HLS BIND_STORAGE variable=edge_embedding_L2 type=fifo impl=BRAM

#ifdef MIN_FIFO_DEPTH   

#pragma HLS STREAM variable=edge_in             depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4 
#pragma HLS STREAM variable=edge_in_idx         depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4
#pragma HLS STREAM variable=edge_embedding_L1   depth=ceildiv(MAX_EDGE, E_BLOCK_PAR) + 4   
#pragma HLS STREAM variable=edge_embedding_L2   depth=ceildiv(MAX_EDGE, E2_BLOCK_PAR) + 4   

#else
    
#pragma HLS STREAM variable=edge_in             depth=MAX_NODE    
#pragma HLS STREAM variable=edge_in_idx         depth=MAX_NODE    
#pragma HLS STREAM variable=edge_embedding_L1   depth=MAX_EDGE    
#pragma HLS STREAM variable=edge_embedding_L2   depth=MAX_EDGE

#endif   
   
    
    
    cout<<"hihi1"<<endl;
    //1
    pre_load_ping_pong(

        //input
        &N_ch0_test[0],
        &E_ch0_test[0],
        //&E_ch1_test[0],
        &I_ch0_test[0],
        &S_ch0_test[0],
        
        //output
        node_src_mp_L1[0],
        node_des_mp_L1[0],
        
        node_in_IN[0],
        src_list[0],
        
        
        edge_in[0],
        edge_in_idx[0],
        
        node_update_tmp1[0],
        node_update_tmp2[0],
        
        
        //parameter
        num_of_node,
        num_of_edges[0],
        num_of_des_node,
        num_of_src_nodes[0]
    );
	
    //2
	message_passing_L1_ping_pong(
        
		edge_list_IN[0],
		
		node_update_tmp1[0], //output BRAM
        node_update_tmp2[0],

        edge_embedding_L1[0], //output-streaming message
        
        edge_in[0], //input-streaming message
        edge_in_idx[0], //input-streaming message
        
        
        node_src_mp_L1[0],
        node_des_mp_L1[0],
        
        node_in_IN[0],
        src_list[0],
        
        //edgeblock
        buf_w0,
        buf_b0,

        //nodeblock
        buf_w1,
        buf_b1,

        num_of_edges[0],
        num_of_node,
        num_of_des_node,
        num_of_src_nodes[0]
    );
	//cout<<"hihi2"<<endl;
    //cout<<"IN pre_load_ping_pong 2 "<<endl;
	//cout<<N_ch0_test[1250+ 1].n_attr[0]<<endl;
	//3
	pre_load_ping_pong(

        //input
        &N_ch0_test[ceildiv(MAX_NODE,READ_N_PAR_PER_CH)],
        &E_ch0_test[ceildiv(MAX_EDGE,E_BLOCK_PAR)],
       // &E_ch1_test[ceildiv(MAX_EDGE,E_BLOCK_PAR)],
        &I_ch0_test[ceildiv(MAX_EDGE,E_BLOCK_PAR)],
        &S_ch0_test[ceildiv(SRC_NODE,READ_N_PAR_PER_CH)],
        
        //output
        node_src_mp_L1[1],
        node_des_mp_L1[1],
        node_in_IN[1],
        src_list[1],
        
        edge_in[1],
        edge_in_idx[1],
        
        node_update_tmp1[1],
        node_update_tmp2[1],
        
        
        //parameter
        num_of_node,
        num_of_edges[1],
        num_of_des_node,
        num_of_src_nodes[1]
    );
	cout<<"hihi2-1"<<endl;
    
    //cout<<num_of_src_nodes[0]<<endl;
    //cout<<num_of_edges[0]<<endl;
    
    //cout<<edge_embedding_L2[0]<<endl;
    //cout<<edge_embedding_L1[0]<<endl;
    //cout<<edge_list_IN[0]<<endl;
    //cout<<node_update_tmp1[0]<<endl;
    //cout<<node_update_tmp2[0]<<endl;
    //4
    message_passing_L2_ping_pong(
    
        edge_embedding_L2[0], //output-streaming message   
        edge_embedding_L1[0],
        edge_list_IN[0], //!!!BRAM from preload
        
        node_update_tmp1[0], //input BRAM
        node_update_tmp2[0],

        //edgeblock2
        buf_w2,
        buf_b2,
            
        num_of_edges[0],
        num_of_node,
        num_of_des_node,
		num_of_src_nodes[0]
    );
	cout<<"hihi2-2"<<endl;
	
	
	
	//5
	message_passing_L1_ping_pong(
        
		edge_list_IN[1],
		
		node_update_tmp1[1], //output BRAM
        node_update_tmp2[1],

        edge_embedding_L1[1], //output-streaming message
        
        edge_in[1], //input-streaming message
        edge_in_idx[1], //input-streaming message
        
        
        node_src_mp_L1[1],
        node_des_mp_L1[1],
        
        node_in_IN[1],
        src_list[1],
        
        //edgeblock
        buf_w0,
        buf_b0,

        //nodeblock
        buf_w1,
        buf_b1,

        num_of_edges[1],
        num_of_node,
        num_of_des_node,
        num_of_src_nodes[1]
    );
    cout<<"hihi3"<<endl;
	
    //6
	
	
	
    pre_load_ping_pong(

        //input
        &N_ch0_test[2 * ceildiv(MAX_NODE,READ_N_PAR_PER_CH)],
        &E_ch0_test[2 * ceildiv(MAX_EDGE,E_BLOCK_PAR)],
        //&E_ch1_test[2 * ceildiv(MAX_EDGE,E_BLOCK_PAR)],
        &I_ch0_test[2 * ceildiv(MAX_EDGE,E_BLOCK_PAR)],
        &S_ch0_test[2 * ceildiv(SRC_NODE,READ_N_PAR_PER_CH)],
        //output
        node_src_mp_L1[0],
        node_des_mp_L1[0],
        node_in_IN[0],
        src_list[0],
        
        
        edge_in[0],
        edge_in_idx[0],
        
        node_update_tmp1[2],
        node_update_tmp2[2],
        
        
        //parameter
        num_of_node,
        num_of_edges[2],
        num_of_des_node,
        num_of_src_nodes[2]
    );
    
    int edge_in_offset = 3*ceildiv(MAX_EDGE, E_BLOCK_PAR);
	int node_in_offset = 3*(ceildiv(MAX_NODE, READ_N_PAR_PER_CH));
	int src_in_offset  = 3*ceildiv(SRC_NODE, READ_N_PAR_PER_CH);
    //int edge_in_offset_2 = 4*(MAX_EDGE/E_BLOCK_PAR);
    
    
    int edge_out_offset = 0;
    //int edge_out_offset_2 = 1*(MAX_EDGE/E_BLOCK_PAR);
	
	
    for(int loop=3; loop<num_of_graph; loop++){
#pragma HLS loop_tripcount max = MAX_NUM_GRAPH -3 
//#pragma HLS loop_tripcount max = TOTAL_EDGE_PER_GRAPH / MAX_EDGE -2 

        //cout<<"loop"<<loop<<endl;
        cout<<edge_in_offset<<" "<<edge_out_offset<<endl;
        
        if(loop % 4 == 3){
			//cout<<"loop A"<<endl;
            //7
            write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[edge_out_offset],
            num_of_edges[loop-3]
            );
            
            //8
            message_passing_L2_ping_pong(
            
                edge_embedding_L2[1], //output-streaming message   
                edge_embedding_L1[1],
                edge_list_IN[1], //!!!BRAM from preload
                
                node_update_tmp1[1], //input BRAM
                node_update_tmp2[1],

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[loop-2],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-2]
            );
            
            //9
            message_passing_L1_ping_pong(
                
                edge_list_IN[0],
                
                node_update_tmp1[2], //output BRAM
                node_update_tmp2[2],

                edge_embedding_L1[0], //output-streaming message
                
                edge_in[0], //input-streaming message
                edge_in_idx[0], //input-streaming message
                
                
                node_src_mp_L1[0],
                node_des_mp_L1[0],
                
                node_in_IN[0],
                src_list[0],
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[loop-1],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-1]
            );
            
            //10
            pre_load_ping_pong(

                //input
                &N_ch0_test[node_in_offset],
                &E_ch0_test[edge_in_offset],
                //&E_ch1_test[edge_in_offset],
                &I_ch0_test[edge_in_offset],
                &S_ch0_test[src_in_offset],
                //output
                node_src_mp_L1[1],
                node_des_mp_L1[1],
                node_in_IN[1],
                src_list[1],
                
                
                edge_in[1],
                edge_in_idx[1],
                
                node_update_tmp1[3],
                node_update_tmp2[3],
                
                
                //parameter
                num_of_node,
                num_of_edges[loop],
                num_of_des_node,
                num_of_src_nodes[loop]
            );     
        }
        else if (loop % 4 == 0){
            cout<<"loop B"<<endl;
            //11
            write_back_ping_pong(
                edge_embedding_L2[1],
                &O3_ch0[edge_out_offset],
                num_of_edges[loop-3]
            );
            //12
            message_passing_L2_ping_pong(
            
                edge_embedding_L2[0], //output-streaming message   
                edge_embedding_L1[0],
                edge_list_IN[0], //!!!BRAM from preload
                
                node_update_tmp1[2], //input BRAM
                node_update_tmp2[2],

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[loop-2],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-2]
            );
            
            //13
            message_passing_L1_ping_pong(
                
                edge_list_IN[1],
                
                node_update_tmp1[3], //output BRAM
                node_update_tmp2[3],

                edge_embedding_L1[1], //output-streaming message
                
                edge_in[1], //input-streaming message
                edge_in_idx[1], //input-streaming message
                
                
                node_src_mp_L1[1],
                node_des_mp_L1[1],
                
                node_in_IN[1],
                src_list[1],
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[loop-1],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-1]
            );
            
            //14
            pre_load_ping_pong(

                //input
                &N_ch0_test[node_in_offset],
                &E_ch0_test[edge_in_offset],
                //&E_ch1_test[edge_in_offset],
                &I_ch0_test[edge_in_offset],
                &S_ch0_test[src_in_offset],
                //output
                node_src_mp_L1[0],
                node_des_mp_L1[0],
                node_in_IN[0],
                src_list[0],
                
                
                edge_in[0],
                edge_in_idx[0],
                
                node_update_tmp1[0],
                node_update_tmp2[0],
                
                
                //parameter
                num_of_node,
                num_of_edges[loop],
                num_of_des_node,
                num_of_src_nodes[loop]
            );
            
          
        }else if(loop % 4 == 1){
            
            cout<<"loop C"<<endl;
            //21
            write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[edge_out_offset],
            num_of_edges[loop-3]
            );
            
            //22
            message_passing_L2_ping_pong(
            
                edge_embedding_L2[1], //output-streaming message   
                edge_embedding_L1[1],
                edge_list_IN[1], //!!!BRAM from preload
                
                node_update_tmp1[3], //input BRAM
                node_update_tmp2[3],

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[loop-2],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-2]
            );
            
            //23
            message_passing_L1_ping_pong(
                
                edge_list_IN[0],
                
                node_update_tmp1[0], //output BRAM
                node_update_tmp2[0],

                edge_embedding_L1[0], //output-streaming message
                
                edge_in[0], //input-streaming message
                edge_in_idx[0], //input-streaming message
                
                
                node_src_mp_L1[0],
                node_des_mp_L1[0],
                
                node_in_IN[0],
                src_list[0],
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[loop-1],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-1]
            );
            
            //24
            pre_load_ping_pong(

                //input
                &N_ch0_test[node_in_offset],
                &E_ch0_test[edge_in_offset],
                //&E_ch1_test[edge_in_offset],
                &I_ch0_test[edge_in_offset],
                &S_ch0_test[src_in_offset],
                //output
                node_src_mp_L1[1],
                node_des_mp_L1[1],
                node_in_IN[1],
                src_list[1],
                
                
                edge_in[1],
                edge_in_idx[1],
                
                node_update_tmp1[1],
                node_update_tmp2[1],
                
                
                //parameter
                num_of_node,
                num_of_edges[loop],
                num_of_des_node,
                num_of_src_nodes[loop]
            );
         
        }
        else {
            cout<<"loop D"<<endl;
            //25
            write_back_ping_pong(
                edge_embedding_L2[1],
                &O3_ch0[edge_out_offset],
                num_of_edges[loop-3]
            );
            //26
            message_passing_L2_ping_pong(
            
                edge_embedding_L2[0], //output-streaming message   
                edge_embedding_L1[0],
                edge_list_IN[0], //!!!BRAM from preload
                
                node_update_tmp1[0], //input BRAM
                node_update_tmp2[0],

                //edgeblock2
                buf_w2,
                buf_b2,
                    
                num_of_edges[loop-2],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-2]
            );
            
            //27
            message_passing_L1_ping_pong(
                
                edge_list_IN[1],
                
                node_update_tmp1[1], //output BRAM
                node_update_tmp2[1],

                edge_embedding_L1[1], //output-streaming message
                
                edge_in[1], //input-streaming message
                edge_in_idx[1], //input-streaming message
                
                
                node_src_mp_L1[1],
                node_des_mp_L1[1],
                
                node_in_IN[1],
                src_list[1],
                
                //edgeblock
                buf_w0,
                buf_b0,

                //nodeblock
                buf_w1,
                buf_b1,

                num_of_edges[loop-1],
                num_of_node,
                num_of_des_node,
                num_of_src_nodes[loop-1]
            );
            
            //28
            pre_load_ping_pong(

                //input
                &N_ch0_test[node_in_offset],
                &E_ch0_test[edge_in_offset],
                //&E_ch1_test[edge_in_offset],
                &I_ch0_test[edge_in_offset],
                &S_ch0_test[src_in_offset],
                //output
                node_src_mp_L1[0],
                node_des_mp_L1[0],
                node_in_IN[0],
                src_list[0],
                
                edge_in[0],
                edge_in_idx[0],
                
                node_update_tmp1[2],
                node_update_tmp2[2],
                
                
                //parameter
                num_of_node,
                num_of_edges[loop],
                num_of_des_node,
                num_of_src_nodes[loop]
            );
        }
        edge_in_offset += ceildiv(MAX_EDGE, E_BLOCK_PAR);
        edge_out_offset += ceildiv(MAX_EDGE, E2_BLOCK_PAR);
		node_in_offset += ceildiv(MAX_NODE, READ_N_PAR_PER_CH);
		src_in_offset  += ceildiv(SRC_NODE, READ_N_PAR_PER_CH);
    }
    
	
    if(num_of_graph % 4 == 2){
        
        //15
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-3)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-3]
        );
        //16
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[0], //output-streaming message   
            edge_embedding_L1[0],
            edge_list_IN[0], //!!!BRAM from preload
            
            node_update_tmp1[0], //input BRAM
            node_update_tmp2[0],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-2],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-2]
        );
        
        //17
        message_passing_L1_ping_pong(
            
            edge_list_IN[1],
            
            node_update_tmp1[1], //output BRAM
            node_update_tmp2[1],

            edge_embedding_L1[1], //output-streaming message
            
            edge_in[1], //input-streaming message
            edge_in_idx[1], //input-streaming message
            
            
            node_src_mp_L1[1],
            node_des_mp_L1[1],
            
            node_in_IN[1],
            src_list[1],
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        );
        //18
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-2)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-2]
        );
        //19
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[1], //output-streaming message   
            edge_embedding_L1[1],
            edge_list_IN[1], //!!!BRAM from preload
            
            node_update_tmp1[1], //input BRAM
            node_update_tmp2[1],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        ); 
        //20
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-1)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-1]
        );
    }else if (num_of_graph % 4 == 3){
        //15
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-3)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-3]
        );
        
        //16
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[1], //output-streaming message   
            edge_embedding_L1[1],
            edge_list_IN[1], //!!!BRAM from preload
            
            node_update_tmp1[1], //input BRAM
            node_update_tmp2[1],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-2],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-2]
        );
        
        //17
        message_passing_L1_ping_pong(
            
            edge_list_IN[0],
            
            node_update_tmp1[2], //output BRAM
            node_update_tmp2[2],

            edge_embedding_L1[0], //output-streaming message
            
            edge_in[0], //input-streaming message
            edge_in_idx[0], //input-streaming message
            
            
            node_src_mp_L1[0],
            node_des_mp_L1[0],
            
            node_in_IN[0],
            src_list[0],
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        );
        //18
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-2)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-2]
        );
        //19
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[0], //output-streaming message   
            edge_embedding_L1[0],
            edge_list_IN[0], //!!!BRAM from preload
            
            node_update_tmp1[2], //input BRAM
            node_update_tmp2[2],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        ); 
        //20
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-1)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-1]
        );
    }else if(num_of_graph % 4 == 0){
        
        //15
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-3)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-3]
        );
        //16
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[0], //output-streaming message   
            edge_embedding_L1[0],
            edge_list_IN[0], //!!!BRAM from preload
            
            node_update_tmp1[2], //input BRAM
            node_update_tmp2[2],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-2],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-2]
        );
        
        //17
        message_passing_L1_ping_pong(
            
            edge_list_IN[1],
            
            node_update_tmp1[3], //output BRAM
            node_update_tmp2[3],

            edge_embedding_L1[1], //output-streaming message
            
            edge_in[1], //input-streaming message
            edge_in_idx[1], //input-streaming message
            
            
            node_src_mp_L1[1],
            node_des_mp_L1[1],
            
            node_in_IN[1],
            src_list[1],
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        );
        //18
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-2)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-2]
        );
        //19
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[1], //output-streaming message   
            edge_embedding_L1[1],
            edge_list_IN[1], //!!!BRAM from preload
            
            node_update_tmp1[3], //input BRAM
            node_update_tmp2[3],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        ); 
        //20
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-1)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-1]
        );
    }else if (num_of_graph % 4 == 1){
        //15
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-3)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-3]
        );
        
        //16
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[1], //output-streaming message   
            edge_embedding_L1[1],
            edge_list_IN[1], //!!!BRAM from preload
            
            node_update_tmp1[3], //input BRAM
            node_update_tmp2[3],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-2],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-2]
        );
        
        //17
        message_passing_L1_ping_pong(
            
            edge_list_IN[0],
            
            node_update_tmp1[0], //output BRAM
            node_update_tmp2[0],

            edge_embedding_L1[0], //output-streaming message
            
            edge_in[0], //input-streaming message
            edge_in_idx[0], //input-streaming message
            
            
            node_src_mp_L1[0],
            node_des_mp_L1[0],
            
            node_in_IN[0],
            src_list[0],
            
            //edgeblock
            buf_w0,
            buf_b0,

            //nodeblock
            buf_w1,
            buf_b1,

            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        );
        //18
        write_back_ping_pong(
            edge_embedding_L2[1],
            &O3_ch0[(num_of_graph-2)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-2]
        );
        //19
        message_passing_L2_ping_pong(
        
            edge_embedding_L2[0], //output-streaming message   
            edge_embedding_L1[0],
            edge_list_IN[0], //!!!BRAM from preload
            
            node_update_tmp1[0], //input BRAM
            node_update_tmp2[0],

            //edgeblock2
            buf_w2,
            buf_b2,
                
            num_of_edges[num_of_graph-1],
            num_of_node,
            num_of_des_node,
            num_of_src_nodes[num_of_graph-1]
        ); 
        //20
        write_back_ping_pong(
            edge_embedding_L2[0],
            &O3_ch0[(num_of_graph-1)*ceildiv(MAX_EDGE, E2_BLOCK_PAR)],
            num_of_edges[num_of_graph-1]
        );
    }
	
	
}
*/


extern "C"{
void su_top_model(
    
  
    const input_node       N_ch0_test[MAX_NUM_GRAPH * ceildiv_even(MAX_NODE, READ_N_PAR_PER_CH)],
    const input_edge0      E_ch0_test[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    //input_edge1      E_ch1_test[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    const input_idx        I_ch0_test[MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    const input_src        S_ch0_test[MAX_NUM_GRAPH * ceildiv_even(SRC_NODE, READ_N_PAR_PER_CH)],
    
    input_edge0      E_ch0_M1_ping[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    //input_edge1      E_ch1_M1_out[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    input_idx        I_ch0_M1_ping[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    input_edge0      E_ch0_M1_pong[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    //input_edge1      E_ch1_M1_in[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    input_idx        I_ch0_M1_pong[ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)],
    
	
	
    input_weight       W1_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W1_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W1_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
    input_weight       W2_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W2_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W2_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
    input_weight       W3_ch0[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W3_ch1[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    input_weight       W3_ch2[(MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1],
    
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
//int part
    //int* num_of_nodes,
    //int* num_of_edges,
    //int* num_of_des_nodes,
    int  num_graphs,
	ap_uint<16> src_num_sum[MAX_NUM_GRAPH]
//======================================


){

//#pragma HLS INTERFACE m_axi port = N_ch0  offset = slave bundle = hbm0  max_read_burst_length=256 num_read_outstanding=16
//#pragma HLS INTERFACE m_axi port = E_ch0  offset = slave bundle = hbm1  max_read_burst_length=256 num_read_outstanding=16
//#pragma HLS INTERFACE m_axi port = I_ch0  offset = slave bundle = hbm2  max_read_burst_length=256 num_read_outstanding=16

#pragma HLS INTERFACE m_axi port = N_ch0_test  bundle = hbm0   max_widen_bitwidth=1024 max_read_burst_length=256 num_read_outstanding=16 depth=ceildiv_even(MAX_NODE, READ_N_PAR_PER_CH)  latency=64
#pragma HLS INTERFACE m_axi port = E_ch0_test  bundle = hbm1   max_widen_bitwidth=1024 max_read_burst_length=256 num_read_outstanding=16 depth=MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR)  latency=64
#pragma HLS INTERFACE m_axi port = I_ch0_test  bundle = hbm15  max_widen_bitwidth=1024 max_read_burst_length=256 num_read_outstanding=16 depth=MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR) latency=64
#pragma HLS INTERFACE m_axi port = S_ch0_test  bundle = hbm16  max_widen_bitwidth=1024 max_read_burst_length=256 num_read_outstanding=16 depth=MAX_NUM_GRAPH * ceildiv_even(SRC_NODE, READ_N_PAR_PER_CH)  latency=64

#pragma HLS INTERFACE m_axi port = E_ch0_M1_ping  bundle = hbm13  max_widen_bitwidth=1024 max_write_burst_length=256 max_read_burst_length=256 num_read_outstanding=16 num_write_outstanding=16 depth=ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR) latency=64
#pragma HLS INTERFACE m_axi port = I_ch0_M1_ping  bundle = hbm14  max_widen_bitwidth=1024 max_write_burst_length=256 max_read_burst_length=256 num_read_outstanding=16 num_write_outstanding=16 depth=ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR) latency=64
#pragma HLS INTERFACE m_axi port = E_ch0_M1_pong  bundle = hbm17  max_widen_bitwidth=1024 max_write_burst_length=256 max_read_burst_length=256 num_read_outstanding=16 num_write_outstanding=16 depth=ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR) latency=64
#pragma HLS INTERFACE m_axi port = I_ch0_M1_pong  bundle = hbm18  max_widen_bitwidth=1024 max_write_burst_length=256 max_read_burst_length=256 num_read_outstanding=16 num_write_outstanding=16 depth=ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E_BLOCK_PAR) latency=64


#pragma HLS INTERFACE m_axi port = W1_ch0 bundle = hbm3  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W1_ch1 bundle = hbm4  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W1_ch2 bundle = hbm5  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W2_ch0 bundle = hbm6  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W2_ch1 bundle = hbm7  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W2_ch2 bundle = hbm8  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W3_ch0 bundle = hbm9  max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W3_ch1 bundle = hbm10 max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64
#pragma HLS INTERFACE m_axi port = W3_ch2 bundle = hbm11 max_read_burst_length=256 max_widen_bitwidth=512 depth=((MAX_WEIGHT_DIM + MAX_BIAS_DIM) / READ_W_PAR_PER_CH + 1) latency=64

#pragma HLS INTERFACE m_axi port = O3_ch0 bundle = hbm12  max_widen_bitwidth=1024 max_write_burst_length=256 depth=MAX_NUM_GRAPH * ceildiv(TOTAL_EDGE_PER_GRAPH,  MAX_EDGE) * ceildiv_even(MAX_EDGE, E2_BLOCK_PAR)  latency=64

/*
#pragma HLS INTERFACE m_axi depth = 1 port = num_of_nodes     offset = slave bundle = control
#pragma HLS INTERFACE m_axi depth = 1 port = num_of_edges     offset = slave bundle = control
#pragma HLS INTERFACE m_axi depth = 1 port = num_of_des_nodes offset = slave bundle = control

*/
#pragma HLS INTERFACE s_axilite         port = num_graphs                      bundle = control 
#pragma HLS INTERFACE s_axilite         port = return                          bundle = control
#pragma HLS INTERFACE s_axilite         port = src_num_sum                     bundle = control
    
    
	weight_t buf_w0[MLP_LAYER][MAX_WEIGHT_DIM];
    weight_t buf_b0[MLP_LAYER][MAX_BIAS_DIM];
#pragma HLS array_partition variable=buf_w0 dim=1 complete
#pragma HLS array_partition variable=buf_w0 dim=2 complete

#pragma HLS array_partition variable=buf_b0 dim=1 complete
#pragma HLS array_partition variable=buf_b0 dim=2 complete
//#pragma HLS array_partition variable=buf_w0 dim=3 cyclic factor = READ_W_PAR_PER_CH


//#pragma HLS array_partition variable=buf_b0 dim=3 cyclic factor = READ_W_PAR_PER_CH

    //nodeblock
    weight_t buf_w1[MLP_LAYER][MAX_WEIGHT_DIM];
    weight_t buf_b1[MLP_LAYER][MAX_BIAS_DIM];
	
#pragma HLS array_partition variable=buf_w1 dim=1 complete
#pragma HLS array_partition variable=buf_w1 dim=2 complete

#pragma HLS array_partition variable=buf_b1 dim=1 complete
#pragma HLS array_partition variable=buf_b1 dim=2 complete
//#pragma HLS array_partition variable=buf_w1 dim=3 cyclic factor = READ_W_PAR_PER_CH

//#pragma HLS array_partition variable=buf_b1 dim=3 cyclic factor = READ_W_PAR_PER_CH

    //edgeblock2
    weight_t buf_w2[MLP_LAYER][MAX_WEIGHT_DIM];
    weight_t buf_b2[MLP_LAYER][MAX_BIAS_DIM];
#pragma HLS array_partition variable=buf_w2 dim=1 complete
#pragma HLS array_partition variable=buf_w2 dim=2 complete

#pragma HLS array_partition variable=buf_b2 dim=1 complete
#pragma HLS array_partition variable=buf_b2 dim=2 complete   

    
    load_weight_v3(
        W1_ch0,
        W1_ch1,
        W1_ch2,
        
        W2_ch0,
        W2_ch1,
        W2_ch2,
        
        W3_ch0,
        W3_ch1,
        W3_ch2,
        buf_w0,
        buf_b0,
        buf_w1,
        buf_b1,
        buf_w2,
        buf_b2
    );
    
    /*
	for(int g=0;g<3;g++){
		cout<<"load in kernel"<<g<<"===="<<endl;
	
		for(int i=0; i<10; i++){
			cout<<"NT["<<g<<"]["<<i<<"]"<<N_ch0_test[g*(MAX_NODE/READ_N_PAR_PER_CH + 1) + i].n_attr[0]<<" "<<N_ch0_test[g*(MAX_NODE/READ_N_PAR_PER_CH + 1) + i].n_attr[1]<<endl;
		}
	}
    */
    /*
    cout<<"print_WEIGHT W0============"<<endl;
    for(int i = 0; i< 4; i++){ 
        int item_int = buf_b0[0][0][i] * 256;
        float item_f = item_int / pow(2, 12);   
        cout<< item_f <<" ";
    }
    cout<<endl;
    
    cout<<"print_WEIGHT W1============"<<endl;
    for(int i = 0; i< 4; i++){ 
        int item_int = buf_b1[0][0][i] * 256;
        float item_f = item_int / pow(2, 12);   
        cout<< item_f <<" ";
    }
    cout<<endl;
    
    cout<<"print_WEIGHT W2============"<<endl;
    for(int i = 0; i< 4; i++){ 
        int item_int = buf_b2[0][0][i] * 256;
        float item_f = item_int / pow(2, 12);   
        cout<< item_f <<" ";
    }
    cout<<endl;
    */
    
    cout <<"hihi2"<<endl;
    
	

    int num_of_edges[MAX_NUM_SHARD];
    int num_of_src_nodes[MAX_NUM_GRAPH];
#pragma HLS array_partition variable=num_of_edges dim=1 complete

    int num_of_node = MAX_NODE;
    int num_of_des_node = MAX_NODE - SRC_NODE;
    int loop_per_graph = MAX_NUM_SHARD;
    int scheduled_edges = 0;
    for (int i = 0; i < MAX_NUM_SHARD; ++i) {
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_NUM_SHARD
        const int remaining = TOTAL_EDGE_PER_GRAPH - i * MAX_EDGE;
        num_of_edges[i] = remaining < MAX_EDGE ? remaining : MAX_EDGE;
        scheduled_edges += num_of_edges[i];
    }
    for (int g = 0; g < MAX_NUM_GRAPH; ++g) {
#pragma HLS PIPELINE II=1
#pragma HLS loop_tripcount max = MAX_NUM_GRAPH
        num_of_src_nodes[g] = g < num_graphs ? int(src_num_sum[g]) : 0;
    }
#ifndef __SYNTHESIS__
    assert(num_graphs >= 1 && num_graphs <= MAX_NUM_GRAPH);
    assert(scheduled_edges == 1649533);
    assert(num_of_edges[MAX_NUM_SHARD - 1] == 49533);
#endif

    
	/*
	cout <<"hihi3aaa"<<endl;
    
     
	cout <<"hihi4aaa"<<endl;
	for(int g=0;g<3;g++){
		cout<<"load in IN"<<g<<"===="<<endl;
	
		for(int i=0; i<10; i++){
			cout<<"NT["<<g<<"]["<<i<<"]"<<N_ch0_test[g*(MAX_NODE/READ_N_PAR_PER_CH + 1) + i].n_attr[0]<<" "<<N_ch0_test[g*(MAX_NODE/READ_N_PAR_PER_CH + 1) + i].n_attr[1]<<endl;
		}
        for(int i=0; i<10; i++){
			cout<<"ET["<<g<<"]["<<i<<"]"<<E_ch0_test[g*ceildiv(TOTAL_EDGE_PER_GRAPH, MAX_EDGE) + i].e_attr[0]<<" "<<E_ch0_test[g*ceildiv(TOTAL_EDGE_PER_GRAPH, MAX_EDGE) + i].e_attr[1]<<endl;
		}
        for(int i=0; i<10; i++){
			cout<<"ET["<<g<<"]["<<i<<"]"<<E_ch1_test[g*ceildiv(TOTAL_EDGE_PER_GRAPH, MAX_EDGE) + i].e_attr[0]<<" "<<E_ch1_test[g*ceildiv(TOTAL_EDGE_PER_GRAPH, MAX_EDGE) + i].e_attr[1]<<endl;
		}
	}
	cout<<N_ch0_test[1250+1].n_attr[0]<<endl;
    */
	
    
	interaction_network_pingpong_1G(
		
		N_ch0_test,
		E_ch0_test,
        //E_ch1_test,
		I_ch0_test,
        S_ch0_test,
		O3_ch0,
        
        E_ch0_M1_ping,
        //E_ch1_M1_ping,
		I_ch0_M1_ping,
		
		E_ch0_M1_pong,
        //E_ch1_M1_pong,
		I_ch0_M1_pong,

		buf_w0,
		buf_b0,
		buf_w1,
		buf_b1,
		buf_w2,
		buf_b2,
		
		num_of_node,
		num_of_edges,
		num_of_des_node,
		num_of_src_nodes,
        loop_per_graph,
        num_graphs
	);
    
    
    /*
    interaction_network_pingpong_1G_test(
		
		N_ch0_test,
		E_ch0_test,
        E_ch1_test,
		I_ch0_test,
        S_ch0_test,
		O3_ch0,

		buf_w0,
		buf_b0,
		buf_w1,
		buf_b1,
		buf_w2,
		buf_b2,
		
		num_of_node,
		num_of_edges,
		num_of_des_node,
		num_of_src_nodes,
        loop_per_graph
	);
    */
    
	
    cout<<"hihi_end"<<endl;
    
}
}
