/* SPDX-License-Identifier: MIT */
#define PIONEER_ATTENTION_FREESTANDING
#include "PioneerQwen35DeviceIO.h"
#include "PioneerQwen35Embedding.h"
#include "PioneerQwen35RootImageLayout.h"
#include "PioneerQ38ProjectionInput.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38ProjectionInputShape.h"

extern "C" unsigned q35_work_matrix_band();

namespace PioneerQwen35 {
namespace {
enum : unsigned {
	Niu=0xffb20000, Bounce=RootBounceAddress, BounceBytes=RootBounceBytes,
	RootRoute=129, Activation=0xa0000, Scales=0x94000, MatrixJob=0x74000, MatrixResult=0x84000
};
constexpr unsigned CounterOffset[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x228};

volatile uint32_t* Words(unsigned address) { return reinterpret_cast<volatile uint32_t*>(address); }
void Fence() { asm volatile("fence" ::: "memory"); }
unsigned Counter(unsigned i) { return *Words(Niu+CounterOffset[i]); }

bool Quiet()
{
	for (unsigned i=0;i<4;i++)
		if (*Words(Niu+0x40+i*0x800)) return false;
	for (unsigned i=0;i<32;i++)
		if (*Words(Niu+0x240+i*4)&255) return false;
	return true;
}

bool Program(const uint32_t* command)
{
	if (!Quiet() || (command[5]!=RootRoute && command[2]!=RootRoute))
		return false;
	for (unsigned i=0;i<10;i++) Words(Niu)[i]=command[i];
	Fence();
	for (unsigned i=0;i<10;i++)
		if (Words(Niu)[i]!=command[i]) return false;
	return true;
}

bool Matches(const uint32_t* command)
{
	if (!Quiet()) return false;
	for (unsigned i=0;i<10;i++)
		if (Words(Niu)[i]!=command[i]) return false;
	return true;
}

bool InBank(const Range& range)
{
	return range.bank<Banks && range.address>=BankBase && range.address<BankLimit
		&& range.bytes && range.bytes<=BankLimit-range.address && !((range.address|range.bytes)&63);
}

__UINT64_TYPE__ Cycles()
{
	for (unsigned i=0;i<16;i++) {
		uint32_t hi, lo, check;
		asm volatile("csrrs %0,0xb80,x0\ncsrrs %1,0xb00,x0\ncsrrs %2,0xb80,x0"
			: "=&r"(hi),"=&r"(lo),"=&r"(check) :: "memory");
		if (hi==check) return (static_cast<__UINT64_TYPE__>(hi)<<32)|lo;
	}
	return 0;
}
}

DeviceIO::DeviceIO(const DeviceLayout& layout,Range ropeTable,volatile RequestProgress& status,
	volatile uint32_t* generatedIds)
	: model(layout), rope(ropeTable), progress(status), tokens(generatedIds), started(Cycles())
{
}

bool DeviceIO::Ready() const
{
	return started && tokens && model.magic==LayoutMagic && model.version==LayoutVersion
		&& model.context && model.context<=MaxPositions && !model.reserved
		&& InBank(rope) && rope.bytes==model.context*Rotary*4 && Quiet()
		&& *Words(Niu+0x14)==RootRoute;
}

bool DeviceIO::TransferWords(Range range,void* local,bool write)
{
	if (!InBank(range) || !local || (reinterpret_cast<__UINTPTR_TYPE__>(local)&3)) return false;
	auto* localBytes=static_cast<unsigned char*>(local);
	unsigned route=PioneerQ38ProjectionRoute(range.bank);
	for (unsigned offset=0;offset<range.bytes;) {
		unsigned bytes=range.bytes-offset;
		if (bytes>BounceBytes) bytes=BounceBytes;
		unsigned address=range.address+offset;
		auto* buffer=Words(Bounce);
		if (write)
			for (unsigned at=0;at<bytes;at+=4) {
				uint32_t bits;
				__builtin_memcpy(&bits,localBytes+offset+at,4);
				buffer[at/4]=bits;
			}
		const uint32_t command[]={write?Bounce:address,0,write?RootRoute:route,
			write?address:Bounce,0,write?route:RootRoute,0,write?0x212U:0,bytes,0};
		if (!Program(command)) return false;
		const unsigned increment[]={write?0U:1U,write?0U:bytes/64,1,write?0U:1U,
			write?1U:0U,write?bytes/64:0U,write?1U:0U};
		uint32_t expected[7];
		for (unsigned i=0;i<7;i++) expected[i]=Counter(i)+increment[i];
		*Words(Niu+0x40)=1;
		(void)*Words(Niu+0x40);
		Fence();
		bool done=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
			done=!*Words(Niu+0x40);
			for (unsigned i=0;i<7;i++) done &= Counter(i)==expected[i];
			if (done) break;
		}
		if (!done || !Matches(command)) return false;
		Fence();
		if (!write)
			for (unsigned at=0;at<bytes;at+=4) {
				uint32_t bits=buffer[at/4];
				__builtin_memcpy(localBytes+offset+at,&bits,4);
			}
		offset+=bytes;
	}
	return true;
}

