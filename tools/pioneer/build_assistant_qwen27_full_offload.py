#!/usr/bin/env python3
"""BUILD ONLY CPU-vocabulary/card-inference worker against frozen llama ABI."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

assert __debug__
args=sys.argv[1:];batched='--batched' in args
if batched:
    assert args.count('--batched')==1;args.remove('--batched')
metrics='--metrics' in args
if metrics:
    assert batched and args.count('--metrics')==1;args.remove('--metrics')
vector_q4='--vector-q4' in args
if vector_q4:
    assert metrics and args.count('--vector-q4')==1;args.remove('--vector-q4')
macro_q4='--macro-q4' in args
if macro_q4:
    assert metrics and not vector_q4 and args.count('--macro-q4')==1;args.remove('--macro-q4')
prepared_q4='--prepared-q4' in args
if prepared_q4:
    assert metrics and not vector_q4 and not macro_q4 and args.count('--prepared-q4')==1;args.remove('--prepared-q4')
prompt_client='--prompt-client' in args
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
if prepared_q4:source=here/'pioneer_assistant_qwen27_prepared_q4_full_offload.cpp'
compile=[tool+'g++','-std=c++17','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-fno-exceptions','-fno-rtti',
         '-fstack-usage','-DPIONEER_Q38_FULL_OFFLOAD_CLIENT=1',*includes,'-c',str(source),'-o',str(obj)]
if batched:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_BATCHED=1')
if metrics:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_METRICS=1')
if vector_q4:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=1')
if macro_q4:compile.insert(1,'-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=1')
if prepared_q4:
    # Preserve the exact ORIGINAL writer identity. No silent cache rekey or
    # regeneration merely because the new loader/runtime changed.
    writer_sources={}
    def writer_collect(path):
        assert path.is_file() and not path.is_symlink()
        relative=path.relative_to(root).as_posix()
        if relative in writer_sources:return
        data=path.read_bytes();writer_sources[relative]=data
        for name in re.findall(r'^\s*#\s*include\s*"([^"\n]+)"',data.decode(),re.M):
            candidate=path.parent/name;assert not candidate.is_symlink()
            candidate=candidate.resolve();candidate.relative_to(root);writer_collect(candidate)
    for name in ('pioneer_q38_native_q4_prepare.cpp','PioneerModelWeightCache.cpp'):writer_collect(here/name)
    fingerprint=hashlib.sha256()
    for name,data in sorted(writer_sources.items()):fingerprint.update(name.encode()+b'\0'+len(data).to_bytes(8,'little')+data)
    writer=fingerprint.hexdigest()
    assert writer=='70c2c7d0ed6c4fc28842960fa445e0a5af12baca2eac31a3326fa6c5341bebe6','original cache writer changed'
    # Keep the original cache writer byte-identical,including its compact
    # statement formatting;do not rekey11GB merely to silence this warning.
    compile[1:1]=['-Wno-misleading-indentation','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=1','-DPIONEER_NATIVE_Q4_PREPARATION_SHA256="'+writer+'"']
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
if vector_q4:
    refusals += [('missing-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4']),
                 ('zero-vector-q4',['-UPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4','-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0']),
                 ('resident-vector-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0'])]
if macro_q4:
    refusals += [('missing-macro-q4',['-UPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4']),
                 ('zero-macro-q4',['-UPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4','-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=0']),
                 ('resident-macro-q4',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0']),
                 ('vector-macro-q4',['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0'])]
if prepared_q4:
    refusals += [('missing-prepared-q4',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4']),
                 ('zero-prepared-q4',['-UPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4','-DPIONEER_Q38_FULL_OFFLOAD_PREPARED_Q4=0']),
                 ('prepared-raw-vector',['-DPIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4=0']),
                 ('prepared-raw-macro',['-DPIONEER_Q38_FULL_OFFLOAD_MACRO_Q4=0']),
                 ('prepared-resident',['-DPIONEER_Q38_FULL_OFFLOAD_RESIDENT=0']),
                 ('prepared-missing-writer',['-UPIONEER_NATIVE_Q4_PREPARATION_SHA256'])]
for label,extra in refusals:
    result=subprocess.run([*compile[:compile.index('-c')],*extra,'-fsyntax-only',str(source)],
                          stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=out)
    assert result.returncode and any(t in result.stderr for t in ('Full-offload client is separate','Full-card metrics requires','Full-card vector Q4 requires','Full-card macro Q4 requires','Full-card prepared Q4 requires','Prepared full-card source requires')),label
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
if prepared_q4:smoke_command.insert(1,'-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"')
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
    if vector_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-vector-q4-20261104-a/HaikuAssistantQwen27FullOffload-vector-q4"'
    if macro_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-macro-q4-20261104-a/HaikuAssistantQwen27FullOffload-macro-q4"'
    if prepared_q4:
        old='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-metrics-20261104-a/HaikuAssistantQwen27FullOffload-metrics"'
        assert ask_command.count(old)==1
        ask_command[ask_command.index(old)]='-DASSISTANT_QWEN27_FULL_OFFLOAD_WORKER="/boot/home/pioneer-q38-full-prepared-q4-20261105-a/HaikuAssistantQwen27FullOffload-prepared-q4"'
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
print('Full-card firmware pairing: '+('prepared Q4 tag97' if prepared_q4 else 'macro Q4 tag93' if macro_q4 else 'vector Q4 tag92' if vector_q4 else 'metrics tag90' if metrics else 'batched tag89' if batched else 'original tag88'),flush=True)
