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
template<class T> void binary_read(std::ifstream& in, T* values, std::size_t count) {
    in.read(reinterpret_cast<char*>(values), sizeof(T)*count);
    if (!in) throw std::runtime_error("Truncated HLS test input");
}

void load_layer(const std::string& folder, const std::string& family, int layer,
                int weight_count, int bias_count, std::vector<input_weight>& target) {
    int index=0;
    for (const auto& part : {std::make_pair('w',weight_count),std::make_pair('b',bias_count)}) {
        std::ifstream in(folder+"/"+family+"_"+part.first+std::to_string(layer)+".txt");
        if (!in) throw std::runtime_error("Missing weight text");
        std::string text((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        for (auto& c : text) if (c==',') c=' ';
        std::istringstream tokens(text);
        for (int i=0;i<part.second;++i) {
            double value;
            if (!(tokens>>value)) throw std::runtime_error("Missing weight scalar");
            target[index++].w_attr[0]=weight_t(value);
        }
        double extra;
        if(tokens>>extra) throw std::runtime_error("Unexpected extra weight scalar");
    }
}

int run_graph(int argc,char** argv) {
    if(argc!=4) throw std::runtime_error("Usage: graph_m_hls_csim input.bin weights_folder output.bin");
    std::ifstream in(argv[1],std::ios::binary);
    std::uint32_t header[3]; binary_read(in,header,3);
    const int stored_graphs=header[0];
    const int graphs=3; // Replay one stored sample to satisfy the unchanged batch pipeline.
    static_assert(MAX_NUM_GRAPH >= 3, "Graph_M needs three batch entries");
    if(stored_graphs!=1 || header[1]!=MAX_NODE || header[2]!=MAX_EDGE)
        throw std::runtime_error("Unexpected Graph_M dimensions / graph batch size");
    const int ns=ceildiv(MAX_NODE,READ_N_PAR_PER_CH);
    const int es=ceildiv(MAX_EDGE,E_BLOCK_PAR);
    const int ss=ceildiv(SRC_NODE,READ_N_PAR_PER_CH);
    const int os=ceildiv(MAX_EDGE,E2_BLOCK_PAR);
    // Extra packed word covers the original ceildiv_even preload loops.
    std::vector<input_node> nodes(MAX_NUM_GRAPH*ceildiv_even(MAX_NODE,READ_N_PAR_PER_CH)+2);
    std::vector<input_edge> edges(MAX_NUM_GRAPH*ceildiv_even(MAX_EDGE,E_BLOCK_PAR)+2);
    std::vector<input_idx> pairs(MAX_NUM_GRAPH*ceildiv_even(MAX_EDGE,E_BLOCK_PAR)+2);
    std::vector<input_src> sources(MAX_NUM_GRAPH*ceildiv_even(SRC_NODE,READ_N_PAR_PER_CH)+2);
    std::vector<output_edge> output(MAX_NUM_GRAPH*ceildiv_even(MAX_EDGE,E2_BLOCK_PAR)+2);
    for(auto& word:output) for(auto& value:word.o_attr) value=-2;
    ap_uint<16> source_counts[MAX_NUM_GRAPH] = {};
    const std::streampos graph_payload_begin=in.tellg();
    for(int g=0;g<graphs;++g) {
        if(g>0) { in.clear(); in.seekg(graph_payload_begin); }
        std::vector<float> features(MAX_NODE*NODE_DIM+MAX_EDGE*EDGE_DIM);
        std::vector<std::uint16_t> endpoints(MAX_EDGE*2);
        binary_read(in,features.data(),features.size());
        binary_read(in,endpoints.data(),endpoints.size());
        std::uint32_t source_count; binary_read(in,&source_count,1);
        if(source_count>SRC_NODE) throw std::runtime_error("Pure-source list exceeds hardware capacity");
        std::vector<std::uint16_t> source_ids(source_count);
        binary_read(in,source_ids.data(),source_ids.size());
        source_counts[g]=source_count;
        for(int v=0;v<MAX_NODE;++v) for(int d=0;d<NODE_DIM;++d)
            nodes[g*ns+v/READ_N_PAR_PER_CH].n_attr[(v%READ_N_PAR_PER_CH)*NODE_DIM+d]=attr_t(features[v*NODE_DIM+d]);
        for(int e=0;e<MAX_EDGE;++e) {
            for(int d=0;d<EDGE_DIM;++d)
                edges[g*es+e/E_BLOCK_PAR].e_attr[(e%E_BLOCK_PAR)*EDGE_DIM+d]=attr_t(features[MAX_NODE*NODE_DIM+e*EDGE_DIM+d]);
            for(int d=0;d<2;++d) {
                if(endpoints[e*2+d]>=MAX_NODE) throw std::runtime_error("Bad node index");
                pairs[g*es+e/E_BLOCK_PAR].i_attr[(e%E_BLOCK_PAR)*2+d]=endpoints[e*2+d];
            }
            output[g*os+e/E2_BLOCK_PAR].o_attr[e%E2_BLOCK_PAR]=-2;
        }
        for(int i=0;i<SRC_NODE;++i)
            sources[g*ss+i/READ_N_PAR_PER_CH].s_attr[i%READ_N_PAR_PER_CH]=
                i<int(source_count)?source_ids[i]:MAX_NODE-1;
    }
    if(in.peek()!=EOF) throw std::runtime_error("Unexpected trailing input bytes");
    std::vector<input_weight> weights[3][3];
    const std::string families[3]={"R1","O","R2"};
    const int wc[3][3]={{80,64,32},{56,64,24},{80,64,8}};
    const int bc[3][3]={{8,8,4},{8,8,3},{8,8,1}};
    for(int f=0;f<3;++f) for(int l=0;l<3;++l) {
        // Original load_weight_v3 preloads base+2 words, including unused padding.
        weights[f][l].resize(MAX_WEIGHT_DIM+MAX_BIAS_DIM+2);
        load_layer(argv[2],families[f],l,wc[f][l],bc[f][l],weights[f][l]);
    }
    su_top_model(nodes.data(),edges.data(),pairs.data(),sources.data(),
        weights[0][0].data(),weights[0][1].data(),weights[0][2].data(),
        weights[1][0].data(),weights[1][1].data(),weights[1][2].data(),
        weights[2][0].data(),weights[2][1].data(),weights[2][2].data(),
        output.data(),graphs,source_counts);
    // Keep one graph of scores on disk; verify every internal replay before overwrite.
    for(int g=0;g<graphs;++g) {
        std::ofstream out(argv[3],std::ios::binary);
        std::uint64_t written=0;
        for(int e=0;e<MAX_EDGE;++e) {
            const attr_t value=output[g*os+e/E2_BLOCK_PAR].o_attr[e%E2_BLOCK_PAR];
            if(value<0 || value>1) throw std::runtime_error("Unwritten or invalid HLS output score");
            const std::int16_t raw=static_cast<std::int16_t>(value.range(15,0).to_uint());
            out.write(reinterpret_cast<const char*>(&raw),sizeof(raw));++written;
        }
        if(!out || written!=std::uint64_t(MAX_EDGE)) throw std::runtime_error("Output count mismatch");
        out.close();
        std::cout<<"GRAPH_M_REPLAY="<<g<<" STORED_GRAPHS=1 INTERNAL_BATCH_GRAPHS="<<graphs
                 <<" OUTPUT_COUNT="<<written<<std::endl;
        if(compare_valid_outputs(argv[1],argv[3]))return 1;
    }
    std::cout<<"GRAPH_M_UNIQUE_SAMPLES=1 INTERNAL_BATCH_GRAPHS="<<graphs
             <<" INTERNAL_OUTPUT_COUNT="<<graphs*MAX_EDGE<<std::endl;
    return 0;
}

int main(int argc,char**argv){
    return run_graph(argc,argv);
}
