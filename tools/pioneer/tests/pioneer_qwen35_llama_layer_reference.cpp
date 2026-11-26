/* SPDX-License-Identifier: MIT */
// Standalone offline CPU oracle for simulator testing. Never used by Work mode.
#include "../PioneerModelWeightCache.h"
#include "llama.h"
#include "ggml-backend.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
static constexpr std::array<const char*,16> traceNames={"attn_norm","linear_attn_qkv_mixed",
    "z","beta","alpha","gate","conv_output_raw","conv_output_silu",
    "final_output","linear_attn_out","attn_residual","attn_post_norm","ffn_out",
    "ffn_gate","ffn_up","ffn_swiglu"};
static constexpr std::array<const char*,16> denseTraceNames={"attn_norm","Qcur_full",
    "gate_reshaped","attn_pregate","gate_sigmoid","attn_gated","attn_output",
    "attn_residual","attn_post_norm","ffn_out",nullptr,nullptr,nullptr,
    "ffn_gate","ffn_up","ffn_swiglu"};
static const char* TraceName(unsigned layer,size_t index) {
    return layer%4==3?denseTraceNames[index]:traceNames[index];
}
struct Capture {std::array<std::array<float,2560>,32> layers{};std::array<bool,32> gotLayer{};
    std::array<float,2560> replacement{};unsigned replaceLayer=32,traceLayer=32;
    bool replaceAttentionResidual=false;
    std::array<std::vector<float>,traceNames.size()> trace{};std::array<bool,traceNames.size()> gotTrace{};
    std::vector<float> logits;bool gotLogits=false,replaced=false;};
