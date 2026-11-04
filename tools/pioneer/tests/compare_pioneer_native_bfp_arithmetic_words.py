#!/usr/bin/env python3
"""Read already-returned diagnostic words;NO device/END/retry/acceptance authority."""
import argparse
import contextlib
import io
import pathlib
import re
import runpy

if not __debug__:
    raise SystemExit('Assertions required')

def compare(text, bits):
    here = pathlib.Path(__file__).resolve().parent
    with contextlib.redirect_stdout(io.StringIO()):
        arithmetic = runpy.run_path(str(here / 'pioneer_native_bfp_arithmetic_test.py'))
    words = {}
    for line in text.splitlines():
        if not line.startswith('NATIVE_BFP_RETURNED_WORD'):
            continue
        match = re.fullmatch(r'NATIVE_BFP_RETURNED_WORD bits(4|8) pass([01]) row([0-9]+) got([0-9a-f]{8}) expected([0-9a-f]{8})', line)
        if not match:
            raise ValueError('Malformed returned-word record')
        format_bits, job, row = map(int, match.group(1, 2, 3))
        if format_bits != bits or row >= 32 or (job, row) in words:
            raise ValueError('Foreign/duplicate/out-of-range returned-word record')
        got, ideal_field = (int(value, 16) for value in match.group(4, 5))
        ideal, predicted = arithmetic['results'][bits][row]
        if ideal_field != ideal:
            raise ValueError('Original strict expected-word field changed')
        words[(job, row)] = got
        if got != predicted:
            raise ValueError(f'Documented arithmetic mismatch pass{job} row{row} got{got:08x} predicted{predicted:08x}')
    if set(words) != {(job, row) for job in range(2) for row in range(32)}:
        raise ValueError('Need ALL64 already-returned words;partial logs are not proof')
    return 64

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('closed_record', type=pathlib.Path)
    parser.add_argument('--bits', type=int, choices=(4, 8), required=True)
    args = parser.parse_args()
    # Diagnostic copies only. Call this after actual parent terminal;this parser
    # cannot authenticate run/lease completion,physical provenance or a log file.
    with args.closed_record.open('rb') as record:
        raw = record.read(1024 * 1024 + 1)
    if len(raw) > 1024 * 1024:
        raise SystemExit('Record exceeds1MiB diagnostic bound')
    try:
        count = compare(raw.decode('utf-8'), args.bits)
    except ValueError as error:
        raise SystemExit(str(error)) from error
    print(f'MATCHES_DOCUMENTED_ARITHMETIC words{count} bits{args.bits};RECORD_PROVENANCE_AND_PHYSICAL_RUN_NOT_AUTHENTICATED,NO_RECLASSIFY_OLD_FAILURE_NO_END_RETRY_OR_DEVICE_AUTHORITY')

if __name__ == '__main__':
    main()
