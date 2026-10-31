#!/usr/bin/env python3
"""Inspect exact private retained-cohort context;NEVER link/install a driver."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys
OBJECT_SHA='473ba8728ae591223cd7a62c8d65e3ed57bd0a7f2de6d6d16be64981f2fbfe2a'
assert len(sys.argv) in (3,4) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==4;assert not inspect or sys.argv[3]=='--inspect-candidate'
assert inspect or OBJECT_SHA,'candidate not independently frozen'
previous,out=map(pathlib.Path,sys.argv[1:3]);here=pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700);saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_token_frontier_native_compile.py'),str(previous),str(out/'unchanged-frontier')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:sys.argv=saved
# The ENTIRE old05d149c9 and f4f8e84b private objects reproduce FIRST.
obj=out/'retained-cohort-token-context-NODRV.o';args=old['args'][:];tool=old['tool']
args[args.index('-c')+1]=str(here/'pioneer_q38_token_retained_cohort_native_context_compile.cpp')
args[args.index('-o')+1]=str(obj);subprocess.run(args,check=True)
(out/'compile-argv.txt').write_text('\n'.join(args)+'\n')
table,symbols,sections=old['records'](obj)
undefined=subprocess.check_output([tool+'nm','-u',str(obj)],text=True)
assert not {'api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device'}&symbols.keys()
header=(here.parent/'PioneerQ38TokenRetainedCohortPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest()=='3e5cec29942498a78227effe268f58a7fa567358d8f58dd7d04922a1ec364803'
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',header.decode());assert len(arrays)==13
for digest,label in arrays:
    names=[name for name in symbols if 'PioneerQ38TokenRetainedCohortPrograms' in name and name.endswith(str(len(label))+label+'E')]
    assert len(names)==1
    assert hashlib.sha256(old['payload'](obj,names[0],symbols,sections,'retained-'+label)).hexdigest()==digest,label
for namespace in ('PioneerQ38TokenFrontierPrograms','PioneerQ38TokenJobBootFoldPrograms',
                  'PioneerQ38TokenPreciseBetaPrograms','PioneerQ38TokenFaultPrograms','PioneerQ38TokenPushedPrograms'):
    assert not any(namespace in name for name in symbols),namespace
rope=[name for name,(_,size) in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size==2097152]
assert len(rope)==1 and rope[0] in old['symbols']
assert old['payload'](obj,rope[0],symbols,sections,'retained-rope')==old['payload'](
    old['obj'],rope[0],old['symbols'],old['sections'],'unchanged-frontier-rope')
demangled=subprocess.check_output([tool+'nm','-C','--defined-only',str(obj)],text=True)
for required in ('Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'Q38TokenFrontierControl(',
                 'pioneer_token_frontier_context_control','pioneer_token_frontier_context_scoped_observe',
                 'PioneerQ38TokenNativeOwner::InspectTerminal<'):
    assert required in demangled,required
for forbidden in ('ReferenceProducts','OriginalSource','ValidationSession','Q38WorkControl','NativeQ38PeerInstructionModel',
                  'Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO','llama_','ggml_'):
    assert forbidden not in table and forbidden not in undefined,forbidden
before,after=old['frames'](old['obj'].with_suffix('.su')),old['frames'](obj.with_suffix('.su'))
for name,size in after.items():assert size<=2048 or name in before and size<=before[name],(name,size)
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:assert digest==OBJECT_SHA,'unqualified private retained-cohort object'
print(digest+' retained-cohort-token-context-NODRV.o')
print('PASS_BUILD_ONLY private retained-cohort SDK/all13 actual arrays+2MiBRoPE exact/old05d+f4 entire objects unchanged/'
      'real same-owner counter adapter/no conventionalABI+CPU oracle+Work+MOCK imports/individual frames bounded;'
      'NOT lifecycle,combined-stack,original-model,physical,timing or Work qualification;DO_NOT_INSTALL')
