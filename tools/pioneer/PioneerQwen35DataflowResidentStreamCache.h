/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_RESIDENT_STREAM_CACHE_H
#define PIONEER_QWEN35_DATAFLOW_RESIDENT_STREAM_CACHE_H

#include "PioneerQwen35DataflowDeviceStream.h"
#include "PioneerQwen35DataflowRoleAsmConstants.h"

namespace PioneerQwen35 {
enum : unsigned {
	DataflowResidentStreamCacheAddress=0xab580,
	DataflowResidentStreamCacheEntries=548,
	DataflowResidentStreamCacheMagic=0x314c5335
};
// Only the compute worker accesses this local cache. A length is recorded
// after a complete, validated fetch of the same sealed graph record. It is a
// size hint, not authority: every shorter fetch revalidates the complete
// typed payload, header, checksum and fetched alignment padding.
struct alignas(64) DataflowResidentStreamCache {
	uint32_t magic;
	DataflowStreamBinding binding;
	uint32_t readBytes[DataflowResidentStreamCacheEntries];
};
static_assert(DataflowResidentStreamCacheAddress>=PIONEER_Q35_ROLE_SNAPSHOT_HANDOFF+12
	&& DataflowResidentStreamCacheAddress+sizeof(DataflowResidentStreamCache)<=DataflowBarrierArrivalBase,
	"resident length cache must not overlap role handoff or barrier arrivals");

inline void DataflowResidentStreamCacheReset(DataflowResidentStreamCache& cache)
{
	DataflowStoreRelease(cache.magic,0);
}
inline bool DataflowResidentStreamCacheBind(DataflowResidentStreamCache& cache,const DataflowStreamBinding& b)
{
	if (!b.Valid() || b.count>DataflowResidentStreamCacheEntries) return false;
	const uint32_t magic=DataflowLoadAcquire(cache.magic);
	if (!magic) {
		for (unsigned i=0;i<DataflowResidentStreamCacheEntries;i++) cache.readBytes[i]=0;
		cache.binding=b;
		DataflowStoreRelease(cache.magic,DataflowResidentStreamCacheMagic);
		return true;
	}
	const auto& old=cache.binding;
	return magic==DataflowResidentStreamCacheMagic && old.magic==b.magic && old.version==b.version
		&& old.epoch==b.epoch && old.core==b.core && old.bank==b.bank && old.route==b.route
		&& old.base==b.base && old.count==b.count;
}
inline unsigned DataflowResidentStreamCacheHint(const DataflowResidentStreamCache& cache,unsigned index)
{
	if (DataflowLoadAcquire(cache.magic)!=DataflowResidentStreamCacheMagic
		|| index>=cache.binding.count || index>=DataflowResidentStreamCacheEntries) return UINT32_MAX;
	const unsigned bytes=cache.readBytes[index];
	return !bytes || (bytes>=64 && bytes<=DataflowStreamRecordBytes && !(bytes&63U))
		?bytes:UINT32_MAX;
}
inline bool DataflowResidentStreamCacheRecord(DataflowResidentStreamCache& cache,unsigned index,
	const DataflowStreamRecord& record,unsigned actualBytes)
{
	const unsigned previous=DataflowResidentStreamCacheHint(cache,index);
	const unsigned expected=DataflowStreamRecordReadBytes(record.header[6]);
	if (previous==UINT32_MAX || !expected || (actualBytes!=DataflowStreamRecordBytes && actualBytes!=expected)
		|| (previous && previous!=expected)) return false;
	cache.readBytes[index]=expected;return true;
}
}
#endif
