/* SPDX-License-Identifier: MIT */
// Actual opt-in Work session, synthetic source and simulated ioctl transport.
#define PIONEER_Q38_MODE_SESSION_MOCK_ONLY
#define PIONEER_Q38_WORK_INFERENCE 1
#define PIONEER_Q38_WORK_BATCH_CLIENT 1
#define PIONEER_Q38_WORK_PHASE_PROFILE 1
#include "pioneer_q38_mode_session_test.cpp"

struct BatchSessionIO:WorkSessionIO {
 unsigned batches=0,singles=0;bool oldDriver=false,partial=false,badNonce=false,badPacket=false,badSupport=false;
 BatchSessionIO(FakeSource& s,unsigned t,unsigned workers):WorkSessionIO(s,t,workers) {}
 bool Call(int fd,uint32_t op,void* data,size_t bytes){
  if(op==TT_PCI_PROBE_RELOAD_STATUS){
   auto& id=*static_cast<tt_pioneer_driver_reload*>(data);assert(bytes==sizeof(id));
   id.driver=55;id.build_tag=oldDriver?0x2026102100010012ULL:0x2026111300010152ULL;
   id.generation=3;id.guard_version=1;return true;
  }
  if(op==TT_PCI_PROBE_Q38_WORK_BATCH_SUPPORT){
   auto& s=*static_cast<PioneerQ38WorkBatchSupport*>(data);assert(bytes==sizeof(s));
   s={PIONEER_Q38_WORK_BATCH_VERSION,badSupport?3U:unsigned(PIONEER_Q38_WORK_BATCH_COUNT),55,0,0x2026111300010152ULL};return true;
  }
  if(op==TT_PCI_PROBE_Q38_WORK_BATCH_RUN){
   auto& p=*static_cast<PioneerQ38WorkBatchPacket*>(data);assert(bytes==sizeof(p) && p.count==2);batches++;
   if(!WorkSessionIO::Call(fd,TT_PCI_PROBE_Q38_WORK_RUN,&p.jobs[0],sizeof(p.jobs[0])))return false;
   if(partial)return false; // partially copied reply is intentionally discarded
   if(!WorkSessionIO::Call(fd,TT_PCI_PROBE_Q38_WORK_RUN,&p.jobs[1],sizeof(p.jobs[1])))return false;
   if(badNonce)p.jobs[1].result.nonce--;
   if(badPacket)p.version++;
   return true;
  }
  if(op==TT_PCI_PROBE_Q38_WORK_RUN)singles++;
  return WorkSessionIO::Call(fd,op,data,bytes);
 }
};

int main(){
 (void)&Check;(void)&CohortHostTestMain;
 Fixture fixture;FakeSource source(fixture.file);const auto* t=source.Model().Find("blk.3.attn_k.weight");assert(t);
 const unsigned index=unsigned(t-source.Model().Tensors().data());
 std::vector<float> input(5120,.125f),output(1024,-99),before=output;
 bool sawSingle=false,sawBatch=false;
 for(unsigned workers:{1U,2U,3U,4U,5U,6U,7U,8U,9U,10U}){
  BatchSessionIO io(source,index,workers);PioneerQ38WorkSession<BatchSessionIO,FakeSource> session(io,source);
  assert(session.Open() && session.Multiply(index,input.data(),5120,output.data(),1024,workers));
  assert(io.workRuns==2*io.batches+io.singles && session.calls==io.workRuns
   && session.runIoctlCalls==io.batches+io.singles);
  if(io.batches)sawBatch=true;
  if(io.singles){assert(io.singles==1);if(io.batches)sawSingle=true;}
  for(unsigned row=0;row<output.size();row++)assert(output[row]==float(row+1));
  assert(session.Finish() && io.ends==1);
 }
 assert(sawBatch && sawSingle); // an odd wave count retains a final single Work call
	{
		Fixture pairFixture;FakeSource pairSource(pairFixture.file);
		const auto* key=pairSource.Model().Find("blk.3.attn_k.weight");
		const auto* value=pairSource.Model().Find("blk.3.attn_v.weight");assert(key&&value);
		const unsigned keyIndex=unsigned(key-pairSource.Model().Tensors().data());
		const unsigned valueIndex=unsigned(value-pairSource.Model().Tensors().data());
		std::vector<float> shared(5120,.25f),keyOutput(1024,-7),valueOutput(1024,-9);
		BatchSessionIO io(pairSource,keyIndex,119);io.allowPairTensor=true;
		PioneerQ38WorkSession<BatchSessionIO,FakeSource> session(io,pairSource);
		assert(session.Open() && session.MultiplyPair(keyIndex,valueIndex,shared.data(),5120,keyOutput.data(),1024,valueOutput.data(),1024));
		assert(io.batches==1 && io.workRuns==2 && io.singles==0 && session.calls==2 && session.runIoctlCalls==1);
		for(unsigned row=0;row<1024;row++)assert(keyOutput[row]==float(row+1) && valueOutput[row]==float(row+1));
		assert(session.Finish() && io.ends==1);
	}
	for(unsigned fault=0;fault<2;fault++){
		Fixture pairFixture;FakeSource pairSource(pairFixture.file);
		const auto* key=pairSource.Model().Find("blk.3.attn_k.weight");
		const auto* value=pairSource.Model().Find("blk.3.attn_v.weight");assert(key&&value);
		const unsigned keyIndex=unsigned(key-pairSource.Model().Tensors().data());
		const unsigned valueIndex=unsigned(value-pairSource.Model().Tensors().data());
		std::vector<float> shared(5120,.25f),keyOutput(1024,-7),valueOutput(1024,-9);
		BatchSessionIO io(pairSource,keyIndex,119);io.allowPairTensor=true;io.partial=fault==0;io.badNonce=fault==1;
		PioneerQ38WorkSession<BatchSessionIO,FakeSource> session(io,pairSource);
		assert(session.Open() && !session.MultiplyPair(keyIndex,valueIndex,shared.data(),5120,keyOutput.data(),1024,valueOutput.data(),1024));
		assert(io.batches==1 && session.runIoctlCalls==1 && keyOutput[0]==-7 && valueOutput[0]==-9
			&& session.Failed() && !io.ends && !session.Finish());
	}
 for(unsigned fault=0;fault<5;fault++){
  BatchSessionIO io(source,index,1);io.oldDriver=fault==0;io.partial=fault==1;io.badNonce=fault==2;
  io.badPacket=fault==3;io.badSupport=fault==4;
  PioneerQ38WorkSession<BatchSessionIO,FakeSource> session(io,source);output=before;
  if(fault==0 || fault==4)assert(!session.Open() && !io.begins && !io.batches);
  else assert(session.Open() && !session.Multiply(index,input.data(),5120,output.data(),1024,1)
   && output==before && session.Failed() && io.batches==1 && !io.ends && !session.Finish());
 }
 puts("PASS opt-in Work batch session: paired waves, candidate identity, odd fallback and partial-reply discard; SIMULATED IO ONLY");
}
