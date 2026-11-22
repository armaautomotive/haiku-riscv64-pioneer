/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DEVICE_STREAM_H
#define PIONEER_QWEN35_DATAFLOW_DEVICE_STREAM_H

#include "PioneerQwen35DataflowAuthorization.h"
#include "PioneerQwen35DataflowBlackholeIO.h"

namespace PioneerQwen35 {

enum : unsigned {
	DataflowStreamBindingMagic=0x31425335,
	DataflowStreamRecordMagic=0x31525335,
	DataflowStreamVersion=1,
	DataflowStreamRecordBytes=2048,
	DataflowStreamHeaderWords=16,
	DataflowStreamPayloadWords=(DataflowStreamRecordBytes/4)-DataflowStreamHeaderWords,
	DataflowStreamMaxRecords=1024,
	// Reuse the second weight page only between completed worker commands.
	DataflowStreamStagingAddress=DataflowWeightBufferAddress+DataflowWeightBufferPageStride
};

struct alignas(64) DataflowStreamRecord {
	uint32_t header[DataflowStreamHeaderWords];
	uint32_t payload[DataflowStreamPayloadWords];
};
static_assert(sizeof(DataflowStreamRecord)==DataflowStreamRecordBytes
	&& DataflowStreamStagingAddress+DataflowStreamRecordBytes<=DataflowLocalBufferEnd,
	"a stream record fits in the idle second weight page");
static_assert(sizeof(DataflowCoreInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowEmbeddingInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowRmsInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowSiluInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowResidualInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowConvInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowDeltaInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowDenseAttentionInvocation)<=DataflowStreamPayloadWords*4
	&& sizeof(DataflowArgmaxInvocation)<=DataflowStreamPayloadWords*4,
	"every resident invocation fits in one bounded stream record");

struct DataflowStreamBinding {
	uint32_t magic,version,epoch,core,bank,route,base,count;
	bool Valid() const {
		return magic==DataflowStreamBindingMagic && version==DataflowStreamVersion
			&& epoch && core<DataflowCoreCount && bank<Banks && route && route<=0xfff
			&& !(base&(DataflowStreamRecordBytes-1)) && base>=BankBase && base<BankLimit
			&& count && count<=DataflowStreamMaxRecords
			&& uint64_t(count)*DataflowStreamRecordBytes<=BankLimit-base;
	}
};

// The kernel must independently prove that the entire lease is reserved,
// immutable until all workers finish, and disjoint from model/state buffers.
// This check alone is not an allocation or authorization to start the card.
inline bool DataflowStreamBindingMatches(const DataflowStreamBinding& b,
	const Range& lease,const unsigned (&routes)[2][Banks],unsigned core)
{
	return b.Valid() && DataflowBankRoutesValid(routes) && core==b.core
		&& lease.bank==b.bank && lease.address>=BankBase && lease.address<BankLimit
		&& lease.bytes && lease.bytes<=BankLimit-lease.address
		&& b.base>=lease.address && b.base-lease.address<=lease.bytes
		&& uint64_t(b.count)*DataflowStreamRecordBytes<=lease.bytes-(b.base-lease.address)
		&& b.route==routes[0][b.bank];
}

inline unsigned DataflowStreamInvocationBytes(unsigned magic)
{
	switch (magic) {
	case DataflowProgramMagic:return sizeof(DataflowCoreInvocation);
	case DataflowEmbeddingMagic:return sizeof(DataflowEmbeddingInvocation);
	case DataflowRmsMagic:return sizeof(DataflowRmsInvocation);
	case DataflowSiluMagic:return sizeof(DataflowSiluInvocation);
	case DataflowResidualMagic:return sizeof(DataflowResidualInvocation);
	case DataflowConvMagic:return sizeof(DataflowConvInvocation);
	case DataflowDeltaMagic:return sizeof(DataflowDeltaInvocation);
	case DataflowDenseAttentionMagic:return sizeof(DataflowDenseAttentionInvocation);
	case DataflowArgmaxMagic:return sizeof(DataflowArgmaxInvocation);
	default:return 0;
	}
}

inline unsigned DataflowStreamInvocationVersion(unsigned magic)
{
	switch (magic) {
	case DataflowProgramMagic:return DataflowProgramVersion;
	case DataflowEmbeddingMagic:return DataflowEmbeddingVersion;
	case DataflowRmsMagic:return DataflowRmsVersion;
	case DataflowSiluMagic:return DataflowSiluVersion;
	case DataflowResidualMagic:return DataflowResidualVersion;
	case DataflowConvMagic:return DataflowConvVersion;
	case DataflowDeltaMagic:return DataflowDeltaVersion;
	case DataflowDenseAttentionMagic:return DataflowDenseAttentionVersion;
	case DataflowArgmaxMagic:return DataflowArgmaxVersion;
	default:return 0;
	}
}

// Detect accidental transfer corruption; this is not an authentication hash.
inline uint32_t DataflowStreamChecksum(const uint32_t* words,unsigned count)
{
	uint32_t hash=2166136261U;
	for (unsigned i=0;i<count;i++) hash=(hash^words[i])*16777619U;
	return hash;
}

// Host-side encoder: the existing authority must first authorize the exact
// program and bind it to this core. Valid() checks shape, not model ownership.
template<class Invocation>
bool DataflowStreamEncode(const Invocation& invocation,unsigned epoch,unsigned step,
	DataflowStreamRecord& record)
{
	const unsigned bytes=sizeof(Invocation);
	if (!epoch || !step || step>DataflowStreamMaxRecords || !invocation.Valid()
		|| invocation.core>=DataflowCoreCount || (bytes&3)
		|| bytes!=DataflowStreamInvocationBytes(invocation.magic)
		|| bytes>sizeof(record.payload)) return false;
	const uintptr_t sourceAddress=reinterpret_cast<uintptr_t>(&invocation);
	const uintptr_t recordAddress=reinterpret_cast<uintptr_t>(&record);
	if (sourceAddress>UINTPTR_MAX-bytes || recordAddress>UINTPTR_MAX-sizeof(record)
		|| !(sourceAddress+bytes<=recordAddress || recordAddress+sizeof(record)<=sourceAddress)) return false;
	for (unsigned i=0;i<DataflowStreamHeaderWords;i++) record.header[i]=0;
	for (unsigned i=0;i<DataflowStreamPayloadWords;i++) record.payload[i]=0;
	const auto* source=reinterpret_cast<const uint8_t*>(&invocation);
	auto* target=reinterpret_cast<uint8_t*>(record.payload);
	for (unsigned i=0;i<bytes;i++) target[i]=source[i];
	record.header[0]=DataflowStreamRecordMagic;record.header[1]=DataflowStreamVersion;
	record.header[2]=epoch;record.header[3]=step;record.header[4]=invocation.core;
	record.header[5]=invocation.magic;record.header[6]=bytes;
	record.header[7]=DataflowStreamChecksum(record.payload,bytes/4);
	return true;
}

// Structural/transfer check only; the command executor must still validate
// its typed invocation, and the kernel must authorize it against the model.
inline bool DataflowStreamRecordStructureValid(const DataflowStreamRecord& r,
	unsigned epoch,unsigned step,unsigned core)
{
	const unsigned bytes=DataflowStreamInvocationBytes(r.header[5]);
	if (!epoch || !step || step>DataflowStreamMaxRecords || core>=DataflowCoreCount
		|| r.header[0]!=DataflowStreamRecordMagic || r.header[1]!=DataflowStreamVersion
		|| r.header[2]!=epoch || r.header[3]!=step || r.header[4]!=core
		|| !bytes || bytes!=r.header[6] || (bytes&3) || bytes>sizeof(r.payload)
		|| r.payload[0]!=r.header[5] || r.payload[2]!=core
		|| r.payload[1]!=DataflowStreamInvocationVersion(r.header[5])
		|| DataflowStreamChecksum(r.payload,bytes/4)!=r.header[7]) return false;
	for (unsigned i=8;i<DataflowStreamHeaderWords;i++) if (r.header[i]) return false;
	for (unsigned i=bytes/4;i<DataflowStreamPayloadWords;i++) if (r.payload[i]) return false;
	return true;
}

// A read failure is an uncertain NoC operation: the caller must stop and
// retain the whole graph. Do not start the copied invocation until the normal
// worker type/shape checks and the stream owner's model authorization pass.
template<class IO>
bool DataflowStreamFetch(IO& io,const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
	DataflowStreamRecord& staging,uint32_t* program,unsigned programBytes)
{
	if (!DataflowStreamBindingMatches(binding,lease,routes,core)
		|| index>=binding.count || !program
		|| programBytes<DataflowProgramRegionBytes
		|| (reinterpret_cast<uintptr_t>(&staging)&63)
		|| (reinterpret_cast<uintptr_t>(program)&3)) return false;
	const uintptr_t start=reinterpret_cast<uintptr_t>(&staging),out=reinterpret_cast<uintptr_t>(program);
	if (start>UINTPTR_MAX-sizeof(staging) || out>UINTPTR_MAX-programBytes
		|| !(start+sizeof(staging)<=out || out+programBytes<=start)) return false;
	const uint32_t address=binding.base+index*DataflowStreamRecordBytes;
	// Invalidate the prior command before issuing a possibly uncertain read.
	// Publish the new magic only after all other words have been copied.
	DataflowStoreRelease(program[0],0);
	if (!io.ReadStream(binding.bank,address,binding.route,
		reinterpret_cast<uint8_t*>(&staging),sizeof(staging))
		|| !DataflowStreamRecordStructureValid(staging,binding.epoch,index+1,core)) return false;
	for (unsigned i=1;i<staging.header[6]/4;i++) program[i]=staging.payload[i];
	DataflowStoreRelease(program[0],staging.payload[0]);return true;
}

class DataflowBlackholeStreamIO {
	DataflowBlackholeNoc<0> noc;
public:
	bool ReadStream(unsigned bank,unsigned address,unsigned route,uint8_t* output,unsigned bytes) {
		return bank<Banks && output==reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(DataflowStreamStagingAddress))
			&& bytes==DataflowStreamRecordBytes && noc.Read(address,route,output,bytes);
	}
};

inline bool DataflowBlackholeStreamFetch(const DataflowStreamBinding& binding,
	const Range& lease,const unsigned (&routes)[2][Banks],unsigned core,unsigned index)
{
	DataflowBlackholeStreamIO io;
	auto* staging=reinterpret_cast<DataflowStreamRecord*>(
		static_cast<uintptr_t>(DataflowStreamStagingAddress));
	auto* program=reinterpret_cast<uint32_t*>(static_cast<uintptr_t>(DataflowProgramAddress));
	return DataflowStreamFetch(io,binding,lease,routes,core,index,*staging,program,DataflowProgramRegionBytes);
}
}
#endif
