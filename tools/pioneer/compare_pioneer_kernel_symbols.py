#!/usr/bin/env python3
"""Compare a live exported-symbol inventory with one or more RISC-V ELF files.

An exact exported-symbol match is evidence about layout, not a full binary hash.
"""
import argparse
import collections
import hashlib
import json
import subprocess
from pathlib import Path

BIND = {'LOCAL': 0, 'GLOBAL': 1, 'WEAK': 2, 'UNIQUE': 10}
TYPE = {'NOTYPE': 0, 'OBJECT': 1, 'FUNC': 2, 'SECTION': 3,
        'FILE': 4, 'COMMON': 5, 'TLS': 6, 'IFUNC': 10}
SPECIAL = {'ABS': 0xfff1, 'COM': 0xfff2, 'UND': 0}


def safe_name(name):
    return bool(name) and all(33 <= ord(char) <= 126 for char in name)


def read_inventory(path, minimum):
    rows = collections.Counter()
    for line in path.read_text().splitlines():
        name, value, size, info, section = line.split('\t')
        if not safe_name(name):
            raise ValueError('invalid symbol name')
        row = (name, int(value, 16), int(size), int(info), int(section))
        if row[3] & 15 != TYPE['SECTION']:
            rows[row] += 1
    if len(rows) < minimum:
        raise ValueError('kernel inventory is unexpectedly small')
    return rows


def read_elf(readelf, path):
    output = subprocess.check_output([str(readelf), '--dyn-syms', '-W', str(path)], text=True)
    rows = collections.Counter()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 8 or not fields[0].endswith(':') or not fields[0][:-1].isdigit():
            continue
        symbol_type, binding, index = fields[3], fields[4], fields[6]
        if symbol_type not in TYPE or binding not in BIND:
            raise ValueError('unsupported ELF symbol type or binding')
        if symbol_type == 'SECTION':
            continue
        section = SPECIAL[index] if index in SPECIAL else int(index)
        name = fields[7].split('@')[0]
        if section and safe_name(name):
            rows[(name, int(fields[1], 16), int(fields[2]),
                  (BIND[binding] << 4) | TYPE[symbol_type], section)] += 1
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('readelf', type=Path)
    parser.add_argument('inventory', type=Path)
    parser.add_argument('references', nargs='+', type=Path)
    parser.add_argument('--minimum-symbols', type=int, default=1000,
                        help='minimum distinct inventory rows (default 1000 for a kernel)')
    args = parser.parse_args()
    if args.minimum_symbols < 1:
        parser.error('minimum symbols must be positive')
    runtime = read_inventory(args.inventory, args.minimum_symbols)
    results = []
    for path in args.references:
        reference = read_elf(args.readelf, path)
        missing = sum((runtime - reference).values())
        extra = sum((reference - runtime).values())
        results.append({'reference': str(path), 'reference_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                        'runtime_symbols': sum(runtime.values()), 'reference_symbols': sum(reference.values()),
                        'matching_symbols': sum((runtime & reference).values()),
                        'runtime_only': missing, 'reference_only': extra, 'exact_exported_symbols': not missing and not extra})
    print(json.dumps({'inventory_sha256': hashlib.sha256(args.inventory.read_bytes()).hexdigest(),
                      'results': results, 'proves_binary_hash': False}, indent=2))
    return 0 if any(item['exact_exported_symbols'] for item in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
