/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_BLACKHOLE_IO_H
#define PIONEER_QWEN35_DATAFLOW_BLACKHOLE_IO_H

#include "PioneerQwen35DataflowKernels.h"
#include "PioneerQ38ProjectionInput.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowNoc0Base=0xffb20000,
	DataflowNoc1Base=0xffb30000,
	DataflowNocMaxTransfer=16384,
	DataflowNocPollLimit=65536,
	DataflowNocInitiators=4,
	DataflowNocQueues=32
};

struct DataflowNocReadTicket {
	uint32_t command[10];
	uint32_t expected[8];
	unsigned active;
};

// Blackhole exposes separate NoC 0 and NoC 1 command/counter banks. This
// freestanding worker issues input/weight reads on NoC 0 and output writes on
// NoC 1 from its resident BRISC service. It can leave one NoC 0 read in flight
// while the Tensix matrix engine consumes the other local weight page.
template<unsigned Noc>
class DataflowBlackholeNoc {
	static_assert(Noc<2,"Blackhole has NoC 0 and NoC 1");
	static constexpr uint32_t Base=Noc==0?DataflowNoc0Base:DataflowNoc1Base;
	static constexpr unsigned CounterOffset[8]={
		0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};

	static volatile uint32_t* Words(uint32_t address)
	{
		return reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(address));
	}

	static void Fence()
	{
		asm volatile("fence rw,rw" ::: "memory");
	}

	static bool LocalRange(uint32_t address,unsigned bytes)
	{
		return address>=DataflowLocalBufferStart && address<DataflowLocalBufferEnd
			&& bytes && bytes<=DataflowLocalBufferEnd-address;
	}

	static uint32_t Counter(unsigned index)
	{
		return *Words(Base+CounterOffset[index]);
	}

	static bool Quiet()
	{
		for (unsigned i=0;i<DataflowNocInitiators;i++)
			if (*Words(Base+0x40+i*0x800)) return false;
		for (unsigned i=0;i<DataflowNocQueues;i++)
			if (*Words(Base+0x240+i*4)&255) return false;
		return true;
	}

	static bool Transfer(bool write,uint32_t remote,unsigned route,
		uint8_t* local,unsigned bytes)
	{
		if (!local || !LocalRange(uint32_t(reinterpret_cast<uintptr_t>(local)),bytes)
			|| !bytes || bytes>DataflowNocMaxTransfer || (bytes&63)
			|| remote<BankBase || remote>=BankLimit || bytes>BankLimit-remote
			|| (remote&63) || route>0xfff || !Quiet()) return false;
		const uint32_t localAddress=uint32_t(reinterpret_cast<uintptr_t>(local));
		const uint32_t self=*Words(Base+0x14);
		if (!self || self>0xfff) return false;
		uint32_t expected[8];
		const unsigned flits=bytes/64;
		const unsigned delta[8]={write?0U:1U,write?flits:0U,1,
			write?0U:1U,write?1U:0U,write?flits:0U,1,write?1U:0U};
		for (unsigned i=0;i<8;i++) expected[i]=Counter(i)+delta[i];
		const uint32_t command[10]={write?localAddress:remote,0,
			write?self:route,write?remote:localAddress,0,
			write?route:self,0,write?0x212U:0U,bytes,0};
		for (unsigned i=0;i<10;i++) {
			Words(Base)[i]=command[i];
			if (Words(Base)[i]!=command[i]) return false;
		}
		if (!Quiet()) return false;
		Fence();
		*Words(Base+0x40)=1;
		(void)*Words(Base+0x40);
		Fence();
		for (unsigned attempt=0;attempt<DataflowNocPollLimit;attempt++) {
			bool complete=!*Words(Base+0x40);
			for (unsigned i=0;i<8;i++) complete &= Counter(i)==expected[i];
			if (!complete) continue;
			Fence();
			for (unsigned i=0;i<10;i++)
				if (Words(Base)[i]!=command[i]) return false;
			return Quiet();
		}
		// The request state is unknown. Return failure to the role wrapper;
		// it publishes the shared failure and issues no further transfer.
		return false;
	}

