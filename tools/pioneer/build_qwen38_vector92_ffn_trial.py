#!/usr/bin/env python3
"""Bind one exact stable generation and a pinned FFN rate variant. No I/O to the board."""
from hashlib import sha256
from pathlib import Path
import sys

from build_qwen38_head_csr_trial import bind_raw_pair

PACKAGES={
    'refetch': ('2026111700010211',
        '36643b63b2cdda95db5b65cbc8ee6fd5304d46a6295ed493e4d20af76d5e4abf',
        '13c7fed8afb53452499c5ee63555ac6c9ee1d0ab6a6d558e9936bc3326c949d5',
        'e90c9d53e8b984654b700472c59ca1cb1e5af75b16546a878238c1af03a76b85'),
    'reuse': ('2026111700010212',
        '24242b69f8bb0c792d65f76973249b8b50b0421cc2e35ed2e7a0c96a62e4ee06',
        '267ea6a5942238a5b1d33122d9b333d23a58d34784836e4924efc054d3dfa8dc',
        '1a1cff23535031b8c26a375391e2f50e953b7776f0bf0243c76f0f83fbf6d25f'),
}


def bind(variant,start):
    return bind_raw_pair(variant,start,PACKAGES,'ffn-retained','retained FFN','FFN_RETAINED')


def main():
    if not __debug__ or len(sys.argv)!=4:
        raise SystemExit('use refetch|reuse OBSERVED_STABLE_GENERATION NEW_SCRIPT')
    variant,start,path=sys.argv[1:]
    bound=bind(variant,start).encode('ascii')
    with Path(path).open('xb') as dest:
        dest.write(bound)
    print(sha256(bound).hexdigest(),path)


if __name__=='__main__':
    main()
