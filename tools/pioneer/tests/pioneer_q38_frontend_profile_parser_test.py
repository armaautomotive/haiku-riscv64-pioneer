#!/usr/bin/env python3
"""Synthetic profile parsing;NEVER a physical/sample/time result."""
import os
import pathlib
import runpy
import tempfile
import inspect_pioneer_q38_frontend_profile as p

assert not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
fixture = runpy.run_path(str(pathlib.Path(__file__).with_name('pioneer_q38_frontier_benchmark_comparator_test.py')))
base = fixture['lines_for'](fixture['records'])
base[-1] = base[-1].replace('observations1000', 'observations3')
begin = 'Q38_FRONTEND_ROWS_BEGIN generation13 rows2;AFTER_CLEAN_END_NOT_ATOMIC_WITH_PRECEDING_COUNT'
end = 'Q38_FRONTEND_ROWS_END rows2;FRONTEND_SAMPLE_COUNTS_NOT_RETIRED_INSTRUCTION_CYCLES_OR_FUNCTION_TIME'
row0 = 'Q38_FRONTEND_ROW index0 begin_us100 end_us110 preceding_count0 ready0 stage9 checked_word63 first_rawffffffff second_raw00060008'
row1 = 'Q38_FRONTEND_ROW index1 begin_us200 end_us210 preceding_count33 ready1 stage11 checked_word63 first_raw00040000 second_raw00064000'
original = [p.HEADER, *base, begin, row0, row1, end]
positive = negative = 0
with tempfile.TemporaryDirectory(prefix='pioneer-q38-frontend-parser-', dir='/private/tmp') as temporary:
    path = pathlib.Path(temporary) / 'SYNTHETIC-NOT-OBSERVED.log'
    def check(lines, okay):
        global positive, negative
        path.write_text('\n'.join(lines) + '\n')
        try:
            generation, rows, records, rate = p.read_profile(path)
        except ValueError:
            assert not okay
            negative += 1
            return
        assert okay and generation == 13 and len(rows) == 2 and records == fixture['records']
        assert rows[0]['first_raw'] == 0xffffffff and rows[0]['second_raw'] == 0x60008 and rate
        positive += 1
    check(original, True)
    for at in range(len(original)):
        check(original[:at] + original[at+1:], False)
        check(original[:at] + [original[at]] + original[at:], False)
    for before, after in (
        ('generation13', 'generation0'), ('generation13', 'generation18446744073709551616'),
        ('rows2;', 'rows3;'), ('index1', 'index0'), ('begin_us200', 'begin_us109'),
        ('end_us210', 'end_us199'), ('preceding_count33', 'preceding_count35'),
        ('ready0 stage9', 'ready1 stage11'), ('ready1 stage11', 'ready0 stage9'),
        ('checked_word63 first_rawffffffff', 'checked_word64 first_rawffffffff'),
        ('first_rawffffffff', 'first_raw100000000'), ('first_rawffffffff', 'first_rawFFFFFFFE'),
        ('observations3;', 'observations4;')):
        check([line.replace(before, after) for line in original], False)
    path.write_text('\n'.join(original) + '\n')
    _, rows, _, _ = p.read_profile(path)
    counts = p.bins(rows, [(0x40000, 0x40004, 'entry'), (0x64000, 0x64004, 'aux')])
    assert counts == {'UNMAPPED_RAW_ADDRESS': 2, 'entry': 1, 'aux': 1} # no subtract4 from60008
print(f'PASS frontend parser/{positive} SYNTHETIC positives/{negative} grammar,order,time,count,stage,base-record refusals/'
      'RAW60008+FFFFFFFF stay unmapped without range widening;NO hardware/time/quality proof')
