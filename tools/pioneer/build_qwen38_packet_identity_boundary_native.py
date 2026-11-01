#!/usr/bin/env python3
"""Build isolated private or bounded diagnostic subject;NEVER install or run."""
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys

PRIVATE_SHA = '6040e42e027bc0e637b074cb229de02bcd456daff875276f35eb2c4722eb309f'
PUBLIC_OBJECT_SHA = '59ed6722eb9715c8d1326e066c785b1f008b686a9b490f6217f5f67f0555cc02'
PUBLIC_MODULE_SHA = 'ba9719dbc45ffb408d386c7c84da7322ae94136b6cbab26e39f6077870366830'
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
old_name = 'cohort-local-sources-benchmark-native.o' if physical else 'cohort-local-sources-context-NODRV.o'
old_object = previous / old_name
old_digest = ('36abb94efaa51fa9db8547eb63ea954ed93f9a81abbcda18710db43aadcbe3a1' if physical else
              'ce078f0f428ca594bce7fef38ea899c9b0f5f69ca8b9111d85981981179c0f43')
assert hashlib.sha256(old_object.read_bytes()).hexdigest() == old_digest
argv = (previous / 'compile-argv.txt').read_bytes()
argv_pins = (('2670a6bcb615b485b476f6e4d610ffed848fb3cc1265112b4e13f2ea9780a126',
              '0820cbcdae75b452655e514f8f1867ec39a801c8d6f005f377f254b9a9ef2747') if physical else
             ('97ae764d8248d563f72a076d9589ccfffc894c611f78415bbc3dcc4a87337272',
              'ed262ec74081ffb339411a37cc09232a7902e3d7468361698ae7044fca90f014'))
assert hashlib.sha256(argv).hexdigest() in argv_pins
compile_args = argv.decode().splitlines()
assert compile_args[0] == '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
assert compile_args.count('-c') == compile_args.count('-o') == 1 and '-shared' not in compile_args
old_source = ('pioneer_q38_token_cohort_local_sources_benchmark_native_compile.cpp' if physical else
              'pioneer_q38_token_cohort_local_sources_native_context_compile.cpp')
assert pathlib.Path(compile_args[compile_args.index('-c') + 1]).name == old_source
assert pathlib.Path(compile_args[compile_args.index('-o') + 1]) == old_object
out.mkdir(mode=0o700)
tool = compile_args[0][:-3]
control = out / 'unchanged-cohort-sources.o'
compile_args[compile_args.index('-c') + 1] = str(here / 'tests' / old_source)
compile_args[compile_args.index('-o') + 1] = str(control)
subprocess.run(compile_args, check=True)
assert control.read_bytes() == old_object.read_bytes(), 'ENTIRE qualified old SDK object must reproduce FIRST'
link = None
if physical:
    old_module = previous / 'tenstorrent_pci_probe-token-COHORT-SOURCES34-BENCHMARK'
    assert hashlib.sha256(old_module.read_bytes()).hexdigest() == '844ecfe2fc3b8396f0c9787c3a5458544abb91a91011d8f9844f3e137c3d03f5'
    raw_link = (previous / 'link-argv.txt').read_bytes()
    assert hashlib.sha256(raw_link).hexdigest() in (
        '68d9bb2be3a1dfcc531a01ff690edef16ba7d6886895b872f7b18647fdf993ed',
        '6388d5d828b2fbfe8ad3ca7d584d2cd60c3204041fda8813d70d62eeab151ad0')
    link = raw_link.decode().splitlines()
    assert link[6] == str(old_object) and link[-1] == str(old_module)
    link[6], link[-1] = str(control), str(out / 'unchanged-cohort-sources-driver')
    subprocess.run(link, check=True)
    assert pathlib.Path(link[-1]).read_bytes() == old_module.read_bytes(), 'ENTIRE old844ec module must reproduce FIRST'
new_source = ('pioneer_q38_token_packet_identity_boundary_benchmark_native_compile.cpp' if physical else
              'pioneer_q38_token_packet_identity_boundary_native_context_compile.cpp')
obj = out / ('packet-identity-boundary-benchmark-native.o' if physical else 'packet-identity-boundary-context-NODRV.o')
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
header = (here / 'PioneerQ38TokenPacketIdentityBoundaryPrograms.h').read_bytes()
assert hashlib.sha256(header).hexdigest() == 'de0ae6307f235640a067360de2b3acf5a0e30c7de00b06c27322d4d90c1e3243'
arrays = re.findall(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=', header.decode())
assert len(arrays) == 13
for digest, label in arrays:
    found = [name for name in symbols if 'PioneerQ38TokenPacketIdentityBoundaryPrograms' in name
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
    module = out / 'tenstorrent_pci_probe-token-PACKET-IDENTITY-BOUNDARY34-BENCHMARK'
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
for required in ('Q38TokenNativeControl(', 'ObserveQ38TokenNativeFrontier(',
                 'PioneerQ38TokenNativeOwner::ObserveCommittedFrontier<', 'Q38TokenFrontierControl(',
                 'PioneerQ38TokenPacketIdentityBoundaryPrograms', 'BeginQ38TokenNativeBackendScoped(',
                 'FinishQ38TokenNativeBackendScoped('):
    assert required in table, required
for forbidden in ('ReferenceProducts', 'OriginalSource', 'ValidationSession', 'Q38WorkControl',
                  'NativeQ38PeerInstructionModel', 'Q38NativePeerTestCpu', 'PioneerQ38ObservedTrace',
                  'SyntheticIO', 'llama_', 'ggml_', 'PioneerQ38TokenCohortLocalSourcesPrograms', 'PioneerQ38TokenCommitBoundaryPrograms',
                  'PioneerQ38TokenRetainedCohortPrograms', 'PioneerQ38TokenFrontierPrograms',
                  'PioneerQ38TokenJobBootFoldPrograms', 'PioneerQ38TokenPreciseBetaPrograms',
                  'PioneerQ38TokenFaultPrograms', 'PioneerQ38TokenPushedPrograms'):
    assert forbidden not in table and forbidden not in undefined, forbidden
if physical:
    graph = out / 'packet-identity-boundary-callgraph.o'
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
    report = {'kind': 'PARTIAL_ACTUAL_PACKET_IDENTITY_PUBLIC_STACK_AUDIT', 'roots': results,
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
print('BUILD_ONLY new packet-identity-boundary ' + ('bounded6607+6608 diagnostic' if physical else 'private renamedABI NODRV')
      + '/ENTIRE old subject reproduced FIRST/all13 arrays+2MiBRoPE exact/individual frames bounded/'
        'noCPU oracle+Work+TEST imports;NOT lifecycle,full-model,physical,timing or Work qualification', flush=True)
