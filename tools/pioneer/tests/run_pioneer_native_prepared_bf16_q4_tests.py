#!/usr/bin/env python3
"""Source-frozen main/clean native115 qualification;NO staging/device access."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
assert __debug__ and not os.environ.get('PYTHONOPTIMIZE') and len(sys.argv)==7
sdk,private,original,discovery,out=map(Path,sys.argv[1:6]);generation=int(sys.argv[6])
assert all(p.is_absolute() for p in (sdk,private,original,discovery,out)) and 2<=generation<2**63
assert sdk.is_dir() and private.is_dir() and original.is_file() and discovery.is_dir() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent;root=here.parents[2]
assert not out.is_relative_to(root)
out.mkdir(mode=0o700);host=os.environ.get('CXX','c++')
public={Path(__file__).resolve()};private_paths=set()
def collect(command):
 data=subprocess.check_output(list(map(str,command)),text=True,cwd=out)
 for rule in data.replace('\\\n','').splitlines():
  if not rule.strip():continue
  assert ':' in rule
  for word in rule.split(':',1)[1].split():
   p=Path(os.path.abspath(word))
   if p.is_relative_to(root):public.add(p)
   elif p.is_relative_to(private):private_paths.add(p)
for name in ('pioneer_native_prepared_bf16_q4_lease_test.cpp','pioneer_native_prepared_bf16_q4_private_test.cpp',
             'pioneer_native_compact_q4_stream_native_test.cpp'):
 include=discovery/'endpoints-discovery' if name.endswith('_native_test.cpp') else discovery/'native-discovery'
 collect([host,'-std=c++17','-DCOHORT_SCOPE_COUNTER_TEST','-DPIONEER_NATIVE_PREPARED_BF16_Q4_COMPARE=1','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1',
          '-I'+str(include),'-I'+str(discovery/'native-discovery'),'-I'+str(private),'-MM',here/name])
collect([host,'-std=c++17','-I'+str(private),'-MM',here.parent/'pioneer_prepared_bf16_q4_export.cpp'])
collect([host,'-std=c++17','-MM',here.parent/'PioneerModelWeightCache.cpp'])
for line in (discovery/'native-discovery/build-argv.txt').read_text().splitlines():
 args=line.split('\t')
 if '-c' not in args:continue
 args=[a for a in args if a!='-fstack-usage'];j=args.index('-o');del args[j:j+2];args[args.index('-c')]='-M';collect(args)
for name in ('build_native_prepared_bf16_q4_diagnostic.py','build_native_prepared_bf16_q4_client.py',
             'build_q38_prepared_bf16_q4_compare.py','build_q38_factored_q4_compare.py','pioneer_native_prepared_bf16_q4_physical.sh',
             'blackhole_q38_prepared_bf16_q4_compare.cpp','blackhole_q38_prepared_bf16_q4_compare_start.S','blackhole_q38_prepared_bf16_q4_compare.ld',
             'blackhole_q38_factored_q4_compare.cpp','blackhole_q38_factored_q4_compare_start.S','blackhole_q38_factored_q4_compare.ld','blackhole_q38_q4_kernel_clock.cpp'):
 public.add(here.parent/name)
for name in ('run_pioneer_native_prepared_bf16_q4_native_tests.py','pioneer_native_prepared_bf16_q4_parent_test.py',
             'pioneer_native_prepared_bf16_q4_output.py','pioneer_native_prepared_bf16_q4_launch_admission_test.py',
             'pioneer_dram_sweep_native_lifecycle_test.cpp','pioneer_native_factored_q4_compare_lease_test.cpp',
             'pioneer_q38_q4_kernel_timing_image_test.cpp'):
 public.add(here/name)
# Collect firmware producer and the client with their ACTUAL compile flags.
cross=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++')
collect([cross,'-std=c++17','-ffreestanding','-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_PREPARED_BF16_COMPARE_BUILD=1',
         '-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1','-M',here.parent/'blackhole_q38_prepared_bf16_q4_compare.cpp'])
collect([cross,'-std=c++17','-ffreestanding','-march=rv32i','-mabi=ilp32','-DPIONEER_Q38_FACTORED_Q4_COMPARE_BUILD=1',
         '-DPIONEER_Q38_FACTORED_CONFIGURED_COMPUTE=1','-DPIONEER_Q38_Q4_FACTORED_BFP8_BUILD=1','-M',here.parent/'blackhole_q38_factored_q4_compare.cpp'])
collect([host,'-std=c++17','-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED=1','-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=1',
         '-MM',here/'pioneer_native_factored_q4_compare_lease_test.cpp'])
client=(discovery/'client-discovery/build-argv.txt').read_text().strip().split('\t')
j=client.index('-o');del client[j:j+2]
client=[a for a in client if not a.startswith(('-L','-B','-l')) and not a.endswith('.a')]
client+=['-M'];collect(client)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in public}
private_frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in private_paths}
clean=out/'clean-source';clean_pins={}
preexisting={'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadMetricsRole.h',
 'src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadProtocol.h','src/add-ons/kernel/drivers/misc/PioneerQ38FullOffloadRole.h',
 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc','src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeEnd.inc',
 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp','tools/pioneer/NATIVE_BUILD_STATUS.md','tools/pioneer/build_qwen38_full_offload_native.py'}
for p in public:
 relative=str(p.relative_to(root));target=clean/relative;target.parent.mkdir(parents=True,exist_ok=True)
 if relative in preexisting:
  target.write_bytes(subprocess.check_output(['git','show','HEAD:'+relative],cwd=root))
 else:shutil.copyfile(p,target)
 clean_pins[relative]=hashlib.sha256(target.read_bytes()).hexdigest()
exits={};commands=[]
def same():
 assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
 assert all(hashlib.sha256((clean/p).read_bytes()).hexdigest()==digest for p,digest in clean_pins.items())
 assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in private_frozen.items())
def run(name,args):
 same();commands.append([name,*map(str,args)])
 with (out/(name+'.stdout')).open('xb') as stdout,(out/(name+'.stderr')).open('xb') as stderr:
  r=subprocess.run(list(map(str,args)),stdout=stdout,stderr=stderr,cwd=out,
                   env=dict(os.environ,ASAN_OPTIONS='quarantine_size_mb=32',UBSAN_OPTIONS='halt_on_error=1'))
 exits[name]=r.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,sort_keys=True,indent=2)+'\n')
 assert r.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_PREPARED115_LOCAL',name,r.returncode,flush=True)
for label,source in (('main',root),('clean',clean)):
 t=source/'tools/pioneer';tests=t/'tests';native=out/(label+'-native');client=out/(label+'-client')
 run(label+'-native',[sys.executable,t/'build_native_prepared_bf16_q4_diagnostic.py',sdk,private,original,native])
 run(label+'-client',[sys.executable,t/'build_native_prepared_bf16_q4_client.py',sdk,private,native,client,generation])
 run(label+'-endpoints',[sys.executable,tests/'run_pioneer_native_prepared_bf16_q4_native_tests.py',native,private,out/(label+'-endpoints')])
 for name,unit in (('lease','pioneer_native_prepared_bf16_q4_lease_test.cpp'),('private','pioneer_native_prepared_bf16_q4_private_test.cpp')):
  args=[host,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
        '-I'+str(native),'-I'+str(private),tests/unit]
  if name=='private':args+=[t/'PioneerModelWeightCache.cpp']
  binary=out/(label+'-'+name)
  run(label+'-'+name+'-compile',[*args,'-o',binary])
  run(label+'-'+name,[binary,native/'image5120',*([original] if name=='private' else [])])
 run(label+'-parent',[sys.executable,tests/'pioneer_native_prepared_bf16_q4_parent_test.py'])
 run(label+'-reader',[sys.executable,tests/'pioneer_native_prepared_bf16_q4_output.py','--self-test'])
 run(label+'-idle',[sys.executable,tests/'pioneer_native_prepared_bf16_q4_launch_admission_test.py'])
 # Changed shared lease/layout must still execute and restore the old114 image.
 run(label+'-old114-build',[sys.executable,t/'build_q38_factored_q4_compare.py',sdk,out/(label+'-old114'),'--configured'])
 binary=out/(label+'-old114-lease')
 run(label+'-old114-compile',[host,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-ffp-contract=off','-fsanitize=address,undefined',
  '-DPIONEER_NATIVE_FACTORED_Q4_CONFIGURED=1','-DPIONEER_NATIVE_FACTORED_Q4_MANUAL_LAST=1',tests/'pioneer_native_factored_q4_compare_lease_test.cpp','-o',binary])
 run(label+'-old114-lease',[binary,out/(label+'-old114')])
same()
for name in ('tenstorrent_pci_probe-native-q4-fpu-stream115','tenstorrent_pci_probe-default43-control','PioneerNativePreparedBf16Q4Data.h',
             'PioneerNativeCompactQ4StreamGeneratedImage.h'):
 assert (out/('main-native/'+name)).read_bytes()==(out/('clean-native/'+name)).read_bytes(),name
assert (out/'main-client/pioneer-native-prepared-bf16-q4-compare').read_bytes()==(out/'clean-client/pioneer-native-prepared-bf16-q4-compare').read_bytes()
for name in ('lease','private','parent','reader','idle','old114-lease'):
 assert (out/('main-'+name+'.stdout')).read_bytes()==(out/('clean-'+name+'.stdout')).read_bytes(),name
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'clean-source-sha256.txt').write_text(''.join(clean_pins[p]+'  '+p+'\n' for p in sorted(clean_pins)))
(out/'private-source-sha256.txt').write_text(''.join(private_frozen[p]+'  '+str(p.relative_to(private))+'\n' for p in sorted(private_frozen)))
(out/'test-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
print('PASS_PREPARED115_QUALIFICATION children'+str(len(exits))+'_stderr0_publicSources'+str(len(frozen))+
      '_privateSources'+str(len(private_frozen))+'_mainCleanNativeClientDataExact_preexistingWorkExcluded_NO_HARDWARE_OR_TPS',flush=True)