bool DeviceIO::Transfer(Range range,void* local) { return TransferWords(range,local,false); }
bool DeviceIO::Write(Range range,const void* local) { return TransferWords(range,const_cast<void*>(local),true); }

bool DeviceIO::Embed(unsigned token,float* hidden,unsigned values)
{
	namespace E=PioneerQwen35Embedding;
	Range range;
	unsigned lane;
	if (values!=Hidden || !model.ResolveEmbedding(token,range,lane) || range.bytes!=E::GroupBytes)
		return false;
	auto* tile=reinterpret_cast<unsigned char*>(0x90000);
	for (unsigned block=0;block<E::Blocks;block++)
		if (!Transfer({range.bank,range.address+block*E::TileBytes,E::TileBytes},tile)
			|| !E::DecodeBlock(tile,E::TileBytes,lane,hidden+block*E::BlockValues,E::BlockValues))
			return false;
	return true;
}

bool DeviceIO::ReadWeight(unsigned layer,unsigned role,unsigned offset,float* output,unsigned values)
{
	const auto* tensor=model.Weight(layer,role);
	if (!tensor || tensor->type || values>Hidden || offset>ConvChannels*4) return false;
	unsigned first=0, rows=1, byteOffset=offset*4;
	if (layer<Layers && !Dense(layer) && role==ConvWeights) {
		if ((offset&63) || values!=64) return false;
		first=offset>>2;
		rows=16;
		byteOffset=0;
	}
	Range range;
	return tensor->place.Resolve(first,rows,byteOffset,values*4,range) && Transfer(range,output);
}

bool DeviceIO::ReadRope(unsigned at,float* output,unsigned values)
{
	return at==position && position<model.context && values==Rotary
		&& Transfer({rope.bank,rope.address+position*Rotary*4,Rotary*4},output);
}

bool DeviceIO::ReadConv(unsigned layer,unsigned slot,unsigned channel,float* output,unsigned values)
{
	Range range;
	return position && slot==(position&1) && values==48 && model.ResolveConv(layer,slot,channel,range)
		&& Transfer(range,output);
}

bool DeviceIO::WriteConv(unsigned layer,unsigned slot,unsigned channel,const float* input,unsigned values)
{
	Range range;
	return slot==((position&1)^1) && values==48 && model.ResolveConv(layer,slot,channel,range)
		&& Write(range,input);
}

bool DeviceIO::ReadDelta(unsigned layer,unsigned slot,unsigned head,float* output,unsigned values)
{
	Range range;
	return position && slot==(position&1) && values==DeltaStateWords && model.ResolveDelta(layer,slot,head,range)
		&& Transfer(range,output);
}

bool DeviceIO::WriteDelta(unsigned layer,unsigned slot,unsigned head,const float* input,unsigned values)
{
	Range range;
	return slot==((position&1)^1) && values==DeltaStateWords && model.ResolveDelta(layer,slot,head,range)
		&& Write(range,input);
}

bool DeviceIO::ReadKv(unsigned layer,unsigned head,unsigned at,uint32_t* k,uint32_t* v,unsigned words)
{
	Range key,value;
	return at<=position && words==DenseHeadWidth/2 && model.ResolveKv(layer,head,at,false,key)
		&& model.ResolveKv(layer,head,at,true,value) && Transfer(key,k) && Transfer(value,v);
}

bool DeviceIO::WriteKv(unsigned layer,unsigned head,unsigned at,const uint32_t* k,const uint32_t* v,unsigned words)
{
	Range key,value;
	return at==position && words==DenseHeadWidth/2 && model.ResolveKv(layer,head,at,false,key)
		&& model.ResolveKv(layer,head,at,true,value) && Write(key,k) && Write(value,v);
}

bool DeviceIO::StageInput(unsigned type,const float* input,unsigned inputs)
{
	if (!PioneerQwen35Projection::Supports(type,inputs)) return false;
	unsigned scalesPerChunk=type==8?8:1;
	for (unsigned chunk=0;chunk<inputs/256;chunk++) {
		PioneerQ38ProjectionInput packed;
		// Q5_K uses the same 256-lane Q8_K activation as Q4_K.
		if (!PioneerQ38ProjectionQuantize(type==13?12:type,input+chunk*256,256,&packed)) return false;
		for (unsigned w=0;w<128;w++) Words(Activation)[chunk*128+w]=packed.operand[w];
		for (unsigned w=0;w<scalesPerChunk;w++) Words(Scales)[chunk*scalesPerChunk+w]=packed.scale[w];
	}
	Fence();
	return true;
}

