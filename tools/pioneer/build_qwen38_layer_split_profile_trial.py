#!/usr/bin/env python3
"""Bind the guarded v4 diagnostic to an observed stable generation. No I/O to the board."""
from hashlib import sha256
from pathlib import Path
import sys

from build_qwen38_head_csr_trial import bind_raw_pair

PACKAGE={
    'candidate': ('2026111700010213',
        '4b6f208da0ed2960d7d3e9a6617f3e354737b9ec4b7b7c4cf0c62d25a3fbd9ec',
        '5c650f81142f5fc38c8f6ac24cd62ef9c4136b65144a750c47a8c8a01af148f6',
        'ec90ba4575b78457c64d72a43846346b64923dc16e7aee1e1b6a808361a37889'),
    'live': ('2026111700010214',
        'dacf779941dfdb94abe2ef5391519896b641a5812b43b7d78eb88cd4a274f2fc',
        '6ccb9f48445671ba9845a4c4cf5f4c3d58e773c72bf946c9f5be0232e8f78dc1',
        'c334f4e5ea0f3a2f3f02e5acee5a7126e96b401212d022b746a55922a7c5741b'),
}


def bind(start,live=False):
    assert type(live) is bool
    return bind_raw_pair('live' if live else 'candidate',start,PACKAGE,'layer-split','layer split profile','LAYER_SPLIT')


def main():
    live=len(sys.argv)==4 and sys.argv[3]=='--live-image-check'
    if not __debug__ or (len(sys.argv)!=3 and not live):
        raise SystemExit('use OBSERVED_STABLE_GENERATION NEW_SCRIPT [--live-image-check]')
    generation,path=sys.argv[1:3]
    output=bind(generation,live).encode('ascii')
    with Path(path).open('xb') as dest:
        dest.write(output)
    print(sha256(output).hexdigest(),path)


if __name__=='__main__':
    main()
