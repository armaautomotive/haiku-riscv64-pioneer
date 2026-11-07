/* SPDX-License-Identifier: MIT */
// OFFBOARD ONLY: execute the producer's actual Tensix words against a strict
// independent unpack/broadcast observer, not a model or silicon qualification.
#include "pioneer_q38_native_bfp_machine.h"
#include "../PioneerQ38NativeBfp4Band.h"
#include "../PioneerNativeBfp4BandFixture.h"

struct Band4Machine : NativeBfpMachine {
 unsigned tiles, clears=0;
 explicit Band4Machine(unsigned k):tiles(k/32) { assert(PioneerQ38NativeBfp4Band::Width(k)); }
 void UnpackBand(unsigned which) {
  assert(which<2 && thread[0]==0 && thread[5]==4);
  const unsigned base=64+which*48,format=config[base]&15;
  assert(format==(which?5U:7U) && (config[base+8]&15)==format);
  assert(config[base]==(which?0x00200015U:0x01000017U));
  assert(config[base+1]==(which?0x00010001U:0x00040001U) && config[base+2]==1);
  assert(!(config[base+3]>>24) && (config[base+8]&~0x12fU)==0 && !(config[base+9]&0x1f0));
  assert((config[base+9]&1) && config[base+10]==0x17fff && !config[base+11] && !config[base+28]);
  const unsigned address=(config[base+12]+1)*16,tile=unpacks[which]%tiles;
  assert(address==(which?0x124000+tile*80:0x90000+tile*592));
  assert(xStart[which]==0 && xEnd[which]==(which?31U:1023U));
  assert(address>=16 && address+(which?64U:576U)<=0x130000);
  for(unsigned h=0;h<16;h++)assert(!l1[address-16+h]);
  auto& s=source[which][unpackBank[which]];if(s.pending || s.owner)throw Unknown;
  unsigned dest=which?config[61]:config[49]+(config[84]&65535);
  if(which){assert(!(dest&1));dest/=2;}
  for(unsigned i=0;i<(which?32U:1024U);i++){
   unsigned row=(dest+i)/16,col=(dest+i)%16;
   if(!which){assert(row>=4);row=(row-4)&63;if(config[base+8]&0x100){unsigned low=row&15;row=(row&~15U)|col;col=low;}}
   assert(row<64);s.values[row][col]=Datum(address,i,format,which?0:64);
  }
  s.pending=true;s.due=cycle+delay[which];unpacks[which]++;
 }
 void Broadcast(unsigned op){
  assert(thread[2]==1 && thread[3]==1 && !config[2] && !config[3]);
  assert((config[1]&0xe0000000)==0x60000000 && !(config[1]&0x1ffff));
  assert((config[0]&0x4210)==0x4210 && ((config[0]>>5)&15)==5 && (config[0]&15)==7);
  assert(!thread[11] && !thread[13] && !thread[48] && ((thread[29]>>13)&3)==2);
  unsigned dr=op&1023;assert((dr==32 || dr==40) && (op&~0x00c003ffU)==0x26084000);
  auto& a=source[0][matrixBank[0]];auto& b=source[1][matrixBank[1]];if(!a.owner || !b.owner)throw Unknown;
  unsigned ar=rwcA&0x30,br=rwcB&63,phase=(fidelity+thread[11])&3;
  assert(ar<=48 && br==(ar/16&1) && dr==(ar<32?32U:40U) && (phase==0 || phase==2));visits[phase]++;
  for(unsigned col=0;col<16;col++){
   long double sum=0;for(unsigned k=0;k<16;k++)sum+=static_cast<long double>(Part(b.values[br][k],phase,false))*Part(a.values[ar+k][col],phase,true);
   if(phase!=dropPhase)for(unsigned row=0;row<7;row+=2)destination[dr+row][col]=float(float(sum)+(valid[dr+row]?destination[dr+row][col]:0));
  }
  valid[dr]=true;for(unsigned row=4;row<8;row++)valid[dr+row]=true;
  if(!omitFlip)for(unsigned w=0;w<2;w++)if(op&(1U<<(22+w))){source[w][matrixBank[w]].owner=false;matrixBank[w]^=1;}
  fidelity=(fidelity+((thread[29]>>13)&3))&3;matrixOps++;mathPending=true;
 }
 void Op(unsigned op){
  if(op==0x10180000){assert(clears<2 && unpacks[0]==clears*tiles && unpacks[1]==unpacks[0] && matrixOps==clears*tiles*8);clears++;NativeBfpMachine::Op(op);}
  else if(op==0x420000c1 || op==0x428000c1){cycle++;Advance();UnpackBand((op>>23)&1);}
  else if((op>>24)==0x26){cycle++;Advance();Broadcast(op);}
  else NativeBfpMachine::Op(op);
 }
};

