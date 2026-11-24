#!/usr/bin/env python3
"""Exercise the fixed reset-word observation wrapper on a disposable fake host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import pioneer_qwen35_recovery_qualify_script_test as common

TAG='2026112400030023'
CANDIDATE='ef3ec14e432cd71f0c58b5c30ca4c56c4fc67fd32dc74b41ac0e37973918b893'
CLIENT='930ad7bc907bda364c5edb5f6520e649f947bb09ebb9f280303fbe074d4af918'
SCRIPT=common.ROOT/'tools/pioneer/pioneer_qwen35_risc_sample_qualify_af.sh'

def exercise(case):
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp)
        home,old_stage,installed,guard,_,env=common.fixture(root)
        stage=home/'pioneer-q35-risc-sample-20261124-af';old_stage.rename(stage)
        guard.write_text('new guard')
        admin=home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.write_text(admin.read_text().replace("'2026112300030002')", "'2026112300030002','2026112400030023')"))
        common.put(root/'bin/sha256sum', '''#!/usr/bin/env python3
import hashlib,os,sys
p=sys.argv[1]
if p.endswith('/Qwen3.5-4B-Q4_K_M.gguf'): h='00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4'
elif p.endswith('/Qwen3.8-27B-Q4_K_M.gguf'): h='bad' if os.environ.get('MOCK_CASE')=='bad_model' else 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'
else: h=hashlib.sha256(open(p,'rb').read()).hexdigest()
print(h,p)
''')
        if case=='busy_27b':
            common.put(root/'bin/ps', '#!/bin/sh\nprintf "Id Team\\n1 kernel_team\\n2 pioneer_assistant_qwen27_bf16_full_offload\\n"\n')
        common.put(stage/'client', '''#!/usr/bin/env python3
import json,os,sys
assert sys.argv[1:]==['--qualify-risc-reset-sample']
p=os.environ['MOCK_STATE'];s=json.load(open(p));assert s['driver']==60
case=os.environ['MOCK_CASE']
if case in ('retained','retained_running','retained_swap'):
 s['cold']=True;s['active']=True;json.dump(s,open(p,'w'));raise SystemExit(1)
print('P100A_Q35_FLEET_INSPECTION replied=1 status=0 stage=4 core=120 l2cpu=0x0000000f restore_failed=0 prepared=120',file=sys.stderr)
if case!='invalid': print('P100A_Q35_RISC_RESET_SAMPLE replied=1 status=0 sampled=1 restore_failed=0 words=ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff,ffffffff',file=sys.stderr)
print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr)
''')
        state=Path(env['MOCK_STATE']);state.write_text(json.dumps(dict(driver=55,tag='2026102100010012',generation=2,quiesced=False,cold=False,active=False)))
        source=SCRIPT.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
        source=source.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
        for old,path in ((common.OLD_DRIVER,installed),(common.NEW_GUARD,guard),(common.ADMIN,admin),
                         (CANDIDATE,stage/'candidate'),(CLIENT,stage/'client')):
            assert old in source
            source=source.replace(old,hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper=root/'run.sh';common.put(wrapper,source)
        result=subprocess.run(['bash',str(wrapper),'2'],env=dict(env,MOCK_CASE=case),stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        lock=home/'.pioneer-q35-dataflow-exclusive'
        if case in ('bad_model','busy_27b'):
            assert result.returncode and installed.read_text()=='stable' and not lock.exists()
            assert not (stage/'.qualification-once').exists();return
        if case in ('retained','retained_running','retained_swap'):
            assert result.returncode and installed.read_text()=='candidate' and lock.is_dir()
            assert (stage/'state').read_text().strip()=='QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
            if case=='retained_running': (stage/'state').write_text('QUALIFICATION_RUNNING\n')
            if case=='retained_swap': (stage/'state').write_text('RESTORE_REQUIRED\n')
            restore=common.RESTORE.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
            restore=restore.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
            for old,path in ((common.OLD_DRIVER,stage/'restore.incoming'),(common.NEW_GUARD,guard),(common.ADMIN,admin),(CANDIDATE,stage/'candidate')):
                restore=restore.replace(old,hashlib.sha256(path.read_bytes()).hexdigest())
            recovery=root/'restore.sh';common.put(recovery,restore)
            premature=subprocess.run(['bash',str(recovery),str(stage)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            assert premature.returncode and installed.read_text()=='candidate' and lock.is_dir()
            state.write_text(json.dumps(dict(driver=60,tag=TAG,generation=1,quiesced=False,cold=False,active=False)))
            result=subprocess.run(['bash',str(recovery),str(stage)],env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            assert result.returncode==0,(result.stdout,result.stderr)
            assert installed.read_text()=='stable' and not lock.exists();return
        assert (result.returncode==0)==(case=='success'),(result.stdout,result.stderr)
        assert installed.read_text()=='stable' and not lock.exists()
        assert (stage/'state').read_text().strip()==('RISC_RESET_SAMPLE_PASSED_STABLE_RESTORED' if case=='success' else 'RISC_RESET_SAMPLE_FAILED_STABLE_RESTORED')
        assert subprocess.run(['bash',str(wrapper),'4'],env=dict(env,MOCK_CASE='success'),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode

for case in ('success','invalid','bad_model','busy_27b','retained','retained_running','retained_swap'):
    exercise(case)
print('PASS read-only reset-word trial gates, retained fault and post-boot restore; SIMULATED HOST ONLY')
