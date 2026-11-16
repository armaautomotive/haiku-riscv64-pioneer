#!/usr/bin/env python3
"""BUILD ONLY CPU-vocabulary/card-inference worker against frozen llama ABI."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys
from pioneer_native_q4_cache_recipe import recipe as native_q4_cache_recipe

assert __debug__
args=sys.argv[1:];batched='--batched' in args
if batched:
    assert args.count('--batched')==1;args.remove('--batched')
metrics='--metrics' in args
if metrics:
    assert batched and args.count('--metrics')==1;args.remove('--metrics')
resident='--resident' in args
if resident:
    assert metrics and not any(k in args for k in ('--vector-q4','--macro-q4','--prepared-q4'))
    assert args.count('--resident')==1;args.remove('--resident')
vector_q4='--vector-q4' in args
if vector_q4:
    assert metrics and args.count('--vector-q4')==1;args.remove('--vector-q4')
macro_q4='--macro-q4' in args
if macro_q4:
    assert metrics and not vector_q4 and args.count('--macro-q4')==1;args.remove('--macro-q4')
spill_q4='--spill-q4' in args
if spill_q4:
    assert macro_q4 and batched and metrics and not resident and args.count('--spill-q4')==1;args.remove('--spill-q4')
spill_q4_sfpnop='--spill-q4-sfpnop' in args
if spill_q4_sfpnop:
    assert macro_q4 and batched and metrics and not resident and not spill_q4 and args.count('--spill-q4-sfpnop')==1
    args.remove('--spill-q4-sfpnop');spill_q4=True
prepared_byte_lut_profile='--prepared-q4-activation-byte-lut-phase-profile' in args
if prepared_byte_lut_profile:
    assert args.count('--prepared-q4-activation-byte-lut-phase-profile')==1 \
        and not any(flag in args for flag in ('--batched','--metrics','--resident','--vector-q4','--macro-q4',
            '--spill-q4','--spill-q4-sfpnop','--prepared-q4','--prepared-q4-current-worker',
            '--prepared-q4-activation-lut-worker','--prepared-q4-activation-byte-lut-worker',
            '--counter-unroll','--phase-profile','--prompt-client'))
    args.remove('--prepared-q4-activation-byte-lut-phase-profile')
    batched=True;metrics=True
prepared_q4=prepared_byte_lut_profile or '--prepared-q4' in args
if prepared_q4 and not prepared_byte_lut_profile:
    assert metrics and not vector_q4 and not macro_q4 and args.count('--prepared-q4')==1;args.remove('--prepared-q4')
readiness_diagnostic='--readiness-diagnostic' in args
if readiness_diagnostic:
    assert prepared_q4 and args.count('--readiness-diagnostic')==1
    args.remove('--readiness-diagnostic')
gdn_l1_tag206='--gdn-l1-tag206' in args
if gdn_l1_tag206:
    assert prepared_q4 and args.count('--gdn-l1-tag206')==1
    args.remove('--gdn-l1-tag206')
prepared_current='--prepared-q4-current-worker' in args
if prepared_current:
    assert batched and metrics and not resident and not vector_q4 and not macro_q4 and not prepared_q4 \
        and '--counter-unroll' not in args and '--phase-profile' not in args \
        and args.count('--prepared-q4-current-worker')==1
    args.remove('--prepared-q4-current-worker');prepared_q4=True
prepared_lut='--prepared-q4-activation-lut-worker' in args
if prepared_lut:
    assert batched and metrics and not resident and not vector_q4 and not macro_q4 and not prepared_q4 \
        and not prepared_current and '--counter-unroll' not in args and '--phase-profile' not in args \
        and args.count('--prepared-q4-activation-lut-worker')==1
    args.remove('--prepared-q4-activation-lut-worker');prepared_q4=True
prepared_byte_lut=prepared_byte_lut_profile or '--prepared-q4-activation-byte-lut-worker' in args
if prepared_byte_lut and not prepared_byte_lut_profile:
    assert batched and metrics and not resident and not vector_q4 and not macro_q4 and not prepared_q4 \
        and not prepared_current and not prepared_lut and '--counter-unroll' not in args and '--phase-profile' not in args \
        and args.count('--prepared-q4-activation-byte-lut-worker')==1
    args.remove('--prepared-q4-activation-byte-lut-worker');prepared_q4=True
prompt_client='--prompt-client' in args
counter_unroll=prepared_byte_lut_profile or '--counter-unroll' in args
if counter_unroll and not prepared_byte_lut_profile:
    assert prepared_q4 and args.count('--counter-unroll')==1;args.remove('--counter-unroll')
phase_profile=prepared_byte_lut_profile or '--phase-profile' in args
if phase_profile and not prepared_byte_lut_profile:
    assert counter_unroll and args.count('--phase-profile')==1;args.remove('--phase-profile')
if prompt_client:
    assert metrics and args.count('--prompt-client')==1;args.remove('--prompt-client')
assert len(args)==3
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
build,libraries,out=map(pathlib.Path,args)
assert build.is_absolute() and libraries.is_absolute() and out.is_absolute()
assert build.is_dir() and libraries.is_dir() and not out.exists() and not out.is_symlink()
pins={'libllama.so.0.2.0':'0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af',
      'libggml.so.0.21.0':'a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16',
      'libggml-base.so.0.21.0':'9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389',
      'libggml-cpu.so.0.21.0':'69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2'}
for name,digest in pins.items():assert hashlib.sha256((libraries/name).read_bytes()).hexdigest()==digest
here=pathlib.Path(__file__).resolve().parent;root=here.parent.parent
out.mkdir(mode=0o700);headers=out/'headers';headers.mkdir()
# Read only the frozen commit,not dirty submodule headers or objects.
archive=subprocess.Popen(['git','-C',str(root/'src/libs/llama.cpp'),'archive',
                          'c060ca974c773c7c3d17fd1b66dc9d312bc292c0','include/llama.h','ggml/include'],stdout=subprocess.PIPE)
unpack=subprocess.run(['tar','-x','-C',str(headers)],stdin=archive.stdout)
archive.stdout.close();assert archive.wait()==0 and unpack.returncode==0
sdk=build/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release=build/'objects/haiku/riscv64/release';tool=str(build/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
includes=['-I'+str(sdk/'headers'),'-idirafter',str(sdk/'headers/posix')]
for path in [sdk/'headers/os',*sorted((sdk/'headers/os').iterdir())]:
    if path.is_dir():includes+=['-isystem',str(path)]
includes+=['-I'+str(headers/'include'),'-I'+str(headers/'ggml/include')]
libflags=['-L'+str(p) for p in sorted((build/'build_packages').glob('gcc_syslibs-*/lib'))]
source=here/'pioneer_assistant_qwen27_full_offload.cpp';obj=out/'worker.o';binary=out/'HaikuAssistantQwen27FullOffload'
if vector_q4:source=here/'pioneer_assistant_qwen27_vector_q4_full_offload.cpp'
if macro_q4:source=here/'pioneer_assistant_qwen27_macro_q4_full_offload.cpp'
if prepared_current:source=here/'pioneer_assistant_qwen27_prepared_current_q4_full_offload.cpp'
elif prepared_lut:source=here/'pioneer_assistant_qwen27_prepared_lut_q4_full_offload.cpp'
elif prepared_byte_lut:source=here/'pioneer_assistant_qwen27_prepared_byte_lut_q4_full_offload.cpp'
elif prepared_q4:source=here/'pioneer_assistant_qwen27_prepared_q4_full_offload.cpp'
if counter_unroll:source=here/'pioneer_assistant_qwen27_prepared_counter_q4_full_offload.cpp'
if phase_profile:source=here/'pioneer_assistant_qwen27_prepared_profile_q4_full_offload.cpp'
compile=[tool+'g++','-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
         '-fstack-usage','-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1',*includes,'-c',str(source),'-o',str(obj)]
if batched:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1')
if metrics:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1')
if resident:compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=1','-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT_WORKER=1']
if vector_q4:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1')
if macro_q4:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=1')
if spill_q4:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4=1')
if spill_q4_sfpnop:
    compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP=1',
                  '-DPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP_CLIENT=1']
if counter_unroll:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL=1')
if phase_profile:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE=1')
if prepared_q4:
    # The current exact source set includes the separately reviewed,
    # reader-only receipt/reopen changes. Keep the on-disk writer recipe key
    # unchanged; unknown source edits are refused by the identity resolver.
    writer_source,writer=native_q4_cache_recipe(root)
    compile[1:1]=['-Wno-misleading-indentation','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1','-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="'+writer+'"']
    (out/'cache-recipe-source.txt').write_text('SOURCE_FINGERPRINT '+writer_source+'\nUNCHANGED_CACHE_RECIPE '+writer+'\n')
    if prepared_current:compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER=1']
if readiness_diagnostic:compile.insert(1,'-DPIONEER_Q38_READINESS_DIAGNOSTIC=1')
if gdn_l1_tag206:compile.insert(1,'-DPIONEER_Q38_EXPECTED_DRIVER_TAG=0x2026111500010206ULL')
if prepared_lut:compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER=1']
if prepared_byte_lut:compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER=1']
if prepared_byte_lut_profile:compile[1:1]=['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE=1']
objects=[str(obj)]
if prepared_q4:
    cache_obj=out/'weight-cache.o'
    cache_compile=[tool+'g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'headers/private/libroot'),*includes,'-c',str(here/'PioneerModelWeightCache.cpp'),'-o',str(cache_obj)]
    subprocess.run(cache_compile,check=True);objects.append(str(cache_obj))
link=[tool+'g++',*objects,'-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),
      '-L'+str(release/'system/libnetwork'),'-L'+str(libraries),*libflags,'-Wl,-rpath-link,'+str(libraries),
      *['-l:'+name for name in pins],'-lnetwork','-o',str(binary)]
subprocess.run(compile,check=True);subprocess.run(link,check=True)
refusals=[('missing-client',['-UPIONEER_Q38_FULL_OFFLOAD_CLIENT']),
          ('wrong-client',['-UPIONEER_Q38_FULL_OFFLOAD_CLIENT','-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=2']),
          ('fast-math',['-ffast-math'])]
for macro in ('_KERNEL_MODE','PIONEER_Q38_CPU_VALIDATION','PIONEER_Q38_FULL_MODEL_CPU_REFERENCE',
              'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE',
              'PIONEER_Q38_TOKEN_HIDDEN_VALIDATION_CLIENT','PIONEER_Q38_NATIVE_Q8_LAYOUT'):
    refusals.append((macro+'-defined0',['-D'+macro+'=0']))
refusals.append(('zero-batched',['-UPIONEER_Q38_FULL_OFFLOAD_BATCHED','-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=0']))
if metrics:refusals.append(('zero-metrics',['-UPIONEER_Q38_FULL_OFFLOAD_METRICS','-DPIONEER_Q38_FULL_OFFLOAD_METRICS=0']))
if resident:
    refusals += [('missing-resident',['-UPIONEER_Q38_FULL_OFFLOAD_RESIDENT']),
                 ('zero-resident',['-UPIONEER_Q38_FULL_OFFLOAD_RESIDENT','-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0'])]
if vector_q4:
    refusals += [('missing-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4']),
                 ('zero-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4','-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0']),
                 ('resident-vector-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0'])]
if macro_q4:
    refusals += [('missing-macro-q4',['-UPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4']),
                 ('zero-macro-q4',['-UPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4','-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=0']),
                 ('resident-macro-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0']),
                 ('vector-macro-q4',['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0'])]
if spill_q4:
    refusals += [('zero-spill-q4',['-UPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4','-DPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4=0']),
                 ('resident-spill-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=1']),
                 ('prepared-spill-q4',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1'])]
    if spill_q4_sfpnop:
        refusals += [('missing-spill-q4-sfpnop',['-UPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP']),
                     ('zero-spill-q4-sfpnop',['-UPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP','-DPIONEER_Q38_FULL_OFFLOAD_SPILL_Q4_SFPNOP=0'])]
if prepared_q4:
    refusals += [('missing-prepared-q4',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4']),
                 ('zero-prepared-q4',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=0']),
                 ('prepared-raw-vector',['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0']),
                 ('prepared-raw-macro',['-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=0']),
                 ('prepared-resident',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0']),
                 ('prepared-missing-writer',['-UPIONEER_NATIVE_Q4_PREPARATION_SHA256'])]
    if readiness_diagnostic:
        refusals += [('missing-readiness-diagnostic',['-UPIONEER_Q38_READINESS_DIAGNOSTIC']),
                     ('zero-readiness-diagnostic',['-UPIONEER_Q38_READINESS_DIAGNOSTIC','-DPIONEER_Q38_READINESS_DIAGNOSTIC=0'])]
    if gdn_l1_tag206:
        refusals += [('gdn-l1-tag206-without-prepared-q4',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4']),
                     ('wrong-gdn-l1-driver-tag',['-UPIONEER_Q38_EXPECTED_DRIVER_TAG','-DPIONEER_Q38_EXPECTED_DRIVER_TAG=0x2026111500010205ULL'])]
    if prepared_current:
        refusals += [('missing-current-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER']),
                     ('zero-current-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER=0'])]
    if prepared_lut:
        refusals += [('missing-activation-lut-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER']),
                     ('zero-activation-lut-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER=0']),
                     ('activation-lut-with-current-worker',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER=1'])]
    if prepared_byte_lut:
        refusals += [('missing-activation-byte-lut-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER']),
                     ('zero-activation-byte-lut-worker',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_WORKER=0']),
                     ('activation-byte-lut-with-current-worker',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_CURRENT_WORKER=1']),
                     ('activation-byte-lut-with-activation-lut-worker',['-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_LUT_WORKER=1'])]
    if prepared_byte_lut_profile:
        refusals += [('missing-activation-byte-lut-phase-profile',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE']),
                     ('zero-activation-byte-lut-phase-profile',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4_ACTIVATION_BYTE_LUT_PHASE_PROFILE=0']),
                     ('activation-byte-lut-phase-profile-old-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026111500010104ULL'])]
if counter_unroll:
    refusals += [('missing-counter-unroll',['-UPIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL']),
                 ('zero-counter-unroll',['-UPIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL','-DPIONEER_Q38_FULL_OFFLOAD_COUNTER_UNROLL=0'])]
if phase_profile:
    refusals += [('missing-phase-profile',['-UPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE']),
                 ('zero-phase-profile',['-UPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE','-DPIONEER_Q38_FULL_OFFLOAD_PHASE_PROFILE=0']),
                 ('old-phase-client',['-DPIONEER_Q38_TOKEN_PHASE_PROFILE_CLIENT=1'])]
for label,extra in refusals:
    result=subprocess.run([*compile[:compile.index('-c')],*extra,'-fsyntax-only',str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=out)
    assert result.returncode and any(t in result.stderr for t in ('Full-offload client is separate','Full-card spill Q4 requires','Tag101 Assistant requires','Resident worker requires','Full-card metrics requires','Full-card resident lifecycle requires','Full-card vector Q4 requires','Full-card macro Q4 requires','Full-card prepared Q4 requires','Full-card byte-LUT plus phase profile requires the distinct tag105 role marker','Full-card prepared Q4 tag105 requires exact paired byte-LUT, counter and phase-profile roles','Current prepared-Q4 Assistant worker requires its explicit tag102 role','Prepared-Q4 activation-LUT Assistant worker requires its separate tag103 role','Prepared-Q4 byte-index activation-LUT Assistant worker requires its separate tag104 role','Tag105 requires the exact byte-LUT workers paired only with the phase-profile ROOT','Full-card prepared counter Q4 requires','Full-card phase profile requires','Prepared full-card source requires')),label
(out/'role-refusals.txt').write_text('\n'.join('PASS '+label for label,_ in refusals)+'\n')
undefined=subprocess.check_output([tool+'nm','-u',str(binary)],text=True)
llama_imports={r.split()[-1] for r in undefined.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
expected={'llama_log_set','llama_model_default_params','llama_model_free','llama_model_get_vocab',
          'llama_model_load_from_file_ptr','llama_token_to_piece','llama_tokenize','llama_vocab_eos',
          'llama_vocab_eot','llama_vocab_is_eog','llama_vocab_n_tokens'}
assert llama_imports==expected,(llama_imports,expected)
for name in ('llama_decode','llama_init_from_model','llama_get_logits','ggml_backend','ggml_threadpool','PioneerQ38Ggml'):
    assert name not in undefined,name
rows=[r.split('\t') for r in obj.with_suffix('.su').read_text().splitlines()]
assert rows and all(len(r)==3 and r[2]=='static' and int(r[1])<=32768 for r in rows)
smoke=out/'pioneer-q38-full-offload-smoke'
smoke_command=[tool+'g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',*includes,
               '-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),'-L'+str(release/'system/libnetwork'),
               *libflags,str(here/'pioneer_q38_full_offload_smoke.cpp'),'-lnetwork','-o',str(smoke)]
if gdn_l1_tag206:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-readiness-tag206-20261116-b/readiness-diagnostic-tag206"')
elif prepared_byte_lut_profile:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-profile-byte-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-profile-byte-lut-q4"')
elif phase_profile:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-profile-q4"')
elif counter_unroll:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-counter-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-counter-q4"')
elif prepared_current:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-current-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-current-q4"')
elif prepared_lut:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-lut-q4"')
elif prepared_byte_lut:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-byte-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-byte-lut-q4"')
elif prepared_q4:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"')
elif spill_q4_sfpnop:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-spill-q4-sfpnop-20261115-a/HaikuAssistantQwen27FullOffload-spill-q4-sfpnop"')
elif spill_q4:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-spill-q4-20261115-a/HaikuAssistantQwen27FullOffload-spill-q4"')
elif macro_q4:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-macro-q4-20261104-a/HaikuAssistantQwen27FullOffload-macro-q4"')
elif vector_q4:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-vector-q4-20261104-a/HaikuAssistantQwen27FullOffload-vector-q4"')
elif metrics:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"')
elif batched:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-batched-20261104-a/HaikuAssistantQwen27FullOffload-batched"')
subprocess.run(smoke_command,check=True)
if metrics:
    rate=out/'pioneer-q38-full-offload-rate-smoke';rate_command=smoke_command[:]
    rate_command[rate_command.index(str(here/'pioneer_q38_full_offload_smoke.cpp'))]=str(here/'pioneer_q38_full_offload_rate_smoke.cpp')
    rate_command[rate_command.index('-o')+1]=str(rate)
    subprocess.run(rate_command,check=True)
    rate_imports=subprocess.check_output([tool+'nm','-u',str(rate)],text=True)
    assert 'llama_' not in rate_imports and 'ggml_' not in rate_imports
    print(hashlib.sha256(rate.read_bytes()).hexdigest(),rate.name,flush=True)
if prompt_client:
    ask=out/'pioneer-q38-full-offload-ask'
    ask_command=[tool+'g++','-std=c++17','-O2','-ffp-contract=off','-fno-exceptions','-fno-rtti',
                 '-Wall','-Wextra','-Werror','-DPIONEER_Q38_FULL_OFFLOAD_PROMPT_CLIENT=1',
                 '-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"',
                 *includes,'-B'+str(release/'system/glue'),'-L'+str(release/'system/libroot'),
                 '-L'+str(release/'system/libnetwork'),*libflags,
                 str(here/'pioneer_q38_full_offload_ask.cpp'),
                 str(root/'src/apps/assistant/P100AQwen27FullOffload.cpp'),'-lnetwork','-o',str(ask)]
    if resident:
        ask_command.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT_CLIENT=1')
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/config/non-packaged/servers/HaikuAssistantQwen27FullOffload"'
    if vector_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-vector-q4-20261104-a/HaikuAssistantQwen27FullOffload-vector-q4"'
    if macro_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-macro-q4-20261104-a/HaikuAssistantQwen27FullOffload-macro-q4"'
    if spill_q4_sfpnop:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-spill-q4-sfpnop-20261115-a/HaikuAssistantQwen27FullOffload-spill-q4-sfpnop"'
    elif spill_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-spill-q4-20261115-a/HaikuAssistantQwen27FullOffload-spill-q4"'
    if prepared_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
    if prepared_current:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-current-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-current-q4"'
    if prepared_lut:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-lut-q4"'
    if prepared_byte_lut:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-byte-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-byte-lut-q4"'
    if counter_unroll:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-counter-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-counter-q4"'
    if phase_profile:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-counter-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-counter-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-profile-q4"'
    if prepared_byte_lut_profile:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-profile-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-profile-q4"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-profile-byte-lut-q4-20261115-a/HaikuAssistantQwen27FullOffload-prepared-profile-byte-lut-q4"'
    subprocess.run(ask_command,check=True)
    ask_imports=subprocess.check_output([tool+'nm','-u',str(ask)],text=True)
    for name in ('llama_','ggml_','ioctl','mmap',' kill',' system',' popen'):
        assert name not in ask_imports,name
    # Haiku's ordinary executable glue includes a weak pthread_cancel hook,
    # also present in the unchanged qualified no-model smoke client. Do not
    # mistake that startup hook for a model CPU pool;no strong pthread API
    # or additional weak thread hook is admitted in the new client.
    ask_threads={tuple(r.split()) for r in ask_imports.splitlines() if 'pthread_' in r}
    smoke_imports=subprocess.check_output([tool+'nm','-u',str(smoke)],text=True)
    smoke_threads={tuple(r.split()) for r in smoke_imports.splitlines() if 'pthread_' in r}
    assert ask_threads=={('w','pthread_cancel')} and ask_threads.issubset(smoke_threads),ask_threads
    (out/'ask-imports.txt').write_text(ask_imports)
    print(hashlib.sha256(ask.read_bytes()).hexdigest(),ask.name,flush=True)
if not batched:
    assert hashlib.sha256(binary.read_bytes()).hexdigest()=='8cc39a66185e3adb1606a92a91c4f9efeb5943d4e1439cfedc41214b75e8aa5d'
    assert hashlib.sha256(smoke.read_bytes()).hexdigest()=='f172e77c58c4eac025ebe7ac53c686a01dadceacf31f1e25f00f00ca6b651081'
imports=subprocess.check_output([tool+'nm','-u',str(smoke)],text=True)
assert 'llama_' not in imports and 'ggml_' not in imports
commands=[compile,link,smoke_command,rate_command] if metrics else [compile,link,smoke_command]
if prepared_q4:commands.insert(0,cache_compile)
if prompt_client:commands.append(ask_command)
(out/'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands)+'\n')
(out/'llama-imports.txt').write_text('\n'.join(sorted(llama_imports))+'\n')
print(hashlib.sha256(binary.read_bytes()).hexdigest(),binary.name,flush=True)
print(hashlib.sha256(smoke.read_bytes()).hexdigest(),smoke.name,flush=True)
print('PASS BUILD_ONLY vocabulary-only full-card worker;exact11 vocabulary/model-container API imports;'+str(len(refusals))+' role refusals;NO context,decode,logits,threadpool or hybrid backend imports;frozen libraries/headers',flush=True)
print('NOT installed or physically/numerically qualified;no performance result',flush=True)
print('Full-card firmware pairing: '+('prepared phase-profile ROOT plus byte-LUT workers tag105' if prepared_byte_lut_profile else 'SFPNOP spill macro Q4 tag101' if spill_q4_sfpnop else 'spill macro Q4 tag100' if spill_q4 else 'resident lifecycle tag91' if resident else 'prepared profile Q4 tag99' if phase_profile else 'prepared counter Q4 tag98' if counter_unroll else 'prepared Q4 byte-index activation-LUT worker tag104' if prepared_byte_lut else 'prepared Q4 activation-LUT worker tag103' if prepared_lut else 'current prepared Q4 worker tag102' if prepared_current else 'prepared Q4 tag97' if prepared_q4 else 'macro Q4 tag93' if macro_q4 else 'vector Q4 tag92' if vector_q4 else 'metrics tag90' if metrics else 'batched tag89' if batched else 'original tag88'),flush=True)
