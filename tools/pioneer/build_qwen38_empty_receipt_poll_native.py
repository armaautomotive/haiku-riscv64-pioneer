#!/usr/bin/env python3
"""Build isolated private or bounded diagnostic subject;NEVER install or run."""
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys

PRIVATE_SHA = '342f01b799ac11300bb0c85792e94a70a6d2922f8df1fa4ec428edc6aedc2c5f'
PUBLIC_OBJECT_SHA = 'c09481c314a59395f95431716ebfc4fa764d48c2ece5f776191bc8fea36dbe2a'
PUBLIC_MODULE_SHA = '5a3ca6b662e3e7cb332a366c04b3650357a52e3bfe601cf0829e9e84e5e67008'
args = sys.argv[1:]
inspect = '--inspect-candidate' in args
if inspect:
    assert args.count('--inspect-candidate') == 1
    args.remove('--inspect-candidate')
assert len(args) == 3 and args[0] in ('--private', '--physical')
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
physical = args[0] == '--physical'
assert inspect or (PUBLIC_OBJECT_SHA and PUBLIC_MODULE_SHA if physical else PRIVATE_SHA)
previous, out = map(pathlib.Path, args[1:])
here = pathlib.Path(__file__).resolve().parent
assert previous.is_absolute() and out.is_absolute() and not out.exists() and not out.is_symlink()
old_name = 'counter-gather-benchmark-native.o' if physical else 'counter-gather-context-NODRV.o'
old_object = previous / old_name
old_digest = ('575f669babf2de0f9141a1e865ec2352655119d5856ca2bb88a2eb0ce74cd9cf' if physical else
              'b90c8a84d30bf33f33619ee7e9fffc37a032f580ebd9d8b175bd2dbeb3e26b0b')
assert hashlib.sha256(old_object.read_bytes()).hexdigest() == old_digest
argv = (previous / 'compile-argv.txt').read_bytes()
argv_pins = (('8f43de549d66a05bfb8e941551c6f531f895b36b00fce4b7bcc719dfa2143366',
              'a16fd831ce3c067e8a22716bf2f44d1080983c2173ba79718bf7f3e56222e8d3') if physical else
             ('0437609e3205d7f358f0e90a7d6291571e1adac9d583ef4c7d829aa9f92e053a',
              '753ff65f382775df171b01611e91b7871da52d5d59ff430b2a1e5b7b380457f9'))
assert hashlib.sha256(argv).hexdigest() in argv_pins
compile_args = argv.decode().splitlines()
assert compile_args[0] == '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert compile_args.count('-c') == compile_args.count('-o') == 1 and '-shared' not in compile_args
old_source = ('pioneer_q38_token_counter_gather_benchmark_native_compile.cpp' if physical else
              'pioneer_q38_token_counter_gather_native_context_compile.cpp')
assert pathlib.Path(compile_args[compile_args.index('-c') + 1]).name == old_source
assert pathlib.Path(compile_args[compile_args.index('-o') + 1]) == old_object
out.mkdir(mode=0o700)
tool = compile_args[0][:-3]
control = out / 'unchanged-counter.o'
compile_args[compile_args.index('-c') + 1] = str(here / 'tests' / old_source)
compile_args[compile_args.index('-o') + 1] = str(control)
subprocess.run(compile_args, check=True)
assert control.read_bytes() == old_object.read_bytes(), 'ENTIRE qualified old SDK object must reproduce FIRST'
link = None
if physical:
    old_module = previous / 'tenstorrent_pci_probe-token-COUNTER-GATHER34-BENCHMARK'
    assert hashlib.sha256(old_module.read_bytes()).hexdigest() == '0b8d1530b3f6de3462be23e923be169c82a8b15c76e83657c35c2d268b70405c'
    raw_link = (previous / 'link-argv.txt').read_bytes()
    assert hashlib.sha256(raw_link).hexdigest() in (
        'bc2ee9e6e0cef990155fd464ce64a706e2de482f4739ea4f7c6fb52e85b38d7a',
        '845346006ede739c58a47ddcb3021253f801706e7091a3441f89037c50952e94')
    link = raw_link.decode().splitlines()
    assert link[6] == str(old_object) and link[-1] == str(old_module)
    link[6], link[-1] = str(control), str(out / 'unchanged-counter-driver')
    subprocess.run(link, check=True)
    assert pathlib.Path(link[-1]).read_bytes() == old_module.read_bytes(), 'ENTIRE old0b8d module must reproduce FIRST'
