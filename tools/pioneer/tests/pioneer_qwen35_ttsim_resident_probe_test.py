#!/usr/bin/env python3
"""Guard the exact-image simulator probe against running graph commands."""
from pathlib import Path
import struct
import tempfile

from pioneer_qwen35_ttsim_service_test import validate_resident_normal_probe, validate_resident_normal_prefix

with tempfile.TemporaryDirectory(prefix='q35-resident-probe-') as temporary:
    image = Path(temporary)
    marker = image / 'profiled-compile-only.marker'; marker.touch()
    validate_resident_normal_probe(image, True, False, False, True, True, True, False)
    for flags in ((True, True, False, True, True, True, False),
                  (True, False, True, True, True, True, False),
                  (True, False, False, False, True, True, False),
                  (True, False, False, True, False, True, False),
                  (True, False, False, True, True, False, False),
                  (True, False, False, True, True, True, True)):
        try:
            validate_resident_normal_probe(image, *flags)
        except ValueError:
            pass
        else:
            raise AssertionError('unsafe resident probe mode accepted')
    qualifier = image / 'compact-qualification.marker'; qualifier.touch()
    try:
        validate_resident_normal_probe(image, True, False, False, True, True, True, False)
    except ValueError:
        pass
    else:
        raise AssertionError('qualification image accepted as resident')
    qualifier.unlink()
    fixture = image / 'fixture'; fixture.mkdir()
    (fixture / 'layout.txt').write_text('C 4 55 56 0\n')
    command = bytearray(128); struct.pack_into('<I', command, 20, 433)
    struct.pack_into('<2I', command, 32, 42, 0)
    (fixture / 'command.bin').write_bytes(command)
    templates = bytearray(120 * 512)
    for core in range(120):
        struct.pack_into('<I', templates, core * 512 + 32, 433)
    (fixture / 'templates.bin').write_bytes(templates)
    validate_resident_normal_prefix(image, True, False, False, True, True, fixture, False, False)
    for warm, other in ((True, False), (False, True)):
        try:
            validate_resident_normal_prefix(image, True, False, False, True, True, fixture, warm, other)
        except ValueError:
            pass
        else:
            raise AssertionError('unsafe normal-counter graph mode accepted')
    struct.pack_into('<I', templates, 73 * 512 + 32, 436)
    (fixture / 'templates.bin').write_bytes(templates)
    try:
        validate_resident_normal_prefix(image, True, False, False, True, True, fixture, False, False)
    except ValueError:
        pass
    else:
        raise AssertionError('profile-enabling ticket accepted')
    struct.pack_into('<I', templates, 73 * 512 + 32, 433)
    (fixture / 'templates.bin').write_bytes(templates)
    struct.pack_into('<I', command, 20, 436)
    (fixture / 'command.bin').write_bytes(command)
    try:
        validate_resident_normal_prefix(image, True, False, False, True, True, fixture, False, False)
    except ValueError:
        pass
    else:
        raise AssertionError('profile-enabling command accepted')
    marker.unlink()
    try:
        validate_resident_normal_probe(image, True, False, False, True, True, True, False)
    except ValueError:
        pass
    else:
        raise AssertionError('missing profile marker accepted')
print('PASS resident normal-counter startup/stop probe guards; NO HARDWARE')
