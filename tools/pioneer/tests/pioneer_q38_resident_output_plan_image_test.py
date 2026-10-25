#!/usr/bin/env python3
"""Actual unchanged ROOT/119 peers consume the canonical factory schedules."""
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
factory = r'''
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputPlan.h"
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 const unsigned layers[]={0,62,30},positions[]={0,8191,5},banks[]={0,6,3};
 for(unsigned variant=0;variant<3;variant++){
  OutputParts parts(layers[variant],positions[variant]);
  OutputCohortFabric f(argv[1],(std::string(argv[1])+"/exact-companion").c_str(),parts,banks[variant],variant&1);
  const auto sources=f.manifest.data()+TT_Q38_COHORT_SOURCE_WORD;
  const auto sequence=f.manifest.data()+TT_Q38_COHORT_SEQUENCE_WORD;
  PioneerQ38GdnIdentity id={1,2,3,4};auto recipe=std::make_unique<PioneerQ38ResidentOutputPlan>();
  assert(recipe->Build(*f.canonical,layers[variant],positions[variant],banks[variant],variant&1,248319,17,id,sources,sequence)
   && recipe->Matches(*f.canonical,layers[variant],positions[variant],banks[variant],variant&1,248319,17,id,sources,sequence));
  // Compare independently constructed old fixture schedules first. Then
  // supply the ACTUAL factory data to emitted instructions. No arithmetic
  // output, completion flag or success receipt is injected by this test.
  for(unsigned i=0;i<TT_Q38_COHORT_WORDS;i++){
   assert(f.manifest[i]==recipe->Manifest()[i]);f.Mem(TT_Q38_COHORT_MANIFEST+i*4)=recipe->Manifest()[i];
  }
  for(unsigned i=0;i<TT_Q38_OUTPUT_COHORT_WORDS;i++){
   assert(f.post[i]==recipe->PostManifest()[i]);f.Mem(TT_Q38_OUTPUT_COHORT_MANIFEST+i*4)=recipe->PostManifest()[i];
  }
  assert(f.Run());f.Verify();f.Inert();
  printf("PASS canonical factory feeds ACTUAL full coordinator variant%u/ALL640Conv+96GDN+24QOL1+ALL5120 matrix outputs/ALL119 same physical peer state/steps%u packets%u maxStack%u;synthetic learned byte endpoints,NOT native model/source/initialized state or throughput\n",variant,f.steps,f.packets,0x70000-f.minStack);
 }
}
'''
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-output-plan-image-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    '-I', str(here), '-x', 'c++', '-', '-o', str(binary)], input=prefix+factory, text=True, check=True)
    subprocess.run([str(binary), str(image)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
