#!/usr/bin/env python3
"""Read-only saved OFFBOARD receipts; never a GPU/native admission test."""
import hashlib
import pathlib

assert __debug__
root = pathlib.Path(__file__).resolve().parents[3]
receipts = root / 'tools/pioneer/benchmarks/qwen38-q4-fpu-stream-offboard-20261104'
pins = {
    'source-sha256.txt': '019a888e8640ae20005bf9295c519fe0db1aade9fe707acc8bc18525081e6b25',
    'build5120.log': 'e0b9e39895fa845427df64700ee425e2cfe8bc1a83b151f9bd36f417586ba35c',
    'build17408.log': 'f4c1c1d3887d63f5d7be9ab59ec0719cbbedefc8b64a54a19dcb35943488d3db',
    'loader.log': '9f74eb111364aab84e91b864f52c7af59285ba4eb3efd9fcbf702819863a18e6',
    'pipeline.log': '882c9531e7cfb7d23167a22ef3583c00d6bb6b273f12e82aae26d56d70450ca8',
    'image5120.log': 'e1e252b9b751293d884213f6fa82bcc060bdc943e21f6ca4dea6054c448a49c6',
    'image17408.log': '4b166c454b0b192da7ea23674ab4ea2fc225cdb6bffdcc48bc608ca897e2f0c1',
    'fault.log': 'c0b2070bc46ca80b91b60e324a51e74b3b809d304f9d8d113bd7e34fbf14a2e9',
    'fraction.log': '3db435027326cbad4458362edf3a2ddedf43a4cbd1806cd43c403bf43af38f39',
    'model.log': '1cbb6c455ac81510e8e841af867c902391de3a0293c5ce98f9d58767345728a5',
}
for tree, regression_sha in (
    ('main', 'ba6c74f25de3b6e31b6fa95996e6aa36a96134591886ff9616dcfbafeebe09a3'),
    ('clean', '91a5d14b49b169594c250651dea650a2f2d58d7d742079c09247286127313029'),
):
    directory = receipts / tree
    for name, expected in {**pins, 'old-regression.log': regression_sha}.items():
        p = directory / name
        assert p.is_file() and not p.is_symlink()
        assert hashlib.sha256(p.read_bytes()).hexdigest() == expected, p
    for name in ('loader', 'pipeline', 'image', 'fault', 'model'):
        assert (directory / (name + '-compile.log')).read_bytes() == b''
    children = ('build5120', 'build17408', 'loader-compile', 'pipeline-compile',
                'image-compile', 'fault-compile', 'model-compile', 'loader',
                'pipeline', 'image5120', 'image17408', 'fault', 'fraction',
                'model', 'old-regression')
    assert len((directory / 'test-argv.txt').read_text().splitlines()) == len(children) == 15
    for name in children:
        assert (directory / (name + '.stderr')).read_bytes() == b''
    source_lines = (directory / 'source-sha256.txt').read_text().splitlines()
    assert len(source_lines) == 15
    for line in source_lines:
        digest, path = line.split('  ')
        assert path.startswith('tools/pioneer/') and '..' not in pathlib.PurePosixPath(path).parts
        assert hashlib.sha256((root / path).read_bytes()).hexdigest() == digest, path
    old = (directory / 'old-regression.log').read_text()
    assert '7e3efc36b1447a3880081caf899175a87378f894ea684d7623572625242e04d4 7036 text.bin' in old
    assert 'all64 words exact,13 failure/numerical controls,strict unchanged Step' in old
print('PASS read-only saved OFFBOARD Q4 full-K stream receipts/main+clean/current15 source hashes;'
      '15 subprocess records/stderr0,576 real-source exact words,128 independent Fraction words;'
      'NO new execution/native/hardware/quality/speed/admission authority')
