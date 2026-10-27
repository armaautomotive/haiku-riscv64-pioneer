/* SPDX-License-Identifier: MIT */
// Included INSIDE the new ROOT namespace. All peer observations are real
// NoC reads; no caller nonce/ready setter or old fresh-boot grammar.
static const uint32_t* Boot(unsigned w){return reinterpret_cast<const uint32_t*>(L::PeerBoots+w*256);}
#if !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
static bool Executing(unsigned pc){
 if(pc&3)return false;
 if(pc>=0x20060 && pc<0x200a4)return false; // STOP/fault, never completion
 return (pc>=0x20000 && pc<0x21130) || (pc>=0x40000 && pc<0x4285c)
  || (pc>=0x60000 && pc<0x64048) || (pc>=0x68000 && pc<0x6a6a0)
  || (pc>=0x70b00 && pc<0x71ff0) || (pc>=0x72000 && pc<0x73760)
  || (pc>=0xb8000 && pc<0xc0aec);
}
#endif
static void Wait(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned phase,bool admission=false){
 unsigned sequence=admission?0:Words(L::Nonces)[w];
 for(unsigned poll=0;poll<65536;poll++){
  if(Register(d,tracked,w,0xffb121b0)!=0x46800)scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);uint32_t s[16];for(unsigned i=0;i<16;i++)s[i]=Words(L::Scratch)[i];
  if(s[3]!=M::Magic || s[2] || s[0]>=0x7fffffff || s[1]>=0x7fffffff || s[5]>3 || s[6]>=64 || s[7]>=8192)scalar_fault_park();
  for(unsigned i=8;i<16;i++)if(s[i])scalar_fault_park();
  unsigned pc=Register(d,tracked,w,0xffb13140);
#if !defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) && !defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
  if(!Executing(pc))scalar_fault_park();
#endif
  // LIVE: this CSR samples the speculative frontend,not retirement.
  // Non-WAIT samples grant nothing;only a real coherent completion and
  // positive WAIT below can finish this finite poll. Status faults still
  // fail immediately. Never mask/repair a PC or infer success from one.
  if(admission){
   if(s[4] || s[5] || s[0]!=s[1])scalar_fault_park();
   sequence=s[1];
  }else{
   unsigned previous=phase?sequence-1:sequence;
   if(s[0]<previous || s[0]>sequence || s[1]<previous || s[1]>sequence
    || (s[4] && s[4]!=unsigned(M::DenseScores)+phase-1))scalar_fault_park();
   if(s[0]!=sequence || s[1]!=sequence)continue;
   if(s[4] || s[5]!=(phase==3?0:phase))scalar_fault_park();
   if(phase && (s[6]!=d[3] || s[7]!=d[4]))scalar_fault_park();
  }
  if(pc!=0x2004c && pc!=0x20050)continue;
  if(admission)Words(L::Nonces)[w]=sequence;
  return;
 }
 scalar_fault_park();
}
static void PhysicalSource(const uint32_t* d,uint32_t* tracked,unsigned w){
 for(unsigned i=0;i<16;i++)if(Register(d,tracked,w,0xffb20000+Source(i))!=Boot(w)[16+i])scalar_fault_park();
}
static void BootSeal(const uint32_t* d,uint32_t* tracked,unsigned w){
 Transfer(d,tracked,w,M::Boot,256,false);
 for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=Boot(w)[i])scalar_fault_park();
 PhysicalSource(d,tracked,w);
}
static void ClosedJob(const uint32_t* d,uint32_t* tracked,unsigned w,bool capture){
 unsigned n=Words(L::Nonces)[w];
 Transfer(d,tracked,w,M::Job,256,false);
 if(capture){
  if(!n)scalar_fault_park(); // this consumer requires a real closed upstream job
  {
   uint32_t j[64];for(unsigned i=0;i<64;i++)j[i]=Words(L::Scratch)[i];
   if(j[1]!=n || j[2]==M::Stop || j[2]==M::DenseScores || j[2]==M::DenseExp
    || !M::JobShape(j,Boot(w),n-1,j[2]==M::DenseValues?2:0))scalar_fault_park();
  }
  Copy(L::PeerJobs+w*256,L::Scratch,64);
 }else for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=Words(L::PeerJobs+w*256)[i])scalar_fault_park();
 if(Register(d,tracked,w,M::Doorbell)!=n)scalar_fault_park();
 // Completion metadata and the immutable external job name the SAME frame.
 Transfer(d,tracked,w,M::Status,64,false);
 if(Words(L::Scratch)[0]!=n || Words(L::Scratch)[1]!=n || Words(L::Scratch)[2] || Words(L::Scratch)[3]!=M::Magic
  || Words(L::Scratch)[4] || Words(L::Scratch)[5]
  || Words(L::Scratch)[6]!=(n?Words(L::PeerJobs+w*256)[5]:0)
  || Words(L::Scratch)[7]!=(n?Words(L::PeerJobs+w*256)[6]:0))scalar_fault_park();
 for(unsigned i=8;i<16;i++)if(Words(L::Scratch)[i])scalar_fault_park();
}
static void Admit(const uint32_t* d,uint32_t* tracked,unsigned w){
 Wait(d,tracked,w,0,true);
#if defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
 if(L::FirstMatrix(d) && Words(L::Nonces)[w]>0x7ffffffd-L::RunDelta(w))scalar_fault_park(); // reserve the entire block AND a real STOP
#endif
 if(w<96 && Words(L::Nonces)[w]>0x7ffffffb)scalar_fault_park();
 Transfer(d,tracked,w,M::Boot,256,false);Copy(L::PeerBoots+w*256,L::Scratch,64);
 const auto* b=Boot(w);
 if(!M::BootShape(b) || b[3]!=w+1 || b[4]!=M::Peer(w+1))scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(b[8+i]!=d[32+i])scalar_fault_park();
#if defined(PIONEER_Q38_MIXED_JOINED_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE)
 if(d[0]==PioneerQ38MixedProjectionRootLayout::Magic){
  if(w<d[7] && b[16]!=Words(L::PartsShadow+w*32)[5])scalar_fault_park();
 }else if(w<96)for(unsigned i=0;i<16;i++)if(b[16+i]!=Frame(w)[16+i])scalar_fault_park();
#elif defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_PROJECTION_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_PROJECTION_ROOT_IMAGE)
 if(w<d[7] && b[16]!=Words(L::PartsShadow+w*32)[5])scalar_fault_park();
#else
 if(w<96)for(unsigned i=0;i<16;i++)if(b[16+i]!=Frame(w)[16+i])scalar_fault_park();
#endif
 PhysicalSource(d,tracked,w);
#if defined(PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE) || defined(PIONEER_Q38_MIXED_RECURRENT_BLOCK_ROOT_IMAGE)
 if(!Words(L::Nonces)[w]){
   if(!L::Virgin(d))scalar_fault_park(); // exact FIRST request/roles of THIS image,never recaptured later
  Transfer(d,tracked,w,M::Job,256,false);
  for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i])scalar_fault_park();
  Copy(L::PeerJobs+w*256,L::Scratch,64);
  if(Register(d,tracked,w,M::Doorbell))scalar_fault_park();
  Transfer(d,tracked,w,M::Status,64,false);
  for(unsigned i=0;i<16;i++)if(Words(L::Scratch)[i]!=(i==3?unsigned(M::Magic):0U))scalar_fault_park();
  // A virgin loop has NO arithmetic receipt. Observe real quiet queues
  // and counters;never read/fabricate an uninitialized proof/nonce.
  for(unsigned i=0;i<4;i++)if(Register(d,tracked,w,0xffb20040+i*0x800))scalar_fault_park();
  for(unsigned i=0;i<32;i++)if(Register(d,tracked,w,0xffb20240+i*4)&255)scalar_fault_park();
  for(unsigned i=0;i<8;i++)Words(L::Counters)[w*8+i]=Register(d,tracked,w,0xffb20000+Counter(i));
  if(Words(L::Counters)[w*8+2]!=Words(L::Counters)[w*8+6])scalar_fault_park();
  BootSeal(d,tracked,w);ClosedJob(d,tracked,w,false);
  for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
  return;
 }
