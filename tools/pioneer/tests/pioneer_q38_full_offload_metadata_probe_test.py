#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[3]
source = (root / 'tools/pioneer/pioneer_q38_full_offload_metadata_probe.cpp').read_text()
assert 'kReloadStatus = B_DEVICE_OP_CODES_END + 0x61e0' in source
assert 'kFullOffloadSupport = B_DEVICE_OP_CODES_END + 0x8800' in source
assert 'ioctl(fd, kReloadStatus' in source and 'ioctl(fd, kFullOffloadSupport' in source
assert source.index('ioctl(fd, kReloadStatus') < source.index('ioctl(fd, kFullOffloadSupport')
assert 'open("/dev/misc/tenstorrent_pci_probe", O_RDWR' in source
assert 'Only metadata ioctls' in source
assert 'NO_GENERATE' not in source and 'Generate(' not in source
for forbidden in ('0x61e1', 'Q38_FULL_OFFLOAD_CONTROL', 'Q38_PREPARED_SOURCE', 'ioctl(fd, B_DEVICE_OP_CODES_END + 0x8801'):
    assert forbidden not in source, forbidden
print('PASS metadata probe calls only reload-status and full-offload-support')