struct Fixture {
 std::vector<std::array<uint8_t,576>> packed;
 std::vector<std::array<uint16_t,32>> input;
 std::array<float,32> expected{};
 explicit Fixture(unsigned k):packed(k/32),input(k/32){
  for(unsigned tile=0;tile<k/32;tile++){
   std::array<float,1024> original{},decoded{};
   // Vary shared exponents across tiles so long reductions exercise real
   // alignment/rounding, not just an exactly representable fixed-point sum.
   for(unsigned r=0;r<32;r++)for(unsigned c=0;c<32;c++)original[r*32+c]=std::ldexp(float(int((r*37+c*19+tile*11)%255)-127)/128,int(tile%13)-6);
   for(unsigned c=0;c<32;c++)input[tile][c]=uint16_t(0x3f00+((c*13+tile*7)%256)+((c&3)==2?0x8000:0));
   assert(PioneerBlackholeBlockFloat::PackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,original.data(),1024,packed[tile].data(),576));
   assert(PioneerBlackholeBlockFloat::UnpackTile(PioneerBlackholeBlockFloat::Format::Bfp4B,packed[tile].data(),576,decoded.data(),1024));
   // Independent scalar reference on logical rows, matching the phasewise
   // FP32 accumulation order. This is ideal arithmetic, not silicon rounding.
   for(unsigned half=0;half<2;half++)for(unsigned phase:{0U,2U})for(unsigned row=0;row<32;row++){
    long double sum=0;for(unsigned c=0;c<16;c++)sum+=static_cast<long double>(NativeBfpMachine::Part(NativeBfpMachine::Float(uint32_t(input[tile][half*16+c])<<16),phase,false))*NativeBfpMachine::Part(decoded[row*32+half*16+c],phase,true);
    expected[row]=float(float(sum)+expected[row]);
   }
  }
 }
 void Stage(Band4Machine& m)const{
  assert(packed.size()==m.tiles && input.size()==m.tiles);
  for(unsigned tile=0;tile<m.tiles;tile++){
   unsigned wa=0x90000+tile*592,ba=0x124000+tile*80;
   memset(m.l1.data()+wa-16,0,16);memcpy(m.l1.data()+wa,packed[tile].data(),576);memset(m.l1.data()+ba-16,0,16);
   for(unsigned c=0;c<32;c++){m.l1[ba+c*2]=uint8_t(input[tile][c]);m.l1[ba+c*2+1]=uint8_t(input[tile][c]>>8);}
  }
 }
};

#ifndef PIONEER_NATIVE_BFP4_MACHINE_ONLY
int main(){
 for(unsigned k:{5120U,6144U,17408U}){
  Fixture f(k);Band4Machine m(k);f.Stage(m);m.delay[0]=100003;m.delay[1]=7;
  // The future device fixture is independently integer-packed. Check every
  // byte/header against the codec-based test fixture before using any oracle.
  for(unsigned tile=0;tile<k/32;tile++){
   for(unsigned offset=0;offset<592;offset+=4){
    unsigned address=0x90000-16+tile*592+offset,word=0;
    for(unsigned b=0;b<4;b++)word|=unsigned(m.l1[address+b])<<(8*b);
    assert(word==PioneerNativeBfp4BandFixture::Word(k,address));
   }
   for(unsigned offset=0;offset<80;offset+=4){
    unsigned address=0x124000-16+tile*80+offset,word=0;
    for(unsigned b=0;b<4;b++)word|=unsigned(m.l1[address+b])<<(8*b);
    assert(word==PioneerNativeBfp4BandFixture::Word(k,address));
   }
  }
  assert(!PioneerNativeBfp4BandFixture::Word(k,0x90001) && !PioneerNativeBfp4BandFixture::Word(32,0x90000));
  assert(PioneerQ38NativeBfp4Band::Run(m,k));assert(m.Clean());auto actual=m.Results();
  for(unsigned row=0;row<32;row++)assert(NativeBfpMachine::Bits(actual[row])==NativeBfpMachine::Bits(f.expected[row]));
  assert(m.clears==1 && m.unpacks[0]==k/32 && m.unpacks[1]==k/32 && m.matrixOps==8*k/32);
  assert(m.visits[0]==4*k/32 && m.visits[2]==4*k/32 && !m.visits[1] && !m.visits[3]);
  printf("PASS K%u: 32 exact ideal phasewise outputs, BFP4 unpack and two-term broadcast; OFFBOARD, no silicon/model/speed claim\n",k);
  if(k==5120){
   for(unsigned phase:{0U,2U}){
    Band4Machine bad(k);f.Stage(bad);bad.dropPhase=phase;assert(PioneerQ38NativeBfp4Band::Run(bad,k));auto out=bad.Results();
    bool differs=false;for(unsigned row=0;row<32;row++)differs|=NativeBfpMachine::Bits(out[row])!=NativeBfpMachine::Bits(f.expected[row]);assert(differs);
   }
   for(unsigned producer:{1U,2U}){
    Band4Machine bad(k);f.Stage(bad);bad.dropProducer=producer;bool refused=false;
    try{PioneerQ38NativeBfp4Band::Run(bad,k);}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
    assert(refused && !bad.matrixOps && !bad.outputReads);
   }
   Band4Machine bad(k);f.Stage(bad);bad.omitFlip=true;bool refused=false;
   try{PioneerQ38NativeBfp4Band::Run(bad,k);}catch(uint32_t marker){refused=marker==NativeBfpMachine::Unknown;}
   assert(refused && !bad.outputReads);
   puts("PASS failure controls: both missing producers, omitted source-bank release, and each missing fidelity term detected; OFFBOARD only");
  }
 }
}
#endif
