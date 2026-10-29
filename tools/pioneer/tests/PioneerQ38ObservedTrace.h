/* SPDX-License-Identifier: MIT */
#if !defined(PIONEER_Q38_TEST_OBSERVED_TRACE) || PIONEER_Q38_TEST_OBSERVED_TRACE != 1
#error Exact HOST observed-output recorder role required
#endif
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_WORK_INFERENCE) || defined(PIONEER_ATTENTION_FREESTANDING) \
 || defined(PIONEER_Q38_GLOBAL_TOKEN_ROOT_IMAGE) || defined(PIONEER_Q38_GLOBAL_WORKER_IMAGE) || defined(PIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE) \
 || defined(PIONEER_Q38_RECURRENT_PREFIX_NATIVE_CONTEXT) || defined(PIONEER_Q38_MIXED_LIVE_NATIVE_CONTEXT) \
 || defined(PIONEER_Q38_FULL_MODEL_CPU_REFERENCE)
#error Observed-output TEST recorder is not kernel,image,native,CPU oracle or Work authority
#endif
#ifndef PIONEER_Q38_OBSERVED_TRACE_H
#define PIONEER_Q38_OBSERVED_TRACE_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
// DATA recorder ONLY: accepts const already-observed words,never expected
// CPU values,emulated memory/processor objects or an accelerator callback.
// It cannot create a native owner,prove STOP,or grant upload/cleanup. The
// instruction harness must call Save only AFTER its genuine full terminal
// verification. Partial/fault children must not publish complete traces.
class PioneerQ38ObservedTrace {
 struct Token {
  std::vector<uint32_t> hidden,logits;std::vector<uint8_t> seen;
  std::array<uint32_t,64> record={};
  unsigned input=0,layers=0,rows=0,selected=0,runnerUp=0;
  bool embedded=false,committed=false;
 };
 std::array<Token,3> token;unsigned embeddings=0,commits=0;bool failed=false,saved=false;
 const bool eosAfterPrompt;
 bool Refuse(){failed=true;return false;}
 static float Float(uint32_t bits){float value;std::memcpy(&value,&bits,4);return value;}
 static bool Finite(const uint32_t* words,unsigned count){if(!words)return false;for(unsigned i=0;i<count;i++)if(!std::isfinite(Float(words[i])))return false;return true;}
 static bool Word(FILE* file,uint32_t word){uint8_t bytes[4];for(unsigned i=0;i<4;i++)bytes[i]=uint8_t(word>>(i*8));return fwrite(bytes,1,4,file)==4;}
 static int Directory(const char* directory){
  if(!directory || directory[0]!='/')return -1;
  struct stat named={},opened={};if(lstat(directory,&named) || !S_ISDIR(named.st_mode))return -1;
  int fd=open(directory,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return -1;
  if(fstat(fd,&opened) || opened.st_dev!=named.st_dev || opened.st_ino!=named.st_ino){close(fd);return -1;}
  return fd;
 }
public:
 static constexpr uint32_t TokenMagic=0x31435451; // DATA format,not a capability
 static constexpr unsigned EosFirst=70253,EosSecond=17; // fixed original-corpus TEST stop configuration
 explicit PioneerQ38ObservedTrace(bool eos=false):eosAfterPrompt(eos){}
 bool Embedding(unsigned position,unsigned input,const uint32_t* actual,unsigned words){
  if(failed || saved || position>=(eosAfterPrompt?2U:3U) || position!=embeddings || words!=5120 || input>=248320 || !Finite(actual,words))return Refuse();
  if((position==0 && input!=248319) || (position==1 && input!=17) || (position==2 && token[1].committed && input!=token[1].selected))return Refuse();
  auto& t=token[position];if(t.embedded)return Refuse();
  t.hidden.resize(65*5120);t.logits.resize(248320);t.seen.resize(248320);
  std::memcpy(t.hidden.data(),actual,words*4);t.input=input;t.embedded=true;embeddings++;return true;
 }
 bool Layer(unsigned position,unsigned layer,const uint32_t* actual,unsigned words){
  if(failed || saved || position>=3 || layer>=64 || words!=5120 || !Finite(actual,words))return Refuse();
  auto& t=token[position];if(!t.embedded || t.committed || layer!=t.layers)return Refuse();
  std::memcpy(t.hidden.data()+(layer+1)*5120,actual,words*4);t.layers++;return true;
 }
 bool Vocabulary(unsigned position,unsigned first,const uint32_t* actual,unsigned rows){
  if(failed || saved || position>=3 || !rows || rows>256 || first>=248320 || rows>248320-first || !Finite(actual,rows))return Refuse();
  auto& t=token[position];if(!t.embedded || t.layers!=64 || t.committed)return Refuse();
  for(unsigned row=0;row<rows;row++)if(t.seen[first+row])return Refuse();
  for(unsigned row=0;row<rows;row++){t.logits[first+row]=actual[row];t.seen[first+row]=1;}t.rows+=rows;return true;
 }
 bool Commit(unsigned position,const uint32_t* actual,unsigned words){
  if(failed || saved || position>=3 || position!=commits || !actual || words!=64)return Refuse();
  auto& t=token[position];
  if(!t.embedded || t.committed || t.layers!=64 || t.rows!=248320 || actual[0]!=TokenMagic || actual[1]!=1
   || actual[2]!=position || actual[3]!=t.input || actual[4]>=248320 || actual[6]!=position+1 || actual[7]!=((position+1)&1)
   || actual[8]!=unsigned(position+1<2) || actual[9]!=unsigned(position+1<(eosAfterPrompt?2U:3U)) || actual[10]!=64 || actual[11]!=973
   || actual[44]!=position+1 || actual[45]!=48 || actual[46]!=119)return Refuse();
  for(unsigned i=47;i<64;i++)if(actual[i])return Refuse();
  unsigned best=0,second=1;if(Float(t.logits[second])>Float(t.logits[best]))std::swap(best,second);
  for(unsigned i=2;i<248320;i++)if(Float(t.logits[i])>Float(t.logits[best])){second=best;best=i;}else if(Float(t.logits[i])>Float(t.logits[second]))second=i;
  if(actual[4]!=best || actual[5]!=t.logits[best] || (position==1 && token[2].embedded && token[2].input!=best)
   || (position==2 && t.input!=token[1].selected) || (eosAfterPrompt && best!=(position?EosSecond:EosFirst)))return Refuse();
  std::memcpy(t.record.data(),actual,256);t.selected=best;t.runnerUp=second;t.committed=true;commits++;return true;
 }
 bool Complete()const{return !eosAfterPrompt && !failed && !saved && embeddings==3 && commits==3;}
 bool SavedComplete()const{return !eosAfterPrompt && !failed && saved && embeddings==3 && commits==3;}
 bool EosComplete()const{return eosAfterPrompt && !failed && !saved && embeddings==2 && commits==2 && !token[2].embedded;}
 bool SavedEosComplete()const{return eosAfterPrompt && !failed && saved && embeddings==2 && commits==2 && !token[2].embedded;}
 unsigned Embeddings()const{return embeddings;}
 unsigned Layers(unsigned position)const{return position<3?token[position].layers:0;}
 // Distinct, deliberately INCOMPLETE diagnostic. The caller must observe
 // the real first verified layer; this does NOT assert a token, STOP,
 // terminal capability, initialized native owner or reusable resources.
 // No full-token reader accepts this family/name/length as a complete trace.
 bool SaveFirstLayerDiagnostic(const char* directory){
  const auto& t=token[0];
  if(failed || saved || embeddings!=1 || commits || !t.embedded || t.layers!=1 || t.rows || t.committed)return Refuse();
  int directoryFd=Directory(directory);if(directoryFd<0)return Refuse();
  int fd=openat(directoryFd,"prefix-0.bin",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
  if(fd<0){close(directoryFd);return Refuse();}
  FILE* f=fdopen(fd,"wb");if(!f){close(fd);close(directoryFd);return Refuse();}
  const uint32_t header[]={0x31504f51,1,0,t.input,2,5120,1,0,0,0,0,0,0,0,0,0};
  bool okay=true;for(auto word:header)okay=Word(f,word)&&okay;
  for(unsigned i=0;i<2*5120;i++)okay=Word(f,t.hidden[i])&&okay;
  okay=Word(f,0x454e4450)&&okay;okay=!fflush(f)&&!ferror(f)&&okay;
  int closed=fclose(f),directoryClosed=close(directoryFd);
  if(!okay || closed || directoryClosed)return Refuse();
  saved=true;return true;
 }
 bool SaveEos(const char* directory,const uint32_t* terminal,unsigned words){
  if(!EosComplete() || !terminal || words!=64)return Refuse();
  const unsigned head[]={0x31545451,1,2,2,1,119,64,973,0,1};
  for(unsigned i=0;i<64;i++){
   if(i<10){if(terminal[i]!=head[i])return Refuse();}
   else if(i==10 || i==11 || i>=44){if(terminal[i])return Refuse();}
  }
  for(unsigned p=0;p<2;p++){
   for(unsigned i=0;i<8;i++)if(token[p].record[12+i]!=terminal[12+i])return Refuse();
   for(unsigned i=0;i<16;i++)if(token[p].record[20+i]!=terminal[20+i])return Refuse();
  }
  int directoryFd=Directory(directory);if(directoryFd<0)return Refuse();
  for(unsigned position=0;position<2;position++){
   const auto& t=token[position];std::string name="eos-token-"+std::to_string(position)+".bin";
   int fd=openat(directoryFd,name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0){close(directoryFd);return Refuse();}
   FILE* f=fdopen(fd,"wb");if(!f){close(fd);close(directoryFd);return Refuse();}
   const uint32_t header[]={0x31454f51,1,position,t.input,t.selected,t.runnerUp,t.logits[t.selected],t.logits[t.runnerUp],65,5120,248320,64,2,2,EosFirst,EosSecond};
   bool okay=true;for(auto word:header)okay=Word(f,word)&&okay;
   for(auto word:t.hidden)okay=Word(f,word)&&okay;for(auto word:t.logits)okay=Word(f,word)&&okay;
   for(auto word:t.record)okay=Word(f,word)&&okay;okay=Word(f,0x454e4445)&&okay;
   okay=!fflush(f)&&!ferror(f)&&okay;int closed=fclose(f);if(!okay || closed){close(directoryFd);return Refuse();}
  }
  int fd=openat(directoryFd,"eos-terminal.bin",O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0){close(directoryFd);return Refuse();}
  FILE* f=fdopen(fd,"wb");if(!f){close(fd);close(directoryFd);return Refuse();}
  const uint32_t header[]={0x31544551,1,2,3,2,EosFirst,EosSecond,119,64,973,0,0,0,0,0,0};
  bool okay=true;for(auto word:header)okay=Word(f,word)&&okay;
  for(unsigned i=0;i<64;i++)okay=Word(f,terminal[i])&&okay;
  okay=Word(f,0x454e4445)&&okay;okay=!fflush(f)&&!ferror(f)&&okay;
  int closed=fclose(f),directoryClosed=close(directoryFd);
  if(!okay || closed || directoryClosed)return Refuse();saved=true;return true;
 } // distinct DATA family;caller still owes actual whole-model+119 STOP verification
 bool Save(const char* directory){
  if(!Complete())return Refuse();
  int directoryFd=Directory(directory);if(directoryFd<0)return Refuse();
  for(unsigned position=0;position<3;position++){
   const auto& t=token[position];std::string name="token-"+std::to_string(position)+".bin";
   int fd=openat(directoryFd,name.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0){close(directoryFd);return Refuse();}
   FILE* f=fdopen(fd,"wb");if(!f){close(fd);close(directoryFd);return Refuse();}
   const uint32_t header[]={0x31444f51,1,position,t.input,t.selected,t.runnerUp,t.logits[t.selected],t.logits[t.runnerUp],65,5120,248320,64,0,0,0,0};
   bool okay=true;for(auto word:header)okay=Word(f,word)&&okay;
   for(auto word:t.hidden)okay=Word(f,word)&&okay;for(auto word:t.logits)okay=Word(f,word)&&okay;
   for(auto word:t.record)okay=Word(f,word)&&okay;okay=Word(f,0x454e444f)&&okay;
   okay=!fflush(f)&&!ferror(f)&&okay;int closed=fclose(f);if(!okay || closed){close(directoryFd);return Refuse();}
  }
  if(close(directoryFd))return Refuse();
  saved=true;return true;
 }
};
#endif
