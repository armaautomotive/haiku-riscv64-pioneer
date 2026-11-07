#!/usr/bin/env python3
"""Build-only, isolated tag117 diagnostic. Never install or touch the device.

The tag92 driver and USER binaries are independently reproduced first. A
private source overlay binds ONLY this diagnostic to the experimental ROOT
and a new tag, while preserving the original source and installed Assistant.
Neither this build nor its checks grant hardware-execution authority.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

assert __debug__ and len(sys.argv) == 5
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
image, native_control, client_control, out = map(Path, sys.argv[1:])
assert all(p.is_absolute() for p in (image, native_control, client_control, out))
assert not out.exists() and not out.is_symlink()
root = Path(__file__).resolve().parents[2]
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(native_control/'tenstorrent_pci_probe-native') == 'c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014'
assert sha(client_control/'HaikuAssistantQwen27FullOffload') == '1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9'
assert sha(client_control/'pioneer-q38-full-offload-rate-smoke') == '9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af'
expected = {
 'boot.bin': (4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
 'text.bin': (130316,'6a7fe5c763e930a6b762a274b46746918a12d691e0cbe1c65cfe9a177b787033'),
 'terminal.bin': (8,'468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2'),
 'conv.bin': (32680,'b3e45e700fb0e883a327616475e8af53a76383763aad0e937408bacccf681892'),
 'program.elf': (0,'a5db3c211bf61419bb5cdfc55060086dd82102cb6a12b803e3a1599431970a87')}
for name,(size,digest) in expected.items():
 p=image/name;assert p.is_file() and not p.is_symlink() and sha(p)==digest and (not size or p.stat().st_size==size),name
out.mkdir(mode=0o700);overlay=out/'source';overlay.mkdir()
native_commands=[r.split('\t') for r in (native_control/'build-argv.txt').read_text().splitlines()]
user_commands=[r.split('\t') for r in (client_control/'build-argv.txt').read_text().splitlines()]
nc=next(c for c in native_commands if '-c' in c and c[c.index('-c')+1].endswith('/pioneer_q38_full_offload_driver.cpp'))
nl=next(c for c in native_commands if '-shared' in c and str(native_control/'native.o') in c)
uc=user_commands[0];ul=user_commands[1];rate=user_commands[3]
assert '-c' in uc and uc[uc.index('-c')+1].endswith('/pioneer_assistant_qwen27_vector_q4_full_offload.cpp')
dep=(native_control/'native.d').read_text().replace('\\\n','').split(':',1)[1]
worker_dep=subprocess.check_output([*uc[:uc.index('-c')],'-MM',uc[uc.index('-c')+1]],text=True).replace('\\\n','').split(':',1)[1]
public=set()
for item in (dep+' '+worker_dep).split():
 p=Path(item).resolve()
 if p.is_relative_to(root):public.add(p)
for command in (nc,uc):public.add(Path(command[command.index('-c')+1]).resolve())
assert public
frozen={p:sha(p) for p in public}
for p in public:
 target=overlay/p.relative_to(root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,target)
def target(path):return overlay/path.relative_to(root)
def rebased(command):
 result=[]
 for value in command:
  # Remap only source/include paths inside the repository; SDK and original
  # reproducible build outputs remain read-only. System paths stay original.
  if value.startswith(str(root)):
   p=Path(value);value=str(target(p)) if p in public else value
  elif value=='-I'+str(root/'tools/pioneer'):
   value='-I'+str(overlay/'tools/pioneer')
  result.append(value)
 return result
commands=[]
def run(command):
 commands.append(list(map(str,command)));subprocess.run(command,check=True,cwd=out)
# The overlay must reproduce both WHOLE original source translation units
# BEFORE its private, explicitly identified diagnostic modifications.
for original,command,filename in ((native_control/'native.o',nc,'original-native.o'),
                                  (client_control/'worker.o',uc,'original-worker.o')):
 check=rebased(command)
 if '-MF' in check:check[check.index('-MF')+1]=str(out/(filename+'.d'))
 check[check.index('-o')+1]=str(out/filename);run(check)
 assert (out/filename).read_bytes()==original.read_bytes(),'unchanged whole overlay TU '+filename
def rewrite(relative,old,new):
 p=overlay/relative;s=p.read_text();assert s.count(old)==1,(relative,old);p.write_text(s.replace(old,new))
driver='src/add-ons/kernel/drivers/misc/'
for name in ('PioneerQ38FullOffloadVectorQ4Role.h','PioneerQ38FullOffloadProtocol.h','tenstorrent_pci_probe.cpp'):
 rewrite(driver+name,'0x2026110400010092','0x2026110600010117')
role=overlay/driver/'PioneerQ38FullOffloadVectorQ4Role.h'
role.write_text('#if !defined(PIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT) || PIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT != 1\n#error Snapshot diagnostic marker required\n#endif\n'+role.read_text())
program=overlay/'tools/pioneer/PioneerQ38FullOffloadVectorQ4Programs.h'
assert sha(program)=='3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389'
text=program.read_text()
def array(label,data):
 words=[int.from_bytes(data[i:i+4],'little') for i in range(0,len(data),4)]
 return ('// SHA256 '+hashlib.sha256(data).hexdigest()+'\nstatic const uint32_t '+label+'[]={\n'
  +'\n'.join(' '+','.join('0x%08xU'%w for w in words[i:i+8])+',' for i in range(0,len(words),8))
  +'\n};\nstatic_assert(sizeof('+label+')=='+str(len(data))+',"exact complete '+label+' DATA");')
for label,name in {'RootBoot':'boot.bin','RootText':'text.bin','RootTerminal':'terminal.bin','RootAux':'conv.bin'}.items():
 pattern=r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t '+label+r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\('+label+r'\)==[0-9]+,"exact complete '+label+r' DATA"\);'
 text,count=re.subn(pattern,lambda _:array(label,(image/name).read_bytes()),text,flags=re.S);assert count==1,label
program.write_text(text)
nc=rebased(nc);nc.insert(1,'-DPIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT=1')
at=next(i for i,s in enumerate(nc) if s.startswith('-DPIONEER_DRIVER_BUILD_TAG='))
assert nc[at].endswith('0x2026110400010092ULL');nc[at]='-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010117ULL'
nc[nc.index('-MF')+1]=str(out/'native.d');nc[nc.index('-o')+1]=str(out/'native.o');run(nc)
nl=[str(out/'native.o') if s==str(native_control/'native.o') else str(out/'tenstorrent_pci_probe-snapshot117') if s==str(native_control/'tenstorrent_pci_probe-native') else s for s in nl];run(nl)
uc=rebased(uc);uc.insert(1,'-DPIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT=1');uc[uc.index('-o')+1]=str(out/'worker.o');run(uc)
ul=[str(out/'worker.o') if s==str(client_control/'worker.o') else str(out/'HaikuAssistantQwen27Snapshot') if s==str(client_control/'HaikuAssistantQwen27FullOffload') else s for s in ul];run(ul)
old_worker=next(s for s in rate if s.startswith('-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER='))
rate[rate.index(old_worker)]='-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="/boot/home/pioneer-q38-job-snapshot-117-a/HaikuAssistantQwen27Snapshot"'
rate[rate.index('-o')+1]=str(out/'pioneer-q38-snapshot-rate-smoke');run(rate)
tool=nc[0].replace('g++','')
imports=subprocess.check_output([tool+'nm','-u',str(out/'HaikuAssistantQwen27Snapshot')],text=True)
control_imports=subprocess.check_output([tool+'nm','-u',str(client_control/'HaikuAssistantQwen27FullOffload')],text=True)
llama_imports=lambda s:{r.split()[-1] for r in s.splitlines() if r.split() and r.split()[-1].startswith('llama_')}
assert llama_imports(imports)==llama_imports(control_imports) and len(llama_imports(imports))==11
for name in ('llama_decode','llama_init_from_model','llama_get_logits','ggml_backend','ggml_threadpool'):
 assert name not in imports,name
assert 'llama_tokenize' in imports
driver_symbols=subprocess.check_output([tool+'nm','-C',str(out/'tenstorrent_pci_probe-snapshot117')],text=True)
for name in ('api_version','init_hardware','init_driver','uninit_driver','publish_devices','find_device',
             'Q38TokenNativeControl(','BeginQ38TokenNativeBackendScoped(','FinishQ38TokenNativeBackendScoped('):
 assert name in driver_symbols,name
for name in ('llama_','ggml_','Q38WorkControl','ReferenceProducts','OriginalSource','SyntheticIO','Q38FullResidentControl('):
 assert name not in driver_symbols,name
frames=[row.split('\t') for row in (out/'native.su').read_text().splitlines()]
assert frames and all(len(row)==3 and row[2]=='static' for row in frames)
token_frames=[int(row[1]) for row in frames if 'Token' in row[0] or 'FullOffload' in row[0]]
assert token_frames and max(token_frames)<=2048
raw=subprocess.check_output([tool+'nm','-S','--defined-only',str(out/'native.o')],text=True)
entries={r[-1]:(int(r[0],16),int(r[1],16)) for r in map(str.split,raw.splitlines()) if len(r)==4}
rows=subprocess.check_output([tool+'objdump','-t',str(out/'native.o')],text=True)
sections={r[-1]:r[-3] for r in map(str.split,rows.splitlines()) if len(r)>=6 and r[-1] in entries}
arrays=re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=',text);assert len(arrays)==14
for digest,label in arrays:
 names=[n for n in entries if 'PioneerQ38FullOffloadVectorQ4Programs' in n and n.endswith(str(len(label))+label+'E')]
 assert len(names)==1,label
 first,size=entries[names[0]];dest=out/(label+'.raw')
 run([tool+'objcopy','-O','binary','-j',sections[names[0]],out/'native.o',dest])
 assert hashlib.sha256(dest.read_bytes()[first:first+size]).hexdigest()==digest,label
for command in (nc,uc):
 for flags in (['-UPIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT'],
               ['-UPIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT','-DPIONEER_Q38_EARLY_JOB_NATIVE_EXPERIMENT=0'],
               ['-DPIONEER_Q38_CPU_VALIDATION=0'],['-DPIONEER_Q38_WORK_INFERENCE=0']):
  probe=[*command[:command.index('-c')],*flags,'-fsyntax-only',command[command.index('-c')+1]]
  result=subprocess.run(probe,cwd=out,capture_output=True,text=True);assert result.returncode,flags
probe=[*nc[:nc.index('-c')],'-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010092ULL',
       '-fsyntax-only',nc[nc.index('-c')+1]]
assert subprocess.run(probe,cwd=out,capture_output=True,text=True).returncode,'old native tag must be refused'
assert all(sha(p)==v for p,v in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'build-argv.json').write_text(json.dumps(commands,indent=2)+'\n')
for name in ('tenstorrent_pci_probe-snapshot117','HaikuAssistantQwen27Snapshot','pioneer-q38-snapshot-rate-smoke'):
 print(name,(out/name).stat().st_size,sha(out/name),flush=True)
print('BUILD ONLY tag117/14 arrays authenticated/old binaries reproduced/no CPU inference imports; NO hardware, full-path correctness or speed qualification')
