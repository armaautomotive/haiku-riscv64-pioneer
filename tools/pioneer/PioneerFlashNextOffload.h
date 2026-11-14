/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_FLASH_NEXT_OFFLOAD_H
#define PIONEER_FLASH_NEXT_OFFLOAD_H

#include "TenstorrentGgufQuant.h"
#include <algorithm>
#include <array>
#include <limits>
#include <vector>

// User-space foundation, NOT an installed driver or a complete Flash-Next
// decoder. Immutable authenticated mmap/source owners and the device session
// must outlive this object. No raw device addresses or implicit CPU fallback.
namespace PioneerFlashNext {
constexpr unsigned Layers=48, Experts=512, Active=10, Hidden=2560, Intermediate=640;

inline unsigned BlockElements(unsigned type)
{
	switch (type) {
		case 0: case 1: case 30: return 1;
		case 7: case 8: case 20: return 32;
		case 12: case 13: case 14: return 256;
		default: return 0;
	}
}
inline unsigned BlockBytes(unsigned type)
{
	switch (type) {
		case 0: return 4;
		case 1: case 30: return 2;
		case 7: return 24;
		case 8: return 34;
		case 20: return 18;
		case 12: return 144;
		case 13: return 176;
		case 14: return 210;
		default: return 0;
	}
}
inline uint32_t Little(const uint8_t* p,unsigned n)
{
	uint32_t value=0; for (unsigned i=0;i<n;i++) value|=uint32_t(p[i])<<(8*i); return value;
}
inline float Float(uint32_t bits) { float value; memcpy(&value,&bits,4); return value; }

// Actual UD-Q4_K_XL uses Q5_1 down experts, Q4_K/Q5_K gate/up,
// IQ4_NL PLE and F32/Q8_0/BF16 shared tensors. Preserve their binary data.
// Binary layout references: upstream ggml-common.h and ggml-quants.c.
inline bool DecodeBlock(unsigned type,const uint8_t* p,float* values)
{
	if (!p || !values || !BlockElements(type)) return false;
	if (type==12) return PioneerDecodeQ4K(p,values);
	if (type==14) return PioneerDecodeQ6K(p,values);
	if (type==8) return PioneerDecodeQ8_0(p,values);
	if (type==0 || type==1 || type==30) {
		values[0]=type==0?Float(Little(p,4)):type==30?Float(Little(p,2)<<16):PioneerHalfFloat(uint16_t(Little(p,2)));
		return isfinite(values[0]);
	}
	float d=PioneerHalfFloat(uint16_t(Little(p,2)));
	if (!isfinite(d)) return false;
	if (type==7) {
		float minimum=PioneerHalfFloat(uint16_t(Little(p+2,2)));
		if (!isfinite(minimum)) return false;
		uint32_t high=Little(p+4,4);
		for (unsigned i=0;i<32;i++) {
			unsigned low=(p[8+i%16]>>((i/16)*4))&15;
			values[i]=d*(low|(((high>>i)&1)<<4))+minimum;
		}
		return true;
	}
	if (type==20) {
		static const int table[]={-127,-104,-83,-65,-49,-35,-22,-10,1,13,25,38,53,69,89,113};
		for (unsigned i=0;i<32;i++) values[i]=d*table[(p[2+i%16]>>((i/16)*4))&15];
		return true;
	}
	if (type==13) {
		float minimum=PioneerHalfFloat(uint16_t(Little(p+2,2)));
		if (!isfinite(minimum)) return false;
		for (unsigned g=0;g<8;g++) {
			unsigned scale=g<4?p[4+g]&63:(p[8+g]&15)|((p[g]>>6)<<4);
			unsigned min=g<4?p[8+g]&63:(p[8+g]>>4)|((p[4+g]>>6)<<4);
			for (unsigned i=0;i<32;i++) {
				unsigned low=(p[48+(g/2)*32+i]>>((g%2)*4))&15;
				unsigned high=((p[16+i]>>g)&1)<<4;
				values[g*32+i]=(d*scale)*(low|high)-minimum*min;
			}
		}
		return true;
	}
	return false;
}

struct Matrix {
	const uint8_t* data=nullptr;
	uint64_t bytes=0;
	unsigned inputs=0,outputs=0,type=0;
	bool Valid() const {
		unsigned block=BlockElements(type),packed=BlockBytes(type);
		if (!data || !block || !packed || !inputs || inputs>10240 || !outputs || outputs>248320 || inputs%block) return false;
		return bytes==uint64_t(inputs/block)*packed*outputs && bytes<=SIZE_MAX
			&& reinterpret_cast<uintptr_t>(data)<=UINTPTR_MAX-bytes;
	}
};
struct Expert {
	Matrix gate,up,down;
	bool Valid() const {
		return gate.Valid() && up.Valid() && down.Valid()
			&& gate.inputs==Hidden && up.inputs==Hidden && down.inputs==Intermediate
			&& gate.outputs==Intermediate && up.outputs==Intermediate && down.outputs==Hidden;
	}
	uint64_t Bytes() const { return gate.bytes+up.bytes+down.bytes; }
};
struct Key {
	unsigned layer=0,expert=0;
	bool Valid() const { return layer<Layers && expert<Experts; }
	unsigned Index() const { return layer*Experts+expert; }
};
struct Route { unsigned expert=0; float weight=0; };

inline bool Disjoint(const void* a,uint64_t an,const void* b,uint64_t bn)
{
	uintptr_t x=reinterpret_cast<uintptr_t>(a),y=reinterpret_cast<uintptr_t>(b);
	if (an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y) return false;
	return x<=y?an<=y-x:bn<=x-y;
}

// Host math for explicitly offloaded experts. Only a single packed block is
// expanded at a time; no full matrix/model expansion or activation quantizing.
// Output is private scratch; the outer token transaction publishes on success.
inline bool Multiply(const Matrix& m,const float* input,float* output)
{
	if (!m.Valid() || !input || !output
		|| !Disjoint(input,uint64_t(m.inputs)*4,output,uint64_t(m.outputs)*4)
		|| !Disjoint(m.data,m.bytes,output,uint64_t(m.outputs)*4)) return false;
	for (unsigned i=0;i<m.inputs;i++) if (!isfinite(input[i])) return false;
	unsigned elements=BlockElements(m.type),packed=BlockBytes(m.type);
	uint64_t rowBytes=uint64_t(m.inputs/elements)*packed;
	for (unsigned row=0;row<m.outputs;row++) {
		double sum=0;
		for (unsigned col=0;col<m.inputs;col+=elements) {
			float values[256];
			if (!DecodeBlock(m.type,m.data+row*rowBytes+(col/elements)*packed,values)) return false;
			for (unsigned lane=0;lane<elements;lane++) sum+=double(values[lane])*input[col+lane];
		}
		output[row]=float(sum);
		if (!isfinite(output[row])) return false;
	}
	return true;
}
inline bool HostExpert(const Expert& e,const float* input,float* output)
{
	if (!e.Valid() || !input || !output) return false;
	std::vector<float> gate(Intermediate),up(Intermediate);
	if (!Multiply(e.gate,input,gate.data()) || !Multiply(e.up,input,up.data())) return false;
	for (unsigned i=0;i<Intermediate;i++) {
		float x=gate[i],z=expf(x>=0?-x:x);
		gate[i]=(x>=0?x/(1+z):x*z/(1+z))*up[i];
		if (!isfinite(gate[i])) return false;
	}
	return Multiply(e.down,gate.data(),output);
}

class Source {
public:
	virtual ~Source()=default;
	// Ready must include the caller's whole-shard authentication and retained
	// immutable backing ownership. Layout inspection alone cannot return true.
	virtual bool Ready() const=0;
	virtual uint64_t Epoch() const=0;
	virtual bool Get(Key key,Expert& expert) const=0;
};
class Device {
public:
	virtual ~Device()=default;
	// Future native adapter owns exclusive BEGIN/END and translates logical
	// slots to driver-private placements. No current driver implements this.
	virtual bool Begin(uint64_t bytes,uint64_t& generation)=0;
	virtual bool Upload(uint64_t generation,unsigned slot,Key key,const Expert& expert)=0;
	virtual bool Run(uint64_t generation,unsigned slot,const float* input,unsigned inputs,float* output,unsigned outputs)=0;
	// Optional one-submission execution of all ten already-resident routed
	// experts. The adapter must combine weighted expert outputs on-device;
	// returning true must mean this was one device transaction, not a host loop.
	virtual bool SupportsRoutedBatch() const { return false; }
	virtual bool RunRouted(uint64_t,const unsigned*,const float*,const float*,float*) { return false; }
	virtual bool End(uint64_t generation)=0;
	// A failed or uncertain device operation must retain/quarantine its leases.
	virtual void Quarantine(uint64_t generation)=0;
};
struct Stats {
	uint64_t hostExperts=0,deviceExperts=0,hostPackedBytes=0,uploadedBytes=0;
	uint64_t routedLayers=0,residentBytes=0;
	unsigned residentExperts=0;
	bool faulted=false,cleanEnd=false;
};

class Offload {
	enum Phase { Empty,Ready,Faulted,Closed } phase=Empty;
	const Source* source=nullptr;
	Device* device=nullptr;
	uint64_t epoch=0,generation=0;
	bool session=false;
	std::vector<int> slots;
	// Reuse per-layer scratch across tokens instead of allocating two 2560-float
	// vectors on every routed-layer call.
	std::array<float,Hidden> resultScratch{},expertScratch{};
	Stats stats;
	bool Stable() const { return source && source->Ready() && source->Epoch()==epoch; }
	bool Fail() {
		if (phase!=Faulted && session) device->Quarantine(generation);
		phase=Faulted; stats.faulted=true; return false;
	}
public:
	Offload()=default;
	Offload(const Offload&)=delete;
	Offload& operator=(const Offload&)=delete;
	~Offload() { if (phase==Ready) Close(); }
	// Explicit static residency: nonresident experts execute on the host.
	// Profiling can choose the pinned IDs; no speculative weight streaming or
	// automatic CPU retry after a device error. A device may opt into one-call
	// routed execution only when all ten selected experts are resident. Budget is expert arena only;
	// shared weights and decoder state must already be reserved separately.
	bool Open(const Source& weights,Device* card,const std::vector<Key>& resident,uint64_t expertBudget) {
		if (phase!=Empty || !weights.Ready() || !weights.Epoch() || resident.size()>Layers*Experts
			|| expertBudget>UINT64_C(28)*1024*1024*1024 || (!resident.empty() && !card)) return false;
		std::vector<int> proposed(Layers*Experts,-1);
		std::vector<Expert> staged;
		staged.reserve(resident.size());
		uint64_t bytes=0,sourceEpoch=weights.Epoch();
		for (unsigned slot=0;slot<resident.size();slot++) {
			Key key=resident[slot]; Expert e;
			if (!key.Valid() || proposed[key.Index()]!=-1 || !weights.Get(key,e) || !e.Valid()
				|| e.Bytes()>expertBudget-bytes) return false;
			proposed[key.Index()]=int(slot); bytes+=e.Bytes(); staged.push_back(e);
		}
		if (!weights.Ready() || weights.Epoch()!=sourceEpoch) return false;
		source=&weights; epoch=sourceEpoch; device=card; slots.swap(proposed);
		if (!resident.empty()) {
			// Treat even a failed BEGIN as uncertain: the adapter must account
			// for partial acquisition rather than infer absence of ownership.
			session=true;
			if (!device->Begin(bytes,generation) || !generation || !Stable()) return Fail();
			for (unsigned slot=0;slot<resident.size();slot++) {
				const Expert& e=staged[slot];
				if (!Stable()
					|| !device->Upload(generation,slot,resident[slot],e) || !Stable()) return Fail();
				stats.uploadedBytes+=e.Bytes();
			}
		}
		stats.residentBytes=bytes; stats.residentExperts=unsigned(resident.size()); phase=Ready; return true;
	}
	bool Routed(unsigned layer,const Route* routes,unsigned count,const float* input,float* output,
		bool (*cancelled)(void*)=nullptr,void* cancellationData=nullptr) {
		if (phase!=Ready) return false;
		if (!Stable()) return Fail();
		if (layer>=Layers || !routes || count!=Active || !input || !output) return false;
		bool selected[Experts]={}; double sum=0;
		for (unsigned i=0;i<count;i++) {
			const auto& r=routes[i];
			if (r.expert>=Experts || selected[r.expert] || !isfinite(r.weight) || r.weight<0) return false;
			selected[r.expert]=true; sum+=r.weight;
		}
		if (fabs(sum-1)>1e-5) return false; // graph supplies normalized top-10 weights
		for (unsigned i=0;i<Hidden;i++) if (!isfinite(input[i])) return false;
		auto& result=resultScratch; auto& one=expertScratch;
	std::fill(result.begin(),result.end(),0.0f);
		unsigned residentSlots[Active]; float routeWeights[Active]; bool allResident=device!=nullptr;
		for (unsigned i=0;i<count;i++) {
			Key key{layer,routes[i].expert}; int slot=slots[key.Index()];
			if (slot<0) allResident=false;
			else residentSlots[i]=unsigned(slot);
			routeWeights[i]=routes[i].weight;
		}
		const bool routedBatch=allResident && device->SupportsRoutedBatch();
		if (routedBatch) {
			if (!Stable()) return Fail();
			if (cancelled && cancelled(cancellationData)) return false;
			std::fill(result.begin(),result.end(),std::numeric_limits<float>::quiet_NaN());
			if (!device->RunRouted(generation,residentSlots,routeWeights,input,result.data()) || !Stable()) return Fail();
			for (float value:result) if (!isfinite(value)) return Fail();
			stats.deviceExperts+=count;
		} else for (unsigned i=0;i<count;i++) {
			if (!Stable()) return Fail();
			if (cancelled && cancelled(cancellationData)) return false;
			Key key{layer,routes[i].expert}; int slot=slots[key.Index()];
			std::fill(one.begin(),one.end(),std::numeric_limits<float>::quiet_NaN());
			if (slot>=0) {
				if (!device->Run(generation,unsigned(slot),input,Hidden,one.data(),Hidden) || !Stable()) return Fail();
				stats.deviceExperts++;
			} else {
				Expert e;
				if (!source->Get(key,e) || !e.Valid() || !Stable() || !HostExpert(e,input,one.data()) || !Stable()) return Fail();
				stats.hostExperts++; stats.hostPackedBytes+=e.Bytes();
			}
			for (unsigned n=0;n<Hidden;n++) {
				if (!isfinite(one[n])) return Fail();
				result[n]+=routes[i].weight*one[n];
				if (!isfinite(result[n])) return Fail();
			}
		}
		if (!Stable()) return Fail();
		if (cancelled && cancelled(cancellationData)) return false;
		// Routed contribution only. The graph must still add the sigmoided
		// shared expert, gated residuals, attention/GDN/PLE and commit token state.
		memcpy(output,result.data(),Hidden*sizeof(float)); stats.routedLayers++; return true;
	}
	bool Close() {
		if (phase==Closed) return stats.cleanEnd;
		if (phase!=Ready) return false;
		if (session && !device->End(generation)) return Fail();
		session=false; phase=Closed; stats.cleanEnd=true; return true;
	}
	Stats Snapshot() const { return stats; }
};
} // namespace PioneerFlashNext
#endif
