#!/usr/bin/env python3
"""Keep the one-shot tag101 test identical to the reviewed tag93 flow."""
import pathlib
import hashlib
import re
import runpy
import subprocess

here=pathlib.Path(__file__).resolve().parent
runpy.run_path(str(here/'pioneer_q38_full_offload_rate_parent_test.py'),run_name='__main__')
old=(here.parent/'pioneer_q38_full_offload_macro_q4_rate_physical.sh').read_text()
path=here.parent/'pioneer_q38_full_offload_spill_q4_sfpnop_rate_physical.sh'
text=path.read_text();subprocess.run(['sh','-n',str(path)],check=True)
changes={
 '# ONE full-card macro-Q4 tag93 rate trial;fixed generation18 admission.':'# ONE full-card SFPNOP-ordered spill-Q4 tag101 rate trial;fixed generation2 admission.',
 '[ "$#" = 1 ] && [ "$1" = --rate-32 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2':'[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ] || exit 2\ncase "$1" in --rate-32|--admission-only) ;; *) exit 2;; esac',
 'pioneer-q38-full-macro-q4-20261104-a':'pioneer-q38-full-spill-q4-sfpnop-20261115-d',
 'tenstorrent_pci_probe-full-macro-q4-93':'tenstorrent_pci_probe-full-spill-q4-sfpnop-101',
 'HaikuAssistantQwen27FullOffload-macro-q4':'HaikuAssistantQwen27FullOffload-spill-q4-sfpnop',
 'candidate_hash=61a8cce47e5294f6f6263fe22a331859c22576f7e268416a3433f00c81e7d79e':'candidate_hash=95b62dfaaa75c3c8f4856c081c1e8fb700e444b99a7a0402b3056efbfcc304a9',
 'worker_hash=103e0c734fbb1f63f2beea6b3de617e42a3699af7450b4ee895e8a056cd81160':'worker_hash=26db652e6fd7e77b63f5008f8fcbeb8d0213e19d1231b6947e0f8ad99ee1e156',
 'client_hash=46f4fcb520cf2c5ffa6767c3b3d17371ff5c1d22aeb5dcd98d15100262baa7b3':'client_hash=48a6e32f76ae339b2007693c6ee871e70c4ec6fafcb1344d35ca6d428e3be1c9',
 '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74':'93f1d09ba0b2e0d0ea73642ccc89bdc5d0bf0cbed4be0ce3bbd47ea94d24008f',
 'native_tag=2026110400010093':'native_tag=2026111500010101',
 'stable_generation=18 native_generation=19 restored_generation=20':'stable_generation=2 native_generation=3 restored_generation=4'}
expected=old
for source,target in changes.items():
 assert expected.count(source)==1 and text.count(target)==1,source
 expected=expected.replace(source,target)
old_idle_start=old.index('idle(){');old_idle_end=old.index('\nhealth(){',old_idle_start)
new_idle_start=text.index('idle(){');new_idle_end=text.index('\nhealth(){',new_idle_start)
old_idle=old[old_idle_start:old_idle_end];new_idle=text[new_idle_start:new_idle_end]
assert 'launcher_file=$stage/direct-launcher.pid' in new_idle
assert '"$(stat -c \'%u:%h:%a\' "$launcher_file")" = 0:1:600' in new_idle
assert 'case "$launcher" in \'\'|*[!0-9]*) return 1;; esac' in new_idle
assert 'awk -v self="$$" -v launcher="$launcher"' in new_idle
assert 'teams++;if($1==self || $1==launcher)next' in new_idle
assert 'pioneer_q38|pioneer-q|pioneer-native-' in new_idle
assert new_idle.count('owners++')==old_idle.count('owners++')==1
assert expected.count(old_idle)==1 and text.count(new_idle)==1
expected=expected.replace(old_idle,new_idle)
normal_gate='idle && metadata stable-before && health || exit 1\n'
admission_gate=normal_gate+'''if [ "$1" = --admission-only ];then
 printf '%s\\n' PASS_TAG101_READONLY_ADMISSION_NO_DRIVER_EXCHANGE_OR_GPU_EXECUTION
 exit 0
fi
'''
assert expected.count(normal_gate)==1 and text.count(admission_gate)==1
expected=expected.replace(normal_gate,admission_gate)
assert text==expected,'NO changes to admission, quiesce/swap/wait, rate checks or stable restore beyond pinned candidate identity'
launch=here.parent/'pioneer_q38_full_offload_spill_q4_sfpnop_launch_once.sh'
launch_text=launch.read_text();subprocess.run(['sh','-n',str(launch)],check=True)
assert launch_text.count('pioneer-q38-full-spill-q4-sfpnop-20261115-d')==1
assert launch_text.count('umask 077')==1
assert launch_text.index('umask 077') < launch_text.index('printf \'%s\\n\' "$$" > "$stage/direct-launcher.pid"')
parent_hash=re.search(r'\$\{digest%% \*\}" = ([0-9a-f]{64})',launch_text)
assert parent_hash and parent_hash.group(1)==hashlib.sha256(path.read_bytes()).hexdigest()
for required in ('test ! -e "$stage/$name" && test ! -L "$stage/$name"',
                 'mkdir "$stage/.direct-launch-once"','direct-parent.exit','direct-parent.stdout',
                 '/bin/sh "$parent" --rate-32'):
 assert required in launch_text,required
print('PASS tag101 SFPNOP parent:exact reviewed trial flow and one-shot restore;immutable hashes/generations only;NO hardware execution granted',flush=True)
