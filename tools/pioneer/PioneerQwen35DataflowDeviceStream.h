/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_DEVICE_STREAM_H
#define PIONEER_QWEN35_DATAFLOW_DEVICE_STREAM_H

#include "PioneerQwen35DataflowAuthorization.h"
#include "PioneerQwen35DataflowBlackholeIO.h"
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
#include "PioneerQwen35DataflowDeltaShardProgram.h"
#endif
#include <stddef.h>

namespace PioneerQwen35 {

enum : unsigned {
	DataflowStreamBindingMagic=0x31425335,
	DataflowStreamRecordMagic=0x31525335,
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	DataflowStreamVersion=3,
#elif defined(PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS)
	DataflowStreamVersion=2,
#else
	DataflowStreamVersion=1,
#endif
	DataflowStreamRecordBytes=2048,
	DataflowStreamHeaderWords=16,
	DataflowStreamPayloadWords=(DataflowStreamRecordBytes/4)-DataflowStreamHeaderWords,
	DataflowStreamMaxRecords=1024,
	// Reuse the second weight page only between completed worker commands.
	DataflowStreamStagingAddress=DataflowWeightBufferAddress+DataflowWeightBufferPageStride
};
enum DataflowStreamFetchStage : unsigned {
	DataflowStreamFetchOkay=0,DataflowStreamFetchBinding=1,DataflowStreamFetchOverlap=2,
	DataflowStreamFetchRead=3,DataflowStreamFetchRecord=4,DataflowStreamFetchPatch=5,
	DataflowStreamFetchPatchedRecord=6
};
inline unsigned DataflowStreamFetchDiagnosticCode(unsigned stage,const DataflowNocDiagnostic& noc) {
	return 0x80000000U | (stage&0xfU) | ((noc.stage&0xfU)<<4)
		| ((noc.index&0xffU)<<8) | ((noc.counterMatchMask&0xffU)<<16)
		| ((noc.initiatorBusyMask&0xfU)<<24);
}

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
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
static_assert(sizeof(DataflowDeltaShardInvocation)<=DataflowStreamPayloadWords*4,
    "sharded delta invocation fits a bounded stream record");
#endif

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
#ifndef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaMagic:return sizeof(DataflowDeltaInvocation);
#endif
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaShardMagic:return sizeof(DataflowDeltaShardInvocation);
#endif
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
#ifndef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaMagic:return DataflowDeltaVersion;
#endif
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaShardMagic:return DataflowDeltaShardVersion;
#endif
	case DataflowDenseAttentionMagic:return DataflowDenseAttentionVersion;
	case DataflowArgmaxMagic:return DataflowArgmaxVersion;
	default:return 0;
	}
}

