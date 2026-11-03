#!/usr/bin/env python3
"""Own c01+aaff/041 exchange endpoints, not actual Haiku/device permission."""
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert sys.argv[1:]==[]
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
# Execute the entire unchanged old control suite FIRST. Reuse only synthetic
# platform/admin endpoints, never its assertions, hash substitutions or script.
sys.argv=[str(here/'pioneer_q38_frontier_failure_link_exchange_test.py'),'--matrix-phase-endpoints']
old=runpy.run_path(sys.argv[0])
sys.argv=[str(pathlib.Path(__file__).resolve())]
production=(here.parent/'pioneer_q38_token_manual_ttsync_exchange.sh').read_text()
pins={
 'stable':'041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
 'candidate':'c01f1cda7549ef3b191454ab6c21d697de143f3f11fcc5c9da48c81dd37c1736',
 'client':'aaffcaa125a02d1629dc49055d45d26af28f3187049dda631a8b5a272a9c165a',
 'guard':'d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
 'admin':'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'}
candidate_name='tenstorrent_pci_probe-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY'
client_name='pioneer-q38-TOKEN-MANUAL-TTSYNC-34-DIAGNOSTIC-ONLY'
admin_source=old['admin_source'].replace('pioneer-q38-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY',client_name)
cases=['none','stable','candidate','client','guard','admin','installed','stage_mode','stale',
       'symlink','hardlink','writable','parent','quiesce','changed_during_quiesce','wait',
       'client_missing','client_symlink','client_hardlink','client_writable','client_changed_during_quiesce']
cases+=['field'+str(i) for i in range(9)]
cases+=['foreign_'+target+'_'+value for target in ('candidate','installed','client')
        for value in ('5ae9-module','db03-module','0925-private','92c1-object','e73d-user-object','559f-old-client')]
root=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-manual-ttsync-exchange-endpoints-'))
valid=refused=0
for mode in ('--apply','--restore'):
    for fault in cases:
        directory=root/(str(valid+refused));stage=directory/'stage';stage.mkdir(parents=True,mode=0o700)
        (directory/'bin').mkdir();(directory/'dev/misc').mkdir(parents=True);(directory/'old').mkdir()
        paths={'stage':stage,'installed':directory/'bin/tenstorrent_pci_probe',
               'device':directory/'dev/misc/tenstorrent_pci_probe','guard':directory/'guard',
               'admin':directory/'old/admin','stable':directory/'old/stable'}
        data={'stable':b'SYNTHETIC-stable041','candidate':b'SYNTHETIC-manual-c01',
              'client':b'SYNTHETIC-manual-aaff','guard':b'SYNTHETIC-guard','admin':admin_source.encode()}
        for key in ('stable','guard','admin'):
            paths[key].write_bytes(data[key]);paths[key].chmod(0o755)
        candidate=stage/candidate_name;client=stage/client_name
        candidate.write_bytes(data['candidate']);candidate.chmod(0o755)
        client.write_bytes(data['client']);client.chmod(0o755)
        paths['installed'].write_bytes(data['stable' if mode=='--apply' else 'candidate'])
        paths['installed'].chmod(0o755);paths['device'].symlink_to('../../bin/tenstorrent_pci_probe')
        transformed=production
        for key,path in paths.items():
            lines=[line for line in transformed.splitlines() if line.startswith(key+'=/')]
            assert len(lines)==1
            transformed=transformed.replace(lines[0],key+'='+str(path))
        for key,pin in pins.items():
            assert transformed.count(pin)==1
            transformed=transformed.replace(pin,hashlib.sha256(data[key]).hexdigest())
        script=directory/'exchange.sh';script.write_text(transformed)
        stub=directory/'stubs';stub.mkdir()
        for name in ('uname','id','stat','sha256sum','ps','sync'):
            path=stub/name;path.write_text(old['stub_source']);path.chmod(0o755)
        if fault in ('stable','guard','admin','installed'):paths[fault].write_bytes(b'CORRUPTED')
        if fault in ('candidate','client'):(candidate if fault=='candidate' else client).write_bytes(b'CORRUPTED')
        if fault.startswith('foreign_'):
            _,target,value=fault.split('_',2)
            (candidate if target=='candidate' else client if target=='client' else paths['installed']).write_bytes(value.encode())
        if fault=='stage_mode':stage.chmod(0o777)
        if fault=='stale':(stage/'driver.incoming').write_bytes(b'STALE')
        if fault=='symlink':candidate.unlink();candidate.symlink_to(paths['stable'])
        if fault=='hardlink':os.link(candidate,directory/'hardlink')
        if fault=='writable':candidate.chmod(0o777)
        if fault=='client_missing':client.unlink()
        if fault=='client_symlink':client.unlink();client.symlink_to(paths['stable'])
        if fault=='client_hardlink':os.link(client,directory/'client-hardlink')
        if fault=='client_writable':client.chmod(0o777)
        env={**os.environ,'PATH':str(stub)+':'+os.environ['PATH'],'FIXTURE_ROOT':str(directory),
             'FIXTURE_FAULT':fault,'FIXTURE_MODE':mode}
        before=paths['installed'].read_bytes()
        result=subprocess.run(['/bin/sh',str(script),mode],env=env,capture_output=True,text=True)
        (directory/'stdout.log').write_text(result.stdout);(directory/'stderr.log').write_text(result.stderr)
        (directory/'exit-status.txt').write_text(str(result.returncode)+'\n')
        calls=(directory/'calls').read_text().splitlines() if (directory/'calls').exists() else []
        if fault=='none':
            assert result.returncode==0,(mode,result.stdout,result.stderr)
            assert paths['installed'].read_bytes()==data['candidate' if mode=='--apply' else 'stable']
            assert len(calls)==3 and calls[1].startswith('--quiesce ') and calls[2].startswith('--wait ')
            assert (stage/'stable041.backup').read_bytes()==data['stable'];valid+=1
        else:
            assert result.returncode!=0,(mode,fault,result.stdout,result.stderr)
            if fault=='wait':
                assert paths['installed'].read_bytes()==data['candidate' if mode=='--apply' else 'stable']
                assert len(calls)==3 and 'NO_AUTOMATIC_ROLLBACK' in result.stdout
            elif fault=='changed_during_quiesce':assert paths['installed'].read_bytes()==b'changed' and len(calls)==2
            elif fault=='client_changed_during_quiesce':
                assert paths['installed'].read_bytes()==before and client.read_bytes()==b'changed' and len(calls)==2
            else:assert paths['installed'].read_bytes()==before
            if fault not in ('wait','quiesce','changed_during_quiesce','client_changed_during_quiesce'):assert len(calls)<=1
            refused+=1
        assert not (stage/'.exchange-lock').exists()
assert valid==2 and refused==94
print('PASS_MANUAL_TTSYNC_EXCHANGE_ENDPOINTS valid'+str(valid)+' refused'+str(refused)+
      ';ALL unchanged controls FIRST;NO actual driver/native/GPU/reset permission;raw fixtures '+str(root))
