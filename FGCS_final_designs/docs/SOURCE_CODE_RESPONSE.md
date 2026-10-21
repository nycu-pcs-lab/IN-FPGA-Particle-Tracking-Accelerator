# Source code request response — final public-tree candidates

This document answers SOURCE_CODE_REQUEST.md using only paths inside this package.
Core files are byte-identical to the final D20/D24 hardware-cost sources and match
the corresponding final run source hashes. See ../provenance/final_design_provenance.json.
No verified Git commit/tag/DOI exists. Local source mtimes and SHA-256 are recorded.
The 8.8 ns result is identified by source/export/run records, not by C++ text alone.
Original HLS target is 5 ns; final operating clocks are 8 ns (M), 8.8 ns (L).

## Candidate selection and answers

1. Carry: use L lines 1114–1123 (static/reset), 1299–1316 (carry matching),
   1470–1472 (carry commit) as separately labeled excerpts, with the merge network
   omitted explicitly. Full function is 1100–1512; old 1114 reference is the first
   static declaration, not the beginning of the function. No code is rewritten here.
2. The first shard call is 6951–6983, init=true. Subsequent branches are all
   init=false and are listed below. MPL1_loop's num_of_graph parameter means shards;
   the top's num_graphs means graphs. Each graph initializes the carry once.
3. keep_n_pre is NOT reset in init=true. Initial keep_idx_pre is reserved dummy
   MAX_NODE-1 (43,277); keep_e_pre is zero. Initial/old dummy payload is not assumed
   zero or useful. NodeBlock does not discard dummy IDs: node_block_end_par_4_join_init
   writes them at line318. Valid edges must never address the reserved dummy node.
4. The last unfinished destination survives each shard return via static carry.
   Sorted destination runs are mandatory. Terminal dummy edges cause the final valid
   group to be emitted; there is no independent general unpadded end-of-graph flush.
   A cut within one destination continues accumulation, rather than treating shards
   as independent graphs. The final 49,533-slot shard uses five lanes in its final word.
5. Graph_M instead calls aggregation_global_par_8_cl at 3679–3689 (definition 950–1336).
   Its local carry is initialized on each function invocation; it is not L's static
   cross-shard aggregator. Do not generalize the L carry listing to M.
6. Both EdgeBlock1 functions are named edge_block. Their actual read is
   pe % (E_BLOCK_PAR/2), not pe/2: lanes 0/4, 1/5, 2/6, 3/7 share replicas 0,1,2,3.
   This matches the quoted paper formula. L tables are [4][43278]; M tables are
   [2][4][9925], with an additional graph ping-pong dimension.
7. RAM_2P URAM binding gives each bank two ports; complete partition separates
   replicas and cyclic node partition factors are L=10, M=2. These banking factors
   are not extra full-table copies. Each node_str packs three 16-bit features.
8. The samples are reconstructed from existing destination-sorted archived inputs;
   this package statically checks binary endpoint ordering and padding. The original
   raw TrackML graph-construction/sorting exporter has not been recovered. Packaging
   does not claim to recreate it. Shard splitting is explicit in the portable L TB.
9. Preprocessing checks in the source audit passed for all 40 archived mappings;
   422 of 640 L shard cuts split one valid destination run. This package includes
   only one M sample and one L sample, not the full dataset/audit archive.

The only recommended paper listings are carry behavior and the actual replica read.
PIPELINE, DATAFLOW and storage pragmas alone are standard HLS mechanisms, not the
claimed contribution. A shortened listing must disclose omitted ranges; the full
function below is verbatim, including original comments and indentation.

## Code availability unknowns

GitHub URL, approved project license, weight/sample redistribution permission,
Zenodo DOI, public release date and fixed Git tag are UNKNOWN or pending. No release
is published here. Local paths are package-relative; no private workstation paths
or host loader files are required. No paper/Item12/Item32 status is changed.

## Exact source excerpts and line numbers

### L carry full

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **1100–1512**.

```cpp
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
```

### L first shard

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **6951–6983**.

```cpp
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
```

### L later pong

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **7021–7053**.

```cpp
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
```

### L later ping

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **7082–7114**.

```cpp
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
```

### L final pong

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **7153–7185**.

```cpp
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
```

### L final ping

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **7210–7242**.

```cpp
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
```

### L direct aggregator call

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **6275–6286**.

```cpp
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
```

### L replicas

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **6810–6818**.

```cpp
	#ifdef __SYNTHESIS__
    node_str          node_src_mp_L1[E_BLOCK_PAR/2][MAX_NODE];
    node_str          node_des_mp_L1[E_BLOCK_PAR/2][MAX_NODE];
	node_str          node_in_IN[NS_BLOCK_PAR/2][MAX_NODE];
	
    index_t           edge_list_IN_ping[MAX_EDGE][2];
    index_t           edge_list_IN_pong[MAX_EDGE][2];
	
	index_t           src_list[SRC_NODE];
```

### L storage pragmas

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **6845–6857**.

```cpp
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
```

### L modulo read

Source: [designs/Graph_L/src/su_top_model.cpp](../designs/Graph_L/src/su_top_model.cpp), lines **4658–4677**.

```cpp
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
```

### M replicas

Source: [designs/Graph_M/src/su_top_model.cpp](../designs/Graph_M/src/su_top_model.cpp), lines **3895–3905**.