bool DeviceIO::StageBand(const TensorSpace& tensor,unsigned first,unsigned groups,float* output)
{
	Range range;
	if (!tensor.ResolveBand(first,groups,range) || !InBank(range)) return false;
	unsigned tile=PioneerQwen35Projection::TileBytes(tensor.type);
	const unsigned descriptor[]={PioneerQwen35Projection::Magic,tensor.type,tile,0,0,tensor.inputs,range.address,groups};
	for (unsigned i=0;i<8;i++) Words(MatrixJob)[i]=descriptor[i];
	uint32_t command[]={range.address,0,PioneerQ38ProjectionRoute(range.bank),0x90000,0,RootRoute,0,0,tile,0};
	if (!Program(command)) return false;
	unsigned before[7];
	for (unsigned i=0;i<7;i++) before[i]=Counter(i);
	if (q35_work_matrix_band()!=groups*(tensor.inputs/16)) return false;
	command[0]=range.address+range.bytes-tile;
	if (!Matches(command)) return false;
	unsigned tiles=groups*(tensor.inputs>>(tensor.type==8?5:8));
	const unsigned increment[]={tiles,range.bytes/64,tiles,tiles,0,0,0};
	for (unsigned i=0;i<7;i++)
		if (Counter(i)-before[i]!=increment[i]) return false;
	for (unsigned i=0;i<8;i++)
		if (Words(MatrixJob)[i]!=descriptor[i]) return false;
	for (unsigned row=0;row<groups*16;row++) {
		float result=PioneerAttentionFloat(Words(MatrixResult)[row]);
		if (!Finite(result)) return false;
		output[row]=result;
	}
	return true;
}

bool DeviceIO::Project(unsigned layer,unsigned role,const float* input,unsigned inputs,float* output,unsigned outputs)
{
	const auto* tensor=model.Weight(layer,role);
	if (!tensor || tensor==&model.tied || tensor->inputs!=inputs || tensor->place.rows!=outputs
		|| !tensor->place.Valid() || !StageInput(tensor->type,input,inputs)) return false;
	for (unsigned part=0;part<tensor->place.count;part++) {
		const auto& fragment=tensor->place.parts[part];
		for (unsigned row=0;row<fragment.rows;) {
			unsigned rows=fragment.rows-row;
			if (rows>256) rows=256;
			unsigned first=fragment.firstRow+row;
			if (!StageBand(*tensor,first,rows/16,output+first)) return false;
			row+=rows;
		}
	}
	return true;
}

bool DeviceIO::TiedArgmax(const float* input,unsigned inputs,unsigned& token)
{
	const auto& tensor=model.tied;
	if (inputs!=Hidden || tensor.inputs!=Hidden || tensor.type!=14 || tensor.place.rows!=Vocabulary
		|| !tensor.place.Valid() || !StageInput(tensor.type,input,inputs)) return false;
	float band[256], best=0;
	unsigned chosen=Vocabulary;
	for (unsigned part=0;part<tensor.place.count;part++) {
		const auto& fragment=tensor.place.parts[part];
		for (unsigned row=0;row<fragment.rows;) {
			unsigned rows=fragment.rows-row;
			if (rows>256) rows=256;
			unsigned first=fragment.firstRow+row;
			if (!StageBand(tensor,first,rows/16,band)) return false;
			for (unsigned i=0;i<rows;i++)
				if (chosen==Vocabulary || band[i]>best) { chosen=first+i; best=band[i]; }
			row+=rows;
		}
	}
	token=chosen;
	return chosen<Vocabulary;
}

bool DeviceIO::PrefillDone(unsigned positions)
{
	if (decodeStarted || positions!=position+1) return false;
	decodeStarted=Cycles();
	if (decodeStarted<started) return false;
	__UINT64_TYPE__ cycles=decodeStarted-started;
	progress.promptCyclesLow=uint32_t(cycles);
	progress.promptCyclesHigh=uint32_t(cycles>>32);
	Fence();
	progress.prefilled=positions;
	return true;
}

bool DeviceIO::Commit(unsigned at,unsigned token,bool sampled,unsigned next)
{
	if (at!=position || at>=model.context || token>=Vocabulary || next>=Vocabulary
		|| sampled!=(decodeStarted!=0) || !Quiet()) return false;
	if (sampled) {
		__UINT64_TYPE__ now=Cycles();
		if (now<decodeStarted || generated>=model.context) return false;
		__UINT64_TYPE__ cycles=now-decodeStarted;
		tokens[generated++]=next;
		progress.decodeCyclesLow=uint32_t(cycles);
		progress.decodeCyclesHigh=uint32_t(cycles>>32);
		progress.generated=generated;
	}
	Fence();
	progress.committed=++position;
	return true;
}

bool DeviceIO::FinishRequest(unsigned positions,unsigned outputs)
{
	if (positions!=position || !outputs || outputs!=generated || !Quiet()) return false;
	Fence();
	progress.terminal=1;
	Fence();
	return true;
}
}
