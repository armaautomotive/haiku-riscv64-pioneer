#!/usr/bin/env python3
"""Saved ACTUAL86 child + FAILED outer + separate restore;READ ONLY,no admission."""
import hashlib
import pathlib
from pioneer_native_q4_fpu_stream_output import verify
from pioneer_native_q4_fpu_stream_parent_output import health, metadata, verify_parent
assert __debug__
folder=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-q4-fpu-stream-physical-20261104'
pins={
    'parent.stdout':'178f86c96b85a1df0e4bc65d97c618069d2b3b0efee0bd75b91db697b18812ff',
    'q4-stream.stdout':'7588578d8d49a8e65c62c0277fe9f4dc46f3c0186a4ef6abd2bed811b439d347',
    'postterminal.stdout':'38b0e9237e19f2e437b832049a59793e7ba70ececf1309f3c17ed95237b05796',
    'restore.stdout':'c91bb8c8577d805c9602eb1a4f1437aa1641b7366e390432df380cda79084319',
    'postflight.stdout':'328c46df56bfe79ddcd2d11db1a5a3d81300a2a4cd8ab95bab6117b499192225',
    'parent.source.sh':'a54de1d4ee8218e7c13be3b2fb77a4fb94f88cbecdc69b342b60ec58e36b4f14',
    'restore.source.sh':'403b429dce68f09682d4f1334975b45e9198ae7a6ebf45a4693c76fc799d84c3',
}
for name,digest in pins.items():assert hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest,name
for name in ('parent.stderr','q4-stream.stderr','postterminal.stderr','restore.stderr','postflight.stderr'):assert (folder/name).read_bytes()==b''
assert (folder/'parent.exit').read_bytes()==b'1\n' # NEVER promote failed outer
for name in ('q4-stream.exit','postterminal.exit','restore.exit','postflight.exit'):assert (folder/name).read_bytes()==b'0\n'
assert (folder/'parent.pid').read_text()=='1372\n' and (folder/'restore.pid').read_text()=='1518\n'
assert (folder/'state').read_text()=='RUNNING_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES\n'
assert (folder/'restore.state').read_text()=='STABLE041_GENERATION10_RESTORED_AFTER_NINE_CLEAN_ENDS_ORIGINAL_PARENT_FAILED\n'
parent=(folder/'parent.stdout').read_text();child=(folder/'q4-stream.stdout').read_text();restored=(folder/'restore.stdout').read_text();post=(folder/'postflight.stdout').read_text()
assert len(parent.splitlines())==607 and len(child.splitlines())==588 and parent.count(child)==1
assert verify(child)=={'samples':9,'words':576,'ends':9,'generation':9,'polls':[1,1,1,1,1,1,2,2,2]}
assert parent.endswith(metadata('2026110400010086',9)+'\n')
try:verify_parent(parent)
except ValueError:pass
else:raise AssertionError('FAILED original outer was promoted to a complete success')
assert '\nmetadata 2026110400010086 7 || exit 1\n' in (folder/'parent.source.sh').read_text()
lines=restored.splitlines();assert len(lines)==26
health(lines[1:10]);health(lines[16:25])
assert lines[0]==lines[10]==metadata('2026110400010086',9)
assert lines[11]==metadata('2026110400010086',9,1)
assert lines[12]=='QUIESCED_CLOSED;NEW_HARDWARE_WORK_BLOCKED_UNTIL_RELOAD'
assert lines[13]==lines[15]==metadata('2026102100010012',10)
assert lines[14]=='HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION'
assert lines[25]=='CLOSED86_RESTORED_STABLE041_NO_NEW_GPU_EXECUTION_NO_RESET_NO_RETRY'
assert post.startswith(metadata('2026102100010012',10)+'\n');health(post.splitlines()[1:10])
for digest,path in (
    ('041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403','add-ons/kernel/drivers/bin/tenstorrent_pci_probe'),
    ('861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74','servers/HaikuAssistantQwen27'),
    ('c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18','servers/HaikuAssistantService'),
    ('0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933','apps/HaikuAssistant')):
    assert digest+'  /boot/home/config/non-packaged/'+path in post
assert '0:1:600:18973870528 /boot/home/models/Qwen3.8-27B-Q4_K_M.gguf' in post
print('PASS saved ACTUAL86 arithmetic/lifecycle:576 exact full-K words/nine ENDs;original57599 CLOSED1/stale7-vs9 retained,NOT promoted;separate restore9216 CLOSED0/stable041gen10/64CPUs/fullRAM/unchanged Assistant;NO new hardware admission/model-quality/full-native-inference/speed proof')
