#!/usr/bin/env python3
"""Actual selected code bytes/journal coverage,not card/lifecycle/performance."""
import hashlib,os,pathlib,re,subprocess,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
args=sys.argv[1:];profile_byte_lut='--profile-byte-lut' in args
if profile_byte_lut:
 assert args.count('--profile-byte-lut')==1;args.remove('--profile-byte-lut')
counter_unroll=profile_byte_lut or '--counter-unroll' in args
latest_worker='--latest-worker' in args
activation_lut_worker='--activation-lut-worker' in args
activation_byte_lut_worker=profile_byte_lut or '--activation-byte-lut-worker' in args
if activation_lut_worker:
 assert args.count('--activation-lut-worker')==1 and not latest_worker and not any(flag in args for flag in
  ('--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block','--fixed-config'))
 args.remove('--activation-lut-worker')
if activation_byte_lut_worker:
 if profile_byte_lut:
  assert not latest_worker and not activation_lut_worker and not any(flag in args for flag in
   ('--activation-byte-lut-worker','--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block','--fixed-config'))
 else:
  assert args.count('--activation-byte-lut-worker')==1 and not latest_worker and not activation_lut_worker and not any(flag in args for flag in
   ('--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block','--fixed-config'))
  args.remove('--activation-byte-lut-worker')
if latest_worker:
 assert args.count('--latest-worker')==1 and not any(flag in args for flag in
  ('--counter-unroll','--phase-profile','--two-product-replay','--instruction-prefetch','--queued-block','--fixed-config'))
 args.remove('--latest-worker')
if counter_unroll and not profile_byte_lut:
 assert args.count('--counter-unroll')==1;args.remove('--counter-unroll')
phase_profile=profile_byte_lut or '--phase-profile' in args
if phase_profile and not profile_byte_lut:
 assert counter_unroll and args.count('--phase-profile')==1;args.remove('--phase-profile')
two_product_replay='--two-product-replay' in args
if two_product_replay:
 assert phase_profile and args.count('--two-product-replay')==1;args.remove('--two-product-replay')
instruction_prefetch='--instruction-prefetch' in args
if instruction_prefetch:
 assert two_product_replay and args.count('--instruction-prefetch')==1;args.remove('--instruction-prefetch')
queued_block='--queued-block' in args
if queued_block:
 assert instruction_prefetch and args.count('--queued-block')==1;args.remove('--queued-block')
fixed_config='--fixed-config' in args
if fixed_config:
 assert queued_block and args.count('--fixed-config')==1;args.remove('--fixed-config')
