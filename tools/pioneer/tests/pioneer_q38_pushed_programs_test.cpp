/* SPDX-License-Identifier: MIT */
// Immutable DATA access only, not native initialization or model execution.
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_MANUAL_TTSYNC
#define PIONEER_Q38_PROGRAM_DATA_TEST_CONSTANT_Q4 1
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_MATRIX_PHASE
#define PIONEER_Q38_PROGRAM_DATA_TEST_CONSTANT_Q4 1
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_CONSTANT_Q4
#define PIONEER_Q38_PROGRAM_DATA_TEST_PHASE_PROFILE 1
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_PHASE_PROFILE
#define PIONEER_Q38_PROGRAM_DATA_TEST_WIDE_GDN 1
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_WIDE_GDN
#define PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_OUTPUT_CLOSURE 1
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_OUTPUT_CLOSURE
#define PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH 1
#endif
#ifndef PIONEER_Q38_PUSHED_PROGRAMS_HEADER
#define PIONEER_Q38_PUSHED_PROGRAMS_HEADER "../PioneerQ38TokenPushedPrograms.h"
#endif
#include PIONEER_Q38_PUSHED_PROGRAMS_HEADER
#ifdef PIONEER_Q38_TOKEN_PROGRAMS_H
#error The distinct G3 package must not silently select the legacy package
#endif
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_MANUAL_TTSYNC
namespace P=PioneerQ38TokenManualTTSyncPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_MATRIX_PHASE)
namespace P=PioneerQ38TokenMatrixPhasePrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_CONSTANT_Q4)
namespace P=PioneerQ38TokenConstantQ4Programs;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PHASE_PROFILE)
namespace P=PioneerQ38TokenPhaseProfilePrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_WIDE_GDN)
namespace P=PioneerQ38TokenWideGdnPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_OUTPUT_CLOSURE)
namespace P=PioneerQ38TokenBatchedOutputClosurePrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH)
namespace P=PioneerQ38TokenTwinBatchPrefetchPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_PREFETCH)
namespace P=PioneerQ38TokenBatchedPrefetchPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PREFETCH)
namespace P=PioneerQ38TokenPrefetchPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FLAT_MATRIX)
namespace P=PioneerQ38TokenFlatMatrixPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_EMPTY_RECEIPT_POLL)
namespace P=PioneerQ38TokenEmptyReceiptPollPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COUNTER_GATHER)
namespace P=PioneerQ38TokenCounterGatherPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_TID_BOUNDARY)
namespace P=PioneerQ38TokenTidBoundaryPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PACKET_IDENTITY_BOUNDARY)
namespace P=PioneerQ38TokenPacketIdentityBoundaryPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COHORT_LOCAL_SOURCES)
namespace P=PioneerQ38TokenCohortLocalSourcesPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COMMIT_BOUNDARY)
namespace P=PioneerQ38TokenCommitBoundaryPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_RETAINED_COHORT)
namespace P=PioneerQ38TokenRetainedCohortPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FRONTIER)
namespace P=PioneerQ38TokenFrontierPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_JOB_BOOT_FOLD)
namespace P=PioneerQ38TokenJobBootFoldPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PRECISE_BETA)
namespace P=PioneerQ38TokenPreciseBetaPrograms;
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FAULT)
namespace P=PioneerQ38TokenFaultPrograms;
#else
namespace P=PioneerQ38TokenPushedPrograms;
#endif
namespace S=PioneerQ38TokenPushedL1;
struct Code {const char* file;unsigned address;};
static std::vector<unsigned char> Bytes(const char* directory,const char* name){
 std::ifstream file(std::string(directory)+"/"+name+".bin",std::ios::binary);assert(file);
 return {std::istreambuf_iterator<char>(file),{}};
}
int main(int argc,char** argv){
 assert(argc==3);setvbuf(stdout,nullptr,_IOLBF,0);
 assert(P::Family==S::Family && P::Family==0x33505451 && P::Version==1 && P::CancelBoundary==1);
 const Code root[]={{"boot",0x6000},{"text",0x40000},{"terminal",0x60000},{"conv",0x64000}};
 const Code peer[]={{"boot",0x6000},{"globalworker",0x20000},{"precise",0x30000},
  {"dense",0x40000},{"ffn",0x60000},{"service",0x68000},
#if defined(PIONEER_Q38_PROGRAM_DATA_TEST_PREFETCH) || defined(PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_PREFETCH) || defined(PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH)
  {"fetch",0x70000},
#endif
  {"affine",0x70b00},{"text",0x72000},{"gdn",0xb8000}};
 unsigned allWords=0,refusals=0;
 for(unsigned worker=0;worker<120;worker++){
  const unsigned count=worker?unsigned(sizeof(peer)/sizeof(peer[0])):4;const Code* sections=worker?peer:root;
  assert(P::CodeSpans(worker)==count);unsigned words=0,last=0;
  for(unsigned section=0;section<count;section++){
   const auto span=P::CodeSpan(worker,section);const auto bytes=Bytes(worker?argv[2]:argv[1],sections[section].file);
   assert(span.address==sections[section].address && span.words && span.address>=last && bytes.size()==span.words*4);
   for(unsigned i=0;i<span.words;i++){
    const unsigned address=span.address+i*4,offset=i*4;uint32_t word=0;
    const uint32_t expected=bytes[offset]|uint32_t(bytes[offset+1])<<8|uint32_t(bytes[offset+2])<<16|uint32_t(bytes[offset+3])<<24;
    assert(P::CodeAddress(worker,words++)==address && S::SnapshotContains(worker,address)
     && P::ProgramWord(worker,address,word) && word==expected);
   }
   last=span.address+span.words*4;
   uint32_t unchanged=0xdeadbeef;
   assert(!P::ProgramWord(worker,last,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
  assert(words==P::CodeWords(worker) && P::CodeAddress(worker,words)==UINT32_MAX
   && P::CodeAddress(worker,UINT32_MAX)==UINT32_MAX);
  allWords+=words;
  const auto invalid=P::CodeSpan(worker,count);assert(invalid.address==UINT32_MAX && !invalid.words);
  for(unsigned address:{0U,0x6001U,0x6004U,0x6e000U,0x74000U,0x83c00U,0x118000U,0x17ffc0U,0xffb20000U,UINT32_MAX}){
   uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(worker,address,unchanged) && unchanged==0xdeadbeef);refusals++;
  }
 }
#if defined(PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_PREFETCH) || defined(PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH)
 assert(P::BatchedPrefetch==1 && P::Q8Batch==4 && P::Q4Batch==2 && P::Q6Batch==2);
 assert(P::TilePrefetch==1 && P::RawBuffers==2 && P::FetchAddress==0x70000 && P::FlatMatrixGrammar==1);
 assert(P::EmptyReceiptPoll==1 && P::ReadSourceBoundary==1 && P::CounterGather==1 && P::CounterGatherReads==8);
 assert(P::TidBoundary==1 && P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH
 assert(P::TwinBatchPrefetch==1 && P::FullBatchBuffers==2 && P::RawCopyBytes==0 && P::SecondBatchAddress==0x9a000);
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_OUTPUT_CLOSURE
 assert(P::BatchedOutputClosure==1 && P::GlobalOutputReadsPerGroup==10);
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_PHASE_PROFILE
 assert(P::TokenPhaseProfile==1 && P::PhaseProfileAddress==0x75710 && P::PhaseProfileWords==60);
 assert(P::RootText[(0x509ac-0x40000)/4]==0x00a00613); // actual relocated unchanged10-read output admission
#else
 assert(P::RootText[(0x504e8-0x40000)/4]==0x00a00613);
#endif
#endif
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_WIDE_GDN
 assert(P::WideGdn==1 && P::WideGdnDeltaAddress==0x8e600 && P::WideGdnDeltaWords==64 && P::WideGdnImports==13);
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_CONSTANT_Q4
 assert(P::ConstantQ4Issue==1 && P::ConstantQ4WorkerRevision==1);
#ifdef PIONEER_Q38_PROGRAM_DATA_TEST_MANUAL_TTSYNC
 assert(P::ManualTTSync==1 && P::ManualTTSyncRevision==1
  && P::ManualTTSyncOverrideClearRequired==1 && P::ManualTTSyncLocalTimeout==0);
 assert(sizeof(P::PeerGdn)==35944 && P::CodeWords(0)==40902 && P::CodeWords(1)==25790 && allWords==3109912);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_MATRIX_PHASE)
 assert(P::MatrixPhase==1 && P::MatrixPhaseVersion==2 && P::MatrixPhaseAddress==0x84500 && P::MatrixPhaseWords==16);
 assert(sizeof(P::PeerGdn)==35944 && P::CodeWords(0)==40902 && P::CodeWords(1)==26229 && allWords==3162153);
#else
 assert(sizeof(P::PeerGdn)==35944 && P::CodeWords(0)==40902 && P::CodeWords(1)==25827 && allWords==3114315);
#endif
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PHASE_PROFILE)
 assert(sizeof(P::PeerGdn)==35944 && P::CodeWords(0)==40902 && P::CodeWords(1)==25726 && allWords==3102296);
#else
 assert(sizeof(P::PeerGdn)==35944 && P::CodeWords(0)==40594 && P::CodeWords(1)==25726 && allWords==3101988);
#endif
 for(unsigned w=1;w<120;w++)for(unsigned i=0;i<64;i++){
  uint32_t unchanged=0xdeadbeef;const unsigned address=P::WideGdnDeltaAddress+i*4;
  assert(S::SnapshotContains(w,address) && !P::ProgramWord(w,address,unchanged) && unchanged==0xdeadbeef);
 }
 assert(S::SavedWords==13478458 && S::SavedBytes==53913832);
#else
 assert(allWords==3086042 && P::CodeWords(0)==40594 && P::CodeWords(1)==25592);
#endif
 const unsigned fetchEnd=0x7050c;
#else
 assert(allWords==3085090 && P::CodeWords(0)==40594 && P::CodeWords(1)==25584);
 const unsigned fetchEnd=0x704ec;
#endif
 for(unsigned w=0;w<120;w++)for(unsigned at:{0x70001U,fetchEnd,0x90000U,0x98000U,0x7570cU}){
  uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(w,at,unchanged) && unchanged==0xdeadbeef);refusals++;
 }
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FLAT_MATRIX)
 assert(P::FlatMatrixGrammar==1 && P::EmptyReceiptPoll==1 && P::ReadSourceBoundary==1 && P::CounterGather==1);
 assert(P::CounterGatherReads==8 && P::TidBoundary==1 && P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3043919 && P::CodeWords(0)==40597 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PREFETCH)
 assert(P::TilePrefetch==1 && P::RawBuffers==2 && P::FetchAddress==0x70000 && P::FlatMatrixGrammar==1);
 assert(P::EmptyReceiptPoll==1 && P::ReadSourceBoundary==1 && P::CounterGather==1 && P::CounterGatherReads==8);
 assert(P::TidBoundary==1 && P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 assert(allWords==3068314 && P::CodeWords(0)==40597 && P::CodeWords(1)==25443);
 for(unsigned w=0;w<120;w++)for(unsigned at:{0x70001U,0x70358U,0x90000U,0x98000U,0x7570cU}){
  uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(w,at,unchanged) && unchanged==0xdeadbeef);refusals++;
 }
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_EMPTY_RECEIPT_POLL)
 assert(P::EmptyReceiptPoll==1 && P::ReadSourceBoundary==1 && P::CounterGather==1 && P::CounterGatherReads==8);
 assert(P::TidBoundary==1 && P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044257 && P::CodeWords(0)==40935 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COUNTER_GATHER)
 assert(P::CounterGather==1 && P::CounterGatherReads==8 && P::TidBoundary==1);
 assert(P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044233 && P::CodeWords(0)==40911 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_TID_BOUNDARY)
 assert(P::TidBoundary==1 && P::FifoEnabledRequired==1 && P::ActiveTid==0 && P::PacketIdentityBoundary==1);
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044164 && P::CodeWords(0)==40842 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PACKET_IDENTITY_BOUNDARY)
 assert(P::PacketIdentityBoundary==1 && P::CohortLocalSources==1 && P::ColdSourceFromStatus==1);
 assert(P::CommitBoundarySeals==1 && P::RetainedConvClosure==1 && P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32);
 assert(P::PreciseBeta==1 && P::JobBootFold==1 && P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044082 && P::CodeWords(0)==40760 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COHORT_LOCAL_SOURCES)
 assert(P::CohortLocalSources==1 && P::ColdSourceFromStatus==1 && P::CommitBoundarySeals==1 && P::RetainedConvClosure==1);
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044109 && P::CodeWords(0)==40787 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_COMMIT_BOUNDARY)
 assert(P::CommitBoundarySeals==1 && P::RetainedConvClosure==1 && P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32);
 assert(P::PreciseBeta==1 && P::JobBootFold==1 && P::TokenFrontier==1 && P::FrontierAddress==0x7570c);
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044230 && P::CodeWords(0)==40908 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_RETAINED_COHORT)
 assert(P::RetainedConvClosure==1 && P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c && S::SnapshotContains(0,P::FrontierAddress));
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044231 && P::CodeWords(0)==40909 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FRONTIER)
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(P::TokenFrontier==1 && P::FrontierAddress==0x7570c && S::SnapshotContains(0,P::FrontierAddress));
 uint32_t unchanged=0xdeadbeef;assert(!P::ProgramWord(0,P::FrontierAddress,unchanged) && unchanged==0xdeadbeef);
 assert(allWords==3044254 && P::CodeWords(0)==40932 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_JOB_BOOT_FOLD)
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1 && P::JobBootFold==1);
 assert(allWords==3044253 && P::CodeWords(0)==40931 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_PRECISE_BETA)
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32 && P::PreciseBeta==1);
 assert(allWords==3044235 && P::CodeWords(0)==40913 && P::CodeWords(1)==25238);