```cpp
    #ifdef __SYNTHESIS__
    node_str          node_update_tmp1[4][NS_BLOCK_PAR/2][MAX_NODE];
	node_str          node_update_tmp2[4][N_BLOCK_PAR/2][MAX_NODE];
    
    node_str          node_src_mp_L1[2][E_BLOCK_PAR/2][MAX_NODE];
    node_str          node_des_mp_L1[2][E_BLOCK_PAR/2][MAX_NODE];
  
    node_str          node_in_IN[2][MAX_NODE];
    edge_list_str     edge_list_IN[2][MAX_EDGE];
    
    index_t           src_list[2][SRC_NODE];
```

### M storage pragmas

Source: [designs/Graph_M/src/su_top_model.cpp](../designs/Graph_M/src/su_top_model.cpp), lines **3946–3951**.

```cpp
#pragma HLS BIND_STORAGE variable=node_update_tmp1  type=RAM_T2P    impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_update_tmp2  type=RAM_T2P    impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_src_mp_L1    type=RAM_2P     impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_des_mp_L1    type=RAM_2P     impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=node_in_IN        type=RAM_2P     impl=URAM latency=3
#pragma HLS BIND_STORAGE variable=edge_list_IN      type=RAM_2P     impl=URAM latency=3
```

### M partition pragmas

Source: [designs/Graph_M/src/su_top_model.cpp](../designs/Graph_M/src/su_top_model.cpp), lines **3964–3971**.

```cpp
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 complete dim=1   
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_src_mp_L1 dim=3 cyclic factor = READ_N_PAR_PER_CH


#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=node_des_mp_L1 dim=3 cyclic factor = READ_N_PAR_PER_CH
```

### M modulo read

Source: [designs/Graph_M/src/su_top_model.cpp](../designs/Graph_M/src/su_top_model.cpp), lines **2792–2804**.

```cpp
                s = uint16_to_DTYPE<index_t>(list_tmp(15,0));
                d = uint16_to_DTYPE<index_t>(list_tmp(31,16));
                
                node_str src_tmp;
                node_str dst_tmp;
                
                src_tmp = node_src[pe%(E_BLOCK_PAR/2)][s];
                dst_tmp = node_des[pe%(E_BLOCK_PAR/2)][d];
                
                for(int dim = 0; dim < NODE_DIM; dim ++){
                    
                    edge_block_in[NODE_DIM+dim](data_w-1, 0) = uint16_to_DTYPE<attr_t>(src_tmp(data_w * (dim+1)-1, data_w *dim));
                    edge_block_in[dim](data_w-1, 0) = uint16_to_DTYPE<attr_t>(dst_tmp(data_w * (dim+1)-1, data_w *dim));
```

### L data types

Source: [designs/Graph_L/src/su_defines.h](../designs/Graph_L/src/su_defines.h), lines **23–50**.

```cpp
constexpr int   data_w = 16;

//typedef ap_fixed<14,7>  attr_t;
//typedef ap_fixed<14,7>  weight_t;
typedef ap_fixed<16,8>  attr_t;
typedef ap_fixed<16,8>  weight_t;
// Sensitivity study: external formats unchanged, fractional bits = 8.
typedef ap_fixed<24,16> sens_agg_t;
typedef ap_fixed<24,16> sens_mac_t;
typedef std::array<sens_agg_t,4> sens_agg_edge;

typedef ap_uint<16>     index_t;


typedef ap_uint<16>     HBM_t;




typedef std::array<attr_t, EDGE_DIM> edge;
typedef std::array<attr_t, NODE_DIM> node;

typedef std::array<attr_t, EDGE_BLOCK_END_DIM> edge_res;

typedef std::array<index_t, 2> idx_pair;

typedef ap_uint<48> node_str; 
typedef attr_t DTYPE;
```

### L capacity

Source: [designs/Graph_L/src/su_defines.h](../designs/Graph_L/src/su_defines.h), lines **61–90**.

```cpp
constexpr int   MAX_NODE = 43278; //40000;//739; //modify
constexpr int   MAX_EDGE = 50000; //60000;//1252; //modify
constexpr int   SRC_NODE = 17980;//14260; //2089;//202;
constexpr int   TOTAL_EDGE_PER_GRAPH = 1649533; //1500000;//1252; //modify
constexpr int   MAX_NUM_GRAPH = 10;
// Include a partial final shard; all counts are logical edge slots.
constexpr int   MAX_NUM_SHARD = (TOTAL_EDGE_PER_GRAPH + MAX_EDGE - 1) / MAX_EDGE;
constexpr int   LAST_SHARD_EDGES = TOTAL_EDGE_PER_GRAPH - (MAX_NUM_SHARD - 1) * MAX_EDGE;
static_assert(TOTAL_EDGE_PER_GRAPH == 1649533, "Graph_L edge capacity changed");
static_assert(LAST_SHARD_EDGES == 49533, "Graph_L remainder must be 49,533 edges");
static_assert((MAX_NUM_SHARD - 1) * MAX_EDGE + LAST_SHARD_EDGES == TOTAL_EDGE_PER_GRAPH,
              "Graph_L shard plan must cover every edge");


constexpr int   MLP_HIDDEN_DIM = 8;
constexpr int   MLP_LAYER = 3;

constexpr int   MAX_WEIGHT_DIM = 80; //EDGE_BLOCK_IN_DIM(10) * MLP_HIDDEN_DIM
constexpr int   MAX_BIAS_DIM = 8; //MLP_HIDDEN_DIM

constexpr int   E_BLOCK_PAR = 8;
constexpr int   N_BLOCK_PAR = 8;
constexpr int   NS_BLOCK_PAR = 4;
constexpr int   E2_BLOCK_PAR = 8;
constexpr int   MERGE_PAR = 4;



constexpr int   READ_W_PAR_PER_CH = 1; //number of channel for load weight
constexpr int   READ_N_PAR_PER_CH = 10; //number per channel for load node
```
