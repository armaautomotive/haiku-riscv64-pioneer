#!/usr/bin/env python3
"""Exercise the actual exchange script in private synthetic FS/API endpoints.

Only exact absolute paths and hash pins are rebound. No actual Pioneer,
installed driver, native kernel proof, network, reset or inference call.
"""
import hashlib
import os
import pathlib
import subprocess
import tempfile
import sys

here=pathlib.Path(__file__).resolve().parent
assert sys.argv[1:] in ([],['--qualified-original-endpoints'],['--batched-output-closure-endpoints'],['--wide-gdn-endpoints'],['--token-phase-endpoints'],['--constant-q4-endpoints'],['--matrix-phase-endpoints']) and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
matrix_phase=sys.argv[1:]==['--matrix-phase-endpoints']
constant_q4=sys.argv[1:]==['--constant-q4-endpoints']
token_phase=sys.argv[1:]==['--token-phase-endpoints']
output_closure=sys.argv[1:]==['--batched-output-closure-endpoints']
wide_gdn=sys.argv[1:]==['--wide-gdn-endpoints']
if matrix_phase:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),'--constant-q4-endpoints'],check=True)
elif constant_q4:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),'--token-phase-endpoints'],check=True)
elif token_phase:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),'--wide-gdn-endpoints'],check=True)
elif wide_gdn:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),'--batched-output-closure-endpoints'],check=True)
elif output_closure:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve())],check=True)
else:
    subprocess.run([sys.executable,str(here/'pioneer_q38_frontier_failure_detail_exchange_test.py')],check=True)
production=(here.parent/'pioneer_q38_frontier_failure_link_exchange.sh').read_text()
original=(here/'fixtures/pioneer_q38_frontier_failure_link_exchange_qualified_INERT.txt').read_bytes()
assert hashlib.sha256(original).hexdigest()=='e37c189e55c63b9a358aad4732d5e9122bf6daa29375ea894315daaf754056a8'
retired=not sys.argv[1:]
if retired:
    subprocess.run([sys.executable,str(pathlib.Path(__file__).resolve()),'--qualified-original-endpoints'],check=True)
elif matrix_phase:
    production=(here.parent/'pioneer_q38_token_matrix_phase_exchange.sh').read_text()
elif constant_q4:
    production=(here.parent/'pioneer_q38_token_constant_q4_exchange.sh').read_text()
elif token_phase:
    production=(here.parent/'pioneer_q38_token_phase_profile_exchange.sh').read_text()
elif wide_gdn:
    production=(here.parent/'pioneer_q38_wide_gdn_exchange.sh').read_text()
elif output_closure:
    production=(here.parent/'pioneer_q38_batched_output_closure_exchange.sh').read_text()
else:
    production=original.decode()
root=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-frontier-detail-exchange-endpoints-'))
print('fixture root',root,flush=True)
pins={
 'stable':'041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
 'candidate':'96a030daf8136143aafa1f518b2ff3323178b5aa7b530c7d9395db5bf447e3e2',
 'guard':'d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
 'admin':'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'}
if output_closure:pins['candidate']='7c85898c5591115521e5f38c9ee546da7909a1da82b9a025046c5f8c81d1b623'
if wide_gdn:pins['candidate']='b8c0d49855490e756dfe1c49bed7933a20f2a3afea19436b37e3b4717417b8ed'
if token_phase:pins['candidate']='0f2066ca48b24ae36550e57dd5334aa29645deb7f65dc8fd7bbbb9629f674e86'
if constant_q4:pins['candidate']='db032591d905a30debf1e9a36f5990301e3db9ba40bfa8cf945677e3f54e8106'
if matrix_phase:
    pins['candidate']='5ae9d64ae4757c4635e4bbc6f59cf8350e8f549427e99e1e99e5897092616e39'
    pins['client']='559f986ddc88bbc2ff8cbe034dcda1546d385b366177f027a245452e605c7a22'
