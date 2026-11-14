/* SPDX-License-Identifier: MIT */
#include "../PioneerFlashNextSource.h"
#include "ggml-quants.h"
#include <cassert>
#include <cstdio>
#include <random>
using namespace PioneerFlashNext;

static void Reference(unsigned type,const uint8_t* p,float* out,unsigned n)
{
	switch (type) {
		case 0: memcpy(out,p,n*4); break;
		case 1: ggml_fp16_to_fp32_row(reinterpret_cast<const ggml_fp16_t*>(p),out,n); break;
		case 30: ggml_bf16_to_fp32_row(reinterpret_cast<const ggml_bf16_t*>(p),out,n); break;
		case 7: dequantize_row_q5_1(reinterpret_cast<const block_q5_1*>(p),out,n); break;
		case 8: dequantize_row_q8_0(reinterpret_cast<const block_q8_0*>(p),out,n); break;
		case 12: dequantize_row_q4_K(reinterpret_cast<const block_q4_K*>(p),out,n); break;
		case 13: dequantize_row_q5_K(reinterpret_cast<const block_q5_K*>(p),out,n); break;
		case 14: dequantize_row_q6_K(reinterpret_cast<const block_q6_K*>(p),out,n); break;
		case 20: dequantize_row_iq4_nl(reinterpret_cast<const block_iq4_nl*>(p),out,n); break;
		default: assert(false);
	}
}
static void Quantization()
{
	std::mt19937 random(0x38f1a5);
	for (unsigned type:{0U,1U,7U,8U,12U,13U,14U,20U,30U}) {
		for (unsigned trial=0;trial<512;trial++) {
			alignas(16) uint8_t packed[256]={};
			for (auto& byte:packed) byte=uint8_t(random());
			unsigned n=BlockElements(type),d=type==14?208:0;
			if (type==0 || type==30) { float v=float(int(random()%1000)-500)/113; uint32_t b; memcpy(&b,&v,4);
				if (type==30) b>>=16;
				for (unsigned i=0;i<BlockBytes(type);i++) packed[i]=uint8_t(b>>(8*i));
			} else {
				packed[d+1]=uint8_t((random()%20)+10);
				if (type==7 || type==12 || type==13) packed[3]=uint8_t((random()%20)+10);
			}
			float actual[256]={},expected[256]={};
			assert(DecodeBlock(type,packed,actual)); Reference(type,packed,expected,n);
			for (unsigned i=0;i<n;i++) {
				if (actual[i]!=expected[i]) { fprintf(stderr,"format%u lane%u %.9g != %.9g\n",type,i,actual[i],expected[i]); assert(false); }
			}
			if (type==0) { packed[0]=0; packed[1]=0; packed[2]=0x80; packed[3]=0x7f; }
			else if (type==30) { packed[0]=0x80; packed[1]=0x7f; }
			else { packed[d]=0; packed[d+1]=0x7c; }
			assert(!DecodeBlock(type,packed,actual));
		}
	}
}

