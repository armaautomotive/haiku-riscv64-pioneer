#!/usr/bin/env python3
"""Connect actual RV32 ROOT job bytes to a separate worker simulator; no card."""
import ast
import hashlib
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import sys
import tempfile

assert (len(sys.argv) in (6,7) and (len(sys.argv)==6 or sys.argv[6] in ('--gate','--qkv'))) or (len(sys.argv)==9 and sys.argv[6] in ('--full','--full-gate','--full-qkv')) or (len(sys.argv) in (12,15) and sys.argv[6]=='--full-qkv' and sys.argv[9]=='--chain-gate' and (len(sys.argv)==12 or sys.argv[12] in ('--chain-output','--cohort')))
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
sdk,root,worker,leaf,private=map(Path,sys.argv[1:6]);here=Path(__file__).resolve().parent
mode=sys.argv[6] if len(sys.argv)==7 else '--gate' if len(sys.argv)>=9 and sys.argv[6]=='--full-gate' else '--qkv' if len(sys.argv)>=9 and sys.argv[6]=='--full-qkv' else '--out'
geometry={'--gate':(6144,5120),'--qkv':(10240,5120),'--out':(5120,6144)}
full=Path(sys.argv[7]) if len(sys.argv)>=9 else None
chain=Path(sys.argv[10]) if len(sys.argv)>=12 else None
chainOutput=Path(sys.argv[13]) if len(sys.argv)==15 and sys.argv[12]=='--chain-output' else None
cohort=(Path(sys.argv[13]),Path(sys.argv[14])) if len(sys.argv)==15 and sys.argv[12]=='--cohort' else None
if cohort:
    for directory in cohort:
        assert directory.is_absolute() and directory.is_dir() and not directory.is_symlink()
    names=('run_pioneer_q38_mixed_recurrent_cohort_tests.py','run_pioneer_q38_mixed_worker_tests.py')
    # Reuse the established pinned ROOT tuple and worker tuple; never accept
    # a similarly named but different executable as the simulated device.
    cohort_pins=(('boot.bin',4,'32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca'),
                 ('text.bin',23448,'6e31ae597aa9b3491e34daa389d6b19eb724a9db18efc37ee00f43699f36b8ef'),
                 ('conv.bin',7500,'9674113d7c69cf7290b03e4bef133a8aa30cb1e6193dc7411b7941bae603cc78'),
                 ('program.elf',43876,'b177dfbee14d830a56d286e1eb0eb9c22143d63d542d08e0ca7b869c9a13777e'))
    tree=ast.parse((here/names[1]).read_text())
    worker_pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
    for directory,pinset in zip(cohort,(cohort_pins,worker_pins)):
        for name,size,digest in pinset:
            data=(directory/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest,(directory,name)
if full:
    assert full.is_absolute() and full.is_file() and not full.is_symlink() and len(sys.argv[8])==64
    info=full.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    rows,width=geometry[mode];assert info.st_size==32+(rows//32)*((width//32)*1088+128)
    assert hashlib.sha256(full.read_bytes()).hexdigest()==sys.argv[8]
if chain:
    assert chain.is_absolute() and chain.is_file() and not chain.is_symlink() and len(sys.argv[11])==64
    info=chain.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    assert info.st_size==32+(6144//32)*((5120//32)*1088+128)
    assert hashlib.sha256(chain.read_bytes()).hexdigest()==sys.argv[11]
if chainOutput:
    assert chainOutput.is_absolute() and chainOutput.is_file() and not chainOutput.is_symlink() and len(sys.argv[14])==64
    info=chainOutput.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    assert info.st_size==32+(5120//32)*((6144//32)*1088+128)
    assert hashlib.sha256(chainOutput.read_bytes()).hexdigest()==sys.argv[14]
roles={'--gate':5,'--qkv':6,'--out':13}
sample_pins={'--out':('1facbbadaef22f9b1fadaa33a4b19f2168bc9b1a244e9df9d7880ddd97b10cb1','7cef223c05f1e78b0a4c7c45cd24327b569a7d7e9cd3196895151ab67a18d65d'),
             '--gate':('c4781e0c6c3963ff1e1b3ac36c1d21fbb1dd74e2785826489cf67cbb139c9f36','a5fadca5d6baeaa921472989cf038dd4aa2a90a666f4c7b3262c8ee141841ff2'),
             '--qkv':('396fac17c3a837d7b440fb2982e4748e8c06c6c7fbc8299acdade184ec268a8a','a81f5268d942344b4c4cfd4d0d02853dc82f36bfe8ab099a5bfa5d47d94de86d')}
assert all(p.is_absolute() and p.is_dir() and not p.is_symlink() for p in (sdk,root,worker,leaf,private))
assert private.stat().st_uid==os.geteuid() and stat.S_IMODE(private.stat().st_mode)==0o700
pins=((root/'program.elf','15461d563708f2087497c1eb14afffac6e55dd701cf1dd846abc29d40f1b3c3f'),
      (root/'bf16.bin','01fa94fa8d319c08d4dcc76b78c6cf67c52336dcd803643103dba33ba577afdd'),
      (root/'text.bin','42e649a98fc59277df0eca7ebe9f209b3caa665053ff4eed4a383503c85b2922'),
      (root/'terminal.bin','6eb1d84addfac7202ad4f0d747c6e64d3b92c03025651c7dabe6c56c9cba95d5'),
      (root/'conv.bin','9273e7f572f1b488169bea54d57f0c313a9bce992a2b73c48796649c510d3fd6'),
      (worker/'program.elf','e83749ca5a7dc9d66983b9a80e0d6868079b12d8defdf31e40708235fb80366d'),
      (leaf/'program.elf','e8a04408d0c1a2d0ffa90d1d92ca77f3bb56f849c1d714723dfae9f3110ba72f'),
      (private/'PioneerNativeBfpModelBandData.h',sample_pins[mode][0]),
      (private/'PioneerNativeBfpModelBandMetadata.h',sample_pins[mode][1]))
for path,digest in pins:
    assert path.is_file() and not path.is_symlink() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,path
for path,_ in pins[-2:]:
    info=path.lstat();assert info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
compiler=os.environ.get('CXX','c++')
common=['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable',
        '-Wno-misleading-indentation','-ffp-contract=off','-fsanitize=address,undefined']
root_roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWD_PLAN_SIMULATION',
 'PIONEER_Q38_HEAD_MATRIX_PLAN_INSPECTION','PIONEER_Q38_HEAD_MATRIX_SIMULATION','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_QGWE_BATCH_DENSE_PLAN','PIONEER_Q38_QGWF_DUAL_BATCH_PLAN')
worker_roles=('PIONEER_Q38_RECURRENT_BFP8_WIDTH_INSPECTION','PIONEER_Q38_QGWF_BATCH4_WORKER',
 'PIONEER_Q38_QGWE_BATCH_DENSE_WORKER','PIONEER_Q38_QGW8_BFP8_BATCH_WORKER','PIONEER_Q38_BFP4_BATCH_INPUT_EXPERIMENT',
 'PIONEER_Q38_BFP8_BATCH_INPUT_EXPERIMENT','PIONEER_Q38_QGWD_SRCA_DENSE_Q8_WORKER','PIONEER_Q38_DENSE_Q8_MATRIX_WORKER_INSPECTION',
 'PIONEER_Q38_DENSE_BF16_SRCA_BASE_CANDIDATE','PIONEER_Q38_HEAD_MATRIX_WORKER_INSPECTION','PIONEER_Q38_HYBRID_WORKER_IMAGE',
 'PIONEER_Q38_BF16_WORKER_IMAGE','PIONEER_Q38_HYBRID_MATRIX_ATTENTION_INSPECTION','PIONEER_Q38_DENSE_BF16_MATRIX_EXPERIMENT')
env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory(prefix='q38-root-worker-bridge-',dir='/private/tmp') as tmp:
    rootbin,workerbin=Path(tmp)/'root',Path(tmp)/'worker'
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    assert subprocess.check_output([tool+'nm','-nC',str(root/'program.elf')])==(root/'program.demangled').read_bytes()
    assert subprocess.check_output([tool+'nm','-n',str(worker/'program.elf')])==(worker/'program.symbols').read_bytes()
    for image,sections in ((root,('bf16','text','terminal','conv')),
                           (worker,('boot','globalworker','bf16producer','dense','cachedfpu','ffn','service','affine','text','gdn')),
                           (leaf,('boot','text'))):
        for section in sections:
            output=Path(tmp)/'section.bin'
            subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,str(image/'program.elf'),str(output)],check=True)
            assert output.read_bytes()==(image/(section+'.bin')).read_bytes(),(image,section)
    for binary,source,selected_roles in ((rootbin,'pioneer_q38_recurrent_root_job_bridge.cpp',root_roles),
                                (workerbin,'pioneer_q38_recurrent_worker_job_bridge.cpp',worker_roles)):
        subprocess.run([compiler,*common,*['-D'+r+'=1' for r in selected_roles],'-I'+str(private),str(here/source),'-o',str(binary)],check=True)
    if chain:
        chainbin=Path(tmp)/'chain'
        subprocess.run([compiler,*common,*['-D'+r+'=1' for r in worker_roles],'-I'+str(private),
                        str(here/'pioneer_q38_recurrent_gate_qkv_worker_bridge.cpp'),'-o',str(chainbin)],check=True)
    boot=subprocess.check_output([str(workerbin),str(worker),str(leaf),'--boot'],env=env);assert len(boot)==119*256
    job=subprocess.run([str(rootbin),str(root),str(roles[mode])],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout
    chunks=geometry[mode][1]//256;assert len(job)==chunks*512+119*256
    if mode=='--qkv':
        warm=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--after-gate'],input=boot,
                            stdout=subprocess.PIPE,check=True,env=env).stdout
        assert len(warm)==len(job)
        for w in range(chunks):
            at=w*512;assert warm[at:at+256]==job[at:at+256]
            coldProducer=list(struct.unpack('<64I',job[at+256:at+512]))
            warmProducer=list(struct.unpack('<64I',warm[at+256:at+512]))
            assert coldProducer[1]==1 and warmProducer[1]==3
            warmProducer[1]=coldProducer[1];assert warmProducer==coldProducer
        for w in range(119):
            at=chunks*512+w*256
            coldJob=list(struct.unpack('<64I',job[at:at+256]));warmJob=list(struct.unpack('<64I',warm[at:at+256]))
            assert coldJob[1]==(2 if w<20 else 1) and warmJob[1]==(4 if w<20 else 2)
            assert coldJob[27]==coldJob[1] and warmJob[27]==warmJob[1]
            warmJob[1],warmJob[27]=coldJob[1],coldJob[27];assert warmJob==coldJob
        if chain:
            gatejob=subprocess.run([str(rootbin),str(root),'5'],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout
            outjob=subprocess.run([str(rootbin),str(root),'13','--after-qkv'],input=boot,stdout=subprocess.PIPE,check=True,env=env).stdout if chainOutput else b''
            args=[str(chainbin),str(worker),str(leaf),str(chain),str(full)]+([str(chainOutput)] if chainOutput else [])
            retained=subprocess.run(args,input=gatejob+warm+outjob,
                                    stdout=subprocess.PIPE,check=True,env=env).stdout
            snapshotSize=119*((2+8+8+16+64+64+256)+2*(2+8+16+64+64+64+16+32+128))*4
            prefix=(6144+10240)*4
            assert len(retained)==prefix+snapshotSize+(5120*4+snapshotSize if chainOutput else 0)
            if cohort:
                cohortbin=Path(tmp)/'cohort'
                flags=re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?',(here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
                subprocess.run([compiler,*common,*flags,str(here/'pioneer_q38_recurrent_projection_cohort_bridge.cpp'),'-o',str(cohortbin)],check=True)
                gated=subprocess.run([str(cohortbin),*map(str,cohort)],input=retained[:prefix],stdout=subprocess.PIPE,check=True,
                                     env={**env,'ASAN_OPTIONS':'quarantine_size_mb=32'}).stdout
                assert len(gated)==6144*4
                for offset in (0,6144*4):
                    invalid=bytearray(retained[:prefix]);struct.pack_into('<I',invalid,offset,0x7fc00000)
                    refused=subprocess.run([str(cohortbin),*map(str,cohort)],input=invalid,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                                           env={**env,'ASAN_OPTIONS':'quarantine_size_mb=32'})
                    assert refused.returncode and not refused.stdout
                print('PASS actual gate/QKV model-weight projection outputs -> separately seeded actual Conv/GDN cohort, all6144 outputs checked; synthetic state/parameters; NOT continuous workers/device/full-model/TPS',flush=True)
            chainSnapshots=retained[prefix:prefix+snapshotSize]
            warmed=subprocess.run([str(rootbin),str(root),'6','--after-gate-prepare'],input=boot+chainSnapshots,
                                  stdout=subprocess.PIPE,check=True,env=env).stdout
            assert warmed==retained[6144*4:(6144+10240)*4]
            badprior=bytearray(chainSnapshots)
            badprior[((2+8+8+16+64+64+256)+(2+8+16+64))*4]^=1
            rejected=subprocess.run([str(rootbin),str(root),'6','--after-gate-prepare'],input=boot+badprior,
                                    stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
            assert rejected.returncode and not rejected.stdout and b'writes=0 matrix=0 descriptors=0' in rejected.stderr
            if chainOutput:
                outRows=retained[prefix+snapshotSize:prefix+snapshotSize+5120*4]
                outSnapshots=retained[prefix+snapshotSize+5120*4:]
                outputRun=subprocess.run([str(rootbin),str(root),'13','--after-qkv-prepare'],input=boot+outSnapshots,
                                         stdout=subprocess.PIPE,check=True,env=env).stdout
                assert outputRun==outRows
                badprior=bytearray(outSnapshots)
                badprior[((2+8+8+16+64+64+256)+(2+8+16+64))*4]^=1
                rejected=subprocess.run([str(rootbin),str(root),'13','--after-qkv-prepare'],input=boot+badprior,
                                        stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
                assert rejected.returncode and not rejected.stdout and b'writes=0 matrix=0 descriptors=0' in rejected.stderr
    runargs=[str(workerbin),str(worker),str(leaf),'--run-full',str(full)] if full else [str(workerbin),str(worker),str(leaf),'--run']
    captured=subprocess.run(runargs,input=job,stdout=subprocess.PIPE,check=True,env=env).stdout
    resultBytes=geometry[mode][0]*4
    stageWords=2+8+16+64+64+64+16+32+128
    snapshotBytes=119*((2+8+8+16+64+64+256)+2*stageWords)*4
    assert len(captured)==resultBytes+snapshotBytes
    result,snapshots=captured[:resultBytes],captured[resultBytes:]
    if chain:assert retained[6144*4:(6144+10240)*4]==result
    stored=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--store'],input=boot+result,
                          stdout=subprocess.PIPE,check=True,env=env).stdout
    assert stored==result
    gathered=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--gather'],input=boot+snapshots,
                            stdout=subprocess.PIPE,check=True,env=env).stdout
    assert gathered==result
    controlled=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--controller'],input=boot+snapshots,
                              stdout=subprocess.PIPE,check=True,env=env).stdout
    assert controlled==result
    if mode=='--gate':
        prepared=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--full-prepare'],input=boot+snapshots,
                                stdout=subprocess.PIPE,check=True,env=env).stdout
        assert prepared==result
        badproducer=bytearray(snapshots)
        badproducer[((2+8+8+16+64+64+256)+stageWords+(2+8+16+64))*4]^=1
        refused=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--full-prepare'],input=boot+badproducer,
                               stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
        assert refused.returncode and not refused.stdout and b'writes=0 matrix=0' in refused.stderr
    rejectedRecipe=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--bad-recipe'],input=boot+snapshots,
                                  stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,env=env)
    assert not rejectedRecipe.stdout and b'packets=0 writes=0' in rejectedRecipe.stderr
    badsource=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--bad-source'],input=boot+snapshots,
                             stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,env=env)
    assert not badsource.stdout
    badscatter=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--bad-scatter'],input=boot+snapshots,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True,env=env)
    assert not badscatter.stdout
    badproof=bytearray(snapshots);badproof[(2+8+8+16+64)*4]^=1
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--gather'],input=boot+badproof,
                            stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--controller'],input=boot+badproof,
                            stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout and b'writes=0' in rejected.stderr
    badjob=bytearray(snapshots);badjob[(2+8+8+16)*4]^=1
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--controller'],input=boot+badjob,
                            stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    nonfinite=bytearray(result);nonfinite[:4]=b'\x00\x00\xc0\x7f'
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode]),'--store'],input=boot+nonfinite,
                            stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badboot=bytearray(boot);badboot[0]^=1
    rejected=subprocess.run([str(rootbin),str(root),str(roles[mode])],input=badboot,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badproducer=bytearray(job);badproducer[256+28*4]=7
    rejected=subprocess.run(runargs,input=badproducer,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
    badjob=bytearray(job);badjob[chunks*512+34*4]=7
    rejected=subprocess.run(runargs,input=badjob,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=env)
    assert rejected.returncode and not rejected.stdout
for path,digest in pins:assert hashlib.sha256(path.read_bytes()).hexdigest()==digest
if full:assert hashlib.sha256(full.read_bytes()).hexdigest()==sys.argv[8]
if chain:assert hashlib.sha256(chain.read_bytes()).hexdigest()==sys.argv[11]
if chainOutput:assert hashlib.sha256(chainOutput.read_bytes()).hexdigest()==sys.argv[14]
print('PASS ROOT jobs -> 119 simulated workers -> actual ROOT packet seals, gather, full seal and scatter via simulated NoC; NO DEVICE')
print('PASS actual ROOT recipe -> dispatch -> gather -> scatter; BfPrepare stubbed; sequential snapshots, NO DEVICE')
if mode=='--gate':print('PASS ROOT actual gate preparation, producer receipts and matrix dispatch; corrupted producer refused before scatter; NO DEVICE')
if chain:print('PASS 119 retained gate -> QKV worker histories and actual QKV ROOT preparation; bad prior receipt refused before producer release; NO DEVICE')
if chainOutput:print('PASS retained gate -> QKV -> output worker histories and actual output ROOT preparation; synthetic output input; NO DEVICE')
print('PASS invalid boot/recipe, wrong-role jobs, corrupt source/receipt, blocked scatter and nonfinite result rejected; NO DEVICE')