candidate_name='tenstorrent_pci_probe-TOKEN-PHASE34-DIAGNOSTIC-ONLY' if token_phase else 'tenstorrent_pci_probe-WIDE-GDN34-DIAGNOSTIC-ONLY' if wide_gdn else 'tenstorrent_pci_probe-BATCHED-OUTPUT-CLOSURE34-DIAGNOSTIC-ONLY' if output_closure else 'tenstorrent_pci_probe-FRONTIER-PREERROR-LINK-DIAGNOSTIC-ONLY'
if constant_q4:candidate_name='tenstorrent_pci_probe-TOKEN-CONSTANT-Q4-34-DIAGNOSTIC-ONLY'
if matrix_phase:candidate_name='tenstorrent_pci_probe-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY'
admin_source='''#!/usr/bin/env python3
import os,pathlib,sys
p=pathlib.Path(os.environ['FIXTURE_ROOT']);fault=os.environ['FIXTURE_FAULT'];args=sys.argv[1:]
with (p/'calls').open('a') as f:f.write(' '.join(args)+'\\n')
if args==['--status']:
 tag='2026102100010012' if os.environ['FIXTURE_MODE']=='--apply' else '2026103100010075'
 fields=['P100A_RELOAD','driver55','tag'+tag,'generation7','quiesced0','cold0','active_owner0','open_count1','guard1;CPU_METADATA_ONLY']
 if fault.startswith('field'):
  i=int(fault[5:]);fields[i]=['BAD','driver54','tag0','generation0','quiesced1','cold1','active_owner1','open_count2','guard0;'][i]
 print(' '.join(fields));sys.exit(0)
if args[0]=='--quiesce':
 assert args[1]==('2026102100010012' if os.environ['FIXTURE_MODE']=='--apply' else '2026103100010075')
 if fault=='quiesce':sys.exit(1)
 if fault=='changed_during_quiesce':(p/'bin/tenstorrent_pci_probe').write_bytes(b'changed')
 if fault=='client_changed_during_quiesce':(p/'stage/pioneer-q38-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY').write_bytes(b'changed')
 sys.exit(0)
if args[0]=='--wait':
 assert args[1]==('2026103100010075' if os.environ['FIXTURE_MODE']=='--apply' else '2026102100010012')
 assert args[2]=='0000000000000007'
 sys.exit(1 if fault=='wait' else 0)
raise AssertionError(args)
'''
stub_source='''#!/usr/bin/env python3
import hashlib,os,pathlib,stat,sys
name=pathlib.Path(sys.argv[0]).name;args=sys.argv[1:];root=pathlib.Path(os.environ['FIXTURE_ROOT'])
with (root/'platform-calls').open('a') as f:f.write(name+'\\n')
if name=='uname':print('Haiku');sys.exit(0)
if name=='id':print('0');sys.exit(0)
if name=='stat':
 p=pathlib.Path(args[2]);s=p.stat();mode=format(stat.S_IMODE(s.st_mode),'o')
 print('0:'+mode if args[1]=='%u:%a' else '0:'+str(s.st_nlink)+':'+mode);sys.exit(0)
if name=='sha256sum':
 p=pathlib.Path(args[0]);print(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+str(p));sys.exit(0)
if name=='ps':
 print('   Id Team');print('1 kernel_team')
 if os.environ['FIXTURE_FAULT']=='parent':print('999 /boot/home/pioneer_q38_authenticate')
 sys.exit(0)
if name=='sync':sys.exit(0)
raise AssertionError((name,args))
'''
cases=[('none','--apply'),('none','--restore')]
cases += [(name,'--apply') for name in ('stable','candidate','guard','admin','installed','stage_mode','stale',
 'symlink','hardlink','writable','parent','quiesce','changed_during_quiesce','wait',
 'retired010c_candidate','retired010c_installed','retired34322_candidate','retired34322_installed',
 'private22a_candidate','private22a_installed',
 'retired5920_candidate','retired5920_installed','private3593_candidate','private3593_installed')]
cases += [('field'+str(i),'--apply') for i in range(9)]
if output_closure or wide_gdn or token_phase or constant_q4 or matrix_phase:
    cases += [(fault,'--apply') for fault in ('retired96a0_candidate','retired96a0_installed')]