struct Weights : Source {
	std::vector<uint8_t> gate,up,down;
	uint64_t epoch=1;
	bool ready=true;
	mutable unsigned gets=0;
	Weights():gate(Hidden/256*144*Intermediate),up(gate.size()),down(Intermediate/32*24*Hidden) {
		for (size_t i=0;i<gate.size();i+=144) {
			gate[i+1]=0x14;
			for (unsigned g=0;g<4;g++) { gate[i+4+g]=1; gate[i+12+g]=1; }
			memset(gate.data()+i+16,0x11,128);
		}
		up=gate;
		for (size_t i=0;i<down.size();i+=24) { down[i+1]=0x14; memset(down.data()+i+8,0x11,16); }
	}
	bool Ready() const override { return ready; }
	uint64_t Epoch() const override { return epoch; }
	bool Get(Key key,Expert& e) const override {
		gets++;
		if (!key.Valid() || !ready) return false;
		e={{gate.data(),gate.size(),Hidden,Intermediate,12},
			{up.data(),up.size(),Hidden,Intermediate,12},
			{down.data(),down.size(),Intermediate,Hidden,7}};
		return true;
	}
	uint64_t Bytes() const { return gate.size()+up.size()+down.size(); }
};
struct Card : Device {
	std::vector<Key> keys;
	unsigned begins=0,uploads=0,runs=0,ends=0,quarantines=0;
	unsigned routedRuns=0;
	unsigned fail=0; // 1 begin,2 upload,3 run,4 partial output,5 end,6 source epoch
	Weights* source=nullptr;
	bool routedBatch=false;
	bool Begin(uint64_t bytes,uint64_t& generation) override {
		assert(bytes); begins++; generation=19; return fail!=1;
	}
	bool Upload(uint64_t gen,unsigned slot,Key key,const Expert& e) override {
		assert(gen==19 && slot==keys.size() && e.Valid()); keys.push_back(key); uploads++; return fail!=2;
	}
	bool Run(uint64_t gen,unsigned slot,const float* input,unsigned n,float* out,unsigned m) override {
		assert(gen==19 && slot<keys.size() && n==Hidden && m==Hidden); runs++;
		for (unsigned i=0;i<(fail==4?Hidden-1:Hidden);i++) out[i]=float(keys[slot].expert+1)*input[0];
		if (fail==6) source->epoch++;
		return fail!=3;
	}
	bool SupportsRoutedBatch() const override { return routedBatch; }
	bool RunRouted(uint64_t gen,const unsigned* slots,const float* weights,const float* input,float* out) override {
		assert(routedBatch && gen==19 && slots && weights && input && out); routedRuns++;
		for(unsigned n=0;n<Hidden;n++)out[n]=0;
		for(unsigned i=0;i<Active;i++){
			assert(slots[i]<keys.size());
			for(unsigned n=0;n<Hidden;n++)out[n]+=weights[i]*float(keys[slots[i]].expert+1)*input[0];
		}
		if(fail==4)out[Hidden-1]=std::numeric_limits<float>::quiet_NaN();
		return fail!=3;
	}
	bool End(uint64_t gen) override { assert(gen==19); ends++; return fail!=5; }
	void Quarantine(uint64_t) override { quarantines++; }
};
static std::array<Route,Active> Routes()
{
	std::array<Route,Active> routes;
	for (unsigned i=0;i<Active;i++) routes[i]={i,0.1f};
	return routes;
}
static void Execution()
{
	Weights source; Expert expert; assert(source.Get({0,0},expert));
	std::vector<float> x(Hidden,0),actual(Hidden,-999); x[0]=1;
	// An independent all-constant matrix oracle, with real Flash-Next sizes
	// and Q4_K gate/up + Q5_1 down. No device arithmetic is being simulated.
	double a=1.0/1024,expected=Intermediate*(a/(1+exp(-a))*a)*a;
	assert(HostExpert(expert,x.data(),actual.data()));
	for (float value:actual) assert(fabs(value-expected)<1e-10);
	assert(!Multiply(expert.gate,x.data(),x.data()));
	Matrix invalid=expert.gate; invalid.bytes--; assert(!Multiply(invalid,x.data(),actual.data()));
	invalid=expert.down; invalid.type=12; assert(!invalid.Valid()); // 640 is not Q4_K block-aligned
	auto routes=Routes(); Card card; Offload mixed;
	assert(mixed.Open(source,&card,{{0,0},{0,9}},2*source.Bytes()));
	assert(mixed.Routed(0,routes.data(),Active,x.data(),actual.data()));
	for (float value:actual) assert(fabs(value-(1.1+0.8*expected))<1e-6);
	auto stats=mixed.Snapshot();
	assert(stats.hostExperts==8 && stats.deviceExperts==2 && stats.routedLayers==1);
	assert(stats.hostPackedBytes==8*source.Bytes() && stats.uploadedBytes==2*source.Bytes());
	assert(mixed.Close() && mixed.Close() && card.ends==1 && !card.quarantines);
	assert(!mixed.Routed(0,routes.data(),Active,x.data(),actual.data()));
	std::vector<Key> allRoutes; for(unsigned i=0;i<Active;i++)allRoutes.push_back({0,i});
	Card fusedCard;fusedCard.routedBatch=true;Offload fused;
	assert(fused.Open(source,&fusedCard,allRoutes,Active*source.Bytes()));
	assert(fused.Routed(0,routes.data(),Active,x.data(),actual.data()));
	for(float value:actual)assert(fabs(value-5.5f)<1e-6);
	auto fusedStats=fused.Snapshot();
	assert(fusedStats.hostExperts==0 && fusedStats.deviceExperts==Active
		&& fusedStats.routedLayers==1 && fusedCard.routedRuns==1 && fusedCard.runs==0);
	assert(fused.Close() && fusedCard.ends==1 && !fusedCard.quarantines);
	Card serialCard;Offload serial;
	assert(serial.Open(source,&serialCard,allRoutes,Active*source.Bytes()));
	assert(serial.Routed(0,routes.data(),Active,x.data(),actual.data()));
	auto serialStats=serial.Snapshot();
	assert(serialStats.hostExperts==0 && serialStats.deviceExperts==Active && serialCard.runs==Active
		&& serialCard.routedRuns==0);
	assert(serial.Close());
	Offload host;
	assert(host.Open(source,nullptr,{},0));
	assert(host.Routed(47,routes.data(),Active,x.data(),actual.data()));
	for (float value:actual) assert(fabs(value-expected)<1e-10);
	assert(host.Close());
	Card closingCard;
	{ Offload automatic; assert(automatic.Open(source,&closingCard,{{0,0}},source.Bytes())); }
	assert(closingCard.ends==1 && !closingCard.quarantines);
}
static void Faults()
{
	Weights source; auto routes=Routes(); std::vector<float> x(Hidden,0),out(Hidden,-77); x[0]=1;
	for (unsigned mode=1;mode<=6;mode++) {
		Card card; card.fail=mode; card.source=&source; Offload runtime;
		bool opened=runtime.Open(source,&card,{{0,0}},source.Bytes());
		if (mode<=2) assert(!opened);
		else {
			assert(opened);
			if (mode==5) assert(!runtime.Close());
			else assert(!runtime.Routed(0,routes.data(),Active,x.data(),out.data()));
		}
		for (float value:out) assert(value==-77);
		assert(runtime.Snapshot().faulted && card.quarantines==1 && !runtime.Close());
		unsigned runs=card.runs;
		assert(!runtime.Routed(0,routes.data(),Active,x.data(),out.data()) && card.runs==runs);
	}
	std::vector<Key> allRoutes; for(unsigned i=0;i<Active;i++)allRoutes.push_back({0,i});
	for(unsigned mode:{3U,4U}){
		Card card;card.routedBatch=true;card.fail=mode;Offload runtime;
		assert(runtime.Open(source,&card,allRoutes,Active*source.Bytes()));
		assert(!runtime.Routed(0,routes.data(),Active,x.data(),out.data()));
		for(float value:out)assert(value==-77);
		assert(runtime.Snapshot().faulted && card.routedRuns==1 && card.runs==0
			&& card.quarantines==1 && !runtime.Close());
	}
	Card card; Offload tooSmall,duplicate;
	assert(!tooSmall.Open(source,&card,{{0,0}},source.Bytes()-1) && !card.begins);
	assert(!duplicate.Open(source,&card,{{0,0},{0,0}},2*source.Bytes()) && !card.begins);
	Offload valid; assert(valid.Open(source,&card,{{0,0}},source.Bytes()));
	auto bad=routes; bad[1].expert=0;
	assert(!valid.Routed(0,bad.data(),Active,x.data(),out.data()));
	bad=routes; bad[9].expert=Experts; assert(!valid.Routed(0,bad.data(),Active,x.data(),out.data()));
	bad=routes; bad[9].weight=-.1f; assert(!valid.Routed(0,bad.data(),Active,x.data(),out.data()));
	bad=routes; bad[9].weight=std::numeric_limits<float>::quiet_NaN(); assert(!valid.Routed(0,bad.data(),Active,x.data(),out.data()));
	bad=routes; bad[9].weight=.2f; assert(!valid.Routed(0,bad.data(),Active,x.data(),out.data()));
	assert(!valid.Routed(48,routes.data(),Active,x.data(),out.data()));
	x[0]=INFINITY; assert(!valid.Routed(0,routes.data(),Active,x.data(),out.data())); x[0]=1;
	assert(!card.runs && !valid.Snapshot().faulted && valid.Close());
	Card cancelledCard; Offload cancelled; assert(cancelled.Open(source,&cancelledCard,{{0,0}},source.Bytes()));
	unsigned boundaries=0;
	auto stop=[](void* opaque) { return ++*static_cast<unsigned*>(opaque)>1; };
	assert(!cancelled.Routed(0,routes.data(),Active,x.data(),out.data(),stop,&boundaries));
	assert(cancelledCard.runs==1 && !cancelled.Snapshot().faulted && cancelled.Close());
	Offload stale; assert(stale.Open(source,nullptr,{},0)); source.epoch++;
	assert(!stale.Routed(0,routes.data(),Active,x.data(),out.data()) && stale.Snapshot().faulted);
	Offload corrupt; assert(corrupt.Open(source,nullptr,{},0)); source.gate[1]=0x7c;
	assert(!corrupt.Routed(0,routes.data(),Active,x.data(),out.data()) && corrupt.Snapshot().faulted);
	for (float value:out) assert(value==-77);
}
struct RangeOwner : WeightOwner {
	uintptr_t base=0;
	uint64_t bytes=0,epoch=1;
	bool authenticated=true;
	bool Authenticated() const override { return authenticated; }
	uint64_t Epoch() const override { return epoch; }
	bool Contains(const uint8_t* p,uint64_t n) const override {
		uintptr_t value=reinterpret_cast<uintptr_t>(p);
		return value>=base && value-base<=bytes && n<=bytes-(value-base);
	}
};
static void SourceBindings()
{
	// Synthetic address extents ONLY, never dereferenced and never asserted to
	// be real authenticated files. Test the mapping/offset authorization seam
	// without allocating a 77 GB model or touching physical hardware.
	RangeOwner owner; owner.base=UINT64_C(0x1000000000);
	std::vector<Tensor> tensors;
	uintptr_t at=owner.base;
	for (unsigned layer=0;layer<Layers;layer++) for (unsigned role=0;role<3;role++) {
		const char* roles[]={"gate","up","down"};
		Tensor t; t.name="blk."+std::to_string(layer)+".ffn_"+roles[role]+"_exps.weight";
		t.type=role==2?7:12; t.rank=3; t.shape[0]=role==2?Intermediate:Hidden;
		t.shape[1]=role==2?Hidden:Intermediate; t.shape[2]=Experts;
		t.bytes=t.shape[0]/BlockElements(t.type)*BlockBytes(t.type)*t.shape[1]*Experts;
		t.data=reinterpret_cast<const uint8_t*>(at); at+=t.bytes;
		tensors.push_back(t);
	}
	owner.bytes=at-owner.base;
	PackedSource source;
	assert(source.Bind("qwen4exp",{&owner},tensors,77) && source.Ready() && source.Epoch()==77);
	for (unsigned layer=0;layer<Layers;layer++) for (unsigned id:{0U,1U,511U}) {
		Expert e; assert(source.Get({layer,id},e) && e.Valid());
		assert(reinterpret_cast<uintptr_t>(e.gate.data)==reinterpret_cast<uintptr_t>(tensors[layer*3].data)+id*e.gate.bytes);
		assert(reinterpret_cast<uintptr_t>(e.down.data)==reinterpret_cast<uintptr_t>(tensors[layer*3+2].data)+id*e.down.bytes);
	}
	Expert e; assert(!source.Get({48,0},e) && !source.Get({0,512},e));
	for (unsigned mode=0;mode<8;mode++) {
		auto bad=tensors;
		if (mode==0) bad.pop_back();
		if (mode==1) bad.push_back(bad[0]);
		if (mode==2) bad[0].rank=2;
		if (mode==3) bad[0].shape[2]=511;
		if (mode==4) bad[0].bytes--;
		if (mode==5) bad[1].data=bad[0].data;
		if (mode==6) bad[0].data=reinterpret_cast<const uint8_t*>(UINTPTR_MAX-64);
		if (mode==7) bad[0].shard=1;
		PackedSource rejected; assert(!rejected.Bind("qwen4exp",{&owner},bad,88) && !rejected.Ready());
	}
	owner.epoch++; assert(!source.Ready() && !source.Get({0,0},e));
	PackedSource wrong; assert(!wrong.Bind("qwen35",{&owner},tensors,1));
	owner.authenticated=false;
	PackedSource unverified; assert(!unverified.Bind("qwen4exp",{&owner},tensors,1));
}
static void Ngram()
{
	std::vector<uint8_t> packed(32*90);
	for (unsigned block=0;block<32*5;block++) {
		packed[block*18+1]=0x30;
		for (unsigned i=0;i<16;i++) packed[block*18+2+i]=uint8_t(block+i);
	}
	RangeOwner owner; owner.base=reinterpret_cast<uintptr_t>(packed.data()); owner.bytes=packed.size();
	Tensor tensor; tensor.name="per_layer_token_embd.weight"; tensor.data=packed.data(); tensor.bytes=packed.size();
	tensor.type=20; tensor.rank=2; tensor.shape[0]=160; tensor.shape[1]=32;
	std::array<uint64_t,16> offsets,vocabs; std::array<uint32_t,16> rows;
	for (unsigned i=0;i<16;i++) { offsets[i]=i*2; vocabs[i]=2; rows[i]=i*2+1; }
	NgramTable table; assert(table.Bind(owner,tensor,offsets,vocabs) && table.PackedBytesPerGather()==1440);
	std::vector<float> output(2560,-77),reference(2560);
	assert(table.Gather(rows.data(),16,output.data(),2560));
	for (unsigned i=0;i<16;i++) Reference(20,packed.data()+rows[i]*90,reference.data()+i*160,160);
	assert(output==reference);
	std::fill(output.begin(),output.end(),-77);
	rows[0]=2; assert(!table.Gather(rows.data(),16,output.data(),2560)); rows[0]=1;
	assert(!table.Gather(rows.data(),15,output.data(),2560) && !table.Gather(rows.data(),16,output.data(),2559));
	packed[90+1]=0x7c; assert(!table.Gather(rows.data(),16,output.data(),2560)); packed[90+1]=0x30;
	owner.epoch++; assert(!table.Ready() && !table.Gather(rows.data(),16,output.data(),2560));
	for (float v:output) assert(v==-77);
	NgramTable overlap; offsets[1]=0; assert(!overlap.Bind(owner,tensor,offsets,vocabs));
}
int main()
{
	// Initialize upstream's FP16 table on platforms that use it.
	ggml_init_params params={1024*1024,nullptr,true}; auto* context=ggml_init(params); assert(context);
	Quantization(); Execution(); Faults(); SourceBindings(); Ngram(); ggml_free(context);
	puts("PASS: 4608 blocks against pristine GGML, real-size Q4/Q5 host expert oracle, mixed placement, all-resident one-call contract/faults, bounded residency, mapped-source slices, PLE gathers, cancellation and lifecycle faults; NO P100A execution");
}
