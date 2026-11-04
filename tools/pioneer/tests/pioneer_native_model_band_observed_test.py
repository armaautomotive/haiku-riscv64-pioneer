#!/usr/bin/env python3
"""Check immutable CLOSED learned-weight hardware records;NO device access."""
import hashlib
import pathlib
import re

if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
reference = (here.parent / 'benchmarks/qwen38-native-model-band-preparation-20261104/learned-reference.log').read_bytes()
assert hashlib.sha256(reference).hexdigest() == '35142021d16a51e50b38657418800f3dcb102ddadc3961ec680d16ebf693ce83'
expected = {}
firsts = (0, 8704, 17376)
for line in reference.decode().splitlines():
    match = re.fullmatch(r'LEARNED_FRACTION_REFERENCE sample([012]) first([0-9]+) row([0-9]+) word([0-9a-f]{8})', line)
    if match:
        sample, first, row = map(int, match.group(1, 2, 3))
        assert first == firsts[sample] and row < 32 and (sample, row) not in expected
        expected[sample, row] = match.group(4)
assert len(expected) == 96
root = here.parent / 'benchmarks/qwen38-native-model-band-physical-20261104'
raw = (root / 'learned.stdout').read_bytes()
assert len(raw) == 18546 and hashlib.sha256(raw).hexdigest() == '71f8686db0d94af820be42c0e87f2f4b1bb4aa53acf3fbfb9b6e92cd2222d7e6'
assert (root / 'learned.stderr').read_bytes() == b''
assert (root / 'physical.stderr').read_bytes() == b''
assert (root / 'postflight.stderr').read_bytes() == b''
assert (root / 'learned.exit').read_bytes() == b'0\n'
assert (root / 'state').read_bytes() == b'STABLE041_RESTORED_LEARNED_BANDS_EXACT192_PASSED\n'
lines = raw.decode().splitlines()
assert len(lines) == 198
assert lines[0] == 'AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET'
assert lines[1] == 'AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK'
for sample, first in enumerate(firsts):
    offset = 2 + sample * 65
    for word, line in enumerate(lines[offset:offset + 64]):
        match = re.fullmatch(r'NATIVE_LEARNED_BAND_RETURNED_WORD sample([012]) first([0-9]+) pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
        assert match and tuple(map(int, match.group(1, 2, 3, 4))) == (sample, first, word // 32, word % 32)
        assert match.group(5) == match.group(6) == expected[sample, word % 32]
    assert lines[offset + 64] == f'PASS_NATIVE_LEARNED_BAND sample{sample} tensor7 first{first} width5120 phases4 dual_jobs2 exact_words64 polls2 generation9;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
assert lines[-1] == 'PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
outer = (root / 'physical.log').read_bytes()
assert outer.count(raw) == 1 and outer.endswith(b'NATIVE_LEARNED_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM\n')
post = (root / 'postflight.log').read_text()
assert post.count('P100A_RELOAD driver55 tag2026102100010012 generation10 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n') == 1
assert re.search(r'^64 .*running at', post, re.M)
assert re.search(r'used/max [0-9]+ / 137034936320\)', post)
assert '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403  /boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe\n' in post
assert '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74  /boot/home/config/non-packaged/servers/HaikuAssistantQwen27\n' in post
assert 'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18  /boot/home/config/non-packaged/servers/HaikuAssistantService\n' in post
print('PASS immutable recorded learned-weight hardware regression:3 authenticated bands/192 exact returned words/3 ENDs,independent Fraction reference,exact SHA/exit/stderr/restored041/gen10/64CPU/fullRAM;NO NEW RUN_OR_FULL_MODEL_QUALITY_SPEED_OR_DEVICE_AUTHORITY')
