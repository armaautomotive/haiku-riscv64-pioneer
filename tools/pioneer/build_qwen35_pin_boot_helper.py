#!/usr/bin/env python3
"""Make a separate pinned candidate helper from the recorded recovery helper.

Build only: no target upload, kexec, reboot, driver operation or card access.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess

BASE = '8e85463e4de06fc8b35d0970e697c310f9532df30747c58cc892a1e9fdd9adff'
OLD = 'memory128-spin-idle7-top-page-nosd-control|memory128-spin-idle7-top-page-nosd-journal-control'
PROFILE = 'memory128-spin-idle7-pin-nosd-qualification'
IMAGE = '3bc902182084edd40b8b98032a57e2f86f44b362be144117e08d0373763d30bf'
ECAM_PROFILE = 'memory128-spin-idle7-ecam-nosd-qualification'
ECAM_IMAGE = 'c4f7223d4f7542068b7c58ecaf4aca9f9a1321ae92ed042898af0d47e96d47f3'
CONTENTION_PROFILE = 'memory128-spin-idle7-ecam-contention-nosd-qualification'
CONTENTION_IMAGE = 'fdc7539b245e64ce12b5db8f28abc037210ecb2b4f7c539b0f1c8e02fc019dba'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('recorded_helper', type=Path)
    parser.add_argument('new_absolute_output', type=Path)
    variant = parser.add_mutually_exclusive_group()
    variant.add_argument('--ecam-qualification', action='store_true',
                        help='select the separate fixed-probe ECAM qualification bundle')
    variant.add_argument('--ecam-contention', action='store_true',
                         help='select the separate bounded contention qualification bundle')
    args = parser.parse_args()
    source = args.recorded_helper.read_bytes()
    if hashlib.sha256(source).hexdigest() != BASE:
        raise ValueError('recorded recovery helper differs from the pinned baseline')
    if not args.new_absolute_output.is_absolute():
        raise ValueError('output path must be absolute')
    text = source.decode('utf-8')
    if text.count(OLD) != 4:
        raise ValueError('unexpected recovery profile layout')
    profile = CONTENTION_PROFILE if args.ecam_contention else ECAM_PROFILE if args.ecam_qualification else PROFILE
    image = CONTENTION_IMAGE if args.ecam_contention else ECAM_IMAGE if args.ecam_qualification else IMAGE
    bundle_dir = ('q35-ecam-contention-20261125' if args.ecam_contention else
                  'q35-ecam-service-20261125' if args.ecam_qualification else 'q35-pin-service-20261125')
    # Admit the qualification profile, enforce no-SD and require 64 CPUs.
    text = text.replace(OLD, OLD + '|' + profile, 3)
    needle = ' ' + OLD + ')\n'
    if text.count(needle) != 1:
        raise ValueError('cannot uniquely locate the recorded artifact branch')
    candidate = (' ' + profile + ')\n'
                 '  # Separate candidate only; the recorded control is unchanged.\n'
                 '  image=$base/memory-2/HaikuPioneerKexecProbe.Image\n'
                 '  image_hash=b5473e6d00a9bcf469fce648b4415f28eda2dbc2510bfc9ed8d61af9034e6e98\n'
                 '  bundle=$base/' + bundle_dir + '/candidate-ram.bfs\n'
                 '  bundle_hash=' + image + '\n'
                 '  memory_gib=128\n'
                 '  ;;\n')
    text = text.replace(needle, candidate + needle)
    fd = os.open(args.new_absolute_output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o700)
    try:
        with os.fdopen(fd, 'w') as output:
            output.write(text)
        subprocess.run(['sh', '-n', str(args.new_absolute_output)], check=True)
    except BaseException:
        args.new_absolute_output.unlink(missing_ok=True)
        raise
    print('helper_sha256=' + hashlib.sha256(args.new_absolute_output.read_bytes()).hexdigest())
    print('PASS private helper generated; not uploaded, loaded or booted')


if __name__ == '__main__':
    main()
