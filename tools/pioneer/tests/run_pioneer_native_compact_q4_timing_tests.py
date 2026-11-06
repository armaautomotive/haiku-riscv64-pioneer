#!/usr/bin/env python3
"""Frozen107 native/client/lifecycle qualification OFFBOARD;never stage/run GPU."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE=Path(__file__).resolve().parent;TOOLS=HERE.parent;ROOT=TOOLS.parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk',type=Path);parser.add_argument('private',type=Path)
    parser.add_argument('original',type=Path);parser.add_argument('out',type=Path)
    args=parser.parse_args()
    assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
    assert all(path.is_absolute() for path in (args.sdk,args.private,args.original,args.out))
    assert not args.out.exists() and not args.out.is_symlink()
    assert not any(key.startswith(('PIONEER_','TT_PIONEER_')) for key in os.environ)
    args.out.mkdir(mode=0o700)
    sources={Path(__file__).resolve()}
    for path in [*TOOLS.glob('*CompactQ4*'),*HERE.glob('*compact_q4*'),
                 TOOLS/'pioneer_native_compact_q4_timing_physical.sh',TOOLS/'build_native_compact_q4_stream_diagnostic.py',
                 TOOLS/'build_native_compact_q4_stream_client.py',TOOLS/'pioneer_native_compact_q4_stream_diagnostic.cpp',
                 ROOT/'src/add-ons/kernel/drivers/misc/pioneer_native_compact_q4_stream_driver.cpp']:
        if path.is_file() and path.suffix in ('.h','.inc','.cpp','.py','.sh'):sources.add(path)
    sources.add(ROOT/'src/add-ons/kernel/drivers/misc/PioneerNativeCompactQ4StreamNative.inc')
    queue=list(sources)
    while queue:
        source=queue.pop()
        for include in re.findall(r'^\s*#include\s+"([^"]+)"',source.read_text(),re.M):
            for base in (source.parent,HERE,TOOLS,ROOT/'src/add-ons/kernel/drivers/misc'):
                child=(base/include).resolve()
                if child.is_file() and ROOT in child.parents:
                    if child not in sources:sources.add(child);queue.append(child)
                    break
    pins={path:hashlib.sha256(path.read_bytes()).hexdigest() for path in sources}
    exits={};commands=[]
    def frozen():
        for path,digest in pins.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
    def run(name,argv):
        frozen();commands.append([name,*map(str,argv)])
        with (args.out/(name+'.stdout')).open('xb') as out,(args.out/(name+'.stderr')).open('xb') as err:
            child=subprocess.run(list(map(str,argv)),stdout=out,stderr=err,check=False,
                env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
        exits[name]=child.returncode
        (args.out/'actual-exits.json').write_text(json.dumps(exits,indent=2,sort_keys=True)+'\n')
        frozen()
        assert child.returncode==0 and (args.out/(name+'.stderr')).stat().st_size==0,name
        print('CLOSED_Q4_TIMING_NATIVE_OFFBOARD',name,0,flush=True)
    sdk=args.sdk;private=args.private;original=args.original
    run('native107',[sys.executable,TOOLS/'build_native_compact_q4_stream_diagnostic.py',sdk,private,original,args.out/'native107','--timing'])
    run('client107',[sys.executable,TOOLS/'build_native_compact_q4_stream_client.py',sdk,private,args.out/'client107','--timing'])
    run('default104',[sys.executable,TOOLS/'build_native_compact_q4_stream_diagnostic.py',sdk,private,original,args.out/'default104'])
    run('default-client104',[sys.executable,TOOLS/'build_native_compact_q4_stream_client.py',sdk,private,args.out/'default-client104'])
    assert hashlib.sha256((args.out/'default104/tenstorrent_pci_probe-native-q4-fpu-stream104').read_bytes()).hexdigest()=='4722e956bfda10a5f8dbc2317bd5dd12d2de1b1099c3d33286b4b6447930f61e'
    assert hashlib.sha256((args.out/'default-client104/pioneer-native-compact-q4-stream-diagnostic').read_bytes()).hexdigest()=='64c8c252d2c423de227e112b8da78045eb271ed0988923c3d15183114a132dc7'
    cxx=['c++','-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined']
    selected=['-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1']
    for label,unit in [('lease','pioneer_native_compact_q4_stream_lease_test.cpp'),
                       ('timer-journal','pioneer_native_compact_q4_timing_lease_test.cpp'),
                       ('real-lease','pioneer_native_compact_q4_stream_real_lease_test.cpp')]:
        binary=args.out/(label+'-test')
        run(label+'-compile',[*cxx,*selected,'-I'+str(args.out/'native107'),'-I'+str(private),HERE/unit,'-o',binary])
        run(label,[binary,args.out/'native107/image5120',args.out/'native107/image17408'])
    run('native-endpoints',[sys.executable,HERE/'run_pioneer_native_compact_q4_stream_native_tests.py',args.out/'native107',private,args.out/'endpoints','--timing'])
    run('roles',[sys.executable,HERE/'pioneer_native_compact_q4_timing_role_test.py'])
    run('client-fixtures',[sys.executable,HERE/'pioneer_native_compact_q4_stream_client_test.py',private,'--timing'])
    run('child-reader',[sys.executable,HERE/'pioneer_native_compact_q4_timing_output.py','--self-test'])
    run('parent-syntax',['sh','-n',TOOLS/'pioneer_native_compact_q4_timing_physical.sh'])
    run('parent',[sys.executable,HERE/'pioneer_native_compact_q4_timing_parent_test.py'])
    frozen()
    (args.out/'source-sha256.txt').write_text(''.join(f'{pins[path]}  {path.relative_to(ROOT)}\n' for path in sorted(pins)))
    (args.out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
    print(f'PASS_TIMED_Q4_NATIVE_OFFBOARD children{len(exits)} strictStderr0 source{len(pins)} default104AndClientExact1 '
          'actualTimedJournal576OriginalWords1 clockUnavailableSafeClose1;NO_STAGE_OR_HARDWARE_TIMING_OR_MODEL_TPS',flush=True)

if __name__=='__main__':main()
