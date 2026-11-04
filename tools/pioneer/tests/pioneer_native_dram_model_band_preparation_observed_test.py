#!/usr/bin/env python3
"""Immutable pre-deployment client/parent receipts;NOT hardware/model/speed proof."""
import hashlib
import pathlib
import re

assert __debug__, 'Assertions required'
here = pathlib.Path(__file__).resolve().parents[1]
folder = here / 'benchmarks/qwen38-native-dram-client-preparation-20261104'
records = {}
for row in (folder / 'SHA256SUMS').read_text().splitlines():
    digest, name = row.split('  ', 1)
    assert re.fullmatch('[0-9a-f]{64}', digest) and pathlib.Path(name).name == name and name not in records
    path = folder / name
    assert path.is_file() and not path.is_symlink() and path.suffix in ('.log', '.stderr', '.txt')
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
    records[name] = digest
assert len(records) == 16 and set(records) == {p.name for p in folder.iterdir() if p.name != 'SHA256SUMS'}
def text(name):
    assert name in records
    return (folder / name).read_text()
for stem in ('client-r1', 'client-r2', 'build-r1', 'build-r2', 'parent-r1', 'parent-r2', 'native-regression'):
    assert not text(stem + '.stderr')
assert 'client:641 ASan/UBSan cases' in text('client-r2.log') and '3 role guards;NO device proof' in text('client-r2.log')
assert '620 flow/output+7 idle+13 prior/source+20 exchange-step+10 one-shot cases' in text('parent-r2.log')
assert 'valid307 refused643' in text('native-regression.log')
build = text('build-r2.log')
assert '0ab145ac3b938238fd3fd139fa8c1e899458932058acd2567f220bb4504cdf27 tenstorrent_pci_probe-native-dram-model-band81' in build
assert '0091244ae265e26075e80fa98f5d6b8b5c6d37304938e3d140d9bab6584e307e pioneer-native-dram-model-band-diagnostic' in build
assert '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc tenstorrent_pci_probe-default43-control' in build
assert 'driver/client/all methods LINKED' in build and 'NO install/endpoint tests/hardware/full-model/speed authority' in build
assert len(text('driver-role-refusals.txt').splitlines()) == 14
parent = here / 'pioneer_native_dram_model_band_physical.sh'
assert hashlib.sha256(parent.read_bytes()).hexdigest() == '577aa3cc051b036f815a94e519b98489fbc6793a4c0e47ee78e7f8539cc4650a'
print('PASS16 immutable diagnostic81 PREPARATION receipts;641 client/620 output-flow+exchange/one-shot fixtures,actual SDK link,unchanged native driver;NO physical trial/full-model/speed proof')
