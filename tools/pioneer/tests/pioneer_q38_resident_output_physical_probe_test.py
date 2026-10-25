#!/usr/bin/env python3
"""Actual combined-image physical probes AFTER the real matrix launch."""
import ast
import hashlib
import os
import pathlib
import re
import subprocess
import sys
import tempfile

assert len(sys.argv) == 2
here = pathlib.Path(__file__).resolve().parent
image = pathlib.Path(sys.argv[1])
# Authenticate the SAME complete ROOT/peer pins as the normal VM runner,
# without running it twice or executing imported orchestration code.
tree = ast.parse((here/'run_pioneer_q38_resident_output_cohort_image_tests.py').read_text())
root_pins = [ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
             and any(isinstance(t, ast.Name) and t.id == 'PINS' for t in n.targets)]
assert len(root_pins) == 1 and len(root_pins[0]) == 5
peer_pins = [ast.literal_eval(n.iter) for n in tree.body if isinstance(n, ast.For) and isinstance(n.iter, ast.Tuple)]
assert len(peer_pins) == 1 and len(peer_pins[0]) == 6
for section, size, digest in root_pins[0]:
    data = (image/(section+'.bin')).read_bytes()
    assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
for section, digest in peer_pins[0]:
    assert hashlib.sha256((image/'exact-companion'/(section+'.bin')).read_bytes()).hexdigest() == digest
source = (here/'pioneer_q38_resident_output_cohort_image_test.cpp').read_text()
entry = '\nint main(int argc,char** argv){'
assert source.count(entry) == 1
# Reuse the EXACT model/byte-endpoint classes, but not their CLI/main.
# Compile from stdin; no source file or live test dependency is rewritten.
prefix = source.split(entry)[0]
probe = r'''
int main(int argc,char** argv){
 assert(argc==2);setvbuf(stdout,nullptr,_IOLBF,0);(void)&RunPersistentGdnPair;
 OutputParts parts(0,0);
 for(unsigned kind=0;kind<4;kind++){
  OutputCohortFabric f(argv[1],(std::string(argv[1])+"/exact-companion").c_str(),parts);
  while(f.matrixLaunches<119 && !f.unknown)f.StepRoot();
  assert(!f.unknown && f.gdnNumericsVerified && f.preparedVerified && !f.resultWrites);
  if(!kind)f.badPcWorker=1;
  if(kind==1)f.badResetWorker=1;
  if(kind==2)f.peerSourceFault=1;
  if(kind==3)f.peerCounterFault=1;
  assert(!f.Run() && f.Mem(0x17ffc8)==TT_Q38_RESIDENT_OUTPUT_COHORT_FAULT);
  assert(f.resultWrites==(kind<2?0U:119U));f.Inert();
  printf("PASS ACTUAL combined ROOT physical probe%u injected AFTER119 real matrix launches/PC or reset rejects before scatter/source or counter rejects final success/UNKNOWN inert;NO device grant\n",kind);
 }
 return 0;
}
'''
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
with tempfile.TemporaryDirectory(prefix='pioneer-output-physical-probe-', dir='/private/tmp') as temporary:
    binary = pathlib.Path(temporary)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-ffp-contract=off',
                    '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', *flags,
                    '-I', str(here), '-x', 'c++', '-', '-o', str(binary)],
                   input=prefix+probe, text=True, check=True)
    subprocess.run([str(binary), str(image)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
