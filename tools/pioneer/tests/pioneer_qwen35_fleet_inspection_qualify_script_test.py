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
                    'o':'e5f5a55347fe9438381626bf26b284827c9e6e524d8c30b32d7f19824d935818',
                    'p':'29721ab1e68fb6cc247319ae7fe07a160e008b746db5574af05dfbb5e74457f5',
                    'q':'5d5585c35d16e5fe5e19d6595ee7b1757d8a2225abb756beb5cb082536b83267',
                    'r':'043707b207d895f523d21178c890432f4e8dafbdb56c0684642001849c8cc7e5',
                    's':'2776db17e4ea8be9e1ddc96395904cc5e6a935eb0706abf800eae8a1bbd8e2fa',
                    't':'28f938ef92e680ee3d51092b379af828925cdaa939538ee511597c8a452d13e1',
                    'u':'eb035341f505dc5c406e0e45946fd87b379b857ba58b5ba6fb3b2b1e503fb08a',
                    'v':'a5c45df13f5a4ba8c352167b99f308a4b81f03930a930e325415a4cc3af41e22',
                    'w':'7fee6e7322394343c54acb628b1ab5d3fcff938cffb873d8e9979a76821f36f2'}
        clients={'l':CLIENT,'m':'4e890c6ed33ebeeebb07b6259e78343b722b9de36579e3e8635dbcbdbefad3dd',
                 'n':'ef76ebfa01d675dd43449f9aa8b7b128477f8962a3ecd093975af1c666d85e1d',
                 'o':'a914af54a1cac088aeed7558f422c425fd6536600dcf96f047ae4adc1b8cf216',
                 'p':'040f1511e3439f372a994d40e2253d4dd03b0b9e9084a1ceb6ccc69590c0ac9c',
                 'q':'152811c4f454fda4d8094a3a54e04e6d620c10eed8ba8d61465f6c63ce6616c5',
                 'r':'3000c7d74eafd055b1257e7d4499cbcf4ba503ac468518a5390259e56255dd37',
                 's':'33083362025c9e7f6ea407a962ebbd6b37fad50ffda03abf22aa4dfcccc6385d',
                 't':'67b0e73d0d2bb4a86ff8537179ef598e87e741d8038e70e4a60fe4fc2d547c02',
                 'u':'30b493d051eda2cb72a3a80ad937c2d4ad02109c8e1e8a2dbedfdc844e617944',
                 'v':'b74d14e2dc9b4ee0a8ee2b082bac5c7632782f76557c27b845cd87d86d7f32c2',
                 'w':'1a6cf9547a3087f74171584702ddf6d49267ce847bcea46a1e3c849464e525dd'}
        tags={'l':'2026112300030003','m':'2026112300030004','n':'2026112300030005','o':'2026112300030006','p':'2026112300030007','q':'2026112300030008','r':'2026112300030009','s':'2026112300030010','t':'2026112300030011','u':'2026112300030012','v':'2026112300030013','w':'2026112300030014'}
        scripts={'l':SCRIPT,'m':common.ROOT/'tools/pioneer/pioneer_qwen35_fleet_inspection_qualify_m.sh',
                 'n':common.ROOT/'tools/pioneer/pioneer_qwen35_role_stage_qualify.sh',
                 'o':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify.sh',
                 'p':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify_p.sh',
                 'q':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify_q.sh',
                 'r':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify_r.sh',
                 's':common.ROOT/'tools/pioneer/pioneer_qwen35_role_prestack_qualify_s.sh',
                 't':common.ROOT/'tools/pioneer/pioneer_qwen35_local_restore_qualify_t.sh',
                 'u':common.ROOT/'tools/pioneer/pioneer_qwen35_role_service_qualify_u.sh',
                 'v':common.ROOT/'tools/pioneer/pioneer_qwen35_role_fault_qualify_v.sh',
                 'w':common.ROOT/'tools/pioneer/pioneer_qwen35_span_qualify_w.sh'}
        candidate,client,tag,script=candidates[variant],clients[variant],tags[variant],scripts[variant]
        stage=home/(('pioneer-q35-supported-span-20261123-' if variant=='w'
                     else 'pioneer-q35-role-fault-20261123-' if variant=='v'
                     else 'pioneer-q35-role-service-20261123-' if variant=='u'
                     else 'pioneer-q35-local-restore-20261123-' if variant=='t'
                     else 'pioneer-q35-prestack-20261123-' if variant in ('o','p','q','r','s')
                     else 'pioneer-q35-role-stage-20261123-' if variant=='n'
                     else 'pioneer-q35-fleet-inspect-20261123-')+variant)
        old_stage.rename(stage)
        guard.write_text('new guard')
        admin=home/'pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload'
        admin.write_text(admin.read_text().replace("'2026112300030002')", "'2026112300030002','2026112300030003','2026112300030004','2026112300030005','2026112300030006','2026112300030007','2026112300030008','2026112300030009','2026112300030010','2026112300030011','2026112300030012','2026112300030013')"))
        admin.write_text(admin.read_text().replace("'2026112300030013')", "'2026112300030013','2026112300030014')"))
        if variant in ('t','u','v','w'):
            # The production wrapper hashes both pinned models. Keep the fake
            # sparse fixtures cheap, while still testing a mismatched hash.
            common.put(root/'bin/sha256sum', '''#!/usr/bin/env python3
import hashlib,os,sys
p=sys.argv[1]
if p.endswith('/Qwen3.5-4B-Q4_K_M.gguf'):
 h='00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4'
elif p.endswith('/Qwen3.8-27B-Q4_K_M.gguf'):
 h='bad' if os.environ.get('MOCK_BAD_MODEL')=='1' else 'c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747'
else:
 h=hashlib.sha256(open(p,'rb').read()).hexdigest()
print(h,p)
''')
        common.put(stage/'client', '''#!/usr/bin/env python3
import json, os, sys
role=os.environ['MOCK_ROLE']
assert sys.argv[1:]==(['--qualify-supported-span'] if role=='6' else ['--qualify-role-service-fault'] if role=='5' else ['--qualify-role-service'] if role=='4' else ['--qualify-role-local-restore'] if role=='3' else ['--qualify-role-prestack'] if role=='2' else ['--qualify-held-role-stage'] if role=='1' else ['--inspect-held-fleet'])
p=os.environ['MOCK_STATE'];s=json.load(open(p));assert s['driver']==60
case=os.environ['MOCK_CASE']
if case=='retained':
 s['cold']=True;s['active']=True;json.dump(s,open(p,'w'));raise SystemExit(1)
if case=='invalid': print('invalid receipt',file=sys.stderr);raise SystemExit(0)
if case=='clean_failure':
 print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr);raise SystemExit(1)
if role=='6':
 print('P100A_Q35_SUPPORTED_SPAN banks=7 pages=7 host_exact=1 clean_end=1;NO_INFERENCE');raise SystemExit(0)
print('P100A_Q35_FLEET_INSPECTION replied=1 status=0 stage=4 core=120 l2cpu=0x0000000f restore_failed=0 prepared=120',file=sys.stderr)
if role!='0': print('P100A_Q35_ROLE_STAGE replied=1 status=0 stage='+('12' if role=='5' else '11' if role=='4' else '10' if role=='3' else '9' if role=='2' else '8')+' captured=1 staged=1 restored=1 mappings_closed=1',file=sys.stderr)
print('P100A_Q35_FLEET_INSPECTION clean_end=1',file=sys.stderr)
''')
        state=Path(env['MOCK_STATE'])
        state.write_text(json.dumps(dict(driver=55,tag='2026102100010012',generation=2,
                                         quiesced=False,cold=False,active=False)))
        if case in ('busy','busy_27b'):
            name='pioneer_assistant_qwen27_bf16_full_offload' if case=='busy_27b' else 'pioneer_q38_diagnostic'
            common.put(root/'bin/ps', f'#!/bin/sh\nprintf "Id Team\\n1 kernel_team\\n2 {name}\\n"\n')
        source=script.read_text().replace('/boot/home',str(home)).replace('0:1:600:',f'{os.getuid()}:1:600:')
        source=source.replace('= 0:1 ]',f'= {os.getuid()}:1 ]').replace('= 0:700 ]',f'= {os.getuid()}:700 ]')
        for old,path in ((common.OLD_DRIVER,installed),(common.NEW_GUARD,guard),(common.ADMIN,admin),
                         (candidate,stage/'candidate'),(client,stage/'client')):
            source=source.replace(old,hashlib.sha256(path.read_bytes()).hexdigest())
        wrapper=root/'inspect.sh';common.put(wrapper,source)
        run_env=dict(env,MOCK_CASE=case,MOCK_BAD_MODEL='1' if case=='bad_model' else '0',
                     MOCK_ROLE='6' if variant=='w' else '5' if variant=='v' else '4' if variant=='u' else '3' if variant=='t' else '2' if variant in ('o','p','q','r','s') else '1' if variant=='n' else '0')
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
        elif case in ('busy','busy_27b','bad_model'):
            assert result.returncode!=0 and installed.read_text()=='stable' and not lock.exists()
            assert not (stage/'.qualification-once').exists()
        else:
            assert (result.returncode==0)==(case=='success'),(result.stdout,result.stderr)
            assert installed.read_text()=='stable' and not lock.exists(),result.stderr
            prefix='SUPPORTED_SPAN' if variant=='w' else 'ROLE_SERVICE_FAULT' if variant=='v' else 'ROLE_SERVICE' if variant=='u' else 'ROLE_LOCAL_RESTORE' if variant=='t' else 'ROLE_PRESTACK' if variant in ('o','p','q','r','s') else 'ROLE_STAGE' if variant=='n' else 'FLEET_INSPECTION'
            expected=prefix+('_PASSED_STABLE_RESTORED' if case=='success' else '_FAILED_STABLE_RESTORED')
            assert (stage/'state').read_text().strip()==expected


for variant in ('l','m','n','o','p','q','r','s','t','u','v','w'):
    for scenario in ('success','clean_failure','invalid','retained','busy'):
        run_case(scenario,variant)
for scenario in ('bad_model','busy_27b'):
    for variant in ('t','u','v','w'):
        run_case(scenario,variant)
print('PASS one-shot fleet inspection, clean restore, busy preflight and retained recovery; SIMULATED HOST ONLY')
