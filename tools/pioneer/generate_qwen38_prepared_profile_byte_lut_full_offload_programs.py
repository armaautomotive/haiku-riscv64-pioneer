#!/usr/bin/env python3
"""Pair the audited tag99 phase-profile ROOT with tag104's exact byte-LUT peers."""
import hashlib
import pathlib
import re
import sys

if not __debug__ or len(sys.argv) != 2:
    raise SystemExit('usage: generate_qwen38_prepared_profile_byte_lut_full_offload_programs.py OUT_HEADER')
here = pathlib.Path(__file__).resolve().parent
output = pathlib.Path(sys.argv[1])
assert output.is_absolute() and not output.exists() and not output.is_symlink()
profile = (here / 'PioneerQ38FullOffloadPreparedProfileQ4Programs.h').read_bytes()
byte_lut = (here / 'PioneerQ38FullOffloadPreparedByteLutQ4Programs.h').read_bytes()
assert hashlib.sha256(profile).hexdigest() == 'e8eb2d83ccbadc01f64b142b7d97c300a273c737be1191afc86c0b65c06943e2'
assert hashlib.sha256(byte_lut).hexdigest() == 'd81ed84163311bc0dd5942dcf6d501048a05e83fbae436d79ea9fc17fd74ab7a'
text = profile.decode('ascii')
peer = byte_lut.decode('ascii')
labels = ('PeerBoot', 'PeerFront', 'PeerPrecise', 'PeerDense', 'PeerFfn',
          'PeerService', 'PeerFetch', 'PeerAffine', 'PeerText', 'PeerGdn')
for label in labels:
    pattern = r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ' + label + r'\[\]=\{.*?\n\};\nstatic_assert\(sizeof\(' + label + r'\)==[0-9]+,"exact complete ' + label + r' DATA"\);'
    source = re.search(pattern, peer, flags=re.S)
    assert source is not None, label
    text, count = re.subn(pattern, source.group(0), text, count=1, flags=re.S)
    assert count == 1, label
text = text.replace('PioneerQ38FullOffloadPreparedProfileQ4ProgramsRole.h',
                    'PioneerQ38FullOffloadPreparedProfileByteLutQ4ProgramsRole.h')
text = text.replace('PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_Q4_PROGRAMS_H',
                    'PIONEER_Q38_FULL_OFFLOAD_PREPARED_PROFILE_BYTE_LUT_Q4_PROGRAMS_H')
text = text.replace('PioneerQ38FullOffloadPreparedProfileQ4Programs',
                    'PioneerQ38FullOffloadPreparedProfileByteLutQ4Programs')
text = text.replace('enum {CounterUnroll=1,PreparedQ4=1,NativeQ4Exact=1,RootSections=5,PreparedCodeAddress=0x6c000};',
                    'enum {CounterUnroll=1,PreparedActivationByteLutWorker=1,PreparedQ4=1,NativeQ4Exact=1,RootSections=5,PreparedCodeAddress=0x6c000};')
assert 'PreparedActivationByteLutWorker=1' in text
assert len(re.findall(r'// SHA256 [0-9a-f]{64}\nstatic const uint32_t ', text)) == 15
output.write_text(text, encoding='ascii')
print(hashlib.sha256(output.read_bytes()).hexdigest(), output.name, flush=True)
print('TAG105 exact tag99 ROOT retained; ten tag104 byte-LUT worker images substituted; no execution authority', flush=True)