new_source = ('pioneer_q38_token_empty_receipt_poll_benchmark_native_compile.cpp' if physical else
              'pioneer_q38_token_empty_receipt_poll_native_context_compile.cpp')
obj = out / ('empty-receipt-poll-benchmark-native.o' if physical else 'empty-receipt-poll-context-NODRV.o')
compile_args[compile_args.index('-c') + 1] = str(here / 'tests' / new_source)
compile_args[compile_args.index('-o') + 1] = str(obj)
subprocess.run(compile_args, check=True)
(out / 'compile-argv.txt').write_text('\n'.join(compile_args) + '\n')


def records(path):
    raw = subprocess.check_output([tool + 'nm', '-S', '--defined-only', str(path)], text=True)
    values = {row[-1]: (int(row[0], 16), int(row[1], 16))
              for row in map(str.split, raw.splitlines()) if len(row) == 4}
    raw = subprocess.check_output([tool + 'objdump', '-t', str(path)], text=True)
    sections = {row[-1]: row[-3] for row in map(str.split, raw.splitlines()) if len(row) >= 6 and row[-1] in values}
    return values, sections


def payload(path, name, values, sections, label):
    start, size = values[name]
    target = out / (label + '.raw')
    subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', sections[name], str(path), str(target)], check=True)
    data = target.read_bytes()
    assert start + size <= len(data)
    return data[start:start + size]


