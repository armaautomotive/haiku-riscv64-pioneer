/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BF16_ROOT_PAIRS_H
#define PIONEER_Q38_BF16_ROOT_PAIRS_H
// Pure integer DATA transform of the authenticated ORIGINAL119-part
// recipe. Reconstruct its seven contiguous bank stripes,then pair rows
// INSIDE each stripe. No weight repack,source/native/initialized grant.
struct PioneerQ38Bf16RootPairs {
 enum : unsigned {Parts=0x168000,Rows=0x169000,Shadow=0x16b000,RowShadow=0x16c000,End=0x16c200};
 static unsigned Bytes(unsigned k,unsigned groups){return k==5120?groups*46080:groups*156672;}
 static unsigned Split(unsigned a,unsigned b,unsigned& remainder){
  unsigned q=0;if(!b){remainder=~0U;return ~0U;}while(a>=b){a-=b;q++;}remainder=a;return q;
 }
 template<class IO> static bool Build(IO& io,unsigned k,unsigned rows){
  if((k!=5120 && k!=17408) || rows!=(k==5120?17408U:5120U))return false;
  unsigned first[7],address[7],groups[7],quota[7],done[7],emitted[7];
  for(unsigned b=0;b<7;b++){first[b]=address[b]=groups[b]=quota[b]=done[b]=emitted[b]=0;}
  for(unsigned w=0;w<119;w++){
   const unsigned bank=io.Part(w,1),n=io.Part(w,2),row=io.Row(w),at=io.Part(w,0);
   if(bank>=7 || !n || n>16 || (row&15) || row>=rows || n*16>rows-row || (at&63)
    || at<0x04000000 || at>0xc4000000-Bytes(k,n) || io.Part(w,4)!=io.Route(bank)
    || io.Part(w,3) || io.Part(w,6) || io.Part(w,7))return false;
   if(!groups[bank]){first[bank]=row;address[bank]=at;}
   else if(row!=first[bank]+groups[bank]*16 || at!=address[bank]+Bytes(k,groups[bank]))return false;
   groups[bank]+=n;
  }
  unsigned total=0,minimum=0;
  for(unsigned b=0;b<7;b++){
   if(!groups[b] || (groups[b]&1) || (first[b]&31))return false;
   for(unsigned a=0;a<b;a++)if(first[b]<first[a]+groups[a]*16 && first[a]<first[b]+groups[b]*16)return false;
   total+=groups[b];
  }
  for(unsigned b=0;b<7;b++){groups[b]/=2;quota[b]=(groups[b]+7)/8;minimum+=quota[b];}
  if(total*16!=rows || minimum>119)return false;
  for(unsigned extra=minimum;extra<119;extra++){
   unsigned best=7;for(unsigned b=0;b<7;b++)if(quota[b]<groups[b]
    && (best==7 || groups[b]*quota[best]>groups[best]*quota[b]
     || (groups[b]*quota[best]==groups[best]*quota[b] && first[b]<first[best])))best=b;
   if(best==7)return false;
   quota[best]++;
  }
  unsigned count=0;
  while(count<119){
   bool progress=false;
   for(unsigned bank=0;bank<7;bank++){
    if(emitted[bank]==quota[bank])continue;
    unsigned remainder=0,n=Split(groups[bank],quota[bank],remainder);
    n+=unsigned(emitted[bank]<remainder);
    if(count>=119 || !n || n>8 || n>groups[bank]-done[bank] || !io.Source(count))return false;
    const unsigned values[]={address[bank]+Bytes(k,done[bank]*2),bank,n*2,0,io.Route(bank),io.Source(count),0,0};
    for(unsigned i=0;i<8;i++)io.Output(count,i,values[i]);
    io.OutputRow(count,first[bank]+done[bank]*32);done[bank]+=n;emitted[bank]++;count++;progress=true;
   }
   if(!progress)return false;
  }
  for(unsigned b=0;b<7;b++)if(done[b]!=groups[b] || emitted[b]!=quota[b])return false;
  return true;
 }
};
#endif
