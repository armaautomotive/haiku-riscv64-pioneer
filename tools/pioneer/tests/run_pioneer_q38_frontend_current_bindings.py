#!/usr/bin/env python3
"""Current qualified module/ROOT DATA bindings;NO device IO or speed proof."""
import hashlib
import os
import pathlib
import runpy
import sys
import tempfile
import inspect_pioneer_q38_frontend_profile as profile

assert len(sys.argv) == 6 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
old, retained, root, flat_module, prefetch_module = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
saved = sys.argv[:]
try:
    # Entire old binding and parser controls FIRST;do not reinterpret old logs
    # or relax the unchanged first-three-token numerical comparison.
    sys.argv = [str(here / 'run_pioneer_q38_frontend_profile_binding_tests.py'), str(old), str(retained)]
    runpy.run_path(sys.argv[0], run_name='__main__')
    sys.argv = [str(here / 'pioneer_q38_frontend_profile_parser_test.py')]
    runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved

drivers = (
    '8612962e38cab47999b8aecf67268c8e7fd651917b8a570fd79f79b78e1b165f',
    '919d4a631ff664549a67bb2372ce2db344ba29473c452c0b273c4b27a2bdf978',
)
root_bytes = profile.stable(root, 191436)
assert len(root_bytes) == 191436
assert hashlib.sha256(root_bytes).hexdigest() == '88e419152d297dbe2ebb0b3913d3e2cae58aa69741b4b03e2667749702c4a674'
for module, driver in zip((flat_module, prefetch_module), drivers):
    assert hashlib.sha256(profile.stable(module, 4*1024*1024)).hexdigest() == driver
ranges = [profile.authenticated_ranges(root, driver) for driver in drivers]
assert ranges[0] == ranges[1], 'same ROOT must have identical symbols,not different invented intervals'
for actual in ranges:
    for name in ('GlobalChain::Scope(', 'GlobalChain::Packet(', 'MixedRecurrentCohort::',
                 'PioneerQ38PushedWorkerLayout::BootShape(', 'PioneerQ38PushedWorkerLayout::JobShape('):
        assert any(symbol.startswith(name) for _, _, symbol in actual), name
    assert not any(symbol.startswith('PioneerQ38PreciseWorkerLayout::JobShape(') for _, _, symbol in actual)
    assert profile.bins([{'first_raw': 0x60008, 'second_raw': 0xffffffff}], actual) == {'UNMAPPED_RAW_ADDRESS': 2}

refused = 0
original_tool = profile.subprocess.check_output
def forbidden_tool(*args, **kwargs):
    raise AssertionError('external symbol tool reached before input authentication')
def deny(path, driver):
    global refused
    profile.subprocess.check_output = forbidden_tool
    try:
        try:
            profile.authenticated_ranges(path, driver)
        except (ValueError, OSError):
            refused += 1
        else:
            raise AssertionError('wrong current ROOT binding accepted')
    finally:
        profile.subprocess.check_output = original_tool

for driver in drivers:
    for prior in (old, retained):
        deny(prior, driver)
for driver in profile.BINDINGS:
    if driver not in drivers:
        deny(root, driver)
deny(root, '0'*64)
with tempfile.TemporaryDirectory(prefix='pioneer-q38-current-root-binding-', dir='/private/tmp') as temporary:
    temporary = pathlib.Path(temporary)
    corrupt = temporary / 'CORRUPT-DATA-NEVER-EXECUTED.elf'
    link = temporary / 'root-symlink.elf'
    link.symlink_to(root)
    for driver in drivers:
        for at in (0, len(root_bytes)//2, len(root_bytes)-1):
            changed = bytearray(root_bytes)
            changed[at] ^= 1
            corrupt.write_bytes(changed)
            deny(corrupt, driver)
        for length in (0, 1, len(root_bytes)-1):
            corrupt.write_bytes(root_bytes[:length])
            deny(corrupt, driver)
        corrupt.write_bytes(root_bytes+b'\0')
        deny(corrupt, driver)
        deny(link, driver)
        deny(pathlib.Path('RELATIVE-NOT-OPENED.elf'), driver)
        deny(temporary, driver)
assert refused == 30, refused
print('PASS current frontend binding/2 actual module hashes+SAME88e4 ROOT/2 identical exact symbol maps/'
      '30 old,wrong,corrupt,truncated,oversized,symlink,relative,directory refusals BEFORE external tool;'
      'RAW60008+FFFFFFFF remain unmapped;NO device IO,retired-PC,cycle,time,quality,Work or hardware-provenance grant')
