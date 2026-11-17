#!/usr/bin/env python3
"""Bind the guarded two-request vector92 resident trial. No device I/O."""
from hashlib import sha256
from pathlib import Path
import sys

from build_qwen38_head_csr_trial import bind_raw_pair

PACKAGE = {
    'vector': ('2026111700010210',
        'f696a2141ecdf9b998de5e425b3be065717ad31f70e8ec97a51457fa728d6dd6',
        'b1a25b91440ba5076be44ece81344cc30a15486a574ed2c942536552da5ba29b',
        'b08cff2bea137306e328b9066b6e62723751669998c6f84b5c206258f634e440'),
}


def bind(start):
    return bind_raw_pair('vector', start, PACKAGE, 'resident', 'vector resident',
                         'VECTOR_RESIDENT', resident=True)


def main():
    if not __debug__ or len(sys.argv) != 3:
        raise SystemExit('use OBSERVED_STABLE_GENERATION NEW_SCRIPT')
    generation, path = sys.argv[1:]
    output = bind(generation).encode('ascii')
    with Path(path).open('xb') as file:
        file.write(output)
    print(sha256(output).hexdigest(), path)


if __name__ == '__main__':
    main()
