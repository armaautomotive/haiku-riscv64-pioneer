#!/usr/bin/env python3
"""TEST byte identities only, not native mapping/initialization/ownership."""
import hashlib
import pathlib
import sys
from pioneer_q38_resident_layer_image_common import PROGRAM_PINS, verify as verify_layer

PINS = tuple((name, size, digest) for name, size, digest in PROGRAM_PINS if name != 'text') + (
    ('text', 128, 'a5b5e13516a96e24d5029ebcf916ab6b44ea1052bf484a8bad73cb8d1a225724'),
    ('link', 5976, 'fb9e6d3368601f65883b49c3f964257370712acac9e37112cda4a16dc041e8b6'),
)
def verify(image):
    image = pathlib.Path(image)
    peer = image/'unchanged-layer/unchanged-output/exact-companion'
    verify_layer(image/'unchanged-layer', peer)
    for section, size, digest in PINS:
        data = (image/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, section
    return peer

if __name__ == '__main__':
    assert len(sys.argv) == 2
    verify(sys.argv[1])
