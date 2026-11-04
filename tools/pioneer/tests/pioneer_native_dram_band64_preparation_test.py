#!/usr/bin/env python3
"""Immutable84 C64 arithmetic PREPARATION,not a physical/model/speed pass."""
import hashlib
import pathlib
import re

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-band64-preparation-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and b'\0' not in path.read_bytes()
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
assert len(records) == 64 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for name in records:
    if name.endswith('.stderr'):
        if name in ('native82-final.stderr', 'native83-final.stderr'):
            assert "use of undeclared identifier 'PioneerNativeDramModelBandVariant'" in text(name)
            assert 'CalledProcessError' in text(name)  # Preserve the actual fixture failure.
        else:
            assert not text(name), name
build = text('build84-qualified.log')
for pin in ('2cb420909e36182df421058853d706b50d7e819357496141eba385787d49cd1e',
            '9aad44e18f6c88e8c8c4e8cddb7234b4a576d3362b16dfb98e5c2fee364e79a3',
            '7b3ca669be8d9e0714f544bd0d74c045329b0490d8f2132187181a1a92be55cd',
            '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'):
    assert pin in build
assert '19 forbidden roles/2 actual cached-policy/image compiler refusals' in build
assert len(text('driver-role-refusals84.txt').splitlines()) == 19
assert len(text('driver-role-refusals81.txt').splitlines()) == 16
assert text('placement-compile-refusals84.txt') == text('placement-compile-refusals81.txt') == (
    'PASS cached-opposite-math-policy-exact-driver-binding\nPASS opposite-math-image-full-driver-instantiation\n')
image = text('image84.log')
assert image.count('C64 ACTUAL descriptors/SAME NIU-unpack L1/wholeL1 guards') == 3
assert image.count('NO normalization/shadow payload copy/scalar payload access') == 3
assert '20 failure controls' in image and '42548783 steps' in image
for pin in ('7b3ca669be8d9e0714f544bd0d74c045329b0490d8f2132187181a1a92be55cd',
            '4fe68cd1c14973761b75596fb966f99b98132df2b7dda3e6808787837b24c0c5',
            '26b4fd4665819d16b549ceb8f9d25e6488f4f6b3ac435d3fcfeab0bfaa7695fe'):
    assert pin in image
lease = text('lease84-full.log')
assert '251 failure/admission/close cases' in lease
assert lease.count('complete221488B L1+184320B GDDR') == 3
for sample in range(3):
    assert lease.count('PASS OFFBOARD actual GDDR learned-band lease sample%d:' % sample) == 1
for name, needle in (
        ('native84-final.log','native84 endpoint fixtures:valid307 refused659'),
        ('client84-final.log','client:659 ASan/UBSan cases'),
        ('parent84-final.log','624 flow/output+7 idle+13 prior/source+20 exchange-step+10 one-shot cases'),
        ('parser84.log','independent84 child parser:1 valid/624 refused'),
        ('role84-final.log','learned-band84 roles14 cases,protocol352 dirty-result-byte refusals'),
        ('role81-final.log','learned-band81 roles12 cases'),
        ('native81-final.log','native81 endpoint fixtures:valid307 refused643'),
        ('client81-final.log','client:641 ASan/UBSan cases'),
        ('parent81-final.log','620 flow/output+7 idle+13 prior/source+20 exchange-step+10 one-shot cases'),
        ('parser81-final.log','independent81 child parser:1 valid/622 refused'),
        ('native83-corrected.log','LOAD ONLY83 endpoint fixtures:valid308 refused669'),
        ('native82-corrected.log','LOAD ONLY82 endpoint fixtures:valid308 refused665'),
        ('lease83-final.log','258 mismatch/failure/admission cases'),
        ('lease82-final.log','162 mismatch/failure/admission cases'),
        ('lease81-final.log','35 failure/admission/close cases')):
    assert needle in text(name), name
for name in ('loader84.log','loader83.log','loader82.log'):
    assert '27 valid/36 refused' in text(name) and 'PASS DRAM loader3 forbidden production roles' in text(name)
for pin in ('0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27',
            '0091244ae265e26075e80fa98f5d6b8b5c6d37304938e3d140d9bab6584e307e'):
    assert pin in text('control81-frozen.log')
for pin in ('cd924896986dabcead8493c9dedca1d7b25fde323fbd972caf67ad93139a9309',
            '13494a0a5fc262b02dc7b7a6353da5e773a5ec01d732efd29d6525ba58a6e5f2',
            'ee740295aeffdb555c5fa0706d092e6d1a467b38b6351bc067bbfe7edbb57a45'):
    assert pin in text('image81-final.log')
for pin in ('7ee39a3615bc93e3b81cd21149fdd020f2e5f3e7a7f1b3db8cca24bffd94a85e',
            '10310865e07c34b476632e8e776f76fae603a7a4adea2ec87f226f1f05798ac0',
            'fa70be17ca4b3cf3889e5176f6a651b2f5afcdefd0511f98aba4745d6c2b04b7'):
    assert pin in text('control83.log')
for pin in ('3df4fdb1d034c39df5c24bc06735012109d3881431c5a9ddaedd16ecc535d3d6',
            'f7ae6e1be5fbcf6618ddd4326715c49166169a0073f868a86ec353519bc4f719',
            '966507e23929a1d28f62f56de0ea3c3a6c9148d9ff8f59c84138f746eea779a0'):
    assert pin in text('control82.log')
assert '3 driver/Work/validation selections and implicit2-phase variant refused' in text('legacy-band.log')
preflight = text('preflight-readonly.log')
assert preflight.startswith('Haiku\nP100A_RELOAD driver55 tag2026102100010012 generation4 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY\n')
assert '64 (Unknown)' in preflight and '/ 137034936320)' in preflight
assert 'STABLE041_RESTORED_LOAD64_ONLY_PAYLOAD_AND_HEADERS_PASSED' in preflight
assert 'a8884f789db23da897578ffb73a1c0849fa8e1e972f6e7701d10647b51df698c' in preflight
assert 'ORIGINAL_METADATA_0:1:600:18973870528' in preflight
assert preflight.endswith('READONLY_NORMAL041_GEN4_64CPU_FULLRAM_C64_TRANSFER_PRESERVED_84_STAGE_ABSENT_NO_GPU_OR_WRITES\n')
source_pins = text('source-SHA256SUMS').splitlines()
assert len(source_pins) == 30
for row in source_pins:
    digest, path = row.split('  ',1)
    assert re.fullmatch('[0-9a-f]{64}',digest) and path.startswith(('src/add-ons/kernel/drivers/misc/','tools/pioneer/'))
    assert '..' not in pathlib.Path(path).parts and not path.endswith(('.bin','.o','.gguf','.cache'))
assert hashlib.sha256((here / 'pioneer_native_dram_model_band64_physical.sh').read_bytes()).hexdigest() == 'df6675b40f0e1313bb2c1c93faaff56cb34ee223dd15eef9bfbd0477e1fe5ad8'
assert '32071 qualified SDK84 driver/client+19 roles/2 compiler refusals CLOSED0' in text('terminal-status.txt')
assert 'No84 staging/driver exchange/GPU launch claimed' in text('terminal-status.txt')
print('PASS64 immutable84 C64 learned-FPU PREPARATION receipts:actual-address sharedL1/all3 widths/251 lease faults/659 client/native307-659;legacy binaries unchanged;NO84 hardware/model/speed proof')
