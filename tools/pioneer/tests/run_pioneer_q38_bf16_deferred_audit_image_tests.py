#!/usr/bin/env python3
"""Build and execute the bookkeeping class as RV32 code, never on hardware."""
import hashlib,json,os,pathlib,re,subprocess,sys
assert __debug__ and len(sys.argv)==3 and not os.environ.get('PYTHONOPTIMIZE')
sdk,out=map(pathlib.Path,sys.argv[1:]);here=pathlib.Path(__file__).resolve().parent;root=here.parents[2]
assert sdk.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
out.mkdir(mode=0o700);image=out/'image';image.mkdir(mode=0o700)
tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-');cxx=os.environ.get('CXX','c++')
source=here/'pioneer_q38_bf16_deferred_audit_rv32.cpp';test=here/'pioneer_q38_bf16_deferred_audit_image_test.cpp'
flags=['-DPIONEER_Q38_TEST_DEFERRED_AUDIT_IMAGE=1','-std=c++11','-O2','-Wall','-Wextra','-Werror',
 '-ffreestanding','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-pic','-fno-pie',
 '-ffunction-sections','-fdata-sections','-fno-unwind-tables','-fno-asynchronous-unwind-tables',
 '-msmall-data-limit=0','-mno-relax','-march=rv32imf','-mabi=ilp32','-ffixed-s11','-fstack-usage']
dependencies={pathlib.Path(__file__).resolve(),here.parent/'blackhole_scalar_start.S',here.parent/'blackhole_scalar.ld'}
for compiler,extra,path in ((tool+'g++',flags,source),(cxx,['-std=c++17'],test)):
 data=subprocess.check_output([compiler,*extra,'-MM',str(path)],text=True)
 dependencies.update(pathlib.Path(p).resolve() for p in data.replace('\\\n','').split(':',1)[1].split())
assert all(p.is_relative_to(root) for p in dependencies)
frozen={p:hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies};exits={};commands=[]
def run(name,args):
 args=list(map(str,args));commands.append([name,*args]);(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 with (out/(name+'.stdout')).open('xb') as a,(out/(name+'.stderr')).open('xb') as b:
  result=subprocess.run(args,stdout=a,stderr=b,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
 exits[name]=result.returncode;(out/'actual-exits.json').write_text(json.dumps(exits,indent=2)+'\n')
 assert result.returncode==0 and not (out/(name+'.stderr')).stat().st_size,name
 print('CLOSED_RV32_LEDGER',name,'actual0_stderr0',flush=True)
run('rv32-compile',[tool+'g++',*flags,'-c',source,'-o',image/'program.o'])
run('assemble',[tool+'as','-march=rv32imf','-mabi=ilp32','-mno-relax',here.parent/'blackhole_scalar_start.S','-o',image/'start.o'])
run('link',[tool+'ld','-m','elf32lriscv','--no-relax','--gc-sections','-T',here.parent/'blackhole_scalar.ld',image/'start.o',image/'program.o','-o',image/'program.elf'])
for section,leaf in (('boot','boot.bin'),('text','code.bin')):
 run('extract-'+section,[tool+'objcopy','-O','binary','-j','.'+section,image/'program.elf',image/leaf])
assert not subprocess.check_output([tool+'nm','-u',str(image/'program.elf')]).strip()
dis=subprocess.check_output([tool+'objdump','-d',str(image/'program.elf')],text=True)
assert not re.search(r'\b(?:s11|csrr|csrw|ecall|ebreak|divu?|remu?)\b',dis)
run('host-compile',[cxx,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',test,'-o',out/'ledger-test'])
run('execute',[out/'ledger-test',image])
assert all(hashlib.sha256(p.read_bytes()).hexdigest()==digest for p,digest in frozen.items())
(out/'source-sha256.txt').write_text(''.join(frozen[p]+'  '+str(p.relative_to(root))+'\n' for p in sorted(frozen)))
(out/'artifact-sha256.json').write_text(json.dumps({p.name:dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in (image/'boot.bin',image/'code.bin',image/'program.elf')},indent=2)+'\n')
print('PASS_RV32_LEDGER_UNIT_ONLY actual7children0;NOT ROOT integration,real worker closure,native,hardware or speed',flush=True)