public:
	bool BeginRead(uint32_t remote,unsigned route,uint8_t* local,unsigned bytes,
		DataflowNocReadTicket& ticket) const
	{
		if (ticket.active || !local
			|| !LocalRange(uint32_t(reinterpret_cast<uintptr_t>(local)),bytes)
			|| !bytes || bytes>DataflowNocMaxTransfer || (bytes&63)
			|| remote<BankBase || remote>=BankLimit || bytes>BankLimit-remote
			|| (remote&63) || route>0xfff || !Quiet()) return false;
		const uint32_t localAddress=uint32_t(reinterpret_cast<uintptr_t>(local));
		const uint32_t self=*Words(Base+0x14);
		if (!self || self>0xfff) return false;
		const unsigned delta[8]={1,0,1,1,0,0,1,0};
		for (unsigned i=0;i<8;i++) ticket.expected[i]=Counter(i)+delta[i];
		const uint32_t command[10]={remote,0,route,localAddress,0,self,0,0,bytes,0};
		for (unsigned i=0;i<10;i++) {
			ticket.command[i]=command[i];
			Words(Base)[i]=command[i];
			if (Words(Base)[i]!=command[i]) return false;
		}
		if (!Quiet()) return false;
		Fence();
		ticket.active=1;
		*Words(Base+0x40)=1;
		(void)*Words(Base+0x40);
		Fence();
		return true;
	}

	bool CompleteRead(DataflowNocReadTicket& ticket) const
	{
		if (!ticket.active) return false;
		for (unsigned attempt=0;attempt<DataflowNocPollLimit;attempt++) {
			bool complete=!*Words(Base+0x40);
			for (unsigned i=0;i<8;i++)
				complete &= Counter(i)==ticket.expected[i];
			if (!complete) continue;
			Fence();
			for (unsigned i=0;i<10;i++)
				if (Words(Base)[i]!=ticket.command[i]) return false;
			if (!Quiet()) return false;
			ticket.active=0;
			return true;
		}
		return false;
	}

	bool Read(uint32_t remote,unsigned route,uint8_t* local,unsigned bytes) const
	{
		return Transfer(false,remote,route,local,bytes);
	}

	bool Write(uint32_t remote,unsigned route,const uint8_t* local,unsigned bytes) const
	{
		return Transfer(true,remote,route,const_cast<uint8_t*>(local),bytes);
	}
};

class DataflowBlackholeReaderOps {
	const DataflowCoreInvocation* fProgram=nullptr;
	DataflowBlackholeNoc<0> fNoc;

