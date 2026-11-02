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

here=pathlib.Path(__file__).resolve().parent
production=(here.parent/'pioneer_dram_sweep_exchange.sh').read_text()
root=pathlib.Path(tempfile.mkdtemp(prefix='pioneer-dram-exchange-endpoints-'))
print('fixture root',root,flush=True)
pins={
 'stable':'041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
 'candidate':'32362d82bfa2179acdf0680c4f99671a6e4a54f580fd35cee16e25b0fc25a8fe',
 'guard':'d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
 'admin':'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'}
admin_source='''#!/usr/bin/env python3
import os,pathlib,sys
p=pathlib.Path(os.environ['FIXTURE_ROOT']);fault=os.environ['FIXTURE_FAULT'];args=sys.argv[1:]
with (p/'calls').open('a') as f:f.write(' '.join(args)+'\\n')
if args==['--status']:
 tag='2026102100010012' if os.environ['FIXTURE_MODE']=='--apply' else '2026110200020076'
 fields=['P100A_RELOAD','driver55','tag'+tag,'generation7','quiesced0','cold0','active_owner0','open_count1','guard1;CPU_METADATA_ONLY']
 if fault.startswith('field'):
  i=int(fault[5:]);fields[i]=['BAD','driver54','tag0','generation0','quiesced1','cold1','active_owner1','open_count2','guard0;'][i]
 print(' '.join(fields));sys.exit(0)
if args[0]=='--quiesce':
 assert args[1]==('2026102100010012' if os.environ['FIXTURE_MODE']=='--apply' else '2026110200020076')
 if fault=='quiesce':sys.exit(1)
 if fault=='changed_during_quiesce':(p/'bin/tenstorrent_pci_probe').write_bytes(b'changed')
 sys.exit(0)
if args[0]=='--wait':
 assert args[1]==('2026110200020076' if os.environ['FIXTURE_MODE']=='--apply' else '2026102100010012')
 assert args[2]=='0000000000000007'
 sys.exit(1 if fault=='wait' else 0)
raise AssertionError(args)
'''
stub_source='''#!/usr/bin/env python3
import hashlib,os,pathlib,stat,sys
name=pathlib.Path(sys.argv[0]).name;args=sys.argv[1:];root=pathlib.Path(os.environ['FIXTURE_ROOT'])
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
 'symlink','hardlink','writable','parent','quiesce','changed_during_quiesce','wait')]
cases += [('field'+str(i),'--apply') for i in range(9)]
cases += [(fault,'--restore') for fault,mode in cases if fault!='none']
passed=0
for index,(fault,mode) in enumerate(cases):
    directory=root/str(index);stage=directory/'stage';stage.mkdir(parents=True,mode=0o700)
    old=directory/'old';old.mkdir();(directory/'bin').mkdir();(directory/'dev/misc').mkdir(parents=True)
    paths={'stage':stage,'installed':directory/'bin/tenstorrent_pci_probe',
           'device':directory/'dev/misc/tenstorrent_pci_probe','guard':directory/'guard',
           'admin':old/'admin','stable':old/'stable'}
    data={'stable':b'synthetic-stable041','candidate':b'synthetic-diagnostic76','guard':b'synthetic-guard',
          'admin':admin_source.encode()}
    for key in ('stable','guard','admin'):
        paths[key].write_bytes(data[key]);paths[key].chmod(0o755)
    candidate=stage/'tenstorrent_pci_probe-dram-sweep-diagnostic76';candidate.write_bytes(data['candidate']);candidate.chmod(0o755)
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
    if fault=='stage_mode':stage.chmod(0o777)
    if fault=='stale':(stage/'driver.incoming').write_bytes(b'stale')
    if fault=='symlink':candidate.unlink();candidate.symlink_to(paths['stable'])
    if fault=='hardlink':os.link(candidate,directory/'hardlink')
    if fault=='writable':candidate.chmod(0o777)
    env={**os.environ,'PATH':str(stub)+':'+os.environ['PATH'],'FIXTURE_ROOT':str(directory),
         'FIXTURE_FAULT':fault,'FIXTURE_MODE':mode}
    result=subprocess.run(['/bin/sh',str(script),mode],env=env,capture_output=True,text=True)
    (directory/'stdout.log').write_text(result.stdout)
    (directory/'stderr.log').write_text(result.stderr)
    (directory/'exit-status.txt').write_text(str(result.returncode)+'\n')
    calls=(directory/'calls').read_text().splitlines() if (directory/'calls').exists() else []
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
        elif fault!='installed':assert paths['installed'].read_bytes()==original
        if fault not in ('wait','quiesce','changed_during_quiesce'):assert len(calls)<=1,(fault,calls)
print('exact exchange script endpoint fixtures:',passed,'valid /',len(cases)-passed,'refused; NO actual install/reset/GPU')
print('raw fixture histories retained at',root)