inline unsigned DataflowStreamCoreWord(unsigned magic)
{
	switch (magic) {
	case DataflowProgramMagic:return offsetof(DataflowCoreInvocation,core)/sizeof(uint32_t);
	case DataflowEmbeddingMagic:return offsetof(DataflowEmbeddingInvocation,core)/sizeof(uint32_t);
	case DataflowRmsMagic:return offsetof(DataflowRmsInvocation,core)/sizeof(uint32_t);
	case DataflowSiluMagic:return offsetof(DataflowSiluInvocation,core)/sizeof(uint32_t);
	case DataflowResidualMagic:return offsetof(DataflowResidualInvocation,core)/sizeof(uint32_t);
	case DataflowConvMagic:return offsetof(DataflowConvInvocation,core)/sizeof(uint32_t);
#ifndef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaMagic:return offsetof(DataflowDeltaInvocation,core)/sizeof(uint32_t);
#endif
#ifdef PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY
	case DataflowDeltaShardMagic:return offsetof(DataflowDeltaShardInvocation,core)/sizeof(uint32_t);
#endif
	case DataflowDenseAttentionMagic:return offsetof(DataflowDenseAttentionInvocation,core)/sizeof(uint32_t);
	case DataflowArgmaxMagic:return offsetof(DataflowArgmaxInvocation,core)/sizeof(uint32_t);
	default:return DataflowStreamPayloadWords;
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
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
template<class Invocation> inline bool DataflowStreamSharingReady(const Invocation&) { return true; }
inline bool DataflowStreamSharingReady(const DataflowCoreInvocation& invocation)
{
	// DataflowStreamEncode has already called Valid(), including the full
	// membership check; this only excludes an unassigned active command.
	return !invocation.assignment.active || invocation.sharing.packing;
}
#endif
template<class Invocation>
bool DataflowStreamEncode(const Invocation& invocation,unsigned epoch,unsigned step,
	DataflowStreamRecord& record)
{
	const unsigned bytes=sizeof(Invocation);
	if (!epoch || !step || step>DataflowStreamMaxRecords || !invocation.Valid()
		|| invocation.core>=DataflowCoreCount || (bytes&3)
		|| bytes!=DataflowStreamInvocationBytes(invocation.magic)
		|| bytes>sizeof(record.payload)
#ifdef PIONEER_QWEN35_ACTIVATION_SHARING_WITNESS
		|| !DataflowStreamSharingReady(invocation)
#endif
		) return false;
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
inline unsigned DataflowStreamRecordReadBytes(unsigned payloadBytes)
{
	const unsigned header=DataflowStreamHeaderWords*sizeof(uint32_t);
	return payloadBytes && payloadBytes<=DataflowStreamPayloadWords*sizeof(uint32_t)
		?((header+payloadBytes+63U)&~63U):0;
}

// A shorter read is valid only when it contains the entire typed payload and
// its final alignment padding. Unread trailing bytes never enter the command.
inline bool DataflowStreamRecordStructureValidBytes(const DataflowStreamRecord& r,
	unsigned epoch,unsigned step,unsigned core,unsigned readBytes)
{
	const unsigned bytes=DataflowStreamInvocationBytes(r.header[5]);
	if (readBytes<DataflowStreamHeaderWords*sizeof(uint32_t) || readBytes>DataflowStreamRecordBytes
		|| (readBytes&63U) || !epoch || !step || step>DataflowStreamMaxRecords || core>=DataflowCoreCount
		|| r.header[0]!=DataflowStreamRecordMagic || r.header[1]!=DataflowStreamVersion
		|| r.header[2]!=epoch || r.header[3]!=step || r.header[4]!=core
		|| !bytes || bytes!=r.header[6] || (bytes&3) || bytes>sizeof(r.payload)
		|| DataflowStreamRecordReadBytes(bytes)>readBytes
		|| (readBytes!=DataflowStreamRecordBytes && DataflowStreamRecordReadBytes(bytes)!=readBytes)
		|| r.payload[0]!=r.header[5] || DataflowStreamCoreWord(r.header[5])>=bytes/4
		|| r.payload[DataflowStreamCoreWord(r.header[5])]!=core
		|| r.payload[1]!=DataflowStreamInvocationVersion(r.header[5])
		|| DataflowStreamChecksum(r.payload,bytes/4)!=r.header[7]) return false;
	for (unsigned i=8;i<DataflowStreamHeaderWords;i++) if (r.header[i]) return false;
	for (unsigned i=bytes/4;i<(readBytes/4)-DataflowStreamHeaderWords;i++) if (r.payload[i]) return false;
	return true;
}
inline bool DataflowStreamRecordStructureValid(const DataflowStreamRecord& r,
	unsigned epoch,unsigned step,unsigned core)
{
	return DataflowStreamRecordStructureValidBytes(r,epoch,step,core,DataflowStreamRecordBytes);
}

// A read failure is an uncertain NoC operation: the caller must stop and
// retain the whole graph. Do not start the copied invocation until the normal
// worker type/shape checks and the stream owner's model authorization pass.
template<class IO,class Transform>
bool DataflowStreamFetchTransform(IO& io,const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
	DataflowStreamRecord& staging,uint32_t* program,unsigned programBytes,Transform& transform,
	unsigned* failedStage=nullptr,unsigned readBytes=DataflowStreamRecordBytes)
{
	if (failedStage) *failedStage=DataflowStreamFetchBinding;
	if (!DataflowStreamBindingMatches(binding,lease,routes,core)
		|| index>=binding.count || !program
		|| programBytes<DataflowProgramRegionBytes || readBytes<DataflowStreamHeaderWords*sizeof(uint32_t)
		|| readBytes>DataflowStreamRecordBytes || (readBytes&63U)
		|| (reinterpret_cast<uintptr_t>(&staging)&63)
		|| (reinterpret_cast<uintptr_t>(program)&3)) return false;
	const uintptr_t start=reinterpret_cast<uintptr_t>(&staging),out=reinterpret_cast<uintptr_t>(program);
	if (failedStage) *failedStage=DataflowStreamFetchOverlap;
	if (start>UINTPTR_MAX-sizeof(staging) || out>UINTPTR_MAX-programBytes
		|| !(start+sizeof(staging)<=out || out+programBytes<=start)) return false;
	const uint32_t address=binding.base+index*DataflowStreamRecordBytes;
	// Invalidate the prior command before issuing a possibly uncertain read.
	// Publish the new magic only after all other words have been copied.
	DataflowStoreRelease(program[0],0);
	if (failedStage) *failedStage=DataflowStreamFetchRead;
	if (!io.ReadStream(binding.bank,address,binding.route,
		reinterpret_cast<uint8_t*>(&staging),readBytes)) return false;
	if (failedStage) *failedStage=DataflowStreamFetchRecord;
	if (!DataflowStreamRecordStructureValidBytes(staging,binding.epoch,index+1,core,readBytes)) return false;
	if (failedStage) *failedStage=DataflowStreamFetchPatch;
	if (!transform(staging)) return false;
	if (failedStage) *failedStage=DataflowStreamFetchPatchedRecord;
	if (!DataflowStreamRecordStructureValidBytes(staging,binding.epoch,index+1,core,readBytes)) return false;
	for (unsigned i=1;i<staging.header[6]/4;i++) program[i]=staging.payload[i];
	DataflowStoreRelease(program[0],staging.payload[0]);
	if (failedStage) *failedStage=DataflowStreamFetchOkay;
	return true;
}

template<class IO>
bool DataflowStreamFetch(IO& io,const DataflowStreamBinding& binding,const Range& lease,
	const unsigned (&routes)[2][Banks],unsigned core,unsigned index,
	DataflowStreamRecord& staging,uint32_t* program,unsigned programBytes)
{
	auto unchanged=[](DataflowStreamRecord&) { return true; };
	return DataflowStreamFetchTransform(io,binding,lease,routes,core,index,
		staging,program,programBytes,unchanged);
}

class DataflowBlackholeStreamIO {
	DataflowBlackholeNoc<0> noc;
	DataflowNocDiagnostic* diagnostic;
public:
	explicit DataflowBlackholeStreamIO(DataflowNocDiagnostic* detail=nullptr):diagnostic(detail) {}
	bool ReadStream(unsigned bank,unsigned address,unsigned route,uint8_t* output,unsigned bytes) {
		return bank<Banks && output==reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(DataflowStreamStagingAddress))
			&& bytes>=64 && bytes<=DataflowStreamRecordBytes && !(bytes&63U)
			&& noc.Read(address,route,output,bytes,diagnostic);
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
