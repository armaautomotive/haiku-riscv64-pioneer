#!/usr/bin/env python3
"""Check the generated qualification profile and unchanged recovery control."""
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[3]
source = repo / 'tools/pioneer/pioneer_linux_boot_haiku_smp.sh'
builder = repo / 'tools/pioneer/build_qwen35_pin_boot_helper.py'
with tempfile.TemporaryDirectory(prefix='q35-pin-boot-helper-') as tmp:
    path = Path(tmp)
    helper = path / 'helper.sh'
    subprocess.run(['python3', str(builder), str(source), str(helper)], check=True)
    text = helper.read_text()
    profile = 'memory128-spin-idle7-pin-nosd-qualification'
    control = 'memory128-spin-idle7-top-page-nosd-control|memory128-spin-idle7-top-page-nosd-journal-control'
    reverted = text.replace(control + '|' + profile, control, 3)
    begin = reverted.index(' ' + profile + ')\n')
    end = reverted.index(' ' + control + ')\n', begin)
    assert reverted[:begin] + reverted[end:] == source.read_text()
    start = text.index('# PIONEER_PRIVATE_PROFILE_FLAGS_BEGIN')
    end = text.index('# PIONEER_PRIVATE_PROFILE_FLAGS_END')
    (path / 'flags').write_text(text[start:end])
    start = text.index('base=/mnt/ssd/pioneer-haiku-kexec-20261013\n')
    end = text.index('\nloader=', start)
    (path / 'artifacts').write_text(text[start:end] + '\n')
    script = '''set -eu
session=offline
profile=memory128-spin-idle7-pin-nosd-qualification
. "$1/flags"
[ "$private_nosd:$journal_view" = 1:0 ]
. "$1/artifacts"
[ "$memory_gib" = 128 ]
[ "$bundle" = "$base/q35-pin-service-20261125/candidate-ram.bfs" ]
[ "$bundle_hash" = 3bc902182084edd40b8b98032a57e2f86f44b362be144117e08d0373763d30bf ]
[ "$image_hash" = b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98 ]
profile=memory128-spin-idle7-top-page-nosd-control
. "$1/artifacts"
[ "$bundle_hash" = 233a2d662c52e7ac6435da49a9202e53f66be2eb914f79bf140eaed2f04f2a67 ]
[ "$bundle" = "$base/smp-kernel-bundle-memory-spin-idle-top-page-7/samsung-ram.bfs" ]
'''
    subprocess.run(['sh', '-c', script, 'test', str(path)], check=True)
    # The new profile must be rejected before any platform access for 2 CPUs.
    rejected = subprocess.run(['sh', str(helper), 'prepare', 'test', '2',
                               'memory128-spin-idle7-pin-nosd-qualification'], capture_output=True)
    assert rejected.returncode != 0 and b'requires 64 CPUs' in rejected.stderr
    # Reusing an existing output and using an altered baseline must fail.
    assert subprocess.run(['python3', str(builder), str(source), str(helper)], capture_output=True).returncode != 0
    altered = path / 'altered.sh'
    altered.write_bytes(source.read_bytes() + b'\n')
    assert subprocess.run(['python3', str(builder), str(altered), str(path / 'bad')], capture_output=True).returncode != 0
    ecam = path / 'ecam-helper.sh'
    subprocess.run(['python3', str(builder), str(source), str(ecam), '--ecam-qualification'], check=True)
    generated = ecam.read_text()
    profile = 'memory128-spin-idle7-ecam-nosd-qualification'
    reverted = generated.replace(control + '|' + profile, control, 3)
    begin = reverted.index(' ' + profile + ')\n')
    end = reverted.index(' ' + control + ')\n', begin)
    assert reverted[:begin] + reverted[end:] == source.read_text()
    start = generated.index('# PIONEER_PRIVATE_PROFILE_FLAGS_BEGIN')
    end = generated.index('# PIONEER_PRIVATE_PROFILE_FLAGS_END')
    (path / 'flags').write_text(generated[start:end])
    start = generated.index('base=/mnt/ssd/pioneer-haiku-kexec-20261013\n')
    end = generated.index('\nloader=', start)
    (path / 'artifacts').write_text(generated[start:end] + '\n')
    ecam_script = script.replace('memory128-spin-idle7-pin-nosd-qualification', profile)
    ecam_script = ecam_script.replace('q35-pin-service-20261125', 'q35-ecam-service-20261125')
    ecam_script = ecam_script.replace('3bc902182084edd40b8b98032a57e2f86f44b362be144117e08d0373763d30bf',
                                      'ed37c2b13f940dad5561553b88c5acdaf3bdda60cb83c87d7e5557fb042cf79f')
    subprocess.run(['sh', '-c', ecam_script, 'test', str(path)], check=True)
    rejected = subprocess.run(['sh', str(ecam), 'prepare', 'test', '2', profile], capture_output=True)
    assert rejected.returncode != 0 and b'requires 64 CPUs' in rejected.stderr
print('PASS separate candidate boot profile, pinned control and refusal checks; no target access')
