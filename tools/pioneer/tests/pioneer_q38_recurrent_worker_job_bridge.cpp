/* SPDX-License-Identifier: MIT */
// Inspection-only worker endpoint. Boot/job bytes cross a host pipe; no card.
#define main recurrent_worker_helper_main
#include "pioneer_q38_recurrent_out_real_worker_test.cpp"
#undef main

int main(int argc,char** argv){
 assert(argc==4 && (!std::strcmp(argv[3],"--boot") || !std::strcmp(argv[3],"--run")));
 if(!std::strcmp(argv[3],"--boot")){
  DenseProducerImage image(argv[1],argv[2],6144,1,1);
  assert(fwrite(image.boot.data(),sizeof(uint32_t),64,stdout)==64 && !fflush(stdout));return 0;
 }
 uint32_t job[64];assert(fread(job,sizeof(uint32_t),64,stdin)==64 && fgetc(stdin)==EOF);
 const unsigned bands=job[20];assert(bands && bands<=8 && job[2]==1 && job[3]==1 && job[18]==6144 && job[34]==13);
 DenseProducerImage image(argv[1],argv[2],6144,bands,1);image.Boot();SetPrivateWeights(image,0);
 for(unsigned chunk=0;chunk<24;chunk++){
  auto producer=std::make_unique<DenseProducerImage>(argv[1],argv[2],6144,bands,chunk+1);producer->Boot();
  for(unsigned i=0;i<256;i++){const unsigned at=chunk*256+i;
   producer->input[i]=uint32_t(uint16_t(0x3f00+(at*13)%256+((at&3)==2?0x8000:0)))<<16;}
  producer->StageProducer(13,1,false,0);producer->Check(1);
  for(unsigned at=0;at<512;at+=4){const unsigned word=producer->bank.at(producer->descriptor[4]+producer->descriptor[12]+at);
   for(unsigned b=0;b<4;b++)image.transfer.root[chunk*512+at+b]=uint8_t(word>>(8*b));}
 }
 image.transfer.job.words[3]=job[19];image.transfer.job.words[5]=job[21];image.transfer.job.words[11]=2;
 assert(HW::JobShape(job,image.boot.data(),1,0));
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);
 // Run a producer first to establish the real prior worker nonce and state.
 // It uses the same input chunk and is not substituted with a host receipt.
 for(unsigned i=0;i<256;i++)image.input[i]=uint32_t(uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0)))<<16;
 image.StageProducer(13,1,false,0);image.Check(1);
 for(unsigned i=0;i<64;i++)image.Put(HW::Job+i*4,job[i]);image.Put(HW::Doorbell,2);
 assert(image.FinishCombined(2) && image.native.Clean());
 for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++)
  assert(image.Get(W::Output+(band*32+row)*4)==PioneerNativeBfpModelBandMetadata::Golden(0)[row]);
 puts("PASS linked ROOT job -> actual producer and matrix-worker instructions with real weight sample; SIMULATED TRANSPORT NO DEVICE");
}
