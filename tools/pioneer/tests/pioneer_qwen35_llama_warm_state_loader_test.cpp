/* SPDX-License-Identifier: MIT */
// Exercise the actual offline oracle state loader without loading a model.
#define main qwen35_offline_oracle_main
#include "pioneer_qwen35_llama_layer_reference.cpp"
#undef main
#include <cassert>
#include <filesystem>
#include <limits>

static void WriteFloats(const std::string& path,size_t count,float first) {
    FILE* out=std::fopen(path.c_str(),"wb");assert(out);
    std::vector<float> values(count,0.0f);if (count) values[0]=first;
    assert(std::fwrite(values.data(),sizeof(float),count,out)==count);
    assert(std::fclose(out)==0);
}

int main() {
    char path[]="/tmp/q35-warm-state-loader-XXXXXX";assert(mkdtemp(path));
    const std::string dir=path,history=dir+"/H-layer1.bin",delta=dir+"/D-layer1.bin";
    const std::string marker=dir+"/worker-audit-complete.marker";
    WriteFloats(history,24576,1.0f);WriteFloats(delta,524288,-2.0f);
    { Capture state{};assert(!LoadWarmState(dir,state,1)); }
    FILE* out=std::fopen(marker.c_str(),"wb");assert(out && std::fclose(out)==0);
    { Capture state{};assert(LoadWarmState(dir,state,1));
      assert(state.forceWarmStateLayer==1 && state.forceWarmState[0].size()==24576
          && state.forceWarmState[1].size()==524288
          && state.forceWarmState[0][0]==1.0f && state.forceWarmState[1][0]==-2.0f); }
    { Capture state{};assert(!LoadWarmState(dir,state,3)); }
    WriteFloats(history,24575,0.0f);
    { Capture state{};assert(!LoadWarmState(dir,state,1)); }
    WriteFloats(history,24576,std::numeric_limits<float>::quiet_NaN());
    { Capture state{};assert(!LoadWarmState(dir,state,1)); }
    WriteFloats(history,24576,1.0f);assert(std::remove(delta.c_str())==0);
    assert(symlink(history.c_str(),delta.c_str())==0);
    { Capture state{};assert(!LoadWarmState(dir,state,1)); }
    std::filesystem::remove_all(dir);
    std::puts("PASS warm state loader rejects missing marker, bad layer, short, nonfinite and symlink inputs; NO HARDWARE");
}
