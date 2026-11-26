/* SPDX-License-Identifier: MIT */
// Standalone offline CPU oracle for simulator testing. Never used by Work mode.
#include "../PioneerModelWeightCache.h"
#include "llama.h"
#include "ggml-backend.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <utility>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
struct Capture {std::array<float,2560> layer0{},layer3{},layer31{};
    std::vector<float> logits;bool got0=false,got3=false,got31=false,gotLogits=false;};
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
    const bool first=std::strcmp(tensor->name,"l_out-0")==0;
    const bool fourth=std::strcmp(tensor->name,"l_out-3")==0;
    const bool last=std::strcmp(tensor->name,"l_out-31")==0;
    const bool logits=std::strcmp(tensor->name,"result_output")==0;
    if (ask) return first || fourth || last || logits;
    if (tensor->type!=GGML_TYPE_F32 || !ggml_is_contiguous(tensor)) return false;
    if (logits) {
        if (ggml_nelements(tensor)!=248320 || ggml_nbytes(tensor)!=248320*sizeof(float)) return false;
        capture.logits.resize(248320);ggml_backend_tensor_get(tensor,capture.logits.data(),0,ggml_nbytes(tensor));
        capture.gotLogits=true;return true;
    }
    if ((!first && !fourth && !last) || ggml_nelements(tensor)!=2560
        || ggml_nbytes(tensor)!=2560*sizeof(float)) return false;
    auto& target=first?capture.layer0:fourth?capture.layer3:capture.layer31;
    ggml_backend_tensor_get(tensor,target.data(),0,target.size()*sizeof(float));
    if (first) capture.got0=true;else if (fourth) capture.got3=true;else capture.got31=true;
    return true;
}
static bool WriteNew(const std::string& path,const std::array<float,2560>& values) {
    FILE* file=std::fopen(path.c_str(),"wbx");if (!file) return false;
    bool okay=std::fwrite(values.data(),sizeof(float),values.size(),file)==values.size();
    if (std::fclose(file)) okay=false;
    if (!okay) std::remove(path.c_str());return okay;
}
static bool BestToken(const Capture& capture,unsigned& best) {
    if (!capture.got0 || !capture.got3 || !capture.got31 || !capture.gotLogits
        || capture.logits.size()!=248320) return false;
    best=0;
    for (unsigned i=0;i<capture.logits.size();i++) {
        if (!std::isfinite(capture.logits[i])) return false;
        if (capture.logits[i]>capture.logits[best]) best=i;
    }
    return true;
}
static bool Save(const std::string& dir,const std::string& prefix,const Capture& capture,unsigned best) {
    if (!WriteNew(dir+"/"+prefix+"layer0.bin",capture.layer0)
        || !WriteNew(dir+"/"+prefix+"layer3.bin",capture.layer3)
        || !WriteNew(dir+"/"+prefix+"layer31.bin",capture.layer31)) return false;
    FILE* logitsFile=std::fopen((dir+"/"+prefix+"logits.bin").c_str(),"wbx");if (!logitsFile) return false;
    const bool wrote=std::fwrite(capture.logits.data(),sizeof(float),capture.logits.size(),logitsFile)==capture.logits.size();
    if (std::fclose(logitsFile) || !wrote) return false;
    FILE* tokenFile=std::fopen((dir+"/"+prefix+"token.txt").c_str(),"wx");if (!tokenFile) return false;
    const bool tokenWrote=std::fprintf(tokenFile,"%u\n",best)>0;
    return std::fclose(tokenFile)==0 && tokenWrote;
}
int main(int argc,char** argv) {
    if (argc!=3 && argc!=4) {std::fprintf(stderr,"usage: %s pinned.gguf empty-output-directory [two]\n",argv[0]);return 2;}
    const bool two=argc==4 && !std::strcmp(argv[3],"two");if (argc==4 && !two) return 2;
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
    Capture capture{};auto config=llama_context_default_params();
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
    std::printf("wrote standalone CPU reference argmax %u%s; NO PIONEER HARDWARE\n",
        firstToken,two?" and next position":"");
}
