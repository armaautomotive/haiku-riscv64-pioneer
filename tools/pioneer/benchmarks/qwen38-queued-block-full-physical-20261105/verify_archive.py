#!/usr/bin/env python3
"""Read-only archived DATA verification,NOT current device/exit authority."""
import hashlib,json,os,pathlib,stat,sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):raise SystemExit('Checks required')
here=pathlib.Path(__file__).resolve().parent
assert len(sys.argv)==1
sys.path.insert(0,str(here.parents[1]/'tests'))
from parse_pioneer_q38_full_offload_rate_log import parse_complete,stable_text
stage='/boot/home/pioneer-q38-full-queued-q4-20261105-a/'
expected={'physical-parent.stdout','physical-parent.stderr','rate.stdout','rate.stderr','parent.exit','rate.exit','state','launcher.stderr','launcher.pid','parent.pid','physical-parent.sh'}
seen=set()
for row in stable_text(here/'remote-sha256.txt').splitlines():
 digest,path=row.split()
 assert len(digest)==64 and set(digest)<=set('0123456789abcdef')
 assert path.startswith(stage) and path[len(stage):] in expected
 name=path[len(stage):];assert name not in seen;seen.add(name)
 local=here/name;info=local.lstat();assert stat.S_ISREG(info.st_mode) and info.st_nlink==1
 assert hashlib.sha256(local.read_bytes()).hexdigest()==digest
 if name=='physical-parent.sh':assert digest=='2196a4686709016be72ac0b106271f7ea9f7124a1b2f123e7456581770e32efd'
 print('VERIFIED_CLOSED_RAW',name,info.st_size,digest)
assert seen==expected
for name in ('scp.stderr','remote-sha256.stderr','physical-parent.stderr','rate.stderr','launcher.stderr','postflight.stderr','strict-main.stderr','strict-clean.stderr'):
 assert stable_text(here/name)==''
assert stable_text(here/'parent.exit')==stable_text(here/'rate.exit')=='0\n'
assert stable_text(here/'state')=='FULL_CARD_RATE32_CLOSED_STABLE_RESTORED\n'
assert stable_text(here/'launcher.pid')=='24689\n' and stable_text(here/'parent.pid')=='24690\n'
post=stable_text(here/'postflight.stdout')
assert 'P100A_RELOAD driver55 tag2026102100010012 generation34 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n' in post
assert post.endswith('PASS_READONLY_POSTFLIGHT_parent_and_child_actual0_protected_pids_absent_idle_stable041_gen34_apps_runtime_guard_64CPU_fullRAM_preserved\n')
data=parse_complete(stable_text(here/'rate.stdout'),stable_text(here/'rate.stderr'),prepared_q4=True,phase_profile=True)
for side in ('main','clean'):
 rows=stable_text(here/('strict-'+side+'.stdout')).splitlines()
 assert len(rows)==2 and json.loads(rows[0])==data
 assert rows[1]=='PASS_COMPLETE_FULL_CARD_RATE_TRANSCRIPT_DATA_ONLY;NOT_BROAD_QUALITY_ISOLATED_LATENCY_OR_NATIVE_CLEANUP_AUTHORITY'
assert data['observed_post_prompt_count_rate_bounds']['lower_tps']=='0.162052093'
assert data['observed_post_prompt_count_rate_bounds']['upper_tps']=='0.162055547'
print('PASS_WHOLE_ARCHIVED_QUEUED_RESULT_DATA_NOT_CURRENT_DEVICE_NATIVE_EXIT_OR_BROAD_QUALITY_AUTHORITY')
