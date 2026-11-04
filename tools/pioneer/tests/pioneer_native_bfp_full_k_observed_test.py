#!/usr/bin/env python3
"""Regression over immutable CLOSED full-K hardware records;NO device access."""
import hashlib
import pathlib
import re
if not __debug__:
    raise SystemExit('Assertions required')
here = pathlib.Path(__file__).resolve().parent
reference = (here.parent / 'benchmarks/qwen38-native-bfp-band-owner-20261104/full-k-owner-r2.log').read_bytes()
assert hashlib.sha256(reference).hexdigest() == 'e16f4282406ea8f21ea3b7bdf3f9100482e3eae1b9fc2e16cbcf4b1c7a3e00cb'
expected = {}
for line in reference.decode().splitlines():
    match = re.fullmatch(r'BAND_LEASE_SYNTHETIC_REFERENCE k(5120|6144|17408) row([0-9]+) word([0-9a-f]{8})', line)
    if match:
        key = tuple(map(int, match.group(1, 2)))
        assert key not in expected
        expected[key] = match.group(3)
assert len(expected) == 96
root = here.parent / 'benchmarks/qwen38-native-bfp-band-physical-20261104'
cases = ((5120, 'k5120', 5, 2, 5071, '5c3a33f4a124860f81714eacf3a234204877c73cb31fe2c9f07c43faacdf491e'),
         (6144, 'remaining', 7, 2, 5071, '7ea5f7c6a9428f8797b97dc419f40428ca6e02929de242549b85b61724572ae1'),
         (17408, 'remaining', 7, 3, 5136, 'e357e6b1bdfc5d36f17a136ed1088b12c0163d2223f68a4dffd4dd14c6de79ba'))
for width, folder, generation, polls, size, digest in cases:
    raw = (root / folder / f'band{width}.stdout').read_bytes()
    assert len(raw) == size and hashlib.sha256(raw).hexdigest() == digest
    assert (root / folder / f'band{width}.stderr').read_bytes() == b''
    assert (root / folder / f'band{width}.exit').read_bytes() == b'0\n'
    lines = raw.decode().splitlines()
    assert len(lines) == 65
    for word, line in enumerate(lines[:64]):
        match = re.fullmatch(r'NATIVE_BFP_BAND_RETURNED_WORD k([0-9]+) pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
        assert match and tuple(map(int, match.group(1, 2, 3))) == (width, word // 32, word % 32)
        assert match.group(4) == match.group(5) == expected[width, word % 32]
    assert lines[-1] == f'PASS_NATIVE_BFP_FULL_K bits8 width{width} rows32 dual_jobs2 phases4 exact_words64 polls{polls} generation{generation};EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_QUALITY_OR_SPEED_CLAIM'
assert (root / 'k5120/state').read_text() == 'STABLE041_RESTORED_FULL_K5120_BFP8_EXACT64_PASSED\n'
assert (root / 'remaining/state').read_text() == 'STABLE041_RESTORED_REMAINING_FULL_K_EXACT128_PASSED\n'
print('PASS immutable recorded full-K hardware regression:3 widths/192 exact returned words/3 END markers,independent prior Fraction reference,exact SHA/exit/stderr/states;NO NEW RUN_OR_MODEL_QUALITY_SPEED_OR_DEVICE_AUTHORITY')
