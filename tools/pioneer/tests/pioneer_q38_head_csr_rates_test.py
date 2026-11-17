#!/usr/bin/env python3
"""Offline compare/failure cases built from the archived raw-Q4 transcript."""
from decimal import Decimal
from pathlib import Path
import re
import runpy
from tempfile import TemporaryDirectory


TOOLS = Path(__file__).resolve().parents[1]
COMPARATOR = runpy.run_path(str(TOOLS / 'compare_qwen38_head_csr_rates.py'))
ARCHIVE = TOOLS / 'benchmarks' / 'qwen38-vector-q4-full-card-20261104'


def altered_times(text, delta_us=0, first_delta=0):
    def row(match):
        line = match[0]
        fields = {key: int(value) for key, value in re.findall(r'([a-z_]+)([0-9]+)', line)}
        # Simulate an independent run, even when the elapsed bounds match.
        fb, fe = (fields[k] + 1000 + first_delta * 5800000 for k in ('first_begin_us', 'first_end_us'))
        lb, le = (fields[k] + 1000 + delta_us for k in ('last_begin_us', 'last_end_us'))
        first = fields['first'] + first_delta
        sampled = fields['last'] - first
        lower = Decimal(sampled) * 1000000 / (le - fb)
        upper = Decimal(sampled) * 1000000 / (lb - fe)
        return re.sub(r'\b(sampled|first|elapsed_min_us|elapsed_max_us|lower_tps|upper_tps|first_begin_us|'
                      r'first_end_us|last_begin_us|last_end_us)[0-9]+(?:\.[0-9]+)?',
                      lambda m: m[1] + str({'sampled': sampled, 'first': first, 'elapsed_min_us': lb-fe,
                          'elapsed_max_us': le-fb, 'lower_tps': f'{lower:.9f}', 'upper_tps': f'{upper:.9f}',
                          'first_begin_us': fb, 'first_end_us': fe, 'last_begin_us': lb, 'last_end_us': le}[m[1]]), line)
    if first_delta:
        text = re.sub(r'^.*Full-card count committed31 prompts31.*\n', '', text, flags=re.M)
    return re.sub(r'^.*Full-card observed count rate .*$', row, text, flags=re.M)


def main():
    real = (ARCHIVE/'rate.stdout').read_text()
    compare, invalid = COMPARATOR['compare'], COMPARATOR['InvalidLog']
    with TemporaryDirectory(prefix='pioneer-q38-rate-pair-') as temporary:
        root = Path(temporary)
        serial, batched = root/'serial', root/'batched'
        for stage in (serial, batched):
            stage.mkdir()
            for name, value in {'parent.exit':'0\n','rate.exit':'0\n','rate.stderr':'','rate.stdout':real}.items():
                (stage/name).write_text(value)
        for delta, outcome in ((0, 'timing_bounds_overlap'), (-1000000, 'candidate_faster_in_observed_pair'),
                               (1000000, 'candidate_slower_in_observed_pair')):
            (batched/'rate.stdout').write_text(altered_times(real, delta))
            data = compare(serial, batched)
            assert data['result'] == outcome and data['window']['sampled_commit_steps'] == 31, data

        def refuse(other, message):
            try:
                compare(serial, other)
            except invalid as error:
                assert message in str(error), error
            else:
                raise AssertionError(message)

        refuse(serial, 'independent run directories')
        (batched/'rate.stdout').write_text(real)
        refuse(batched, 'duplicate trial transcript')
        for file, good, bad, message in (
                ('parent.exit', '0\n', '1\n', 'parent.exit'),
                ('rate.exit', '0\n', '1\n', 'rate.exit'),
                ('rate.stderr', '', 'warning\n', 'unexpected worker stderr'),
                ('rate.stdout', real, real.replace('hex=310a', 'hex=320a', 1), 'answer differs'),
                ('rate.stdout', real, altered_times(real, first_delta=1), 'mismatched commit windows'),
                ('rate.stdout', real, real.rsplit('\n', 2)[0]+'\n', 'suffix')):
            (batched/file).write_text(bad)
            refuse(batched, message)
            (batched/file).write_text(good)
    print('PASS full archived raw-Q4 stream: overlap/faster/slower bounds; eight refusal cases; no hardware')


if __name__ == '__main__':
    main()
