/* SPDX-License-Identifier: MIT */
// Actual RV32 BFP8 leaf, authenticated exported model weights and synthetic
// input in an offline NoC/tensor simulation. No native/card/Work claim.
#define PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION 1
#define PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT 1
#define PIONEER_BFP8_WORKER_IMAGE_ONLY 1
#include "pioneer_q38_bfp8_worker_leaf_image_test.cpp"
#include <fstream>
int main(int argc,char** argv){
 assert(argc==4 || (argc==5 && !std::strcmp(argv[4],"--capture")));
 const bool capture=argc==5;std::array<uint32_t,96> outputs{};
 for(unsigned kind=0;kind<2;kind++){
  std::ifstream f(argv[2+kind],std::ios::binary);std::vector<uint8_t> data{std::istreambuf_iterator<char>(f),{}};
  const unsigned width=5120,tiles=160,bands=2;
  assert(data.size()==32+size_t(bands)*(size_t(tiles)*1088+128));
  uint32_t header[8];std::memcpy(header,data.data(),sizeof(header));
  assert(header[0]==0x31525051 && header[1]==1 && header[3]==width && header[4]==64
   && header[5]==bands && header[6]==tiles && header[7]==1088);
  WorkerImage image(argv[1],kind?119:1,width,bands);
  for(unsigned band=0;band<bands;band++){
   const size_t at=32+size_t(band)*(size_t(tiles)*1088+128);
   std::memcpy(image.transfer.weights.data()+size_t(band)*tiles*1088,data.data()+at,size_t(tiles)*1088);
  }
  for(unsigned i=0;i<width;i++){
   uint16_t value=uint16_t(0x3f00+(i*13)%256+((i&3)==2?0x8000:0));
   image.transfer.root[i*2]=uint8_t(value);image.transfer.root[i*2+1]=uint8_t(value>>8);
  }
  assert(image.Execute());
  for(unsigned band=0;band<bands;band++)for(unsigned row=0;row<32;row++){
   uint32_t expected;const size_t at=32+size_t(band)*(size_t(tiles)*1088+128)+size_t(tiles)*1088+row*4;
   std::memcpy(&expected,data.data()+at,4);
   uint32_t actual=image.l1[image.Index(W::Output+(band*32+row)*4)];assert(actual==expected);
   if(band*32+row<48)outputs[kind*48+band*32+row]=actual;
   if(band==1 && row>=16)assert(!actual);
  }
  if(!capture)std::printf("PASS actual BFP8 RV32 leaf %s: 48 model-derived rows plus16 zero padding; NO DEVICE\n",kind?"beta":"alpha");
 }
 if(capture)assert(fwrite(outputs.data(),4,outputs.size(),stdout)==outputs.size() && !fflush(stdout));
}