static bool SameFile(const struct stat& a,const struct stat& b) {
    if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
        || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
    return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
    return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
static bool Callback(ggml_tensor* tensor,bool ask,void* opaque) {
    auto& capture=*static_cast<Capture*>(opaque);
    unsigned layer=0;char trailing=0;
    const bool isLayer=std::sscanf(tensor->name,"l_out-%u%c",&layer,&trailing)==1 && layer<32;
    unsigned residualLayer=0;char residualTrailing=0;
    const bool isAttentionResidual=std::sscanf(tensor->name,"attn_residual-%u%c",&residualLayer,&residualTrailing)==1
        && residualLayer<32;
    const bool replaceResidual=capture.replaceAttentionResidual && isAttentionResidual
        && residualLayer==capture.replaceLayer;
    const bool logits=std::strcmp(tensor->name,"result_output")==0;
    size_t traceIndex=traceNames.size();
    if (capture.traceLayer<32) for (size_t i=0;i<traceNames.size();i++) {
        const char* prefix=TraceName(capture.traceLayer,i);if (!prefix) continue;
        char name[64];std::snprintf(name,sizeof(name),"%s-%u",prefix,capture.traceLayer);
        if (!std::strcmp(tensor->name,name)) {traceIndex=i;break;}
    }
    if (ask) return isLayer || logits || replaceResidual || traceIndex<traceNames.size();
    if (tensor->type!=GGML_TYPE_F32 || !ggml_is_contiguous(tensor)) return false;
    if (replaceResidual) {
        if (capture.replaced || ggml_nelements(tensor)!=2560
            || ggml_nbytes(tensor)!=capture.replacement.size()*sizeof(float)) return false;
        ggml_backend_tensor_set(tensor,capture.replacement.data(),0,capture.replacement.size()*sizeof(float));
        std::array<float,2560> readback{};
        ggml_backend_tensor_get(tensor,readback.data(),0,readback.size()*sizeof(float));
        if (std::memcmp(readback.data(),capture.replacement.data(),readback.size()*sizeof(float))) return false;
        capture.replaced=true;
    }
    if (traceIndex<traceNames.size()) {
        const int64_t count=ggml_nelements(tensor);
        if (count<=0 || count>32768 || ggml_nbytes(tensor)!=size_t(count)*sizeof(float)) return false;
        auto& target=capture.trace[traceIndex];target.resize(size_t(count));
        ggml_backend_tensor_get(tensor,target.data(),0,target.size()*sizeof(float));
        capture.gotTrace[traceIndex]=true;return true;
    }
    if (logits) {
        if (ggml_nelements(tensor)!=248320 || ggml_nbytes(tensor)!=248320*sizeof(float)) return false;
        capture.logits.resize(248320);ggml_backend_tensor_get(tensor,capture.logits.data(),0,ggml_nbytes(tensor));
        capture.gotLogits=true;return true;
    }
    if (replaceResidual) return true;
    if (!isLayer || ggml_nelements(tensor)!=2560
        || ggml_nbytes(tensor)!=2560*sizeof(float)) return false;
    auto& target=capture.layers[layer];
    ggml_backend_tensor_get(tensor,target.data(),0,target.size()*sizeof(float));
    if (!capture.replaceAttentionResidual && capture.replaceLayer==layer) {
        if (capture.replaced) return false;
        ggml_backend_tensor_set(tensor,capture.replacement.data(),0,capture.replacement.size()*sizeof(float));
        ggml_backend_tensor_get(tensor,target.data(),0,target.size()*sizeof(float));
        if (std::memcmp(target.data(),capture.replacement.data(),target.size()*sizeof(float))) return false;
        capture.replaced=true;
    }
    capture.gotLayer[layer]=true;
    return true;
}
template<typename Values> static bool WriteNew(const std::string& path,const Values& values) {
    FILE* file=std::fopen(path.c_str(),"wbx");if (!file) return false;
    bool okay=std::fwrite(values.data(),sizeof(float),values.size(),file)==values.size();
    if (std::fclose(file)) okay=false;
    if (!okay) std::remove(path.c_str());return okay;
}
static bool BestToken(const Capture& capture,unsigned& best) {
    if (!capture.gotLogits || capture.logits.size()!=248320
        || (capture.replaceLayer<32 && !capture.replaced)) return false;
    for (bool got:capture.gotLayer) if (!got) return false;
    if (capture.traceLayer<32) for (size_t i=0;i<traceNames.size();i++)
        if (TraceName(capture.traceLayer,i) && !capture.gotTrace[i]) return false;
    best=0;
    for (unsigned i=0;i<capture.logits.size();i++) {
        if (!std::isfinite(capture.logits[i])) return false;
        if (capture.logits[i]>capture.logits[best]) best=i;
    }
    return true;
}
static bool Save(const std::string& dir,const std::string& prefix,const Capture& capture,unsigned best) {
    for (unsigned layer=0;layer<capture.layers.size();layer++)
        if (!WriteNew(dir+"/"+prefix+"layer"+std::to_string(layer)+".bin",capture.layers[layer])) return false;
    if (capture.traceLayer<32) for (size_t i=0;i<traceNames.size();i++)
        if (TraceName(capture.traceLayer,i)
            && !WriteNew(dir+"/trace-"+TraceName(capture.traceLayer,i)+"-"+std::to_string(capture.traceLayer)+".bin",capture.trace[i])) return false;
    FILE* logitsFile=std::fopen((dir+"/"+prefix+"logits.bin").c_str(),"wbx");if (!logitsFile) return false;
    const bool wrote=std::fwrite(capture.logits.data(),sizeof(float),capture.logits.size(),logitsFile)==capture.logits.size();
    if (std::fclose(logitsFile) || !wrote) return false;
    FILE* tokenFile=std::fopen((dir+"/"+prefix+"token.txt").c_str(),"wx");if (!tokenFile) return false;
    const bool tokenWrote=std::fprintf(tokenFile,"%u\n",best)>0;
    return std::fclose(tokenFile)==0 && tokenWrote;
}
int main(int argc,char** argv) {
    if (argc!=3 && argc!=4 && argc!=6 && argc!=7 && argc!=8) {
        std::fprintf(stderr,"usage: %s pinned.gguf empty-output-directory [two | (--replace-layer | --replace-attn-residual) N tensor.bin [--trace-layer N | --trace-layer25]]\n",argv[0]);return 2;
    }
    const bool two=argc==4 && !std::strcmp(argv[3],"two");if (argc==4 && !two) return 2;
    Capture capture{};
    if (argc>=6) {
        if (std::strcmp(argv[3],"--replace-layer") && std::strcmp(argv[3],"--replace-attn-residual")) return 2;
        capture.replaceAttentionResidual=!std::strcmp(argv[3],"--replace-attn-residual");
        if (argc==7 && std::strcmp(argv[6],"--trace-layer25")) return 2;
        if (argc==7) capture.traceLayer=25;
        if (argc==8) {
            if (std::strcmp(argv[6],"--trace-layer")) return 2;
            char* traceEnd=nullptr;const unsigned long trace=std::strtoul(argv[7],&traceEnd,10);
            if (!argv[7][0] || *traceEnd || trace>=32) return 2;
            capture.traceLayer=unsigned(trace);
        }
        char* end=nullptr;const unsigned long layer=std::strtoul(argv[4],&end,10);
        if (!argv[4][0] || *end || layer>=32) return 2;
        const int replacementFd=open(argv[5],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if (replacementFd<0) return 1;
        struct stat replacementBefore{},replacementAfter{};
        if (fstat(replacementFd,&replacementBefore) || !S_ISREG(replacementBefore.st_mode)
            || replacementBefore.st_size!=static_cast<off_t>(capture.replacement.size()*sizeof(float))) {
            close(replacementFd);return 1;
        }
        FILE* replacement=fdopen(replacementFd,"rb");if (!replacement) {close(replacementFd);return 1;}
        const bool okay=std::fread(capture.replacement.data(),sizeof(float),capture.replacement.size(),replacement)==capture.replacement.size()
            && std::fgetc(replacement)==EOF && !std::ferror(replacement)
            && !fstat(replacementFd,&replacementAfter) && SameFile(replacementBefore,replacementAfter);
        if (std::fclose(replacement) || !okay) return 1;
        for (float value:capture.replacement) if (!std::isfinite(value)) return 1;
        capture.replaceLayer=unsigned(layer);
    }
    const int fd=open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC);if (fd<0) return 1;
    struct stat before{},after{};PioneerModelWeightCache::Digest digest{};std::string error;
    const bool valid=!fstat(fd,&before) && PioneerModelWeightCache::HashFile(fd,2740937888ULL,digest,error)
        && PioneerModelWeightCache::Hex(digest)=="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4"
        && !fstat(fd,&after) && SameFile(before,after);
    if (!valid) {close(fd);return 1;}
    llama_backend_init();
    auto params=llama_model_default_params();params.n_gpu_layers=0;
    const std::string fdPath="/dev/fd/"+std::to_string(fd);
    llama_model* model=llama_model_load_from_file(fdPath.c_str(),params);if (!model) {close(fd);return 1;}
    auto config=llama_context_default_params();
    config.n_ctx=32;config.n_batch=1;config.n_ubatch=1;config.n_threads=4;config.n_threads_batch=4;
    config.cb_eval=Callback;config.cb_eval_user_data=&capture;
    llama_context* ctx=llama_init_from_model(model,config);if (!ctx) {llama_model_free(model);close(fd);return 1;}
    llama_token token=42;int result=llama_decode(ctx,llama_batch_get_one(&token,1));
    unsigned firstToken=0,secondToken=0;Capture first{},second{};
    if (!result && BestToken(capture,firstToken)) {
        first=std::move(capture);
        if (two) {
            capture=Capture{};token=llama_token(firstToken);
            result=llama_decode(ctx,llama_batch_get_one(&token,1));
            if (!result && BestToken(capture,secondToken)) second=std::move(capture);
            else result=1;
        }
    } else result=1;
    llama_free(ctx);llama_model_free(model);llama_backend_free();
    const bool stable=!fstat(fd,&after) && SameFile(before,after);
    close(fd);
    if (result || !stable) return 1;
    const std::string dir=argv[2];
    if (!Save(dir,"",first,firstToken) || (two && !Save(dir,"position1-",second,secondToken))) return 1;
    std::printf("wrote standalone CPU reference argmax %u%s%s; NO PIONEER HARDWARE\n",
        firstToken,two?" and next position":"",argc>=6?" with tensor replacement":"");
}
