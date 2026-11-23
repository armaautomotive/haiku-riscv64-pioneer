#!/usr/bin/env python3
"""Exercise one-shot inspection and restoration on a disposable fake host."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

import pioneer_qwen35_recovery_qualify_script_test as common

CANDIDATE = 'ed99af51ccb5b6c63a6463aea98ff08588226a58afdec846985cf04efecc5844'
CLIENT = '0c09bf47bfe409134756691027a3271fda66cd97908b7eaecd2ccd24adf7817b'
SCRIPT = common.ROOT/'tools/pioneer/pioneer_qwen35_fleet_inspection_qualify.sh'


def run_case(case, updated=False):
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp)
        home, old_stage, installed, guard, _, env=common.fixture(root)
        suffix='m' if updated else 'l'
        candidate=CANDIDATE if not updated else 'fb891e48d4b32b8aeb83a2c5327a9bdc0cac7e9d2e9b2d51fe99da80f4905df6'
        client=CLIENT if not updated else '4e890c6ed33ebeeebb07b6259e78343b722b9de36579e3e8635dbcbdbefad3dd'
        tag='2026112300030004' if updated else '2026112300030003'
        script=SCRIPT if not updated else common.ROOT/'tools/pioneer/pioneer_qwen35_fleet_inspection_qualify_m.sh'
        stage=home/('pioneer-q35-fleet-inspect-20261123-'+suffix)
        old_stage.rename(stage)
        guard.write_text('new guard')
        admin=home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.write_text(admin.read_text().replace("'2026112300030002')", "'2026112300030002','2026112300030003','2026112300030004')"))
        common.put(stage/'client', '''#!/usr/bin/env python3
import json, os, sys
assert sys.argv[1:]==['--inspect-held-fleet']
p=os.environ['MOCK_STATE'];s=json.load(open(p));assert s['driver']==60
case=os.environ['MOCK_CASE']
if case=='retained':
 s['cold']=True;s['active']=True;json.dump(s,open(p,'w'));raise SystemExit(1)
if case=='invalid': print('invalid receipt',file=sys.stderr);raise SystemExit(0)
if case=='clean_failure':
 print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr);raise SystemExit(1)
print('P100A_Q35_FLEET_INSPECTION replied=1 status=0 stage=4 core=120 l2cpu=0x0000000f restore_failed=0 prepared=120',file=sys.stderr)
print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr)
''')
        state=Path(env['MOCK_STATE'])
        state.write_text(json.dumps(dict(driver=55,tag='2026102100010012',generation=2,
                                         quiesced=False,cold=False,active=False)))
        if case=='busy':
            common.put(root/'bin/ps', '#!/bin/sh\nprintf "Id Team\\n1 kernel_team\\n2 pioneer_q38_diagnostic\\n"\n')
        source=script.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
        source=source.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
        for old,path in ((common.OLD_DRIVER,installed),(common.NEW_GUARD,guard),(common.ADMIN,admin),
                         (candidate,stage/'candidate'),(client,stage/'client')):
            source=source.replace(old,hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper=root/'inspect.sh';common.put(wrapper,source)
        result=subprocess.run(['bash',str(wrapper),'2'],env=dict(env,MOCK_CASE=case),
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        lock=home/'.pioneer-q35-dataflow-exclusive'
        if case=='retained':
            assert result.returncode!=0 and installed.read_text()=='candidate' and lock.is_dir(), result.stderr
            assert (stage/'state').read_text().strip()=='QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
            assert subprocess.run(['bash',str(wrapper),'2'],env=dict(env,MOCK_CASE='success'),
                                  stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode!=0
            state.write_text(json.dumps(dict(driver=60,tag=tag,generation=1,
                                             quiesced=False,cold=False,active=False)))
            restore=common.RESTORE.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
            restore=restore.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
            for old,path in ((common.OLD_DRIVER,stage/'restore.incoming'),(common.NEW_GUARD,guard),
                             (common.ADMIN,admin),(candidate,stage/'candidate')):
                restore=restore.replace(old,hashlib.sha256(path.read_bytes()).hexdigest())
            recovery=root/'restore.sh';common.put(recovery,restore)
            restored=subprocess.run(['bash',str(recovery),str(stage)],env=env,
                                    stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
            assert restored.returncode==0,restored.stderr
            assert installed.read_text()=='stable' and not lock.exists()
        elif case=='busy':
            assert result.returncode!=0 and installed.read_text()=='stable' and not lock.exists()
            assert not (stage/'.qualification-once').exists()
        else:
            assert (result.returncode==0)==(case=='success'),(result.stdout,result.stderr)
            assert installed.read_text()=='stable' and not lock.exists(),result.stderr
            expected='FLEET_INSPECTION_PASSED_STABLE_RESTORED' if case=='success' else 'FLEET_INSPECTION_FAILED_STABLE_RESTORED'
            assert (stage/'state').read_text().strip()==expected


for updated in (False,True):
    for scenario in ('success','clean_failure','invalid','retained','busy'):
        run_case(scenario,updated)
print('PASS one-shot fleet inspection, clean restore, busy preflight and retained recovery; SIMULATED HOST ONLY')
