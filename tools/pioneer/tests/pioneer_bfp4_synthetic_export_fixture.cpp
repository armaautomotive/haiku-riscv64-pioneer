/* SPDX-License-Identifier: MIT */
// TEST ONLY: construct the private sample wire format from the independently
// tested synthetic fixture. It is NOT a learned model or device output.
#define PIONEER_NATIVE_BFP4_MACHINE_ONLY
#include "pioneer_q38_native_bfp4_band_numeric_test.cpp"
#include "../PioneerNativeBfp4BandGolden.h"
#include <vector>
static void Append(std::vector<uint8_t>& out,uint32_t word){for(unsigned i=0;i<4;i++)out.push_back(uint8_t(word>>(8*i)));}
int main(int argc,char** argv){
 assert(argc==2);Fixture f(5120);std::vector<uint8_t> data;
 Append(data,0x34504252);Append(data,1);Append(data,5120);Append(data,0);
 for(const auto& tile:f.packed)data.insert(data.end(),tile.begin(),tile.end());
 for(const auto& tile:f.input)for(auto value:tile){data.push_back(uint8_t(value));data.push_back(uint8_t(value>>8));}
 const auto* expected=PioneerNativeBfp4BandGolden(5120);assert(expected);
 for(unsigned row=0;row<32;row++)Append(data,expected[row]);
 assert(data.size()==102544);FILE* file=fopen(argv[1],"wb");assert(file);
 assert(fwrite(data.data(),1,data.size(),file)==data.size() && !fclose(file));
}
