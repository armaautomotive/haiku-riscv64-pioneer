#!/usr/bin/env python3
"""Actual immutable ROOT instruction/packet attribution; NOT silicon timing."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
pins = runpy.run_path(str(here.parent/'generate_qwen38_resident_output_images.py'))['PINS']
for name, (section, size, digest, _) in pins.items():
    data = ((image if name.startswith('root-') else image/'exact-companion')/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
source = (here/'pioneer_q38_resident_output_cohort_image_test.cpp').read_text()
entry = '\nint main(int argc,char** argv){'
assert source.count(entry) == 1
prefix = source.split(entry)[0]
profile = r'''
struct ProfileOutput:OutputCohortFabric {
 uint64_t packetKind[9]={},packetBytes[9]={};
 using OutputCohortFabric::OutputCohortFabric;
 void Submit()override{
  bool write=command[7]==0x212;const unsigned remote=write?command[3]:command[0],route=write?command[5]:command[2];
  unsigned kind=route==manifest[9]?unsigned(write):write?3:2;
  if(!write && route!=manifest[9] && remote>=0xff000000){
   kind=remote==0xffb13140?4:remote==0xffb121b0?5:8;
   for(unsigned i=0;i<16;i++)if(remote==0xffb20000+SrcOffset(i))kind=6;
   for(unsigned i=0;i<8;i++)if(remote==0xffb20000+Offset(i))kind=7;
  }
  packetKind[kind]++;packetBytes[kind]+=command[8];OutputCohortFabric::Submit();
 }
};
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 struct Symbol {uint32_t address;std::string name;uint64_t steps=0;};
 std::vector<Symbol> symbols;std::ifstream input(std::string(argv[1])+"/program.symbols");assert(input);std::string line;
 while(std::getline(input,line)){
  unsigned address;char type,name[256];
  if(sscanf(line.c_str(),"%x %c %255s",&address,&type,name)==3 && (type=='T' || type=='t' || type=='W'))symbols.push_back({address,name,0});
 }
 assert(!symbols.empty());std::sort(symbols.begin(),symbols.end(),[](const auto& a,const auto& b){return a.address!=b.address?a.address<b.address:a.name<b.name;});
 OutputParts parts(0,0);ProfileOutput f(argv[1],(std::string(argv[1])+"/exact-companion").c_str(),parts);uint64_t total=0;
 while(f.pc!=f.parkPc && !f.unknown){
  auto at=std::upper_bound(symbols.begin(),symbols.end(),f.pc,[](uint32_t pc,const auto& symbol){return pc<symbol.address;});
  assert(at!=symbols.begin());--at;const unsigned before=f.steps;f.StepRoot();assert(f.steps==before+1);at->steps++;total++;
 }
 // The ordinary CLI reports steps AFTER Inert()'s128 deliberate parked
 // self-loop checks. Attribute execution up to park separately, then
 // require the SAME final CLI count after the unchanged inertness check.
 assert(f.Run());f.Verify();assert(total==f.steps && total+128==38933374 && f.packets==23515);
 std::sort(symbols.begin(),symbols.end(),[](const auto& a,const auto& b){return a.steps!=b.steps?a.steps>b.steps:a.address<b.address;});
 uint64_t attributed=0;for(const auto& symbol:symbols){
  attributed+=symbol.steps;if(symbol.steps)printf("ROOT_INSTRUCTIONS %llu entry%08x %s\n",(unsigned long long)symbol.steps,symbol.address,symbol.name.c_str());
 }
 assert(attributed==total);const char* kind[]={"GDDR_READ","GDDR_WRITE","PEER_PAYLOAD_READ","PEER_PAYLOAD_WRITE","PEER_PC_READ","PEER_RESET_READ","PEER_SOURCE_READ","PEER_COUNTER_READ","OTHER_PEER_CSR_READ"};
 uint64_t packets=0;for(unsigned i=0;i<9;i++){
  packets+=f.packetKind[i];printf("ROOT_PACKETS %s count%llu bytes%llu\n",kind[i],(unsigned long long)f.packetKind[i],(unsigned long long)f.packetBytes[i]);
 }
 assert(packets==f.packets);f.Inert();assert(f.steps==38933374 && f.steps==total+128);
 printf("PASS immutable combined ROOT profile:38933246 actual modeled instructions to park+128 test-only parked checks/23515 actual ROOT packets/ALL stages independently verified/no numeric injection or program changes;SYNTHETIC endpoints+clock,NOT silicon cycles/token rate/full-model performance\n");
}
'''
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-output-profile-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    '-I', str(here), '-x', 'c++', '-', '-o', str(binary)], input=prefix+profile, text=True, check=True)
    subprocess.run([str(binary), str(image)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
