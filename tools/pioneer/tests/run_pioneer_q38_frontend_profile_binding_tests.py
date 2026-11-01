#!/usr/bin/env python3
"""Authenticate actual ROOT ELF DATA bindings;NO device or native execution."""
import os
import pathlib
import sys
import inspect_pioneer_q38_frontend_profile as profile

assert len(sys.argv) == 3 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
old, retained = map(pathlib.Path, sys.argv[1:])
old_driver, retained_driver = tuple(profile.BINDINGS)
for elf, driver in ((old, old_driver), (retained, retained_driver)):
    ranges = profile.authenticated_ranges(elf, driver)
    assert any(name.startswith('GlobalChain::Scope(') for _, _, name in ranges)
    assert any(name.startswith('GlobalChain::Packet(') for _, _, name in ranges)
    assert any(name.startswith('MixedRecurrentCohort::') for _, _, name in ranges)
denied = 0
for elf, driver in ((old, retained_driver), (retained, old_driver), (retained, '0'*64)):
    try:
        profile.authenticated_ranges(elf, driver)
    except ValueError:
        denied += 1
    else:
        raise AssertionError('wrong declared binding accepted')
assert denied == 3
print('PASS frontend symbolizer actual951+48b2 ELF snapshots authenticated/2 positive+3 wrong declared-module refusals/'
      'exact code intervals,NO retiredPC/range-widening or hardware provenance claim')
