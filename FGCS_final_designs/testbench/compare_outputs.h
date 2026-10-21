#pragma once
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>
inline int compare_valid_outputs(const std::string& input_path, const std::string& actual_path) {
    const auto slash=input_path.find_last_of("/\\");
    const std::string dir=slash==std::string::npos ? "." : input_path.substr(0,slash);
    std::ifstream e(dir+"/expected_raw.bin",std::ios::binary),m(dir+"/valid_mask.bin",std::ios::binary),a(actual_path,std::ios::binary);
    if(!e||!m||!a)throw std::runtime_error("Missing expected output, mask, or actual output");
    const std::vector<unsigned char> expected((std::istreambuf_iterator<char>(e)),{});
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(m)),{});
    const std::vector<unsigned char> actual((std::istreambuf_iterator<char>(a)),{});
    if(expected.size()!=actual.size()||expected.size()!=mask.size()*2)
        throw std::runtime_error("Output / expected / mask size mismatch");
    std::size_t valid=0,valid_diff=0,padding_diff=0;
    for(std::size_t i=0;i<mask.size();++i) {
        if(mask[i]>1)throw std::runtime_error("Invalid mask byte");
        const bool diff=expected[2*i]!=actual[2*i]||expected[2*i+1]!=actual[2*i+1];
        if(mask[i]){++valid;if(diff)++valid_diff;}else if(diff)++padding_diff;
    }
    std::cout<<"VALID_EDGES="<<valid<<" VALID_DIFF="<<valid_diff<<" PADDING_DIFF="<<padding_diff<<std::endl;
    if(valid_diff)throw std::runtime_error("Valid-edge output differs from archived fixed-point reference");
    return 0;
}
