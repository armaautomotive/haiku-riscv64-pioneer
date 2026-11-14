/* SPDX-License-Identifier: MIT */
// Independent integer decoder/arithmetic over PRIVATE generated BFP8 bands.
// No source codec/reference helpers, device access or private bytes printed.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>

static std::string Read(const std::string& path){
 std::ifstream f(path,std::ios::binary);if(!f)return {};
 return std::string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
}
static bool Array(const std::string& text,const std::string& name,unsigned count,std::vector<uint32_t>& out){
 const auto marker=name+"["+std::to_string(count)+"]={";auto at=text.find(marker);
 if(at==std::string::npos || text.find(marker,at+1)!=std::string::npos)return false;
 at+=marker.size();std::vector<uint32_t> values;values.reserve(count);
 for(unsigned i=0;i<count;i++){
  at=text.find("0x",at);if(at==std::string::npos)return false;
  char* end=nullptr;unsigned long n=std::strtoul(text.c_str()+at,&end,16);
  if(end!=text.c_str()+at+10 || n>UINT32_MAX)return false;
  values.push_back(uint32_t(n));at=size_t(end-text.c_str());
 }
 const auto close=text.find('}',at);if(close==std::string::npos || text.find("0x",at)<close)return false;
 out.swap(values);return true;
}
static uint8_t Byte(const std::vector<uint32_t>& words,unsigned at){return uint8_t(words.at(at/4)>>(8*(at&3)));}
static uint16_t Half(const std::vector<uint32_t>& words,unsigned at){return uint16_t(Byte(words,at)|(unsigned(Byte(words,at+1))<<8));}
static uint16_t Weight(const std::vector<uint32_t>& words,unsigned tile,unsigned row,unsigned col){
 const unsigned start=tile*1104+16;
 const unsigned group=(row/16)*32+(col/16)*16+row%16;
 const unsigned index=64+((row/16)*2+col/16)*256+(row%16)*16+col%16;
 const unsigned code=Byte(words,start+index),mag=code&127;
 if(!mag){assert(!code);return 0;}
 unsigned top=0;for(unsigned x=mag;x>>=1;)top++;
 const int exp=int(Byte(words,start+group))-6+int(top);
 assert(exp>0 && exp<255);
 return uint16_t(((code&128)<<8)|(unsigned(exp)<<7)|((mag<<(7-top))-128));
}
static int64_t FloorDivPow2(int64_t value,unsigned shift){
 if(!shift)return value;
 const uint64_t bias=(UINT64_C(1)<<shift)-1;
 return value>=0?int64_t(uint64_t(value)>>shift):-int64_t((uint64_t(-value)+bias)>>shift);
}
static uint64_t RoundMagnitude(uint64_t value,unsigned shift){
 return shift?(value+(UINT64_C(1)<<(shift-1)))>>shift:value;
}
struct Lane {unsigned exponent;int64_t sum;};
static Lane MakeLane(const uint16_t* a,const uint16_t* b,unsigned phase){
 unsigned exponents[8],products[8];bool negative[8];unsigned largest=0;
 for(unsigned i=0;i<8;i++){
  const unsigned ae=(a[i]>>7)&255,be=(b[i]>>7)&255,am=(a[i]&127)<<3,bm=(b[i]&127)<<3;
  assert(ae!=255 && be!=255 && (ae || !am) && (be || !bm));
  const unsigned ma=(phase&1)?((am>>1)&31):(16|(am>>6));
  const unsigned mb=(phase&2)?((bm&15)<<3):(64|(bm>>4));
  exponents[i]=ae&&be?ae+be:0;products[i]=ae&&be?ma*mb:0;negative[i]=bool((a[i]^b[i])&0x8000);
  if(exponents[i]>largest)largest=exponents[i];
 }
 int64_t sum=0;for(unsigned i=0;i<8;i++){
  const unsigned distance=largest-exponents[i];const uint64_t aligned=distance<16?RoundMagnitude(products[i],distance):0;
  sum+=negative[i]?-int64_t(aligned):int64_t(aligned);
 }
 const unsigned rebased=(largest-127U-((phase&1)?5U:0U)-((phase&2)?7U:0U))&1023U;
 return rebased>0 && rebased<512?Lane{rebased,sum}:Lane{0,0};
}
static uint32_t Instruction(const uint16_t* a,const uint16_t* b,unsigned phase,uint32_t previous){
 const Lane x=MakeLane(a,b,phase),y=MakeLane(a+8,b+8,phase);
 const unsigned de=(previous>>23)&255,dm=de?(0x800000|(previous&0x7fffff)):0;
 unsigned common=de;if(x.exponent>common)common=x.exponent;if(y.exponent>common)common=y.exponent;
 auto aligned=[common](Lane lane){unsigned shift=common-lane.exponent;if(shift>31)shift=31;
  int64_t value=lane.sum*8192;if(shift)value+=int64_t(UINT64_C(1)<<(shift-1));return FloorDivPow2(value,shift);};
 unsigned shift=common-de;if(shift>31)shift=31;const int64_t d=int64_t(RoundMagnitude(dm,shift));
 int64_t fixed=aligned(x)+aligned(y)+(previous>>31?-d:d);if(!fixed)return 0;
 const unsigned sign=fixed<0;uint64_t magnitude=uint64_t(sign?-fixed:fixed);unsigned top=0;
 for(uint64_t n=magnitude;n>>=1;)top++;
 int exp=int(top)+int(common)-23;uint64_t mantissa=top>23?RoundMagnitude(magnitude,top-23):magnitude<<(23-top);
 if(mantissa==(UINT64_C(1)<<24)){mantissa>>=1;exp++;}
 if(exp<=0)return 0;if(exp>=255)return (sign<<31)|0x7f800000U;
 return (sign<<31)|(unsigned(exp)<<23)|(uint32_t(mantissa)&0x7fffffU);
}
int main(int argc,char** argv){
 const bool smallRef=argc==4 && (std::string(argv[2])=="--small-alpha-reference" || std::string(argv[2])=="--small-beta-reference");
 const bool qkvRef=argc==4 && std::string(argv[2])=="--recurrent-qkv-reference-file";
 const bool privateRef=smallRef || qkvRef;
 if((argc!=2 && argc!=3 && !privateRef) || argv[1][0]!='/' || (privateRef && argv[3][0]!='/') || (argc==3 && std::string(argv[2])!="--qgwd-reference"
  && std::string(argv[2])!="--recurrent-reference" && std::string(argv[2])!="--recurrent-out-reference"
  && std::string(argv[2])!="--recurrent-qkv-reference" && std::string(argv[2])!="--small-alpha"
  && std::string(argv[2])!="--small-beta"))return 2;
 const std::string path=argv[1],data=Read(path+"/PioneerNativeBfpModelBandData.h"),meta=Read(path+"/PioneerNativeBfpModelBandMetadata.h");
 if(data.empty() || meta.empty())return 1;
 const bool smallAlpha=(argc==3 && std::string(argv[2])=="--small-alpha") || (smallRef && std::string(argv[2])=="--small-alpha-reference");
 const bool smallBeta=(argc==3 && std::string(argv[2])=="--small-beta") || (smallRef && std::string(argv[2])=="--small-beta-reference");
 const bool small=smallAlpha || smallBeta,reference=argc==3 && !small,recurrent=reference && std::string(argv[2])=="--recurrent-reference";
 const bool recurrentOut=reference && std::string(argv[2])=="--recurrent-out-reference";
 const bool recurrentQkv=(reference && std::string(argv[2])=="--recurrent-qkv-reference") || qkvRef;
 if(reference && !recurrent && !recurrentOut && !recurrentQkv && (meta.find("Width=5120,Rows=1024,Tensor=45,Samples=3")==std::string::npos
  || meta.find("const unsigned rows[3]={0,512,992}")==std::string::npos))return 1;
 if(recurrent && (meta.find("Width=5120,Rows=6144,Tensor=3,Samples=3")==std::string::npos
  || meta.find("const unsigned rows[3]={0,3072,6112}")==std::string::npos))return 1;
 if(recurrentOut && (meta.find("Width=6144,Rows=5120,Tensor=16,Samples=3")==std::string::npos
  || meta.find("const unsigned rows[3]={0,2560,5088}")==std::string::npos))return 1;
 if(recurrentQkv && (meta.find("Width=5120,Rows=10240,Tensor=5,Samples=3")==std::string::npos
  || meta.find("const unsigned rows[3]={0,5120,10208}")==std::string::npos))return 1;
 if(small && (meta.find(smallAlpha?"Width=5120,Rows=48,Tensor=11,Samples=3":"Width=5120,Rows=48,Tensor=12,Samples=3")==std::string::npos
  || meta.find("const unsigned rows[3]={0,32,0}")==std::string::npos))return 1;
 const unsigned tiles=recurrentOut?192:160;
 std::vector<uint32_t> inputs;if(!Array(data,"input",tiles*20,inputs))return 1;
 unsigned checked=0;std::string privateReference;
 for(unsigned sample=0;sample<3;sample++){
  std::vector<uint32_t> weights,golden;
  if(!Array(data,"w"+std::to_string(sample),tiles*276,weights) || !Array(meta,"g"+std::to_string(sample),32,golden))return 1;
  for(unsigned tile=0;tile<tiles;tile++){
   for(unsigned i=0;i<16;i++)if(Byte(weights,tile*1104+i) || Byte(inputs,tile*80+i))return 1;
  }
  for(unsigned row=0;row<32;row++){
   uint32_t value=0;
   for(unsigned tile=0;tile<tiles;tile++)for(unsigned half=0;half<2;half++){
    uint16_t a[16],b[16];for(unsigned c=0;c<16;c++){
     a[c]=Weight(weights,tile,row,half*16+c);b[c]=Half(inputs,tile*80+16+(half*16+c)*2);
    }
    for(unsigned phase=0;phase<4;phase++)value=Instruction(a,b,phase,value);
   }
   if(value!=golden[row]){std::fprintf(stderr,"mismatch sample=%u row=%u got=%08x expected=%08x\n",sample,row,value,golden[row]);return 1;}
   if(small && sample==1 && row>=16 && value)return 1;
   if(smallRef){char line[96];const int n=std::snprintf(line,sizeof(line),"SMALL_INTEGER_REFERENCE sample%u first%u row%u word%08x\n",sample,sample==1?32:0,row,value);
    if(n<0 || unsigned(n)>=sizeof(line))return 1;privateReference.append(line,unsigned(n));}
   if(qkvRef){const unsigned first[]={0,5120,10208};char line[96];const int n=std::snprintf(line,sizeof(line),
    "QKV_INTEGER_REFERENCE sample%u first%u row%u word%08x\n",sample,first[sample],row,value);
    if(n<0 || unsigned(n)>=sizeof(line))return 1;privateReference.append(line,unsigned(n));}
   if(reference){const unsigned dense[]={0,512,992},rec[]={0,3072,6112},out[]={0,2560,5088},qkv[]={0,5120,10208};
    std::printf("%s_INTEGER_REFERENCE sample%u first%u row%u word%08x\n",recurrentQkv?"RECURRENT_QKV":recurrentOut?"RECURRENT_OUT":recurrent?"RECURRENT":"DENSE",sample,(recurrentQkv?qkv:recurrentOut?out:recurrent?rec:dense)[sample],row,value);}
   checked++;
  }
  std::printf("INDEPENDENT_DENSE_BAND sample=%u rows=32 match; NO DEVICE\n",sample);std::fflush(stdout);
 }
 if(privateRef){
  const int fd=open(argv[3],O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);if(fd<0)return 1;
  size_t sent=0;while(sent<privateReference.size()){
   const ssize_t n=write(fd,privateReference.data()+sent,privateReference.size()-sent);if(n<=0){close(fd);return 1;}sent+=size_t(n);
  }
  if(fsync(fd)){close(fd);return 1;}if(close(fd))return 1;
 }
 std::printf("PASS_INDEPENDENT_DENSE_BAND samples=3 rows=%u phase_instructions=%u; NO DEVICE OR SPEED\n",checked,checked*tiles*2*4);
 return std::fflush(stdout)?1:0;
}
