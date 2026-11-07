#!/usr/bin/env python3
"""Exercise the exact one-shot parent with mocked OS/device endpoints only."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

here=Path(__file__).resolve().parent;source=here.parent/'pioneer_hybrid_upload_physical_once.sh'
subprocess.run(['sh','-n',str(source)],check=True)
original=source.read_text()
stable='041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403'
clienthash='560fc11a161ce54867dbe84c557a4a94d3513f4227d94903cef4e2096558e11c'
adminhash='b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f'
guardhash='d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229'
def script(path,text):
    path.write_text('#!/bin/sh\n'+text+'\n');path.chmod(0o755)
def case(root,mode):
    case_dir=root/mode;case_dir.mkdir();stage=case_dir/'stage';stage.mkdir();cache4=case_dir/'cache4'
    if mode=='symlinkcache':
        real=case_dir/'realcache';real.mkdir();cache4.symlink_to(real,target_is_directory=True)
    else:cache4.mkdir()
    cache8=case_dir/'cache8';cache8.mkdir();bin_dir=case_dir/'bin';bin_dir.mkdir()
    shutil.copyfile(here.parent/'pioneer_hybrid_upload_output.awk',stage/'upload-output.awk')
    for name in ('installed','guard'):(case_dir/name).write_text('fixture')
    script(bin_dir/'uname','printf "Haiku\\n"')
    script(bin_dir/'id','printf "0\\n"')
    script(bin_dir/'stat','case "$2" in "%u:%a") echo 0:700;; "%u:%h:%a") case "$3" in */upload-output.awk) echo 0:1:644;; *) echo 0:1:755;; esac;; *) exit 2;; esac')
    script(bin_dir/'sha256sum',f'''case "$1" in
 */upload-test) h={clienthash}; [ "${{MOCK_BAD_HASH:-0}}" != 1 ] || h=bad;;
 */installed) h={stable};; */admin) h={adminhash};; */guard) h={guardhash};;
 */upload-output.awk) h=cef4455c58280294c5e434f453052e67d679685526087029fda87cc3c319af97;; *) exit 2;; esac
printf '%s  %s\\n' "$h" "$1"''')
    script(bin_dir/'ps','printf "Id Team\\n1 kernel_team\\n"\n[ "${MOCK_BUSY:-0}" != 1 ] || printf "2 /boot/home/upload-test\\n"')
    script(bin_dir/'sysinfo','[ "${MOCK_BAD_HEALTH:-0}" != 1 ] || exit 1\nprintf "64 cpus running at 1\\nused/max 0 / 137034936320)\\n"')
    count=case_dir/'admin.count'
    script(case_dir/'admin',f'''n=0
[ ! -f "{count}" ] || n=$(cat "{count}")
n=$((n+1)); printf '%s\\n' "$n" > "{count}"
[ "${{MOCK_BAD_START:-0}}" != 1 ] || [ "$n" != 1 ] || exit 1
[ "${{MOCK_BAD_POST:-0}}" != 1 ] || [ "$n" != 2 ] || exit 1
printf '%s\\n' 'P100A_RELOAD driver55 tag2026102100010012 generation20 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' ''')
    script(stage/'upload-test',r'''[ "$1" = --upload-verified-diagnostic ] || exit 8
printf '%s\n' 'HYBRID_UPLOAD_DIAGNOSTIC source authentication and cache verification; NO DEVICE OPEN YET'
[ "${MOCK_CLIENT:-ok}" != fail ] || exit 7
i=21
while [ "$i" -le 1344 ]; do
 if [ "${MOCK_CLIENT:-ok}" != missing ] || [ "$i" != 42 ]; then printf 'HYBRID_UPLOAD_PREFLIGHT %s/1344\n' "$i"; fi
 i=$((i+21))
done
printf '%s\n' 'HYBRID_UPLOAD_PROGRESS 1048576/21137039360 bytes; NO MODEL EXECUTION'
if [ "${MOCK_CLIENT:-ok}" = backwards ]; then printf '%s\n' 'HYBRID_UPLOAD_PROGRESS 4096/21137039360 bytes; NO MODEL EXECUTION'; fi
printf '%s\n' 'HYBRID_UPLOAD_PROGRESS 21137039360/21137039360 bytes; NO MODEL EXECUTION'
if [ "${MOCK_CLIENT:-ok}" = wrongpass ]; then
 printf '%s\n' 'PASS_HYBRID_UPLOAD_RELEASE fragments3550 spans22005 bytes21137039360 seconds1.000; SPANS_AND_BANKS_ENDED WEIGHTS_RELEASED NO_MODEL_EXECUTION_OR_TOKEN_RATE'
else
 printf '%s\n' 'PASS_HYBRID_UPLOAD_RELEASE fragments3551 spans22005 bytes21137039360 seconds1.000; SPANS_AND_BANKS_ENDED WEIGHTS_RELEASED NO_MODEL_EXECUTION_OR_TOKEN_RATE'
fi
[ "${MOCK_CLIENT:-ok}" != stderr ] || echo unexpected >&2
[ "${MOCK_CLIENT:-ok}" != extra ] || echo unexpected''')
    replacements={
     'PATH=/boot/system/bin:/bin':f'PATH={bin_dir}:/usr/bin:/bin',
     'stage=/boot/home/pioneer-hybrid-upload-test.KtEQ8Xj6':f'stage={stage}',
     'installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe':f'installed={case_dir}/installed',
     'admin=/boot/home/pioneer-p100a-hot-reload-20261019-a/pioneer-p100a-reload':f'admin={case_dir}/admin',
     'guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard':f'guard={case_dir}/guard',
     'cache4=/boot/home/pioneer-hybrid-bfp4-20261107-cache':f'cache4={cache4}',
     'cache8=/boot/home/pioneer-hybrid-bfp8-20261107-cache':f'cache8={cache8}',
    }
    text=original
    for old,new in replacements.items():
        assert text.count(old)==1,old
        text=text.replace(old,new)
    parent=case_dir/'parent.sh';parent.write_text(text)
    env=dict(os.environ);env['MOCK_CLIENT']=mode if mode in ('fail','missing','backwards','wrongpass','stderr','extra') else 'ok'
    if mode=='badhash':env['MOCK_BAD_HASH']='1'
    if mode=='busy':env['MOCK_BUSY']='1'
    if mode=='badhealth':env['MOCK_BAD_HEALTH']='1'
    if mode=='badpost':env['MOCK_BAD_POST']='1'
    if mode=='badstart':env['MOCK_BAD_START']='1'
    result=subprocess.run(['sh',str(parent)],env=env,capture_output=True,text=True)
    n=int(count.read_text()) if count.exists() else 0
    if mode=='ok':
        assert result.returncode==0,(result.stdout,result.stderr)
        assert (stage/'state.clean').read_text().strip()=='UPLOAD_RELEASE_VERIFIED_NO_MODEL_EXECUTION' and n==2
    elif mode in ('badhash','busy','badhealth','badstart','symlinkcache'):
        assert result.returncode!=0 and not (stage/'.upload-once').exists() and not (stage/'upload.stdout').exists()
        assert n==(1 if mode in ('badhealth','badstart') else 0)
    else:
        assert result.returncode==(71 if mode=='badpost' else 70),(mode,result.stdout,result.stderr)
        assert (stage/'.upload-once').is_dir() and (stage/'state.failed').exists() and not (stage/'state.clean').exists()
        assert n==(2 if mode=='badpost' else 1)
    # A claimed stage is spent forever: even the administrative metadata
    # query must not run on a repeated invocation after success or failure.
    if (stage/'.upload-once').exists():
        again=subprocess.run(['sh',str(parent)],env=env,capture_output=True,text=True)
        assert again.returncode!=0 and int(count.read_text())==n

with tempfile.TemporaryDirectory(prefix='pioneer-hybrid-parent-',dir='/private/tmp') as tmp:
    for mode in ('ok','fail','missing','backwards','wrongpass','stderr','extra','badhash','busy','badhealth','badstart','badpost','symlinkcache'):
        case(Path(tmp),mode)
print('PASS one-shot upload parent: actual gate/control flow, 13 fake cases and no repeated metadata; NO DEVICE')
