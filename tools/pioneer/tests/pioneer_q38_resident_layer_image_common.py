#!/usr/bin/env python3
"""TEST byte identities ONLY;NO native image selection/ownership authority."""
import hashlib
import pathlib
import runpy

PROGRAM_PINS = (
    ('boot', 4, 'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
    ('text', 128, '98a89f90feb6fdd4bf6a072db9ef38064a6089d99d63b09d38d6af1c4ae495d3'),
    ('conv', 6472, '479cb028a09ff252c8df32b279facfbf295d1b7cd72a13d2966272406398da51'),
    ('matrix', 14136, '1d2e2e7e4c8f806ff2983e5d9a2ccbd1a4d177940c7e23d73b6de33bae7e7559'),
    ('cohort', 59288, 'fca3da36bc93a1976c2f4609b423c8023a51661f58cc8267848f2e7bbe55c0a7'),
    ('norm', 5404, '60f44ff5f83914fedcaee02d70dd0c1a821edfc4a150e4406430d82d30dbeef1'),
    ('ffn', 30496, 'e0c5de18d44c1ecce93b9e3282e853de1943da2a4ffae7c6ef482d0493ad9f3f'),
)


def verify(image, peer):
    image, peer = pathlib.Path(image), pathlib.Path(peer)
    for section, size, digest in PROGRAM_PINS:
        data = (image/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, section
    directory = pathlib.Path(__file__).resolve().parents[1]
    for name, (section, size, digest, _) in runpy.run_path(str(directory/'generate_qwen38_resident_output_images.py'))['PINS'].items():
        if name.startswith('peer-'):
            data = (peer/(section+'.bin')).read_bytes()
            assert len(data) == size and hashlib.sha256(data).hexdigest() == digest, name
