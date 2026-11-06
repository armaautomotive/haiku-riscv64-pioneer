#!/usr/bin/env python3
"""Saved integration receipts only; not native execution or TPS evidence."""
import hashlib
import json
from pathlib import Path

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
exits=json.loads((here/'actual-exits.json').read_text());assert len(exits)==13 and set(exits.values())=={0}
def manifest(path):
 rows=[line.split('  ',1) for line in path.read_text().splitlines()]
 result={name:digest for digest,name in rows};assert len(result)==len(rows);return result
main=manifest(here/'main/scoped-source-sha256.txt');clean=manifest(here/'clean/scoped-source-sha256.txt')
assert len(main)==38 and main==clean
for name,digest in main.items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
a=manifest(here/'main/native-source-sha256.txt');b=manifest(here/'clean/native-source-sha256.txt')
assert a.keys()==b.keys() and len(a)==373
assert {name for name in a if a[name]!=b[name]}=={
 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenCallScope.inc',
 'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeEnd.inc',
 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp',
 'tools/pioneer/build_qwen38_full_offload_native.py'}
for name,digest in a.items():assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
for copy in ('main','clean'):
 directory=here/copy
 for label in ('native','client-tests','log-reader','parent','roles'):assert not (directory/(label+'.stderr')).read_bytes()
 assert '2893592 b635aab47e10b3bf657b290e27eb2a41460ba7db71866a0f9ecf475d70a0c954' in (directory/'native.stdout').read_text()
 assert 'positive5/refusals551/' in (directory/'roles.stdout').read_text()
 client=directory/'client';assert json.loads((client/'actual-exits.json').read_text())=={
  'protocol-compile':0,'protocol':0,'session-compile':0,'session':0,'sdk-build':0}
 assert 'sources68_' in (directory/'client-tests.stdout').read_text()
 for name in ('protocol-compile','protocol','session-compile','session','sdk-build'):assert not (client/(name+'.stderr')).read_bytes()
 assert '82336 e3c784766d6ff46513157bc9ae5849e5aac73a02f4b9c77e48f7dbfb24f72a26' in (client/'sdk-build.stdout').read_text()
 assert '228264 509856b35bb9f8c7da34c5537f1a459bcd21ca7c95343a680fbb647dcaaa35c7' in (client/'sdk-build.stdout').read_text()
 assert '379 fail-closed COW fixtures' in (client/'session.stdout').read_text()
 assert '199 mode,count,cycle,truncation refusals' in (directory/'log-reader.stdout').read_text()
 assert 'except10 exact subject substitutions' in (directory/'parent.stdout').read_text()
for label in ('footprint','held-read','held-control','journal','journal-faults','journal-capsule',
 'baseline-native','baseline-client','legacy-held-read','legacy-held-control','legacy-native-roles','rate-client','preflight'):
 assert not (here/'main'/(label+'.stderr')).read_bytes(),label
assert 'saved15444538/installed3262923/restored15444538/secondOriginal30889076/' in (here/'main/journal.stdout').read_text()
assert 'faults52/inert preconditionRefusals33/' in (here/'main/journal-faults.stdout').read_text()
assert 'HELD-journal faults27/' in (here/'main/journal-capsule.stdout').read_text()
assert 'ALL120 exact code3262923 words/snapshot15444538 words=61778152bytes/' in (here/'main/footprint.stdout').read_text()
assert '905 EVERY IO-event' in (here/'main/held-read.stdout').read_text()
assert '3561 all3072 request-bits' in (here/'main/held-control.stdout').read_text()
assert '2884184 d8b1c561c9895841c6389576b53027d6e6998b3761dac12f47dbec961fb8bbb9' in (here/'main/baseline-native.stdout').read_text()
for digest in ('0190291350ca5eb56a235fcf6c2d6e3c000a7146f9f4b86fd7cd28a13051feec','aa9aac510e66d7bfba98b61019181ddc6662639badbb7ebc4edcdb2f91d822ef'):
 assert digest in (here/'main/baseline-client.stdout').read_text()
assert (here/'main/preflight.stdout').read_text().endswith('PASS_READ_ONLY_NORMAL041_GEN40_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED_FRESH106_PATH\n')
print('PASS SAVED BF16 tag106 integration/main-clean artifacts/38 scoped sources/373 native dependencies/'
 'actual journal algorithm and observer MOCK tests/old105 artifacts preserved;NO new native execution,model result or speed proof')
