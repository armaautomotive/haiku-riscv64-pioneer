/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN38_GGUF_H
#define PIONEER_QWEN38_GGUF_H
#include "TenstorrentGgufTensor.h"
#include "TenstorrentGgufQuant.h"
#include "PioneerAttentionMath.h"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

// Read-only host-side packed tensor source for the EXACT requested GGUF.
// Length/layout checks are NOT authentication: caller must independently
// verify published FULL SHA256 before execution/upload. NO templates are
// evaluated,NO whole-model expansion,NO GPU/driver/allocator access here.
struct PioneerQ38Tensor {
	std::string name;
	uint64_t inputs=0,outputs=0,offset=0,bytes=0;
	unsigned type=0,rank=0;
};
static inline unsigned PioneerQ38BlockElements(unsigned type)
{
	return type==0?1:type==8?32:(type==12 || type==14)?256:0;
}
static inline unsigned PioneerQ38BlockBytes(unsigned type)
{
	return type==0?4:type==8?34:type==12?144:type==14?210:0;
}
static inline bool PioneerQ38Expected(unsigned layer,unsigned role,PioneerQ38Tensor& out)
{
	struct Kind { const char* name; unsigned inputs,outputs,type,rank; };
	static const Kind common[]={
		{"attn_norm.weight",5120,1,0,1},{"post_attention_norm.weight",5120,1,0,1},
		{"ffn_gate.weight",5120,17408,12,2},{"ffn_up.weight",5120,17408,12,2},
		{"ffn_down.weight",17408,5120,12,2}};
	static const Kind delta[]={
		{"attn_gate.weight",5120,6144,8,2},{"attn_qkv.weight",5120,10240,8,2},
		{"ssm_a",48,1,0,1},{"ssm_dt.bias",48,1,0,1},{"ssm_norm.weight",128,1,0,1},
		{"ssm_alpha.weight",5120,48,8,2},{"ssm_beta.weight",5120,48,8,2},
		{"ssm_conv1d.weight",4,10240,0,2},{"ssm_out.weight",6144,5120,8,2}};
	static const Kind attention[]={
		{"attn_q.weight",5120,12288,8,2},{"attn_k.weight",5120,1024,8,2},
		{"attn_v.weight",5120,1024,8,2},{"attn_q_norm.weight",256,1,0,1},
		{"attn_k_norm.weight",256,1,0,1},{"attn_output.weight",6144,5120,14,2}};
	static const Kind special[]={
		{"token_embd.weight",5120,248320,12,2},{"output.weight",5120,248320,14,2},
		{"output_norm.weight",5120,1,0,1}};
	const Kind* kind=NULL;
	if (layer==64) { if (role>=3) return false; kind=&special[role]; out.name=kind->name; }
	else {
		if (layer>=64 || role>=(layer%4==3?11U:14U)) return false;
		kind=role<5?&common[role]:layer%4==3?&attention[role-5]:&delta[role-5];
		char name[96]; snprintf(name,sizeof(name),"blk.%u.%s",layer,kind->name); out.name=name;
	}
	out.inputs=kind->inputs; out.outputs=kind->outputs; out.type=kind->type; out.rank=kind->rank;
	out.bytes=uint64_t(out.inputs/PioneerQ38BlockElements(out.type))*PioneerQ38BlockBytes(out.type)*out.outputs;
	return true;
}

