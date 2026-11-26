/* SPDX-License-Identifier: MIT */
// Offline same-input comparison against independently traced llama.cpp RoPE.
#include "../PioneerQwen35Preload.h"
#include "../PioneerQwen35DataflowDenseAttentionMath.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace PioneerQwen35;

static bool Read(const char* path,std::vector<float>& values,unsigned count) {
    FILE* input=std::fopen(path,"rb");if (!input) return false;
    values.resize(count);
    const bool okay=std::fread(values.data(),sizeof(float),count,input)==count
        && std::fgetc(input)==EOF && !std::ferror(input);
    return std::fclose(input)==0 && okay;
}
static bool Compare(const char* inputPath,const char* expectedPath,unsigned heads,const float* rope) {
    std::vector<float> input,expected;
    if (!Read(inputPath,input,heads*DenseHeadWidth) || !Read(expectedPath,expected,heads*DenseHeadWidth)) return false;
    for (unsigned head=0;head<heads;head++)
        if (!DataflowDenseRotate(input.data()+head*DenseHeadWidth,rope)) return false;
    float maximum=0;unsigned failed=0;
    for (unsigned i=0;i<input.size();i++) {
        if (!std::isfinite(input[i]) || !std::isfinite(expected[i])) return false;
        const float difference=std::fabs(input[i]-expected[i]);
        if (difference>maximum) maximum=difference;
        if (difference>2e-5f*std::fmax(1.0f,std::fabs(expected[i]))) ++failed;
    }
    std::printf("%u heads: %u/%zu mismatches; max absolute %.9g\n",heads,failed,input.size(),double(maximum));
    return !failed;
}
int main(int argc,char** argv) {
    if (argc!=5) {std::fprintf(stderr,"usage: %s q-before q-after k-before k-after\n",argv[0]);return 2;}
    alignas(64) std::array<uint8_t,4096> page{};
    const Range range{0,0x10000000,32*Rotary*sizeof(float)};
    if (!FillRope(range,0,page.size(),page.data())) return 1;
    const float* rope=reinterpret_cast<const float*>(page.data()+Rotary*sizeof(float));
    if (!Compare(argv[1],argv[2],16,rope) || !Compare(argv[3],argv[4],4,rope)) return 1;
    std::puts("PASS same-input position-one rotary comparison; NO HARDWARE");
}
