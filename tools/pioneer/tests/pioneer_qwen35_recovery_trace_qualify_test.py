#!/usr/bin/env python3
"""Exercise the fixed-tag trace trial and restore on a disposable fake host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

import pioneer_qwen35_recovery_qualify_script_test as common

TAG = '2026112400030022'
CANDIDATE = '19d22e32d4665ffc1dd9ac367284e09daa7505e3b3b12e073c74f3437e5c6d45'
CLIENT = '11a6b6406ab4368336455d9390d03a6913002c037bb077122367f748c5f7604e'
MODEL4 = '00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4'
MODEL27 = 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'


def exercise(outcome):
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        home, old_stage, installed, guard, _, env = common.fixture(root)
        stage = old_stage.with_name(old_stage.name + '-trace')
        old_stage.rename(stage)
        (stage/'guard.previous').write_text('old guard')
        (stage/'guard.previous').chmod(0o755)
        guard.write_text('new guard')
        admin = home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.write_text(admin.read_text().replace("'2026112300030002')", "'2026112300030002','"+TAG+"')"))
        # Avoid reading the deliberately sparse 22 GB fake models. Actual
        # script still calls sha256sum; one test returns a corrupt 27B hash.
        common.put(root/'bin/sha256sum', '''#!/usr/bin/env python3
import hashlib, os, sys
p=sys.argv[1]
if p.endswith('Qwen3.5-4B-Q4_K_M.gguf'): h=os.environ['MOCK_MODEL4_HASH']
elif p.endswith('Qwen3.8-27B-Q4_K_M.gguf'): h=os.environ['MOCK_MODEL27_HASH']
else: h=hashlib.sha256(open(p,'rb').read()).hexdigest()
print(h,p)
''')
        env.update(MOCK_MODEL4_HASH=MODEL4, MOCK_MODEL27_HASH='0'*64 if outcome=='hash' else MODEL27,
                   MOCK_AER_FORMAT='1')
        state = Path(env['MOCK_STATE'])
        state.write_text(json.dumps(dict(driver=55, tag='2026102100010012', generation=2,
                                         quiesced=False, cold=False, active=False)))
        source = (common.ROOT/'tools/pioneer/pioneer_qwen35_recovery_qualify_firmware_trace.sh').read_text()
        source = source.replace('/boot/home', str(home)).replace('0:1:600:', f'{os.getuid()}:1:600:')
        source = source.replace('= 0:1 ]', f'= {os.getuid()}:1 ]').replace('= 0:700 ]', f'= {os.getuid()}:700 ]')
        for expected, path in ((common.OLD_DRIVER, installed), (common.OLD_GUARD, stage/'guard.previous'),
                               (common.NEW_GUARD, guard), (common.ADMIN, admin),
                               (CANDIDATE, stage/'candidate'), (CLIENT, stage/'client')):
            assert expected in source
            source=source.replace(expected,hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper=root/'trace.sh';common.put(wrapper,source)
        def run(mode, **extra):
            return subprocess.run(['bash',str(wrapper),mode,'2'],env=dict(env,**extra),
                                  stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        prepared=run('--prepare-existing-guard')
        if outcome=='hash':
            assert prepared.returncode and installed.read_text()=='stable'
            assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
            return
        assert prepared.returncode==0,(prepared.stdout,prepared.stderr)
        result=run('--run',**({'MOCK_FAIL':'1'} if outcome in ('failure','interrupted') else
                              {'MOCK_REJECT':'1'} if outcome=='reject' else {}))
        if outcome=='success':
            assert result.returncode==0,(result.stdout,result.stderr)
            assert installed.read_text()=='stable' and not (home/'.pioneer-q35-dataflow-exclusive').exists()
            return
        if outcome=='reject':
            assert result.returncode and installed.read_text()=='stable'
            assert (stage/'state').read_text().strip()=='PRE_RESET_PCI_REJECTED_STABLE_RESTORED'
            assert not (home/'.pioneer-q35-dataflow-exclusive').exists()
            return
        assert result.returncode and installed.read_text()=='candidate'
        assert (stage/'state').read_text().strip()=='QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
        assert (home/'.pioneer-q35-dataflow-exclusive').is_dir()
        if outcome=='interrupted':
            (stage/'state').write_text('RESTORE_REQUIRED\n')
        state.write_text(json.dumps(dict(driver=60,tag=TAG,generation=1,quiesced=False,cold=False,active=False)))
        restore=common.RESTORE.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
        restore=restore.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
        for expected,path in ((common.OLD_DRIVER,stage/'restore.incoming'),(common.NEW_GUARD,guard),
                              (common.ADMIN,admin),(CANDIDATE,stage/'candidate')):
            restore=restore.replace(expected,hashlib.sha256(path.read_bytes()).hexdigest())
        restore_path=root/'restore.sh';common.put(restore_path,restore)
        result=subprocess.run(['bash',str(restore_path),str(stage)],env=env,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        assert result.returncode==0,(result.stdout,result.stderr)
        assert installed.read_text()=='stable' and not (home/'.pioneer-q35-dataflow-exclusive').exists()


for outcome in ('success','reject','failure','interrupted','hash'):
    exercise(outcome)
print('PASS fixed recovery trace trial, model hash gate and post-boot restore; SIMULATED HOST ONLY')
