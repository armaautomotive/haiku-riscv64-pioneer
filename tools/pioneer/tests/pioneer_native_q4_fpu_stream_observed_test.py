#!/usr/bin/env python3
"""READ ONLY saved native86 OFFBOARD receipts;NO GPU/admission authority."""
import hashlib
import pathlib

assert __debug__
root = pathlib.Path(__file__).resolve().parents[3]
base = root / 'tools/pioneer/benchmarks/qwen38-native-q4-fpu-stream-offboard-20261104'
common = {
    'source-sha256.txt': 'ddacdade60bc60f232419bef5f679f876e48019c777cd121828ef419fb4c4af6',
    'native-source-sha256.txt': '0b723e7bf56fbb0ff95c29379130e3e7c4041c7aecfcd41d9612abf0c337772b',
    'sdk.log': '1340e55da0413628a9421494e9647ddc3b1e3230988cce23f083c865d23cbb79',
    'contract.log': '594774f7fe1eefb01223fc6bd2f7ea7d132dcccb0a1a76b21a42e87f62ac8cb2',
    'lease.log': '4fef0275bc83d4a190c122e88fd0d9e70f9024a27725bc03af1af7b1b215e613',
    'real_lease.log': 'bd7ce88e009c5212b3e70071055e9bf1d6bdc4bfc1dcb49763b3aa76fa64d7aa',
}
for tree, native_sha in (
    ('main', '7d0787cba952ac4c89fea3eef9dc0ff8ddc2bdcbf38b23a25b46d885a9edc4a3'),
    ('clean', '3dd11d221f06ea78165bf36c8b4dc41ec894e56de035305022fb2f985e32fc93'),
):
    directory = base / tree
    for name, digest in {**common, 'native.log': native_sha}.items():
        p = directory / name
        assert p.is_file() and not p.is_symlink()
        assert hashlib.sha256(p.read_bytes()).hexdigest() == digest, p
    for manifest in ('source-sha256.txt', 'native-source-sha256.txt'):
        lines = (directory / manifest).read_text().splitlines()
        assert len(lines) == (14 if manifest == 'source-sha256.txt' else 9)
        for line in lines:
            digest, path = line.split('  ')
            assert path.startswith(('tools/pioneer/', 'src/add-ons/kernel/drivers/misc/'))
            assert '..' not in pathlib.PurePosixPath(path).parts
            assert hashlib.sha256((root / path).read_bytes()).hexdigest() == digest, path
    labels = ('sdk', 'contract-compile', 'contract', 'lease-compile', 'lease',
              'real_lease-compile', 'real_lease', 'native')
    assert len((directory / 'test-argv.txt').read_text().splitlines()) == len(labels) == 8
    for label in labels:
        assert (directory / (label + '.stderr')).read_bytes() == b''
        if label.endswith('-compile'):
            assert (directory / (label + '.log')).read_bytes() == b''
    assert len((directory / 'driver-role-refusals.txt').read_text().splitlines()) == 19
    assert len((directory / 'placement-compile-refusals.txt').read_text().splitlines()) == 2
    assert 'valid317 refused673' in (directory / 'native.log').read_text()
    assert '55 failure/refusal cases' in (directory / 'lease.log').read_text()
    assert 'lease576 exact words over9 bands' in (directory / 'real_lease.log').read_text()
print('PASS READ-ONLY saved native86 OFFBOARD main/clean/14 current sources/native bodies/'
      '8 strict child records each;NO new execution/native ARC/physical GPU/model quality/'
      'speed/hardware-admission authority')