	bool OwnsWeight(unsigned bank,uint32_t address,unsigned bytes) const
	{
		if (!fProgram || bank>=Banks || bytes!=fProgram->circularBuffer[0].pageBytes)
			return false;
		for (unsigned i=0;i<fProgram->assignment.segmentCount;i++) {
			const auto& segment=fProgram->assignment.weight[i];
			if (segment.bank==bank && address>=segment.address
				&& address-segment.address<=segment.bytes
				&& bytes<=segment.bytes-(address-segment.address)
				&& !((address-segment.address)%bytes)) return true;
		}
		return false;
	}

public:
	bool Bind(const DataflowCoreInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool ReadActivation(const RowSpace& source,unsigned rows,uint8_t* output,
		unsigned bytes) const
	{
		if (!fProgram || &source!=&fProgram->input || !source.Valid()
			|| rows!=fProgram->inputRows
			|| fProgram->inputBase>source.rows
			|| rows>source.rows-fProgram->inputBase
			|| source.rowBytes!=sizeof(float) || source.unit!=16
			|| bytes!=rows*sizeof(float) || !output
			|| reinterpret_cast<uintptr_t>(output)!=DataflowActivationCacheAddress)
			return false;
		const unsigned rangeEnd=fProgram->inputBase+rows;
		unsigned covered=0;
		for (unsigned i=0;i<source.count && covered<rows;i++) {
			const auto& part=source.parts[i];
			if ((part.firstRow&15) || (part.rows&15))
				return false;
			const unsigned partEnd=part.firstRow+part.rows;
			if (partEnd<=fProgram->inputBase || part.firstRow>=rangeEnd) continue;
			const unsigned rangeFirst=part.firstRow>fProgram->inputBase
				?part.firstRow:fProgram->inputBase;
			const unsigned rangeLast=partEnd<rangeEnd?partEnd:rangeEnd;
			if (rangeFirst!=fProgram->inputBase+covered
				|| rangeLast<=rangeFirst || ((rangeLast-rangeFirst)&15)) return false;
			const unsigned partRows=rangeLast-rangeFirst;
			const unsigned localStart=rangeFirst-part.firstRow;
			for (unsigned localRow=0;localRow<partRows;) {
				unsigned chunkRows=DataflowNocMaxTransfer/sizeof(float);
				if (chunkRows>partRows-localRow) chunkRows=partRows-localRow;
				chunkRows&=~15U;
				const unsigned chunkBytes=chunkRows*sizeof(float);
				const uint64_t remote=uint64_t(part.address)
					+uint64_t(localStart+localRow)*sizeof(float);
				const unsigned destinationRow=covered+localRow;
				if (!chunkRows || remote>UINT32_MAX || remote>=BankLimit
					|| chunkBytes>BankLimit-remote
					|| !fNoc.Read(uint32_t(remote),fProgram->bankRoute[0][part.bank],
						output+destinationRow*sizeof(float),chunkBytes)) return false;
				localRow+=chunkRows;
			}
			covered+=partRows;
		}
		return covered==rows;
	}

	bool ReadWeight(unsigned bank,uint32_t address,uint8_t* output,unsigned bytes) const
	{
		return fProgram && bank<Banks && OwnsWeight(bank,address,bytes)
			&& fNoc.Read(address,fProgram->bankRoute[0][bank],output,bytes);
	}

	bool BeginWeightRead(unsigned bank,uint32_t address,uint8_t* output,
		unsigned bytes,DataflowNocReadTicket& ticket) const
	{
		return fProgram && bank<Banks && OwnsWeight(bank,address,bytes)
			&& fNoc.BeginRead(address,fProgram->bankRoute[0][bank],output,bytes,ticket);
	}

	bool CompleteWeightRead(DataflowNocReadTicket& ticket) const
	{
		return fProgram && fNoc.CompleteRead(ticket);
	}

	bool PackActivation(unsigned type,unsigned inputs,unsigned chunk,
		const float* source,uint8_t* output,unsigned bytes) const
	{
		if (!fProgram || type!=DataflowActivationPackingType(fProgram->type)
			|| inputs!=fProgram->inputs || chunk>=inputs/DataflowActivationChunkInputs
			|| !source || !output || bytes!=DataflowInputPageBytes) return false;
		return PioneerQ38ProjectionQuantize(type,
			source+chunk*DataflowActivationChunkInputs,
			DataflowActivationChunkInputs,
			reinterpret_cast<PioneerQ38ProjectionInput*>(output));
	}
};

class DataflowBlackholeWriterOps {
	const DataflowCoreInvocation* fProgram=nullptr;
	DataflowBlackholeNoc<1> fNoc;

	bool OwnsOutput(unsigned bank,uint32_t address,unsigned bytes) const
	{
		if (!fProgram || bank>=Banks || bytes!=DataflowOutputPageBytes
			|| (address&63)) return false;
		for (unsigned i=0;i<fProgram->assignment.segmentCount;i++) {
			const auto& segment=fProgram->assignment.output[i];
			if (segment.bank==bank && address>=segment.address
				&& address-segment.address<=segment.bytes
				&& bytes<=segment.bytes-(address-segment.address)
				&& !((address-segment.address)%bytes)) return true;
		}
		return false;
	}

public:
	bool Bind(const DataflowCoreInvocation& program)
	{
		fProgram=nullptr;
		if (!program.Valid()) return false;
		fProgram=&program;
		return true;
	}

	bool WriteOutput(unsigned bank,uint32_t address,const uint8_t* input,
		unsigned bytes) const
	{
		return fProgram && OwnsOutput(bank,address,bytes)
			&& fNoc.Write(address,fProgram->bankRoute[1][bank],input,bytes);
	}
};

} // namespace PioneerQwen35

#endif
