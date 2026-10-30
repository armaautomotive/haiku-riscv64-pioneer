#!/usr/bin/env python3
"""Exact separately named G3 program DATA; no native or device execution."""
import ast
import hashlib
import os
import pathlib
import runpy
import subprocess
import sys
import tempfile

assert len(sys.argv) == 3 and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root, worker = map(pathlib.Path, sys.argv[1:])
assert root.is_absolute() and worker.is_absolute()
generator = here.parent/'generate_qwen38_token_programs.py'
committed = here.parent/'PioneerQ38TokenPushedPrograms.h'
digest = '0fedf0ef8937c31bcd4e3a92ef14ed6afd145e0e61187ca105fa6f577e0ac830'

def records(path, key):
    tree = ast.parse(path.read_text())
    return next(ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == key for t in n.targets))

inputs = [(root, records(here.parent/'build_qwen38_token_root.py', 'PINS_CANCEL_F32_PUSHED')),
          (worker, records(here.parent/'build_qwen38_pushed_worker_image.py', 'PINS'))]
assert sum(len(pins) for _, pins in inputs) == 15
helpers = runpy.run_path(str(generator))
snapshots = tuple(helpers['authenticated'](path, pins) for path, pins in inputs)
section_map = [(source, name[:-4], name, 0) for source in snapshots for name in source if name.endswith('.bin')]
assert len(section_map) == 13
coverage = helpers['require_complete_sections']
coverage(section_map, snapshots)
map_refusals = 0
for at in range(len(section_map)+1):
    if at < len(section_map):
        cases = (section_map[:at]+section_map[at+1:], section_map+[section_map[at]])
    else:
        unit = section_map[0]
        cases = ([(dict(unit[0]), *unit[1:])]+section_map[1:],)
    for bad in cases:
        try:
            coverage(bad, snapshots)
        except ValueError as error:
            assert 'section coverage' in str(error), error
            map_refusals += 1
        else:
            raise AssertionError('incomplete/duplicated/foreign section map accepted')
assert map_refusals == 27
with tempfile.TemporaryDirectory(prefix='pioneer-q38-pushed-programs-', dir='/private/tmp') as temporary:
    directory = pathlib.Path(temporary)
    generated = directory/'programs.h'
    subprocess.run([sys.executable, str(generator), str(root), str(worker), str(generated), '--pushed-completion'], check=True)
    assert generated.read_bytes() == committed.read_bytes() and hashlib.sha256(generated.read_bytes()).hexdigest() == digest
    refusals = 0
    for source_dir, pins in inputs:
        for name, size, expected in pins:
            source = (source_dir/name).read_bytes()
            assert len(source) == size and hashlib.sha256(source).hexdigest() == expected
            case = directory/f'bad-{refusals}'
            case.mkdir()
            for path in source_dir.iterdir():
                if path.is_file():
                    (case/path.name).symlink_to(path)
            target = case/name
            target.unlink()  # private symlink only; never write through to the real artifact
            corrupt = bytearray(source)
            corrupt[len(corrupt)//2] ^= 1
            for bad in (corrupt, source[:-1]):
                target.write_bytes(bad)
                output = directory/f'NO_OUTPUT-{refusals}'
                result = subprocess.run([sys.executable, str(generator), str(case if source_dir == root else root),
                                         str(case if source_dir == worker else worker), str(output), '--pushed-completion'], capture_output=True)
                assert result.returncode == 2 and name.encode() in result.stderr and not output.exists(), result.stderr
                refusals += 1
    assert refusals == 30
    result = subprocess.run([sys.executable, str(generator), str(root), str(worker), str(generated), '--pushed-completion'], capture_output=True)
    assert result.returncode == 2 and hashlib.sha256(generated.read_bytes()).hexdigest() == digest
    refusals += 1
    dangling = directory/'NO_DANGLING'
    absent = directory/'absent-target'
    dangling.symlink_to(absent)
    result = subprocess.run([sys.executable, str(generator), str(root), str(worker), str(dangling), '--pushed-completion'], capture_output=True)
    assert result.returncode == 2 and dangling.is_symlink() and not absent.exists()
    refusals += 1
    for role in ('PIONEER_Q38_WORK_INFERENCE', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            output = directory/f'NO_ROLE-{refusals}'
            result = subprocess.run([sys.executable, str(generator), str(root), str(worker), str(output), '--pushed-completion'],
                                    capture_output=True, env={**os.environ, role:value})
            assert result.returncode == 2 and b'role overrides' in result.stderr and not output.exists()
            refusals += 1
    assert refusals == 36
    binary = directory/'test'
    common = [os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror']
    subprocess.run([*common, '-O1', '-g', '-fsanitize=address,undefined', '-I'+str(here.parent),
                    '-DPIONEER_Q38_PUSHED_PROGRAMS_HEADER="'+str(generated)+'"',
                    str(here/'pioneer_q38_pushed_programs_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(root), str(worker)], check=True,
                   env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    # Separate DATA namespaces must not replace or reselect legacy code or
    # journals, regardless of include order. This is not a native role test.
    old = '#include "'+str(here.parent/'PioneerQ38TokenPrograms.h')+'"\n'
    new = '#include "'+str(committed)+'"\n'
    for precise in (False, True):
        prefix = '#define PIONEER_Q38_TOKEN_PRECISE_PROGRAMS 1\n' if precise else ''
        for first, second in ((old, new), (new, old)):
            source = (prefix+first+second+new+
                      'static_assert(PioneerQ38TokenPushedPrograms::Family==0x33505451);\n'+
                      'static_assert(PioneerQ38TokenPushedL1::SavedWords==13478458);\n'+
                      'static_assert(PioneerQ38TokenPrograms::Family=='+hex(0x32505451 if precise else 0x31505451)+');\n'+
                      'static_assert(PioneerQ38TokenL1::SavedWords=='+str(13356602 if precise else 11406906)+');\n')
            subprocess.run([*common, '-fsyntax-only', '-x', 'c++', '-'], input=source, text=True, check=True)
    print(f'PASS separate QTP3 program DATA/reproduced fixed header/15 complete artifacts/{refusals} artifact+overwrite+symlink+role refusals/{map_refusals} section-map refusals/4 legacy-coexistence controls;NO native or launch grant')