#elif defined(PIONEER_Q38_PROGRAM_DATA_TEST_FAULT)
 assert(P::FaultMarker==0x51465431 && P::FaultSavedLinkOffset==32);
 assert(allWords==3048876 && P::CodeWords(0)==40913 && P::CodeWords(1)==25277);
#else
 assert(allWords==3048868 && P::CodeWords(0)==40905 && P::CodeWords(1)==25277);
#endif
 for(unsigned worker:{120U,UINT32_MAX}){
  uint32_t unchanged=0xdeadbeef;const auto span=P::CodeSpan(worker,0);
  assert(!P::CodeSpans(worker) && !P::CodeWords(worker) && P::CodeAddress(worker,0)==UINT32_MAX
   && span.address==UINT32_MAX && !span.words && !P::ProgramWord(worker,0x6000,unchanged) && unchanged==0xdeadbeef);refusals++;
 }
 const uint32_t* arrays[]={P::RootBoot,P::RootText,P::RootTerminal,P::RootAux,P::PeerBoot,P::PeerFront,
  P::PeerPrecise,P::PeerDense,P::PeerFfn,P::PeerService,
#if defined(PIONEER_Q38_PROGRAM_DATA_TEST_PREFETCH) || defined(PIONEER_Q38_PROGRAM_DATA_TEST_BATCHED_PREFETCH) || defined(PIONEER_Q38_PROGRAM_DATA_TEST_TWIN_BATCH_PREFETCH)
  P::PeerFetch,
#endif
  P::PeerAffine,P::PeerText,P::PeerGdn};
 for(const auto* array:arrays){
  const uint32_t before=*array;assert(!P::ProgramWord(0,0x6000,*const_cast<uint32_t*>(array)) && *array==before);refusals++;
 }
 printf("PASS distinct QTP3 immutable programs/all%zu sections/ALL120 code%u words byte-exact to authenticated artifacts/covered by proposed journal/%u gap+runtime+invalid+immutable-output refusals;DATA ONLY,not native/restore/model/physical/Work/TPS qualification\n",sizeof(arrays)/sizeof(arrays[0]),allWords,refusals);
}
