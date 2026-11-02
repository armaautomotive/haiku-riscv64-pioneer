#!/usr/bin/env python3
"""PRIVATE QFL1 object only; old whole subjects reproduce before inspection."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys

PRIVATE_SHA='3593260a72978b739120eb26a9ba866a622f7a6369b0ffbda4e4bce6c8b518e3'
assert len(sys.argv) in (2,3) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
inspect=len(sys.argv)==3
assert not inspect or sys.argv[2]=='--inspect-candidate'
assert inspect or PRIVATE_SHA is not None,'unfrozen QFL1 requires explicit inspect-candidate'
here=pathlib.Path(__file__).resolve().parent
out=pathlib.Path(sys.argv[1]);assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved=sys.argv[:]
try:
    sys.argv=[str(here/'build_qwen38_frontier_failure_detail_native.py'),str(out/'unchanged-qualified-QFD1')]
    old=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
sys.stdout.flush()
assert old['sha'](old['obj'])=='22a862c26595460dcd2c5e720bbe24079d2cb3a2b371e25d11337602dbbff5c3'
obj=out/'frontier-failure-link-NODRV.o'
argv=old['compile'](old['private'],obj,here/'tests/pioneer_q38_frontier_failure_link_native_context_compile.cpp')
(out/'compile-argv.txt').write_text('\n'.join(argv)+'\n')
before,before_sections=old['records'](old['obj']);after,after_sections=old['records'](obj)
for index,name in enumerate(old['arrays']+old['rope']):
    assert name in after and before[name][1]==after[name][1]
    assert old['payload'](old['obj'],name,before,before_sections,'qfl-old-data-'+str(index))==old['payload'](obj,name,after,after_sections,'qfl-new-data-'+str(index)),name
assert not set(old['abi'])&after.keys(),'PRIVATE cannot export conventional driver ABI'
for name in ('init_hardware','init_driver','uninit_driver','publish_devices','find_device'):
    for prefix in ('pioneer_q38_token_context_body_','pioneer_q38_token_context_only_'):
        symbol=prefix+name
        assert old['payload'](old['obj'],symbol,before,before_sections,'qfl-old-'+symbol)==old['payload'](obj,symbol,after,after_sections,'qfl-new-'+symbol)
table=subprocess.check_output([old['tool']+'nm','-C','--defined-only',str(obj)],text=True)
undefined=subprocess.check_output([old['tool']+'nm','-u',str(obj)],text=True)
assert 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<' in table
for word in ('Q38WorkControl','llama_','ggml_','ReferenceProducts','OriginalSource','ValidationSession',
             'NativeQ38PeerInstructionModel','Q38NativePeerTestCpu','PioneerQ38ObservedTrace','SyntheticIO'):
    assert word not in table and word not in undefined,word
previous,current=old['frames'](old['obj']),old['frames'](obj)
for name,size in current.items():assert size<=2048 or name in previous and size<=previous[name],(name,size)
observed=[(name,size) for name,size in current.items() if 'ObserveCommittedFrontier' in name];assert observed
digest=hashlib.sha256(obj.read_bytes()).hexdigest()
print('PASS entire old feaa/private+a0f0/retired-public+22a/QFD1 private reproduce FIRST;all14 firmware arrays+2MiB RoPE/'
      'all10 renamed ABI bodies exact;conventional ABI absent/no Work,CPU reference or HOST engine;'
      'static frames<=2048 or unchanged larger predecessor;NOT whole kernel/IRQ/indirect stack proof',flush=True)
print('QFL1_observer_frames '+repr(observed),flush=True)
print(digest+' '+obj.name,flush=True)
if inspect:
    print('UNFROZEN_QFL1_PRIVATE_BUILD_ONLY;NO public module,installation,native lifecycle,hardware cause or TPS qualification',flush=True)
else:
    assert digest==PRIVATE_SHA,'frozen independent PRIVATE QFL1 identity required'
    print('FROZEN_QFL1_PRIVATE_BUILD_ONLY;NO installable ABI,native lifecycle,hardware cause or TPS qualification',flush=True)