if wide_gdn or token_phase or constant_q4 or matrix_phase:
    cases += [(fault,'--apply') for fault in ('old7c85_candidate','old7c85_installed','privatede2f_candidate','privatede2f_installed')]
if token_phase or constant_q4 or matrix_phase:
    cases += [(fault,'--apply') for fault in ('oldb8c0_candidate','oldb8c0_installed','private2076_candidate','private2076_installed')]
if constant_q4 or matrix_phase:
    cases += [(fault,'--apply') for fault in ('old0f20_candidate','old0f20_installed',
        'private5e2c_candidate','private5e2c_installed','object657a_candidate','object657a_installed',
        'user3edc_candidate','user3edc_installed')]
if matrix_phase:
    cases += [(fault,'--apply') for fault in ('olddb03_candidate','olddb03_installed',
        'private1a04_candidate','private1a04_installed','objecta498_candidate','objecta498_installed',
        'user559f_candidate','user559f_installed','client','client_missing','client_symlink',
        'client_hardlink','client_writable','client_changed_during_quiesce')]
cases += [(fault,'--restore') for fault,mode in cases if fault!='none']
passed=0
for index,(fault,mode) in enumerate(cases):
    directory=root/str(index);stage=directory/'stage';stage.mkdir(parents=True,mode=0o700)
    old=directory/'old';old.mkdir();(directory/'bin').mkdir();(directory/'dev/misc').mkdir(parents=True)
    paths={'stage':stage,'installed':directory/'bin/tenstorrent_pci_probe',
           'device':directory/'dev/misc/tenstorrent_pci_probe','guard':directory/'guard',
           'admin':old/'admin','stable':old/'stable'}
    data={'stable':b'synthetic-stable041','candidate':b'synthetic-phase0f20' if token_phase else b'synthetic-wideb8c0' if wide_gdn else b'synthetic-corrected7c85' if output_closure else b'synthetic-link96a0','guard':b'synthetic-guard',
          'admin':admin_source.encode()}
    if constant_q4:data['candidate']=b'synthetic-constant-q4-db03'
    if matrix_phase:
        data['candidate']=b'synthetic-matrix-phase-5ae9';data['client']=b'synthetic-matrix-client-559f'
    for key in ('stable','guard','admin'):
        paths[key].write_bytes(data[key]);paths[key].chmod(0o755)
    candidate=stage/candidate_name;candidate.write_bytes(data['candidate']);candidate.chmod(0o755)
    if matrix_phase:
        client=stage/'pioneer-q38-TOKEN-MATRIX-PHASE-34-DIAGNOSTIC-ONLY'
        client.write_bytes(data['client']);client.chmod(0o755)
    original=data['stable' if mode=='--apply' else 'candidate']
    paths['installed'].write_bytes(original);paths['installed'].chmod(0o755)
    paths['device'].symlink_to('../../bin/tenstorrent_pci_probe')
    transformed=production
    for key,path in paths.items():
        lines=[line for line in transformed.splitlines() if line.startswith(key+'=/')];assert len(lines)==1,(key,lines)
        transformed=transformed.replace(lines[0],key+'='+str(path))
    for key,pin in pins.items():
        assert transformed.count(pin)==1;transformed=transformed.replace(pin,hashlib.sha256(data[key]).hexdigest())
    script=directory/'exchange.sh';script.write_text(transformed)
    stub=directory/'stubs';stub.mkdir()
    for name in ('uname','id','stat','sha256sum','ps','sync'):
        file=stub/name;file.write_text(stub_source);file.chmod(0o755)
    if fault in ('stable','guard','admin','installed'):paths[fault].write_bytes(b'corrupted')
    if fault=='candidate':candidate.write_bytes(b'corrupted')
    if fault.startswith(('retired010c_','retired34322_','private22a_','retired5920_','private3593_','retired96a0_','old7c85_','privatede2f_','oldb8c0_','private2076_','old0f20_','private5e2c_','object657a_','user3edc_','olddb03_','private1a04_','objecta498_','user559f_')):
        target=candidate if fault.endswith('_candidate') else paths['installed']
        target.write_bytes(('synthetic-'+fault.split('_')[0]).encode())
    if fault=='stage_mode':stage.chmod(0o777)
    if fault=='stale':(stage/'driver.incoming').write_bytes(b'stale')
    if fault=='symlink':candidate.unlink();candidate.symlink_to(paths['stable'])
    if fault=='hardlink':os.link(candidate,directory/'hardlink')
    if fault=='writable':candidate.chmod(0o777)
    if fault=='client':client.write_bytes(b'corrupted')
    if fault=='client_missing':client.unlink()
    if fault=='client_symlink':client.unlink();client.symlink_to(paths['stable'])
    if fault=='client_hardlink':os.link(client,directory/'client-hardlink')
    if fault=='client_writable':client.chmod(0o777)
    env={**os.environ,'PATH':str(stub)+':'+os.environ['PATH'],'FIXTURE_ROOT':str(directory),
         'FIXTURE_FAULT':fault,'FIXTURE_MODE':mode}
    installed_before=paths['installed'].read_bytes() # includes deliberate foreign/corrupt fixture bytes
    result=subprocess.run(['/bin/sh',str(script),mode],env=env,capture_output=True,text=True)
    (directory/'stdout.log').write_text(result.stdout)
    (directory/'stderr.log').write_text(result.stderr)
    (directory/'exit-status.txt').write_text(str(result.returncode)+'\n')
    calls=(directory/'calls').read_text().splitlines() if (directory/'calls').exists() else []
    if retired and mode=='--apply':
        assert result.returncode==1 and result.stderr.strip()=='STOP_96a0_DIAGNOSTIC_ALREADY_OBSERVED_NO_REAPPLY'
        assert not calls and not (directory/'platform-calls').exists(), 'retired APPLY must stop BEFORE platform/admin access'
        assert paths['installed'].read_bytes()==installed_before
        assert not (stage/'stable041.backup').exists() and not (stage/'.exchange-lock').exists()
        incoming=stage/'driver.incoming'
        if fault=='stale':assert incoming.read_bytes()==b'stale'
        else:assert not incoming.exists()
        continue
    if fault=='none':
        assert result.returncode==0,(fault,result.stdout,result.stderr)
        assert paths['installed'].read_bytes()==data['candidate' if mode=='--apply' else 'stable']
        assert len(calls)==3 and calls[1].startswith('--quiesce ') and calls[2].startswith('--wait ')
        assert (stage/'stable041.backup').read_bytes()==data['stable'];passed+=1
    else:
        assert result.returncode!=0,(fault,result.stdout,result.stderr)
        if fault=='wait':
            assert paths['installed'].read_bytes()==data['candidate' if mode=='--apply' else 'stable'] and len(calls)==3
            assert 'NO_AUTOMATIC_ROLLBACK' in result.stdout
        elif fault=='changed_during_quiesce':assert paths['installed'].read_bytes()==b'changed' and len(calls)==2
        elif fault=='client_changed_during_quiesce':
            assert paths['installed'].read_bytes()==installed_before and len(calls)==2 and client.read_bytes()==b'changed'
        else:assert paths['installed'].read_bytes()==installed_before
        if fault not in ('wait','quiesce','changed_during_quiesce','client_changed_during_quiesce'):assert len(calls)<=1,(fault,calls)
        if fault.startswith('retired96a0_'):
            assert not calls and not (stage/'stable041.backup').exists() and not (stage/'driver.incoming').exists()
print('exact '+('NEW5ae9+559f sampler diagnostic' if matrix_phase else 'NEWdb03 constant-Q4 diagnostic' if constant_q4 else 'NEW0f20 token-phase diagnostic' if token_phase else 'NEWb8c0 returning-wide-GDN' if wide_gdn else 'NEW7c85 corrected-output' if output_closure else 'NEW96a0')+' exchange script endpoint fixtures:',passed,'valid /',len(cases)-passed,'refused; NO actual install/reset/GPU')
if retired:print('RETIRED96a0 APPLY stops before platform/admin/copy/lock;entire original2way endpoints FIRST;guarded RESTORE unchanged')
print('raw fixture histories retained at',root)
