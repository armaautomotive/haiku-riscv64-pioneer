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


def run_case(case, variant='l'):
    with tempfile.TemporaryDirectory() as temp:
        root=Path(temp)
        home, old_stage, installed, guard, _, env=common.fixture(root)
        candidates={'l':CANDIDATE,'m':'fb891e48d4b32b8aeb83a2c5327a9bdc0cac7e9d2e9b2d51fe99da80f4905df6',
                    'n':'2940a8c1f306067fc3170da96f3a0ec02da9320286b980caf9fbf8d122f46b2e',
                    'o':'e5f5a55347fe9438381626bf26b284827c9e6e524d8c30b32d7f19824d935818'}
        clients={'l':CLIENT,'m':'4e890c6ed33ebeeebb07b6259e78343b722b9de36579e3e8635dbcbdbefad3dd',
                 'n':'ef76ebfa01d675dd43449f9aa8b7b128477f8962a3ecd093975af1c666d85e1d',
                 'o':'a914af54a1cac088aeed7558f422c425fd6536600dcf96f047ae4adc1b8cf216'}
        tags={'l':'2026112300030003','m':'2026112300030004','n':'2026112300030005','o':'2026112300030006'}
        scripts={'l':SCRIPT,'m':common.ROOT/'tools/pioneer/pioneer_qwen35_fleet_inspection_qualify_m.sh',
                 'n':common.ROOT/'tools/pioneer/pioneer_qwen35_role_stage_qualify.sh',
                 'o':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify.sh'}
        candidate,client,tag,script=candidates[variant],clients[variant],tags[variant],scripts[variant]
        stage=home/(('pioneer-q35-prestack-20261123-' if variant=='o'
                     else 'pioneer-q35-role-stage-20261123-' if variant=='n'
                     else 'pioneer-q35-fleet-inspect-20261123-')+variant)
        old_stage.rename(stage)
        guard.write_text('new guard')
        admin=home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.write_text(admin.read_text().replace("'2026112300030002')", "'2026112300030002','2026112300030003','2026112300030004','2026112300030005','2026112300030006')"))
        common.put(stage/'client', '''#!/usr/bin/env python3
import json, os, sys
role=os.environ['MOCK_ROLE']
assert sys.argv[1:]==(['--qualify-role-prestack'] if role=='2' else ['--qualify-held-role-stage'] if role=='1' else ['--inspect-held-fleet'])
p=os.environ['MOCK_STATE'];s=json.load(open(p));assert s['driver']==60
case=os.environ['MOCK_CASE']
if case=='retained':
 s['cold']=True;s['active']=True;json.dump(s,open(p,'w'));raise SystemExit(1)
if case=='invalid': print('invalid receipt',file=sys.stderr);raise SystemExit(0)
if case=='clean_failure':
 print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr);raise SystemExit(1)
print('P100A_Q35_FLEET_INSPECTION replied=1 status=0 stage=4 core=120 l2cpu=0x0000000f restore_failed=0 prepared=120',file=sys.stderr)
if role!='0': print('P100A_Q35_ROLE_STAGE replied=1 status=0 stage='+('9' if role=='2' else '8')+' captured=1 staged=1 restored=1 mappings_closed=1',file=sys.stderr)
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
        run_env=dict(env,MOCK_CASE=case,MOCK_ROLE='2' if variant=='o' else '1' if variant=='n' else '0')
        result=subprocess.run(['bash',str(wrapper),'2'],env=run_env,
                              stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        lock=home/'.pioneer-q35-dataflow-exclusive'
        if case=='retained':
            assert result.returncode!=0 and installed.read_text()=='candidate' and lock.is_dir(), result.stderr
            assert (stage/'state').read_text().strip()=='QUALIFICATION_FAILED_RETAIN_FOR_INSPECTION'
            assert subprocess.run(['bash',str(wrapper),'2'],env=dict(run_env,MOCK_CASE='success'),
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
            prefix='ROLE_PRESTACK' if variant=='o' else 'ROLE_STAGE' if variant=='n' else 'FLEET_INSPECTION'
            expected=prefix+('_PASSED_STABLE_RESTORED' if case=='success' else '_FAILED_STABLE_RESTORED')
            assert (stage/'state').read_text().strip()==expected


for variant in ('l','m','n','o'):
    for scenario in ('success','clean_failure','invalid','retained','busy'):
        run_case(scenario,variant)
print('PASS one-shot fleet inspection, clean restore, busy preflight and retained recovery; SIMULATED HOST ONLY')
