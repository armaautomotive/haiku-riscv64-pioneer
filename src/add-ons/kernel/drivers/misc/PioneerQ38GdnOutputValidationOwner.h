/* SPDX-License-Identifier: MIT */
// Included INSIDE the SAME actual decoder,not a parallel owner or a Work
// readback mode. Original Scope/NIU/PCI/reset/clock/ALLhost proofs apply.
#if !defined(PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_VALIDATION_NATIVE != 1 || !defined(PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE) || PIONEER_Q38_GDN_OUTPUT_EXECUTION_NATIVE != 1
#error Exact separate DEBUG55 same-owner output observer required
#endif
 bool outputValidationTraffic=false,outputReadbackDone=false;unsigned outputValidationPage=0;
 template<class IO> bool OutputValidationTransferGuard(IO& io){
  // Guard() has ALREADY proved actual context/Scope/mappings/NIUs/PCI and
  // BOTH reset observations. This ONE pending fixed read is not a running
  // ROOT;no other command/counter drift becomes an accepted baseline.
  if(!output || !output->done || !output->stopped || !output->rootProof || !output->image[0]
   || !gdnTimedReadbackDone || !complete || phase!=16 || projection->running[0]
   || resets[0]!=0x47800 || outputReadbackDone || outputValidationPage>=5)return false;
  const auto* m=output->bootstrap.manifest;
  const uint32_t expected[]={m[4]+TT_Q38_GDN_OUTPUT_RESULT+outputValidationPage*4096,0,m[8],0x80000,0,129,0,0,4096,0};
  for(unsigned i=0;i<10;i++)if(command[0][i]!=expected[i] || io.ResidentNocRead(i*4)!=expected[i])return false;
  if(io.ResidentNocRead(0x40)>1)return false;
  for(unsigned i=1;i<4;i++)if(io.ResidentNocRead(0x40+i*0x800))return false;
  const unsigned delta[]={1,64,1,1,0,0,1,0};
  for(unsigned i=0;i<8;i++)if(uint32_t(io.ResidentNocRead(PioneerQ38PanelCounter(i))-counts[0][i])>delta[i])return false;
  return true;
 }
 template<class IO> bool OutputValidationHeld(IO& io){
  if(!GdnOutputCompleted() || !gdnTimedReadbackDone || outputValidationTraffic || outputReadbackDone)return false;
  for(unsigned w=0;w<120;w++)if(projection->running[w] || resets[w]!=0x47800 || !Guard(io,w) || !OutputCurrent(io,w))return false;
  return true;
 }
 template<class IO> bool OutputValidationReadback(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  if(r.outputReadbackPages || r.outputReadbackWords || r.outputOperandWords || r.outputScaleWords || !OutputValidationHeld(io))return false;
  r.worker=0;if(!Guard(io,0) || !OutputCurrent(io,0))return false;
  // Compact native inputs are actual ROOT-local QOG1 results,not CPU
  // arithmetic or a caller-provided numeric vector. They are fixed READS.
  for(unsigned i=0;i<3072;i++){
   r.word=TT_Q38_GDN_OUTPUT_VALIDATION_OPERANDS+i;uint32_t v;
   if(!io.ProjectionGraphMemoryReadChecked(0,0xa0000+i*4,v))return false;
   for(unsigned half=0;half<2;half++){
    uint16_t operand=uint16_t(v>>(half*16));
    if((operand&0x7f80)!=0x4000 || operand==0xc000)return false;
   }
   values[r.word]=v;r.outputOperandWords++;
  }
  for(unsigned i=0;i<192;i++){
   r.word=TT_Q38_GDN_OUTPUT_VALIDATION_SCALES+i;uint32_t v;
   if(!io.ProjectionGraphMemoryReadChecked(0,0xab000+i*4,v) || (v&0x80000000)
    || (v && (v<0x2f800000 || v>0x47800000)))return false;
   values[r.word]=v;r.outputScaleWords++;
  }
  if(!Guard(io,0) || !OutputCurrent(io,0) || counts[0][2]!=counts[0][6])return false;
  for(unsigned page=0;page<5;page++){
   outputValidationPage=page;r.word=TT_Q38_GDN_OUTPUT_VALIDATION_RESULT+page*1024;
   if(!Guard(io,0) || !OutputCurrent(io,0))return false;
   for(unsigned word=0;word<1024;word++)if(!Write(io,0,0x80000+word*4,0x7fc50000+page*1024+word))return false;
   const auto* m=output->bootstrap.manifest;
   const uint32_t next[]={m[4]+TT_Q38_GDN_OUTPUT_RESULT+page*4096,0,m[8],0x80000,0,129,0,0,4096,0};
   for(unsigned i=0;i<10;i++){
    io.ResidentNocWrite(i*4,next[i]);if(io.ResidentNocRead(i*4)!=next[i])return false;command[0][i]=next[i];
   }
   if(!Guard(io,0))return false;
   uint64_t began=io.Q38CohortNow();if(began>UINT64_MAX-UINT64_C(30000000))return false;
   uint32_t expected[8];const unsigned delta[]={1,64,1,1,0,0,1,0};
   for(unsigned i=0;i<8;i++)expected[i]=counts[0][i]+delta[i];
   outputValidationTraffic=true;io.ResidentNocWrite(0x40,1);bool done=false;
   for(unsigned poll=0;poll<65536;poll++){
    if(!Guard(io,0))return false;
    uint64_t now=io.Q38CohortNow();if(now<began || now-began>=UINT64_C(30000000))return false;
    bool exact=io.ResidentNocRead(0x40)==0;
    for(unsigned i=0;i<8;i++)exact &= io.ResidentNocRead(PioneerQ38PanelCounter(i))==expected[i];
    if(exact){done=true;break;}io.Pause();
   }
   if(!done || !PioneerQ38GraphCsrIdle(io))return false;
   for(unsigned i=0;i<10;i++)if(io.ResidentNocRead(i*4)!=next[i])return false;
   for(unsigned i=0;i<8;i++)counts[0][i]=expected[i];
   outputValidationTraffic=false;if(!Guard(io,0) || !OutputCurrent(io,0))return false;
   for(unsigned word=0;word<1024;word++){
    r.word=TT_Q38_GDN_OUTPUT_VALIDATION_RESULT+page*1024+word;uint32_t v;
    if(!io.ProjectionGraphMemoryReadChecked(0,0x80000+word*4,v) || (v&0x7fffffff)>=0x7f800000)return false;
    values[r.word]=v;r.outputReadbackWords++;
   }
   r.outputReadbackPages++;if(!Guard(io,0) || !OutputCurrent(io,0))return false;
  }
  if(!OutputValidationHeld(io))return false;
  outputReadbackDone=r.outputReadbackPages==5 && r.outputReadbackWords==5120 && r.outputOperandWords==3072 && r.outputScaleWords==192;
  return outputReadbackDone;
 }
public:
 template<class IO> bool ReadGdnOutputValidationInput(IO& io,uint32_t* values,PioneerQ38ResidentDecoderResult& r){
  uintptr_t address=reinterpret_cast<uintptr_t>(values);
  if(!values || (address&3) || address>UINTPTR_MAX-TT_Q38_GDN_RENDEZVOUS_VALIDATION_WORDS*4
   || output || phase!=12 || !gatedOutput.Current() || gdnTimedReadbackDone || r.gdnValidationWorkers || r.gdnValidationWords)return false;
  // Keep phase12 and genuine held-only producer live. Readback does NOT
  // mint or mutate an execution epoch;native54 still claims it itself.
  return GdnTimedValidationReadback(io,values,r)?true:Fail(io,r);
 }
private:
