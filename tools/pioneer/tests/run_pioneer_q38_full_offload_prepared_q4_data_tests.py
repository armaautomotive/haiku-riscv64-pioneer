#!/usr/bin/env python3
"""Actual selected code bytes/journal coverage,not card/lifecycle/performance."""
import hashlib,os,pathlib,re,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
args=sys.argv[1:];counter_unroll='--counter-unroll' in args
if counter_unroll:
 assert args.count('--counter-unroll')==1;args.remove('--counter-unroll')
phase_profile='--phase-profile' in args
if phase_profile:
 assert counter_unroll and args.count('--phase-profile')==1;args.remove('--phase-profile')
two_product_replay='--two-product-replay' in args
if two_product_replay:
 assert phase_profile and args.count('--two-product-replay')==1;args.remove('--two-product-replay')
assert len(args)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root,worker,out=map(pathlib.Path,args)
assert all(p.is_absolute() for p in (root,worker,out)) and not out.exists();out.mkdir(mode=0o700)
header=here.parent/('PioneerQ38FullOffloadTwoProductReplayQ4Programs.h' if two_product_replay else 'PioneerQ38FullOffloadPreparedProfileQ4Programs.h' if phase_profile else 'PioneerQ38FullOffloadPreparedCounterQ4Programs.h' if counter_unroll else 'PioneerQ38FullOffloadPreparedQ4Programs.h');generated=out/'programs.h'
subprocess.run([sys.executable,str(here.parent/'generate_qwen38_prepared_q4_full_offload_programs.py'),str(root),str(worker),str(generated),*(['--counter-unroll'] if counter_unroll else []),*(['--phase-profile'] if phase_profile else []),*(['--two-product-replay'] if two_product_replay else [])],check=True)
assert generated.read_bytes()==header.read_bytes() and hashlib.sha256(header.read_bytes()).hexdigest()==('3b5cf6b84791f97b0195b28d12df0f69879b96db44c06dc2a90f0354dd1fd4b5' if two_product_replay else 'e8eb2d83ccbadc01f64b142b7d97c300a273c737be1191afc86c0b65c06943e2' if phase_profile else 'c3bf57c0dfd9320c87a2f0bcba0b2ec1d6d6e0212688a364c8a46290061b4dad' if counter_unroll else '94368390b71f3ca725d6e791a04dd4b892e0655d7764515b9ce5a35ca9c6e84b')
role=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()));assert len(required)==23
definitions={name:'1' for name in required}
definitions.update(PIONEER_DRIVER_BUILD_TAG='0x2026110500010100ULL' if two_product_replay else '0x2026110500010099ULL' if phase_profile else '0x2026110500010098ULL' if counter_unroll else '0x2026110500010097ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
 PIONEER_Q38_FULL_OFFLOAD_METRICS='1',PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4='1')
if counter_unroll:definitions['PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL']='1'
if phase_profile:definitions['PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE']='1'
if two_product_replay:definitions['PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY']='1'
(out/'full_offload_prepared_q4_role_defines.inc').write_text(''.join('#define '+k+' '+v+'\n' for k,v in sorted(definitions.items())))
command=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
 '-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1','-I'+str(out),str(here/'pioneer_q38_full_offload_vector_q4_data_test.cpp'),'-o',str(out/'test')]
subprocess.run(command,check=True)
subprocess.run([str(out/'test'),str(root),str(worker)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
