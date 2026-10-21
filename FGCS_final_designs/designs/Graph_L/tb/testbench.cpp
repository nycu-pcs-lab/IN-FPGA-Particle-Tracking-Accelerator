// Portable release testbench; kernel source is unchanged.
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>
#include <iterator>
#include <utility>
#include "top_decl.h"
#include "../../../testbench/compare_outputs.h"
template<class T> void binary_read(std::ifstream& in,T* values,std::size_t count) {
    in.read(reinterpret_cast<char*>(values),sizeof(T)*count);
    if(!in)throw std::runtime_error("Truncated Graph_L input");
}
void load_layer(const std::string& folder,const std::string& family,int layer,int wc,int bc,std::vector<input_weight>& target) {
    int index=0;
    for(const auto& part:{std::make_pair('w',wc),std::make_pair('b',bc)}) {
        std::ifstream in(folder+"/"+family+"_"+part.first+std::to_string(layer)+".txt");
        if(!in)throw std::runtime_error("Missing weight file");
        std::string text((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        for(auto& c:text)if(c==',')c=' ';
        std::istringstream tokens(text);
        for(int i=0;i<part.second;++i) {
            double value;if(!(tokens>>value))throw std::runtime_error("Missing weight scalar");
            target[index++].w_attr[0]=weight_t(value);
        }
        double extra;if(tokens>>extra)throw std::runtime_error("Unexpected weight scalar");
    }
}
int main(int argc,char** argv) {
    if(argc!=4)throw std::runtime_error("Usage: graph_l_hls_csim input.bin weights output.bin");
    std::ifstream in(argv[1],std::ios::binary);
    std::uint32_t header[3];binary_read(in,header,3);
    if(header[0]!=1 || header[1]!=MAX_NODE || header[2]!=TOTAL_EDGE_PER_GRAPH)
        throw std::runtime_error("Unexpected Graph_L dimensions");
    const int words=MAX_NUM_SHARD*ceildiv_even(MAX_EDGE,E_BLOCK_PAR)+2;
    const int stride=ceildiv(MAX_EDGE,E_BLOCK_PAR);
    const int out_stride=ceildiv(MAX_EDGE,E2_BLOCK_PAR);
    std::vector<input_node> nodes(MAX_NUM_GRAPH*ceildiv_even(MAX_NODE,READ_N_PAR_PER_CH)+2);
    std::vector<input_src> sources(MAX_NUM_GRAPH*ceildiv_even(SRC_NODE,READ_N_PAR_PER_CH)+2);
    std::vector<input_edge0> edges0(MAX_NUM_GRAPH*words),intermediate0(words),unused0(words);
    std::vector<input_idx> pairs(MAX_NUM_GRAPH*words),intermediate_indices(words),unused_indices(words);
    std::vector<output_edge> output(MAX_NUM_GRAPH*MAX_NUM_SHARD*ceildiv_even(MAX_EDGE,E2_BLOCK_PAR)+2);
    std::vector<float> features(MAX_NODE*3+TOTAL_EDGE_PER_GRAPH*4);
    std::vector<std::uint16_t> endpoints(TOTAL_EDGE_PER_GRAPH*2);
    binary_read(in,features.data(),features.size());binary_read(in,endpoints.data(),endpoints.size());
    std::uint32_t count;binary_read(in,&count,1);
    if(count>SRC_NODE)throw std::runtime_error("Source capacity exceeded");
    std::vector<std::uint16_t> source_ids(count);binary_read(in,source_ids.data(),count);
    if(in.peek()!=EOF)throw std::runtime_error("Trailing Graph_L input bytes");
    for(auto& word:nodes)for(auto& value:word.n_attr)value=0;
    for(auto& word:sources)for(auto& value:word.s_attr)value=MAX_NODE-1;
    for(auto& word:edges0)for(auto& value:word.e_attr)value=0;
    for(auto& word:pairs)for(auto& value:word.i_attr)value=MAX_NODE-1;
    for(auto& word:output)for(auto& value:word.o_attr)value=-2;
    for(int v=0;v<MAX_NODE;++v)for(int d=0;d<3;++d)
        nodes[v/READ_N_PAR_PER_CH].n_attr[(v%READ_N_PAR_PER_CH)*3+d]=attr_t(features[v*3+d]);
    for(int i=0;i<int(count);++i)sources[i/READ_N_PAR_PER_CH].s_attr[i%READ_N_PAR_PER_CH]=source_ids[i];
    for(int e=0;e<TOTAL_EDGE_PER_GRAPH;++e) {
        const int local=e%MAX_EDGE,lane=local%E_BLOCK_PAR,word=(e/MAX_EDGE)*stride+local/E_BLOCK_PAR;
        for(int d=0;d<4;++d) {
            const attr_t value=attr_t(features[MAX_NODE*3+e*4+d]);
            edges0[word].e_attr[lane*4+d]=value;
        }
        for(int d=0;d<2;++d) {
            if(endpoints[e*2+d]>=MAX_NODE)throw std::runtime_error("Invalid endpoint");
            pairs[word].i_attr[lane*2+d]=endpoints[e*2+d];
        }
    }
    std::vector<input_weight> weights[3][3];
    const std::string family[3]={"R1","O","R2"};
    const int wc[3][3]={{80,64,32},{56,64,24},{80,64,8}},bc[3][3]={{8,8,4},{8,8,3},{8,8,1}};
    for(int f=0;f<3;++f)for(int l=0;l<3;++l) {
        weights[f][l].resize(MAX_WEIGHT_DIM+MAX_BIAS_DIM+2);
        load_layer(argv[2],family[f],l,wc[f][l],bc[f][l],weights[f][l]);
    }
    ap_uint<16> source_counts[MAX_NUM_GRAPH]={};source_counts[0]=count;
    su_top_model(nodes.data(),edges0.data(),
        pairs.data(),sources.data(),intermediate0.data(),
        intermediate_indices.data(),unused0.data(),
        unused_indices.data(),
        weights[0][0].data(),weights[0][1].data(),weights[0][2].data(),
        weights[1][0].data(),weights[1][1].data(),weights[1][2].data(),
        weights[2][0].data(),weights[2][1].data(),weights[2][2].data(),
        output.data(),1,source_counts);
    std::ofstream out(argv[3],std::ios::binary);
    int processed_edges=0;
    for(int e=0;e<TOTAL_EDGE_PER_GRAPH;++e) {
        const int local=e%MAX_EDGE,word=(e/MAX_EDGE)*out_stride+local/E2_BLOCK_PAR;
        const attr_t score=output[word].o_attr[local%E2_BLOCK_PAR];
        if(score<0 || score>1)throw std::runtime_error("Unwritten or invalid output score");
        const std::int16_t raw=static_cast<std::int16_t>(score.range(15,0).to_uint());
        out.write(reinterpret_cast<const char*>(&raw),sizeof(raw));++processed_edges;
    }
    assert(processed_edges==1649533);
    if(!out)throw std::runtime_error("Cannot write full Graph_L output");
    std::cout<<"GRAPH_L_NATIVE_OUTPUT_COUNT="<<processed_edges<<" LAST_SHARD="<<LAST_SHARD_EDGES<<std::endl;
    out.close();
    return compare_valid_outputs(argv[1],argv[3]);
}
