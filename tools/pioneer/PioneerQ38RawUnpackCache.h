/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RAW_UNPACK_CACHE_H
#define PIONEER_Q38_RAW_UNPACK_CACHE_H
#if !defined(PIONEER_Q38_WORK_RAW_UNPACK_CACHE) || !defined(PIONEER_Q38_WORK_DIRECT_SRCB) || !defined(PIONEER_Q38_STATIC_VECTOR)
#error Separate original-layout OFFLINE Q4 raw-Dst reuse experiment ONLY
#endif
#if defined(PIONEER_Q38_WORK_FRONTEND_BARRIER) || defined(PIONEER_Q38_WORK_BATCH4_FETCH) || defined(PIONEER_Q38_WORK_BATCH_RAW_INPUT)
#error Raw-Dst reuse has a distinct original-layout scheduling role
#endif
// Within EACH original Q4 block, panels0/2 read identical packed bytes,
// as do1/3. Keep the TWO fully unpacked planes in existing physical
// Dst0..15/16..31. Original sequential panel order and FP32 rounding stay.
// Metadata96..111/128..191 and matrix dots32..47 do NOT alias these rows.
// This is DATA scheduling,not an owner/cache capability across jobs/blocks.
static inline bool PioneerQ38RawCacheActive(unsigned type,bool work){return work && type==12;}
static inline bool PioneerQ38RawCacheUpper(unsigned type,unsigned panel,bool work){return PioneerQ38RawCacheActive(type,work) && (panel&1);}
static inline bool PioneerQ38RawCacheReuse(unsigned type,unsigned panel,bool work){return PioneerQ38RawCacheActive(type,work) && (panel&2);}
template<class E> static void PioneerQ38RawCachePublish(E& e){
 // Blackhole C5 waits for the CURRENT unpacker SrcA bank to be returned.
 // UNPACR_NOP/7 performs the SAME bank handoff as original FlipSrc;do
 // NOT use unsupported SETDVALID or omit source ownership on reused data.
 // Full RISC-facing drain remains after publication,BEFORE any consumer.
 // https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/STALLWAIT.md
 // https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/UNPACR_NOP_SETDVALID.md
 e.Op(0xa2ff8020);e.Op(0x43000007);e.Drain();
}
#endif
