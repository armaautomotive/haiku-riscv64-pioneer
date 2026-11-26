#!/usr/bin/env python3
"""Reject cross-use of compact simulator and resident qualification images."""
from pathlib import Path
import subprocess
import sys
import tempfile

from pioneer_qwen35_ttsim_service_test import validate_compact_qualification

with tempfile.TemporaryDirectory(prefix='q35-qualification-markers-') as temporary:
    image = Path(temporary)
    validate_compact_qualification(image, False, False, False, False, False, False)
    markers = [image / 'compact-qualification.marker', image / 'ttsim-qualification.marker']
    for marker in markers:
        marker.touch()
    validate_compact_qualification(image, True, True, True, True, True, False)
    for flags in ((False, True, True, True, True, False),
                  (True, False, True, True, True, False),
                  (True, True, False, True, True, False),
                  (True, True, True, False, True, False),
                  (True, True, True, True, False, False),
                  (True, True, True, True, True, True)):
        try:
            validate_compact_qualification(image, *flags)
        except ValueError:
            pass
        else:
            raise AssertionError('unsafe compact qualification mode accepted')
    markers[1].unlink()
    try:
        validate_compact_qualification(image, True, True, True, True, True, False)
    except ValueError:
        pass
    else:
        raise AssertionError('missing simulator-only marker accepted')
    validate_compact_qualification(image, True, False, True, True, True, False, True)
    for flags in ((False, False, True, True, True, False, True),
                  (True, True, True, True, True, False, True),
                  (True, False, True, True, False, False, True),
                  (True, False, True, True, True, True, True)):
        try:
            validate_compact_qualification(image, *flags)
        except ValueError:
            pass
        else:
            raise AssertionError('invalid normal-counter probe accepted')
    markers[1].symlink_to(markers[0])
    try:
        validate_compact_qualification(image, True, True, True, True, True, False)
    except ValueError:
        pass
    else:
        raise AssertionError('symlink marker accepted')
    markers[1].unlink()
    (image / 'profiled-compile-only.marker').touch()
    runner = Path(__file__).with_name('pioneer_qwen35_ttsim_service_test.py')
    args = [sys.executable, '-B', str(runner), str(image / 'unused.so'), str(image), str(image),
            '--flash-hybrid', '--compact-two-region', '--compact-qualification',
            '--projection-fixture', str(image)]
    result = subprocess.run(args, capture_output=True, text=True)
    assert result.returncode != 0 and 'profiled compact image is compile-only' in result.stderr
    (image / 'profiled-compile-only.marker').unlink()
    result = subprocess.run(args + ['--qualification-normal-counter-probe'], capture_output=True, text=True)
    assert result.returncode != 0 and 'requires the profiled compile-only image marker' in result.stderr
print('PASS qualification simulator mode and marker refusals; NO HARDWARE')
