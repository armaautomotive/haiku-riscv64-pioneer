#!/usr/bin/env python3
"""Authenticate actual ROOT ELF DATA bindings;NO device or native execution."""
import os
import pathlib
import sys
import inspect_pioneer_q38_frontend_profile as profile

assert len(sys.argv) in (3, 4) and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
old, retained = map(pathlib.Path, sys.argv[1:3])
old_driver = '9ad9e86bd5e45c5cb11dfe286f3528452dfe7b0e4630f67546acbb43a03064e8'
retained_driver = '1f3bcff838ff9f95d5f450c53000f02f80ddcd073aa5a5cc3018ee4208e0847c'
cases = ((old, old_driver), (retained, retained_driver))
if len(sys.argv) == 4:
    cases += ((pathlib.Path(sys.argv[3]), '78322cfa94db2519284efa12ad69cfa77981446f03081fc95aee2ee6234de8d1'),)
for elf, driver in cases:
    ranges = profile.authenticated_ranges(elf, driver)
    assert any(name.startswith('GlobalChain::Scope(') for _, _, name in ranges)
    assert any(name.startswith('GlobalChain::Packet(') for _, _, name in ranges)
    assert any(name.startswith('MixedRecurrentCohort::') for _, _, name in ranges)
    assert profile.bins([{'first_raw': 0x60008, 'second_raw': 0xffffffff}], ranges) == {'UNMAPPED_RAW_ADDRESS': 2}
denied = 0
wrong = tuple((elf, other_driver) for elf, driver in cases for _, other_driver in cases if driver != other_driver)
wrong += ((cases[-1][0], '0'*64),)
for elf, driver in wrong:
    try:
        profile.authenticated_ranges(elf, driver)
    except ValueError:
        denied += 1
    else:
        raise AssertionError('wrong declared binding accepted')
assert denied == len(cases)*(len(cases)-1)+1
print('PASS frontend symbolizer actual ROOT ELF snapshots authenticated/' + str(len(cases)) + ' positive+'
      + str(denied) + ' wrong declared-module refusals/exact code intervals/RAW60008+FFFFFFFF unmapped;'
        'NO retiredPC/range-widening,native execution or hardware provenance claim')
