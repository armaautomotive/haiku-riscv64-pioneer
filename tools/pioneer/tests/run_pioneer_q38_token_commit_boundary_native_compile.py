#!/usr/bin/env python3
"""Inspect exact private commit-boundary SDK object; NEVER link/install a driver."""
import hashlib
import os
import pathlib
import re
import subprocess
import sys

OBJECT_SHA = 'e008a0656dc3cb65899e8b893c4b4d1d45339ccd8cfce8f1bbc3e1a0f29c97d5'
assert len(sys.argv) in (3, 4) and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
inspect = len(sys.argv) == 4
assert not inspect or sys.argv[3] == '--inspect-candidate'
assert inspect or OBJECT_SHA, 'private candidate not independently frozen'
previous, out = map(pathlib.Path, sys.argv[1:3])
here = pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old = previous / 'retained-cohort-token-context-NODRV.o'
assert hashlib.sha256(old.read_bytes()).hexdigest() == '473ba8728ae591223cd7a62c8d65e3ed57bd0a7f2de6d6d16be64981f2fbfe2a'
argv = (previous / 'compile-argv.txt').read_bytes()
assert hashlib.sha256(argv).hexdigest() in (
    'dc0636b60a4c65bdb156c962153129e60d268b9ab8477ef53e5363ae4b886376',
    '1bfddfe6bd95ce5d7c7fd10a895169e1e991deb1ee2853dbc81325c3a7147f9b')
args = argv.decode().splitlines()
assert args[0] == '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert args.count('-c') == args.count('-o') == 1 and '-shared' not in args
assert pathlib.Path(args[args.index('-c') + 1]).name == 'pioneer_q38_token_retained_cohort_native_context_compile.cpp'
assert pathlib.Path(args[args.index('-o') + 1]) == old
out.mkdir(mode=0o700)
control = out / 'unchanged-retained-context.o'
args[args.index('-c') + 1] = str(here / 'pioneer_q38_token_retained_cohort_native_context_compile.cpp')
args[args.index('-o') + 1] = str(control)
subprocess.run(args, check=True)
assert control.read_bytes() == old.read_bytes(), 'ENTIRE old private SDK must reproduce first'
obj = out / 'commit-boundary-token-context-NODRV.o'
args[args.index('-c') + 1] = str(here / 'pioneer_q38_token_commit_boundary_native_context_compile.cpp')
args[args.index('-o') + 1] = str(obj)
subprocess.run(args, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(args) + '\n')
tool = args[0][:-3]


def records(path):
    table = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(path)], text=True)
    values = {row[-1]: (int(row[0], 16), int(row[1], 16))
              for row in map(str.split, table.splitlines()) if len(row) == 4}
    raw = subprocess.check_output([tool + 'objdump', '-t', str(path)], text=True)
    sections = {row[-1]: row[-3] for row in map(str.split, raw.splitlines()) if len(row) >= 6 and row[-1] in values}
    return table, values, sections


def payload(path, name, values, sections, label):
    start, size = values[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', sections[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert start + size <= len(data)
    return data[start:start + size]


table, symbols, sections = records(obj)
_, old_symbols, old_sections = records(control)
undefined = subprocess.check_output([tool + 'nm', '-u', str(obj)], text=True)
assert not {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} & symbols.keys()
header = (here.parent / 'PioneerQ38TokenCommitBoundaryPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == 'f17464e86b23df5210830446fa894980deba0f1f9a469fd9676db788190df182'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 13
for digest, label in arrays:
    matches = [name for name in symbols if 'PioneerQ38TokenCommitBoundaryPrograms' in name
               and name.endswith(str(len(label)) + label + 'E')]
    assert len(matches) == 1
    assert hashlib.sha256(payload(obj, matches[0], symbols, sections, label)).hexdigest() == digest, label
for namespace in ('PioneerQ38TokenRetainedCohortPrograms', 'PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms'):
    assert not any(namespace in name for name in symbols), namespace
rope = [name for name, (_, size) in symbols.items()
        if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and rope[0] in old_symbols
assert payload(obj, rope[0], symbols, sections, 'new-rope') == payload(control, rope[0], old_symbols, old_sections, 'old-rope')
demangled = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(obj)], text=True)
for required in ('Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'Q38TokenFrontierControl(',
                 'pioneer_token_frontier_context_control', 'pioneer_token_frontier_context_scoped_observe',
                 'PioneerQ38TokenNativeOwner::InspectTerminal<'):
    assert required in demangled, required
for forbidden in ('ReferenceProducts', 'OriginalSource', 'ValidationSession', 'Q38WorkControl',
                  'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace',
                  'SyntheticIO', 'llama_', 'ggml_'):
    assert forbidden not in table and forbidden not in undefined, forbidden


def frames(path):
    result = {}
    for line in path.read_text().splitlines():
        row = line.split('\t')
        match = re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)', row[0])
        assert len(row) == 3 and match and row[2] == 'static', line
        result[match[1]] = int(row[1])
    return result


before, after = frames(control.with_suffix('.su')), frames(obj.with_suffix('.su'))
for name, size in after.items():
    assert size <= 2048 or name in before and size <= before[name], (name, size)
digest = hashlib.sha256(obj.read_bytes()).hexdigest()
if not inspect:
    assert digest == OBJECT_SHA, 'unqualified private commit-boundary object'
print(digest + ' commit-boundary-token-context-NODRV.o')
print('PASS_BUILD_ONLY distinct private commit-boundary SDK/entireold473ba872 unchanged/all13 actual arrays+'
      '2MiBRoPE exact/actual same-owner frontier+terminal+lifecycle adapters/no conventionalABI+CPU oracle+Work+'
      'MOCK imports/individual frames bounded;NOT combined-stack,original model,native execution,physical,'
      'timing or Work qualification;DO_NOT_INSTALL')
