/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FLAT_MATRIX_GRAMMAR_H
#define PIONEER_Q38_FLAT_MATRIX_GRAMMAR_H
// Pure QGW3 DATA grammar. UNSELECTED: no production caller,
// native owner, source/counter/receipt authority or arithmetic implementation.
// Compose the old QGW3->QGW2->QGW1->QMW1 normalizations without materializing
// their temporary arrays. Preserve unsigned boundary behavior for ALL jobs.
namespace PioneerQ38FlatMatrixGrammar {
using Word=__UINT32_TYPE__;
static_assert(sizeof(Word)==4,"exact DATA words");
enum {Magic=0x33574751,JobWitness=0x334f4751,Hidden=5120,Vocabulary=248320,
 GlobalMatrix=12,ActivationMagic=0x31415051,Chunks=68,ChunkValues=256,
 ReceiptMagic=0x33504d51,Root=129,Mailbox=0x118000,Stride=256,
 PushBuffer=0x83c00,ReceiptWords=64};
static inline unsigned Peer(unsigned worker){
 unsigned x=1;while(worker>=10){worker-=10;x++;}if(x>=8)x+=2;
 return x|((worker+2)<<6);
}
static inline bool BootShape(const Word* b){
 if(!b || b[0]!=Magic || b[1]!=1 || b[2]!=64 || !b[3] || b[3]>=120
  || b[4]!=Peer(b[3]) || !b[5] || b[5]!=b[16] || b[6]!=8192 || b[7]!=64)return false;
 for(unsigned i=8;i<16;i+=2)if(!(b[i]|b[i+1]))return false;
 for(unsigned i=32;i<40;i++)if(b[i])return false;
 static const Word extension[]={JobWitness,1,Hidden,Vocabulary,GlobalMatrix,1,
  ActivationMagic,Chunks,ChunkValues,ReceiptMagic,Root,Mailbox,Stride,PushBuffer,ReceiptWords};
 for(unsigned i=0;i<15;i++)if(b[40+i]!=extension[i])return false;
 for(unsigned i=55;i<64;i++)if(b[i])return false;
 return true; // b[16..31] remain source DATA,not current physical authority.
}
static inline bool JobShape(const Word* j,const Word* b,
 unsigned previous,unsigned densePhase){
 if(!j || j[0]!=Magic || !BootShape(b))return false;
 const bool globalStop=j[2]==2 && j[5]==64;
 if(j[2]==GlobalMatrix || globalStop){
  if(densePhase || previous>=0x7ffffffeU || j[1]!=previous+1U || !j[1]
   || j[1]>=0x7fffffffU || j[3]!=b[3] || j[4]!=b[4] || j[5]!=64
   || j[6]>=8192 || !j[7] || j[7]>=0x80000000U)return false;
  for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
  if(globalStop){for(unsigned i=16;i<64;i++)if(j[i])return false;return true;}
  if(j[16]!=0x31483851 || j[17]!=14 || j[18]!=3392 || j[19] || j[20] || j[21]!=Hidden
   || j[22]<0x04000000 || (j[22]&63) || !j[23] || j[23]>16 || j[25]!=1 || j[26]!=1
   || j[27]>=9 || (j[27]==8 && b[3]>21) || (j[28]&15) || j[28]>=Vocabulary
   || j[23]*16>Vocabulary-j[28] || j[29]!=Vocabulary || j[30]!=1 || j[31]!=JobWitness)return false;
  const unsigned bytes=j[23]*20*3392;
  if(j[22]>0xc4000000U-bytes)return false;
  static const Word routes[]={786,978,1170,785,977,1169,1361};
  bool route=false;for(unsigned i=0;i<7;i++)route|=j[24]==routes[i];if(!route)return false;
  for(unsigned i=32;i<64;i++)if(j[i])return false;
  return true;
 }
 if(!j[1] || j[1]>=0x7fffffffU || j[1]!=previous+1U || !j[2] || j[2]>11
  || j[3]!=b[3] || j[4]!=b[4] || j[5]>=64 || j[6]>=8192
  || !j[7] || j[7]>=0x80000000U)return false;
 for(unsigned i=8;i<16;i++)if(j[i]!=b[i])return false;
 // QGW2's activation input-token word was normalized to zero for QMW1.
 for(unsigned i=27;i<64;i++)if(!(j[2]==3 && i==27) && j[i])return false;
 // Keep the old UNSIGNED predicate,including its wraparound corner. Do not
 // silently strengthen the DATA grammar to densePhase==0 in this rewrite.
 if(densePhase && j[2]!=9U+densePhase)return false;
 if(j[2]>=9){
  if(b[3]>96 || j[5]%4!=3 || j[2]!=9U+densePhase)return false;
  for(unsigned i=16;i<27;i++)if(j[i])return false;
 }else{
  if(j[26]>1 || (j[26] && j[2]!=1))return false;
  if(j[2]==2)for(unsigned i=16;i<27;i++)if(j[i])return false;
  if((j[2]==6 || j[2]==7 || j[2]==8) && j[5]%4==3)return false;
 }
 if(j[2]==3){
  if(b[3]>Chunks || j[16]!=ActivationMagic || j[17]!=256 || j[18]!=576
   || j[19]!=j[7] || j[20]!=12 || j[27]>=Vocabulary)return false;
  for(unsigned i=21;i<64;i++)if(i!=27 && j[i])return false;
 }
 return true; // j[16..25] are checked by separate capsule/recipe consumers.
}
static inline bool OrdinaryMatrixJobShape(const Word* j,const Word* b,
 unsigned previous,unsigned densePhase){
 return j && j[2]==1 && JobShape(j,b,previous,densePhase);
}
}
#endif
