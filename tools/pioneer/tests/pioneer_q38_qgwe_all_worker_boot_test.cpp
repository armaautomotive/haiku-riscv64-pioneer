/* SPDX-License-Identifier: MIT */
// Execute all distinct QGWE worker entries with synthetic boot records.
#define main pioneer_unused_head_worker_main
#include "pioneer_q38_hybrid_head_worker_image_test.cpp"
#undef main
int main(int argc,char** argv){
 assert(argc==3 || argc==4);unsigned total=0;std::ifstream input;
 if(argc==4){input.open(argv[3],std::ios::binary);assert(input);}
 for(unsigned peer=1;peer<120;peer++){
  auto image=std::make_unique<HybridImage>(argv[1],argv[2],peer,5120,1);
  if(argc==4){
   input.read(reinterpret_cast<char*>(image->boot.data()),sizeof(image->boot));assert(input.gcount()==sizeof(image->boot));
   for(unsigned i=0;i<16;i++)image->source[i]=image->boot[16+i];
   for(unsigned i=0;i<64;i++)image->Put(HW::Boot+i*4,image->boot[i]);
  }
  image->Boot();assert(image->boot[0]==0x45574751 && image->boot[3]==peer
   && image->boot[4]==HW::Peer(peer) && image->boot[40]==0x454f4751
   && image->Get(HW::Status+12)==0x45574751 && !image->transfer.issues && !image->stores);
  total++;
 }
 assert(total==119);
 if(argc==4)assert(input.peek()==EOF);
 puts(argc==4?"PASS actual QGWE entry instructions for all 119 startup-derived synthetic worker boots; NO ROOT, DEVICE, INFERENCE OR SPEED"
             :"PASS actual QGWE entry instructions for all 119 distinct synthetic worker boots; NO ROOT, DEVICE, INFERENCE OR SPEED");
}