symbols, sections = records(obj)
old_symbols, old_sections = records(control)
header = (here / 'PioneerQ38TokenEmptyReceiptPollPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == 'a96f194bf1ac5172e29e752c3e1f894a692105b2d74bb8b8100fc5a41ca6becf'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 13
for digest, label in arrays:
    found = [name for name in symbols if 'PioneerQ38TokenEmptyReceiptPollPrograms' in name
             and name.endswith(str(len(label)) + label + 'E')]
    assert len(found) == 1 and hashlib.sha256(payload(obj, found[0], symbols, sections, label)).hexdigest() == digest, label
rope = [name for name, (_, size) in symbols.items()
        if 'PioneerQ38TokenRope' in name and name.endswith('DataE') and size == 2097152]
assert len(rope) == 1 and rope[0] in old_symbols
assert payload(obj, rope[0], symbols, sections, 'new-rope') == payload(control, rope[0], old_symbols, old_sections, 'old-rope')


def frames(path):
    values = {}
    for line in path.read_text().splitlines():
        row = line.split('\t')
        found = re.fullmatch(r'.*?:[0-9]+:[0-9]+:(.*)', row[0])
        assert len(row) == 3 and found and row[2] == 'static', line
        values[found[1]] = int(row[1])
    return values


before, after = frames(control.with_suffix('.su')), frames(obj.with_suffix('.su'))
for name, size in after.items():
    assert size <= 2048 or name in before and size <= before[name], (name, size)
module = None
if physical:
    module = out / 'tenstorrent_pci_probe-token-EMPTY-RECEIPT-POLL34-BENCHMARK'
    link[6], link[-1] = str(obj), str(module)
    subprocess.run(link, check=True)
    (out / 'link-argv.txt').write_text('\n'.join(link) + '\n')
table = subprocess.check_output([tool + 'nm', '-C', '--defined-only', str(module or obj)], text=True)
undefined = subprocess.check_output([tool + 'nm', '-u', str(module or obj)], text=True)
abi = ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device')
if physical:
    assert all(name in symbols for name in abi)
else:
    assert not set(abi) & symbols.keys()
    # Prove actual renamed bodies AND wrappers remain inert,not merely
    # that conventional export names disappeared. The independent tiny
    # expected functions are compiled from the existing NODRV control.
    for name, kind in (('init_hardware', 'refusal'), ('init_driver', 'refusal'),
                       ('uninit_driver', 'inert'), ('publish_devices', 'null'), ('find_device', 'null')):
        expected = payload(obj, 'pioneer_token_context_expected_' + kind, symbols, sections, 'inert-' + kind)
        for prefix in ('pioneer_q38_token_context_body_', 'pioneer_q38_token_context_only_'):
            assert payload(obj, prefix + name, symbols, sections, prefix + name) == expected, (prefix, name)
    print('ACTUAL_PRIVATE5_BODIES_AND5_WRAPPERS_BYTEEXACT_INDEPENDENT_INERT_CONTROLS', flush=True)
for required in ('Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'Q38TokenFrontierControl(',
                 'PioneerQ38TokenEmptyReceiptPollPrograms', 'BeginQ38TokenNativeBackendScoped(',
                 'FinishQ38TokenNativeBackendScoped('):
    assert required in table, required
for forbidden in ('ReferenceProducts', 'OriginalSource', 'ValidationSession', 'Q38WorkControl',
                  'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace',
                  'SyntheticIO', 'llama_', 'ggml_', 'PioneerQ38TokenCounterGatherPrograms', 'PioneerQ38TokenTidBoundaryPrograms', 'PioneerQ38TokenPacketIdentityBoundaryPrograms', 'PioneerQ38TokenCohortLocalSourcesPrograms', 'PioneerQ38TokenCommitBoundaryPrograms',
                  'PioneerQ38TokenRetainedCohortPrograms', 'PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined, forbidden
if physical:
    graph = out / 'empty-receipt-poll-callgraph.o'
    graph_args = compile_args[:]
    graph_args[graph_args.index('-o') + 1] = str(graph)
    subprocess.run([*graph_args, '-fcallgraph-info=su'], check=True)
    assert graph.read_bytes() == obj.read_bytes()
    sys.path.insert(0, str(here / 'tests'))
    from audit_pioneer_q38_stack_graph import audit, function_symbols, parse_graph, symbol
    nodes, edges = parse_graph(graph.with_suffix('.ci').read_text())
    roots = [key for key in nodes if symbol(key) == '_ZL12ProbeControlPvjS_m']
    assert len(roots) == 1
    functions = function_symbols(subprocess.check_output([tool + 'objdump', '-t', str(graph)], text=True))
    results = audit(nodes, edges, functions, roots)
    assert len(results) == 1 and not results[0]['cycles'] and not results[0]['dynamic']
    assert results[0]['known_module_chain_bytes'] and not results[0]['whole_kernel_stack_qualified']
    report = {'kind': 'PARTIAL_ACTUAL_EMPTY_RECEIPT_POLL_PUBLIC_STACK_AUDIT', 'roots': results,
              'module_sha256': hashlib.sha256(module.read_bytes()).hexdigest(),
              'object_sha256': hashlib.sha256(obj.read_bytes()).hexdigest(), 'whole_kernel_stack_qualified': False,
              'limits': 'Known module only;unknown kernel/IRQ/entry/indirect costs excluded. NOT a Work grant.'}
    (out / 'partial-public-entry-stack.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PARTIAL knownModuleChain' + str(results[0]['known_module_chain_bytes']) + '/unresolved'
          + str(len(results[0]['unresolved'])) + ';wholeKernelStackQualified=FALSE', flush=True)
for path, expected in ((obj, PUBLIC_OBJECT_SHA if physical else PRIVATE_SHA), *([(module, PUBLIC_MODULE_SHA)] if physical else [])):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if not inspect:
        assert digest == expected, (path, digest)
    print(digest + ' ' + path.name, flush=True)
print('BUILD_ONLY new empty-receipt-poll ' + ('bounded6607+6608 diagnostic' if physical else 'private renamedABI NODRV')
      + '/ENTIRE old subject reproduced FIRST/all13 arrays+2MiBRoPE exact/individual frames bounded/'
        'noCPU oracle+Work+TEST imports;NOT lifecycle,full-model,physical,timing or Work qualification', flush=True)
