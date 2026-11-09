#!/usr/bin/env python3
"""Synthetic stream and refusal checks; never accesses the Pioneer or disks."""
import hashlib
import importlib.util
import io
from pathlib import Path

path = Path(__file__).resolve().parent.parent / 'pioneer_mac_raw_backup.py'
spec = importlib.util.spec_from_file_location('pioneer_mac_raw_backup', path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class ShortWriter(io.BytesIO):
    def write(self, data):
        return super().write(data[:3])

data = b'abcdef0123456789' * 100
target = ShortWriter()
assert module.copy_stream(io.BytesIO(data), target, len(data)) == hashlib.sha256(data).hexdigest()
assert target.getvalue() == data
for content, size in [(data[:-1], len(data)), (data + b'x', len(data))]:
    try:
        module.copy_stream(io.BytesIO(content), io.BytesIO(), size)
    except RuntimeError:
        pass
    else:
        raise AssertionError('short or extra stream accepted')
compile(module.REMOTE, '<remote>', 'exec')
assert 'os.O_RDONLY|os.O_EXCL' in module.REMOTE
assert "path.open('xb'" in path.read_text()
print('PASS synthetic exact stream, partial writes, short/extra refusals and remote syntax; NO disk or device access')