#endif
 ClosedJob(d,tracked,w,true);
 const auto* previous=reinterpret_cast<const uint32_t*>(L::PeerJobs+w*256);
 bool dense=previous[2]==M::DenseValues;
 Transfer(d,tracked,w,dense?unsigned(W::Summary):0x83f00U,256,false);
 const auto* p=Words(L::Scratch);unsigned after=dense?16:8;
 if(dense){
  if(p[0]!=W::Magic || p[1]!=Words(L::Nonces)[w] || p[2]!=3 || p[48]!=previous[7]
   || p[49]!=w+1 || p[50]!=previous[5] || p[51]!=previous[6])scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(p[24+i]!=b[16+i])scalar_fault_park();
  for(unsigned i=0;i<8;i++)if(p[40+i]!=b[8+i])scalar_fault_park();
 }else{
  if(p[17]!=Words(L::Nonces)[w] || p[18]!=w+1 || p[19]!=0x32504651 || p[30]!=b[16] || p[31]!=b[4]
   || p[48]!=(previous[2]==1?0:previous[2]))scalar_fault_park();
  for(unsigned i=0;i<16;i++)if(p[32+i]!=b[16+i])scalar_fault_park();
 }
 for(unsigned i=0;i<8;i++)Words(L::Counters)[w*8+i]=p[after+i];
 for(unsigned i=0;i<8;i++){
  if(Words(L::Counters)[w*8+i]!=Register(d,tracked,w,0xffb20000+Counter(i)))scalar_fault_park();
  if(i==6 && Words(L::Counters)[w*8+6]!=Words(L::Counters)[w*8+2])scalar_fault_park();
 }
}
static void Job(unsigned w,unsigned phase,unsigned sequence=0){
 const auto* f=Frame(w);if(!sequence)sequence=Words(L::Nonces)[w]+1;
 for(unsigned i=0;i<64;i++)Words(L::Scratch)[i]=0;
 const uint32_t first[]={M::Magic,sequence,unsigned(M::DenseScores)+phase-1,w+1,M::Peer(w+1),f[3],f[4],f[40]};
 for(unsigned i=0;i<8;i++){Words(L::Scratch)[i]=first[i];Words(L::Scratch)[8+i]=f[32+i];}
}
static void Snapshot(const uint32_t* d,uint32_t* tracked,unsigned w,unsigned phase){
 const auto* f=Frame(w);Wait(d,tracked,w,phase);BootSeal(d,tracked,w);
 Transfer(d,tracked,w,W::Header,256,false);for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=f[i])scalar_fault_park();
 unsigned sequence=Words(L::Nonces)[w];uint32_t j[64];Job(w,phase,sequence);
 for(unsigned i=0;i<64;i++)j[i]=Words(L::Scratch)[i];
 Transfer(d,tracked,w,M::Job,256,false);for(unsigned i=0;i<64;i++)if(Words(L::Scratch)[i]!=j[i])scalar_fault_park();
 if(Register(d,tracked,w,M::Doorbell)!=sequence)scalar_fault_park();
 Transfer(d,tracked,w,W::Job,64,false);uint32_t internal[16];for(unsigned i=0;i<16;i++)internal[i]=Words(L::Scratch)[i];
 if(!W::JobShape(internal,f,sequence,phase) || Register(d,tracked,w,W::Doorbell)!=sequence)scalar_fault_park();
 Transfer(d,tracked,w,W::Summary,256,false);Copy(L::Proofs+w*256,L::Scratch,64);
 const auto* p=reinterpret_cast<const uint32_t*>(L::Proofs+w*256);
 unsigned reads=phase==2?0:f[46]>>1,flits=phase==2?0:f[47]>>1;
 if(p[0]!=W::Magic || p[1]!=sequence || p[2]!=phase || p[3]!=f[48] || p[4]!=f[49] || p[5]!=6 || p[6]!=reads || p[7]!=flits)scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  unsigned delta=i==1?flits:i==0 || i==2 || i==3 || i==6?reads:0U;
  if(p[8+i]!=Words(L::Counters)[w*8+i] || uint32_t(p[16+i]-p[8+i])!=delta)scalar_fault_park();
 }
 for(unsigned i=0;i<16;i++)if(p[24+i]!=f[16+i])scalar_fault_park();
 for(unsigned i=0;i<8;i++)if(p[40+i]!=f[32+i])scalar_fault_park();
 const unsigned expected[]={f[40],f[15],f[3],f[4],f[5],f[6],f[46],f[47]};
 for(unsigned i=0;i<8;i++)if(p[48+i]!=expected[i])scalar_fault_park();
 for(unsigned i=56;i<64;i++)if(p[i])scalar_fault_park();
 for(unsigned i=0;i<8;i++){
  unsigned actual=Register(d,tracked,w,0xffb20000+Counter(i));
  if(actual!=p[16+i])scalar_fault_park();
  Words(L::Counters)[w*8+i]=actual;
  if(i==6 && actual!=Words(L::Counters)[w*8+2])scalar_fault_park();
 }
 if(phase==2 || phase==3){
  Transfer(d,tracked,w,W::Parameters,64,false);
  for(unsigned i=0;i<16;i++)if(Words(L::Scratch)[i]!=Words(L::Parameters)[f[5]*16+i])scalar_fault_park();
 }
 if(phase==1 || phase==2){
  Transfer(d,tracked,w,W::Local,256,false);Copy(L::Reports+w*256,L::Scratch,64);
  const auto* r=reinterpret_cast<const uint32_t*>(L::Reports+w*256);
  if(r[0]!=phase || r[1]!=f[49] || r[2]!=f[48] || r[3]!=f[15])scalar_fault_park();
  for(unsigned h=0;h<6;h++){
   if((r[4+h]&0x7fffffff)>=0x7f800000 || (!f[49] && r[4+h]!=0xff7fffff))scalar_fault_park();
   if(phase==1)Words(L::Maxima)[w*6+h]=r[4+h];
   else if(r[4+h]!=Words(L::Maxima)[w*6+h] || r[19+h*4]!=1 || !PioneerQ38DensePositiveSum::Record(r[16+h*4],r[17+h*4],int(r[18+h*4])))scalar_fault_park();
  }
  for(unsigned i=10;i<64;i++)if((phase==1 || i<16 || i>=40) && r[i])scalar_fault_park();
 }
}
static void Final(const uint32_t* d,uint32_t* tracked,unsigned w){
 Wait(d,tracked,w,0);BootSeal(d,tracked,w);
 if(w<96){Job(w,3,Words(L::Nonces)[w]);Copy(L::PeerJobs+w*256,L::Scratch,64);}
 ClosedJob(d,tracked,w,false);
 for(unsigned i=0;i<8;i++)if(Register(d,tracked,w,0xffb20000+Counter(i))!=Words(L::Counters)[w*8+i])scalar_fault_park();
}
