/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GGUF_H
#define PIONEER_QWEN35_GGUF_H
#include "TenstorrentGgufTensor.h"
#include "PioneerAttentionMath.h"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

// Exact 4B packed source only. Open validates structure, not SHA-256: the
// caller must authenticate the complete original on this same FILE before
// using it. No model evaluation, device access, fallback, or upload here.
struct PioneerQ35Tensor {
 std::string name;
 uint64_t inputs=0,outputs=0,offset=0,bytes=0;
 unsigned type=0,rank=0;
};
static inline unsigned PioneerQ35Block(unsigned type) {
 return type==0?1:type==8?32:(type==12 || type==13 || type==14)?256:0;
}
static inline unsigned PioneerQ35BlockBytes(unsigned type) {
 return type==0?4:type==8?34:type==12?144:type==13?176:type==14?210:0;
}
struct PioneerQ35Expected {
 const char* name; unsigned inputs,outputs,type,alternate,rank;
};
static inline const PioneerQ35Expected* PioneerQ35Role(unsigned layer,unsigned role) {
 static const PioneerQ35Expected common[]={
  {"attn_norm.weight",2560,1,0,0,1},{"post_attention_norm.weight",2560,1,0,0,1},
  {"ffn_gate.weight",2560,9216,12,12,2},{"ffn_up.weight",2560,9216,12,12,2},
  {"ffn_down.weight",9216,2560,12,14,2}};
 static const PioneerQ35Expected recurrent[]={
  {"attn_gate.weight",2560,4096,12,12,2},{"attn_qkv.weight",2560,8192,13,13,2},
  {"ssm_a",32,1,0,0,1},{"ssm_dt.bias",32,1,0,0,1},{"ssm_norm.weight",128,1,0,0,1},
  {"ssm_alpha.weight",2560,32,8,8,2},{"ssm_beta.weight",2560,32,8,8,2},
  {"ssm_conv1d.weight",4,8192,0,0,2},{"ssm_out.weight",4096,2560,13,13,2}};
 static const PioneerQ35Expected attention[]={
  {"attn_q.weight",2560,8192,12,12,2},{"attn_k.weight",2560,1024,12,12,2},
  {"attn_v.weight",2560,1024,12,14,2},{"attn_q_norm.weight",256,1,0,0,1},
  {"attn_k_norm.weight",256,1,0,0,1},{"attn_output.weight",4096,2560,12,12,2}};
 static const PioneerQ35Expected special[]={
  {"token_embd.weight",2560,248320,14,14,2},{"output_norm.weight",2560,1,0,0,1}};
 if (layer==32) return role<2?&special[role]:nullptr;
 if (layer>=32) return nullptr;
 if (role<5) return &common[role];
 role-=5;
 return layer%4==3?(role<6?&attention[role]:nullptr):(role<9?&recurrent[role]:nullptr);
}

class PioneerQwen35Gguf {
 FILE* source=nullptr;
 struct stat identity{};
 std::vector<PioneerQ35Tensor> tensors,empty;
 std::map<std::string,unsigned> names;
 uint64_t epoch=0;
 bool ready=false;
 const PioneerQ35Tensor* Lookup(const char* name) const {
  auto found=names.find(name); return found==names.end()?nullptr:&tensors[found->second];
 }
public:
 static constexpr uint64_t FullBytes=UINT64_C(2740937888);
 static constexpr unsigned MaxPackedRead=1024*1024;
 bool Stable() const {
  struct stat now{};
  return ready && source && !fstat(fileno(source),&now) && S_ISREG(now.st_mode)
   && now.st_dev==identity.st_dev && now.st_ino==identity.st_ino && now.st_size==identity.st_size
   && now.st_mtime==identity.st_mtime && now.st_ctime==identity.st_ctime
#if defined(__APPLE__)
   && now.st_mtimespec.tv_nsec==identity.st_mtimespec.tv_nsec && now.st_ctimespec.tv_nsec==identity.st_ctimespec.tv_nsec;
#else
   && now.st_mtim.tv_nsec==identity.st_mtim.tv_nsec && now.st_ctim.tv_nsec==identity.st_ctim.tv_nsec;
#endif
 }
 uint64_t Epoch() const { return epoch; }
 const std::vector<PioneerQ35Tensor>& Tensors() const { return ready?tensors:empty; }
 const PioneerQ35Tensor* Find(const char* name) const { return ready && name?Lookup(name):nullptr; }
 // The same physical tensor serves token lookup and output projection.
 const PioneerQ35Tensor* TiedHead() const { return Find("token_embd.weight"); }
 bool Open(FILE* file) {
  ready=false; source=nullptr; tensors.clear(); names.clear();
  if (epoch==UINT64_MAX) return false;
  epoch++;
  if (!file || fstat(fileno(file),&identity) || !S_ISREG(identity.st_mode)
   || uint64_t(identity.st_size)!=FullBytes || fseeko(file,0,SEEK_SET)) return false;
  PioneerGgufReader r(file,FullBytes);
  if (r.Number(4)!=0x46554747 || r.Number(4)!=3 || r.Number(8)!=426) return false;
  uint64_t count=r.Number(8); if (!r.good || count>100000) return false;
  struct Required { const char* key; unsigned value; };
  static const Required required[]={
   {"qwen35.block_count",32},{"qwen35.context_length",262144},{"qwen35.embedding_length",2560},
   {"qwen35.feed_forward_length",9216},{"qwen35.attention.head_count",16},{"qwen35.attention.head_count_kv",4},
   {"qwen35.attention.key_length",256},{"qwen35.attention.value_length",256},{"qwen35.ssm.conv_kernel",4},
   {"qwen35.ssm.state_size",128},{"qwen35.ssm.group_count",16},{"qwen35.ssm.time_step_rank",32},
   {"qwen35.ssm.inner_size",4096},{"qwen35.full_attention_interval",4},{"qwen35.rope.dimension_count",64},
   {"general.file_type",15}};
  bool fields[sizeof(required)/sizeof(*required)]={},arch=false,name=false,epsilon=false,base=false,sections=false;
  unsigned alignment=32; std::set<std::string> metadata;
  for (uint64_t i=0;i<count;i++) {
   char key[513]; if (!r.String(key,sizeof(key)) || !metadata.insert(key).second) return false;
   unsigned type=unsigned(r.Number(4)); bool handled=false;
   for (unsigned f=0;f<sizeof(fields)/sizeof(*fields);f++) if (!strcmp(key,required[f].key)) {
    if (type!=4 || r.Number(4)!=required[f].value) return false;
    fields[f]=true; handled=true; break;
   }
   if (handled) continue;
   if (!strcmp(key,"general.architecture") || !strcmp(key,"general.name")) {
    char value[64]; if (type!=8 || !r.String(value,sizeof(value))) return false;
    if (!strcmp(key,"general.architecture")) { if (strcmp(value,"qwen35")) return false; arch=true; }
    else { if (strcmp(value,"Qwen3.5-4B")) return false; name=true; }
   } else if (!strcmp(key,"general.alignment")) {
    if (type!=4) return false;
    alignment=unsigned(r.Number(4));
   } else if (!strcmp(key,"qwen35.attention.layer_norm_rms_epsilon")) {
    if (type!=6 || r.Number(4)!=PioneerAttentionBits(1e-6f)) return false;
    epsilon=true;
   } else if (!strcmp(key,"qwen35.rope.freq_base")) {
    if (type!=6 || r.Number(4)!=PioneerAttentionBits(10000000.0f)) return false;
    base=true;
   } else if (!strcmp(key,"qwen35.rope.dimension_sections")) {
    if (type!=9 || r.Number(4)!=5 || r.Number(8)!=4) return false;
    const unsigned expected[]={11,11,10,0};
    for (unsigned j=0;j<4;j++) if (r.Number(4)!=expected[j]) return false;
    sections=true;
   } else if (!r.Value(type)) return false;
  }
  if (!arch || !name || !epsilon || !base || !sections || !r.good
   || !alignment || alignment>4096 || (alignment&(alignment-1))) return false;
  for (bool field:fields) if (!field) return false;
  unsigned formats[15]={};
  for (unsigned i=0;i<426;i++) {
   char text[1025]; if (!r.String(text,sizeof(text)) || !names.emplace(text,unsigned(tensors.size())).second) return false;
   PioneerQ35Tensor entry; entry.name=text; entry.rank=unsigned(r.Number(4));
   if (entry.rank!=1 && entry.rank!=2) return false;
   entry.inputs=r.Number(8); entry.outputs=entry.rank==2?r.Number(8):1;
   entry.type=unsigned(r.Number(4)); entry.offset=r.Number(8);
   unsigned block=PioneerQ35Block(entry.type),bytes=PioneerQ35BlockBytes(entry.type);
   if (!r.good || !block || !entry.inputs || entry.inputs>=UINT64_C(0x100000000)
    || !entry.outputs || entry.outputs>=UINT64_C(0x100000000) || entry.inputs%block || entry.offset%alignment) return false;
   uint64_t row=entry.inputs/block*bytes;
   if (entry.outputs>UINT64_MAX/row) return false;
   entry.bytes=row*entry.outputs; formats[entry.type]++; tensors.push_back(entry);
  }
  if (formats[0]!=177 || formats[8]!=48 || formats[12]!=131 || formats[13]!=48 || formats[14]!=22) return false;
  uint64_t start=(r.position+alignment-1)&~uint64_t(alignment-1),end=start;
  if (!r.good || start>FullBytes) return false;
  std::vector<PioneerQ35Tensor*> ordered;
  for (auto& entry:tensors) ordered.push_back(&entry);
  std::sort(ordered.begin(),ordered.end(),[](const PioneerQ35Tensor* a,const PioneerQ35Tensor* b){return a->offset<b->offset;});
  for (auto* entry:ordered) {
   if (entry->offset>FullBytes-start || entry->bytes>FullBytes-start-entry->offset) return false;
   entry->offset+=start; if (entry->offset<end) return false; end=entry->offset+entry->bytes;
  }
  unsigned checked=0;
  for (unsigned layer=0;layer<=32;layer++) for (unsigned role=0;;role++) {
   const auto* expected=PioneerQ35Role(layer,role); if (!expected) break;
   char text[96];
   if (layer==32) snprintf(text,sizeof(text),"%s",expected->name);
   else snprintf(text,sizeof(text),"blk.%u.%s",layer,expected->name);
   const auto* entry=Lookup(text);
   if (!entry || entry->inputs!=expected->inputs || entry->outputs!=expected->outputs
    || (entry->type!=expected->type && entry->type!=expected->alternate) || entry->rank!=expected->rank) return false;
   checked++;
  }
  if (checked!=426) return false;
  source=file; ready=true;
  if (!Stable()) { ready=false; source=nullptr; return false; }
  return true;
 }
 bool ReadPacked(unsigned tensor,uint64_t relative,unsigned bytes,void* destination,unsigned capacity) {
  if (!Stable() || tensor>=tensors.size() || !destination || !bytes || bytes>capacity || bytes>MaxPackedRead) return false;
  const auto& entry=tensors[tensor];
  if (relative>entry.bytes || bytes>entry.bytes-relative || fseeko(source,off_t(entry.offset+relative),SEEK_SET)) return false;
  return fread(destination,1,bytes,source)==bytes && Stable();
 }
};
#endif