assert len(args)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent;root,worker,out=map(pathlib.Path,args)
assert all(p.is_absolute() for p in (root,worker,out)) and not out.exists();out.mkdir(mode=0o700)
header=here.parent/('PioneerQ38FullOffloadFixedConfigQ4Programs.h' if fixed_config else 'PioneerQ38FullOffloadQueuedQ4Programs.h' if queued_block else 'PioneerQ38FullOffloadInstructionPrefetchQ4Programs.h' if instruction_prefetch else 'PioneerQ38FullOffloadTwoProductReplayQ4Programs.h' if two_product_replay else 'PioneerQ38FullOffloadPreparedProfileByteLutQ4Programs.h' if profile_byte_lut else 'PioneerQ38FullOffloadPreparedProfileQ4Programs.h' if phase_profile else 'PioneerQ38FullOffloadPreparedCounterQ4Programs.h' if counter_unroll else 'PioneerQ38FullOffloadPreparedByteLutQ4Programs.h' if activation_byte_lut_worker else 'PioneerQ38FullOffloadPreparedLutQ4Programs.h' if activation_lut_worker else 'PioneerQ38FullOffloadPreparedCurrentQ4Programs.h' if latest_worker else 'PioneerQ38FullOffloadPreparedQ4Programs.h');generated=out/'programs.h'
generator='generate_qwen38_prepared_profile_byte_lut_full_offload_programs.py' if profile_byte_lut else 'generate_qwen38_prepared_byte_lut_full_offload_programs.py' if activation_byte_lut_worker else 'generate_qwen38_prepared_q4_full_offload_programs.py'
generator_args=[str(generated)] if profile_byte_lut else [*([str(worker)] if activation_byte_lut_worker else [str(root),str(worker)]),str(generated),*(['--activation-lut-worker'] if activation_lut_worker else []),*(['--latest-worker'] if latest_worker else []),*(['--counter-unroll'] if counter_unroll else []),*(['--phase-profile'] if phase_profile else []),*(['--two-product-replay'] if two_product_replay else []),*(['--instruction-prefetch'] if instruction_prefetch else []),*(['--queued-block'] if queued_block else []),*(['--fixed-config'] if fixed_config else [])]
subprocess.run([sys.executable,str(here.parent/generator),*generator_args],check=True)
assert generated.read_bytes()==header.read_bytes() and hashlib.sha256(header.read_bytes()).hexdigest()==('b03e1e7b92c09d527bb81d9568f6e5a586fb6960aa9758b97ebf2fe1fa1af30d' if profile_byte_lut else 'd81ed84163311bc0dd5942dcf6d501048a05e83fbae436d79ea9fc17fd74ab7a' if activation_byte_lut_worker else 'a6ed6aa3278d229b4e24d3e0ee92a24b73f9542834c2bfbb075fac6c7f2a347b' if fixed_config else 'a43ddbef9de5155b94a4db36dbcc67b15181a3650d56cbdd3eb04b328f9e263b' if queued_block else '490ea103cd04cc05768dd9c002dbe8c138613e846c4bd781e2869c4fc23ed513' if instruction_prefetch else '3b5cf6b84791f97b0195b28d12df0f69879b96db44c06dc2a90f0354dd1fd4b5' if two_product_replay else 'e8eb2d83ccbadc01f64b142b7d97c300a273c737be1191afc86c0b65c06943e2' if phase_profile else 'c3bf57c0dfd9320c87a2f0bcba0b2ec1d6d6e0212688a364c8a46290061b4dad' if counter_unroll else '39fbf5204e4301d4fb6df2f89767989f893d57e256e89b56bb24a77e9314dfb7' if activation_lut_worker else '640a316ef7b5636be9e25724c1facef661deae4cc1440f736320c626ace5c39b' if latest_worker else '94368390b71f3ca725d6e791a04dd4b892e0655d7764515b9ce5a35ca9c6e84b')
role=here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h'
required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role.read_text()));assert len(required)==23
definitions={name:'1' for name in required}
definitions.update(PIONEER_DRIVER_BUILD_TAG='0x2026111500010105ULL' if profile_byte_lut else '0x2026111500010104ULL' if activation_byte_lut_worker else '0x2026111500010103ULL' if activation_lut_worker else '0x2026111500010102ULL' if latest_worker else '0x2026110500010103ULL' if fixed_config else '0x2026110500010102ULL' if queued_block else '0x2026110500010101ULL' if instruction_prefetch else '0x2026110500010100ULL' if two_product_replay else '0x2026110500010099ULL' if phase_profile else '0x2026110500010098ULL' if counter_unroll else '0x2026110500010097ULL',PIONEER_Q38_FULL_OFFLOAD_BATCHED='1',
 PIONEER_Q38_FULL_OFFLOAD_METRICS='1',PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4='1')
if counter_unroll:definitions['PIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL']='1'
if latest_worker:definitions['PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER']='1'
if activation_lut_worker:definitions['PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER']='1'
if activation_byte_lut_worker:definitions['PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER']='1'
if profile_byte_lut:definitions['PIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE']='1'
if phase_profile or profile_byte_lut:definitions['PIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE']='1'
if two_product_replay:definitions['PIONEER_Q38_FULL_OFFLOAD_TWO_PRODUCT_REPLAY']='1'
if instruction_prefetch:definitions['PIONEER_Q38_FULL_OFFLOAD_INSTRUCTION_PREFETCH']='1'
if queued_block:definitions['PIONEER_Q38_FULL_OFFLOAD_QUEUED_Q4']='1'
if fixed_config:definitions['PIONEER_Q38_FULL_OFFLOAD_FIXED_CONFIG_Q4']='1'
(out/'full_offload_prepared_q4_role_defines.inc').write_text(''.join('#define '+k+' '+v+'\n' for k,v in sorted(definitions.items())))
command=[os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
 '-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1',*(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER=1'] if latest_worker else []),
 *(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER=1'] if activation_lut_worker else []),
 *(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER=1'] if activation_byte_lut_worker else []),
 *(['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE=1'] if profile_byte_lut else []),
 '-I'+str(out),str(here/'pioneer_q38_full_offload_vector_q4_data_test.cpp'),'-o',str(out/'test')]
subprocess.run(command,check=True)
subprocess.run([str(out/'test'),str(root),str(worker)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