class PioneerQwen38Gguf {
	FILE* source=NULL;
	struct stat identity={};
	std::vector<PioneerQ38Tensor> tensors;
	std::vector<PioneerQ38Tensor> empty;
	std::map<std::string,unsigned> nameIndex;
	bool ready=false;
	uint64_t epoch=0;
	const PioneerQ38Tensor* Lookup(const char* name) const {
		if (!name) return NULL;
		auto found=nameIndex.find(name);
		return found==nameIndex.end()?NULL:&tensors[found->second];
	}
public:
	static const uint64_t FullBytes=UINT64_C(18973870528);
	static const unsigned MaxPackedRead=1024*1024;
	bool Ready() const { return ready; }
	uint64_t Epoch() const { return epoch; }
	const std::vector<PioneerQ38Tensor>& Tensors() const { return ready?tensors:empty; }
	const PioneerQ38Tensor* Find(const char* name) const { return ready?Lookup(name):NULL; }
	bool Stable() const {
		struct stat current={};
		return ready && source && !fstat(fileno(source),&current) && S_ISREG(current.st_mode)
			&& current.st_size==identity.st_size && current.st_ino==identity.st_ino && current.st_mtime==identity.st_mtime;
	}
	bool Open(FILE* file) {
		ready=false; source=NULL; tensors.clear(); nameIndex.clear();
		if (epoch==UINT64_MAX) return false;
		epoch++;
		if (!file || fstat(fileno(file),&identity) || !S_ISREG(identity.st_mode)
			|| uint64_t(identity.st_size)!=FullBytes || fseeko(file,0,SEEK_SET)) return false;
		PioneerGgufReader r(file,FullBytes);
		if (r.Number(4)!=0x46554747 || r.Number(4)!=3 || r.Number(8)!=851) return false;
		uint64_t count=r.Number(8); if (!r.good || count>100000) return false;
		struct Required { const char* key; unsigned value; };
		static const Required required[]={
			{"qwen35.block_count",64},{"qwen35.context_length",262144},{"qwen35.embedding_length",5120},
			{"qwen35.feed_forward_length",17408},{"qwen35.attention.head_count",24},{"qwen35.attention.head_count_kv",4},
			{"qwen35.attention.key_length",256},{"qwen35.attention.value_length",256},{"qwen35.ssm.conv_kernel",4},
			{"qwen35.ssm.state_size",128},{"qwen35.ssm.group_count",16},{"qwen35.ssm.time_step_rank",48},
			{"qwen35.ssm.inner_size",6144},{"qwen35.full_attention_interval",4},{"qwen35.rope.dimension_count",64}};
		bool fields[15]={},arch=false,name=false,epsilon=false,base=false,sections=false,recurrent=false;
		unsigned alignment=32; std::set<std::string> metadata;
		for (uint64_t i=0;i<count;i++) {
			char key[513]; if (!r.String(key,sizeof(key)) || !metadata.insert(key).second) return false;
			unsigned type=unsigned(r.Number(4)); bool handled=false;
			for (unsigned f=0;f<15;f++) if (!strcmp(key,required[f].key)) {
				if (type!=4 || r.Number(4)!=required[f].value) return false;
				fields[f]=true; handled=true; break;
			}
			if (handled) continue;
			if (!strcmp(key,"general.architecture") || !strcmp(key,"general.name")) {
				char value[64]; if (type!=8 || !r.String(value,sizeof(value))) return false;
				if (!strcmp(key,"general.architecture")) { if (strcmp(value,"qwen35")) return false; arch=true; }
				else { if (strcmp(value,"Qwen3.8-27B")) return false; name=true; }
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
			} else if (!strcmp(key,"qwen35.attention.recurrent_layers")) {
				if (type!=9 || r.Number(4)!=7 || r.Number(8)!=64) return false;
				for (unsigned j=0;j<64;j++) if (r.Number(1)!=unsigned(j%4!=3)) return false;
				recurrent=true;
			} else if (!r.Value(type)) return false;
		}
		if (!arch || !name || !epsilon || !base || !sections || !recurrent || !r.good
			|| !alignment || alignment>4096 || (alignment&(alignment-1))) return false;
		for (bool field:fields) if (!field) return false;
		for (unsigned i=0;i<851;i++) {
			char text[1025]; if (!r.String(text,sizeof(text)) || !nameIndex.emplace(text,unsigned(tensors.size())).second) return false;
			PioneerQ38Tensor entry; entry.name=text; entry.rank=unsigned(r.Number(4));
			if (entry.rank!=1 && entry.rank!=2) return false;
			entry.inputs=r.Number(8); entry.outputs=entry.rank==2?r.Number(8):1;
			entry.type=unsigned(r.Number(4)); entry.offset=r.Number(8);
			unsigned block=PioneerQ38BlockElements(entry.type),bytes=PioneerQ38BlockBytes(entry.type);
			if (!r.good || !block || !entry.inputs || entry.inputs>=UINT64_C(0x100000000)
				|| !entry.outputs || entry.outputs>=UINT64_C(0x100000000) || entry.inputs%block || entry.offset%alignment) return false;
			uint64_t rowBytes=entry.inputs/block*bytes;
			if (entry.outputs>UINT64_MAX/rowBytes) return false;
			entry.bytes=rowBytes*entry.outputs; tensors.push_back(entry);
		}
		uint64_t start=(r.position+alignment-1)&~uint64_t(alignment-1),end=start;
		if (!r.good || start>FullBytes) return false;
		std::vector<PioneerQ38Tensor*> byOffset;
		for (auto& entry:tensors) byOffset.push_back(&entry);
		std::sort(byOffset.begin(),byOffset.end(),[](const PioneerQ38Tensor* a,const PioneerQ38Tensor* b){return a->offset<b->offset;});
		for (auto* entry:byOffset) {
			if (entry->offset>FullBytes-start || entry->bytes>FullBytes-start-entry->offset) return false;
			entry->offset+=start;
			if (entry->offset<end) return false;
			end=entry->offset+entry->bytes;
		}
		for (unsigned layer=0;layer<=64;layer++) for (unsigned role=0;;role++) {
			PioneerQ38Tensor expected; if (!PioneerQ38Expected(layer,role,expected)) break;
			const auto* entry=Lookup(expected.name.c_str());
			if (!entry || entry->inputs!=expected.inputs || entry->outputs!=expected.outputs
				|| entry->type!=expected.type || entry->rank!=expected.rank || entry->bytes!=expected.bytes) return false;
		}
		source=file; ready=true;
		if (!Stable()) { ready=false; source=NULL; return false; }
		return true;
	}
	bool ReadPacked(unsigned tensor,uint64_t relative,unsigned bytes,void* destination,unsigned capacity) {
		if (!Stable() || tensor>=tensors.size() || !destination || !bytes || bytes>capacity || bytes>MaxPackedRead) return false;
		const auto& entry=tensors[tensor];
		if (relative>entry.bytes || bytes>entry.bytes-relative || fseeko(source,off_t(entry.offset+relative),SEEK_SET)) return false;
		return fread(destination,1,bytes,source)==bytes && Stable();
	}
	bool ReadRow(unsigned tensor,unsigned row,float* destination,unsigned capacity) {
		if (!ready || tensor>=tensors.size() || !destination) return false;
		const auto& entry=tensors[tensor];
		if (row>=entry.outputs || capacity<entry.inputs || entry.inputs>17408) return false;
		unsigned block=PioneerQ38BlockElements(entry.type),bytes=PioneerQ38BlockBytes(entry.type);
		unsigned rowBytes=unsigned(entry.bytes/entry.outputs);
		uint8_t packed[32768]; if (rowBytes>sizeof(packed) || !ReadPacked(tensor,uint64_t(row)*rowBytes,rowBytes,packed,sizeof(packed))) return false;
		for (unsigned at=0;at<entry.inputs;at+=block) {
			const uint8_t* p=packed+(at/block)*bytes; bool good=true;
			if (entry.type==0) {
				uint32_t bits=uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
				destination[at]=PioneerAttentionFloat(bits); good=(bits&0x7f800000)!=0x7f800000;
			} else if (entry.type==8) good=PioneerDecodeQ8_0(p,destination+at);
			else if (entry.type==12) good=PioneerDecodeQ4K(p,destination+at);
			else if (entry.type==14) good=PioneerDecodeQ6K(p,destination+at);
			else return false;
			if (!good) return false;
		}
		return true;
	}
};
#endif
