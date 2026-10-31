 template<class IO> __attribute__((noinline)) bool InspectTerminal(IO& io,PioneerQ38TokenTerminalResult& r,bool recheck){
  namespace C=PioneerQ38GlobalChainLayout;namespace R=C::R;namespace T=PioneerQ38TokenLayout;namespace W=PioneerQ38SelectedGlobalWorker;
  const PioneerQ38TokenTerminalResult pristine;const auto* a=reinterpret_cast<const uint8_t*>(&r);
  const auto* b=reinterpret_cast<const uint8_t*>(&pristine);
  for(unsigned i=0;i<sizeof(r);i++)if(a[i]!=b[i])return false;
  if(terminalObserved!=recheck || holding || held || !ExecutionStarted() || !Scope(io))return false;
  r.started=1;r.phase=9;
  if(!Sources(io,0))return Fail(io,r);
  uint32_t terminal[64],boot[64],job[64],status[16],counters[8],progress[3],cancel=0;
  uint32_t marker=0,error=0;
  if(!io.TokenMemoryReadChecked(0,R::Status+12,marker) || marker!=T::Magic
   || !io.TokenMemoryReadChecked(0,R::Status+8,error) || error)return Fail(io,r);
  uint32_t word=0;
  r.firstPC=io.RuntimePcRead();
  if(r.firstPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.firstPC,word) || !Sources(io,0))return Fail(io,r);
  r.secondPC=io.RuntimePcRead();
  if(r.secondPC==0x60000 || !PioneerQ38TokenPrograms::ProgramWord(0,r.secondPC,word) || !Sources(io,0))return Fail(io,r);
  if(!io.TokenMemoryReadChecked(0,T::TerminalLocal,terminal[0]))return Fail(io,r);
  if(!terminal[0])return recheck ? Fail(io,r) : Sources(io,0) ? true : Fail(io,r); // a published marker cannot regress
  if(terminal[0]!=T::TerminalMagic)return Fail(io,r);
  for(unsigned i=1;i<64;i++){r.word=i;if(!io.TokenMemoryReadChecked(0,T::TerminalLocal+i*4,terminal[i]))return Fail(io,r);}
  if(recheck && !PioneerQ38TokenTerminalRecheckData(terminal,observedTerminal,r.word))return Fail(io,r);
  if(r.firstPC!=0x60004 || r.secondPC!=0x60004)return true; // publication may precede STOP;no terminal grant
  if(!PioneerQ38GraphCsrIdle(io) || !startup->ReadBoot(0,boot))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,R::Boot+i*4,actual) || actual!=boot[i])return Fail(io,r);}
  for(unsigned i=0;i<3;i++)if(!io.TokenMemoryReadChecked(0,C::Progress+i*4,progress[i]))return Fail(io,r);
  auto* raw=reinterpret_cast<uint8_t*>(staging);
  if(!startup->ReadSession(0,raw,512))return Fail(io,r);
  for(unsigned i=0;i<128;i++)staging[i]=uint32_t(raw[i*4])|uint32_t(raw[i*4+1])<<8|uint32_t(raw[i*4+2])<<16|uint32_t(raw[i*4+3])<<24;
  if(!T::Session(staging,boot,plan.Bank(),plan.Workspace())
   || !PioneerQ38TokenTerminalShape(terminal,boot,progress,staging[5],staging[6],staging[32]!=0,cancelRequested))return Fail(io,r);
  for(unsigned i=0;i<16;i++)if(!io.TokenMemoryReadChecked(0,T::CancelLocal+i*4,cancel)
   || cancel!=(i==0 && cancelRequested?unsigned(T::CancelMagic):0U))return Fail(io,r);
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=terminal[36+i])return Fail(io,r);
  const unsigned stopLayer=terminal[2]==3 && terminal[10]==1?terminal[6]-1:64;
  for(unsigned w=1;w<120;w++){
   r.phase=10;r.worker=w;
   if(!Sources(io,0) || !startup->ReadBoot(w,boot))return Fail(io,r);
   uint32_t nonce=0;
   if(!io.TokenMemoryReadChecked(0,R::Nonces+(w-1)*4,nonce) || !nonce || nonce>=0x7fffffff
    || (recheck && nonce!=terminalNonces[w]))return Fail(io,r);
   for(unsigned i=0;i<64;i++){uint32_t cached=0;r.word=i;
    if(!io.TokenMemoryReadChecked(0,R::PeerBoots+(w-1)*256+i*4,cached) || cached!=boot[i]
     || !io.TokenMemoryReadChecked(0,R::PeerJobs+(w-1)*256+i*4,job[i]))return Fail(io,r);}
#if defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
   const bool jobShape=PreciseTerminalJobShape(job,boot,nonce-1);
#else
   const bool jobShape=W::JobShape(job,boot,nonce-1,0);
#endif
   if(!jobShape || job[1]!=nonce || job[2]!=W::Stop || job[3]!=w || job[4]!=W::Peer(w)
    || job[5]!=stopLayer || job[6]!=terminal[4] || job[7]!=(terminal[2]==3?9998U:9999U))return Fail(io,r);
   for(unsigned i=0;i<8;i++)if(!io.TokenMemoryReadChecked(0,R::Counters+(w-1)*32+i*4,counters[i]))return Fail(io,r);
   if(counters[2]!=counters[6] || !Sources(io,w) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
   for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
    if(!io.TokenMemoryReadChecked(w,W::Boot+i*4,actual) || actual!=boot[i]
     || !io.TokenMemoryReadChecked(w,W::Job+i*4,actual) || actual!=job[i])return Fail(io,r);}
   uint32_t doorbell=0;if(!io.TokenMemoryReadChecked(w,W::Doorbell,doorbell) || doorbell!=nonce)return Fail(io,r);
   for(unsigned i=0;i<16;i++)if(!io.TokenMemoryReadChecked(w,W::Status+i*4,status[i]))return Fail(io,r);
   if(status[0]!=nonce || status[1]!=nonce || status[2] || status[3]!=W::Magic || status[4]!=W::Stop || status[5]
    || status[6]!=stopLayer || status[7]!=terminal[4])return Fail(io,r);
   for(unsigned i=8;i<16;i++)if(status[i])return Fail(io,r);
   r.firstPC=io.RuntimePcRead();r.secondPC=io.RuntimePcRead();
   if((r.firstPC!=0x20060 && r.firstPC!=0x20064) || (r.secondPC!=0x20060 && r.secondPC!=0x20064))return Fail(io,r);
   for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=counters[i])return Fail(io,r);
   if(!Sources(io,w) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
   for(unsigned i=0;i<8;i++){
    if(recheck && terminalCounters[w][i]!=counters[i])return Fail(io,r);
    terminalCounters[w][i]=counters[i];
   } // authenticated current closed STOP journal ONLY;rechecks cannot adopt a changed journal
   terminalNonces[w]=nonce;
  }
  r.phase=11;r.worker=0;
  if(!Sources(io,0) || !PioneerQ38GraphCsrIdle(io))return Fail(io,r);
  for(unsigned i=0;i<64;i++){uint32_t actual=0;r.word=i;
   if(!io.TokenMemoryReadChecked(0,T::TerminalLocal+i*4,actual) || actual!=terminal[i])return Fail(io,r);}
  for(unsigned i=0;i<8;i++)if(io.ResidentNocRead(PioneerQ38PanelCounter(i))!=terminal[36+i])return Fail(io,r);
  r.firstPC=io.RuntimePcRead();r.secondPC=io.RuntimePcRead();
  if(r.firstPC!=0x60004 || r.secondPC!=0x60004 || !Sources(io,0))return Fail(io,r);
  for(unsigned i=0;i<64;i++){
   if(recheck && observedTerminal[i]!=terminal[i])return Fail(io,r);
   observedTerminal[i]=terminal[i];
  }
  for(unsigned i=0;i<8;i++)terminalCounters[0][i]=terminal[36+i];
  terminalObserved=true;
  r.ready=1;r.cancelled=terminal[2]==3;r.committed=terminal[3];r.position=terminal[4];r.completedLayers=terminal[6];r.stage=terminal[10];
  return true;
 } // READ-ONLY genuine STOP observation. NO HOLD/reset/NIU commands,counter recapture,restore/END/disposal or reusable state grant.
