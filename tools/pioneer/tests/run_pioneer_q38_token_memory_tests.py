#!/usr/bin/env python3
"""Full-token journal tests with simulated endpoints. NEVER actual device IO."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
args = sys.argv[1:]
hybrid_header = None
if '--hybrid-programs' in args:
    index=args.index('--hybrid-programs')
    assert args.count('--hybrid-programs')==1 and index+1<len(args)
    hybrid_header=pathlib.Path(args[index+1]);del args[index:index+2]
    assert hybrid_header.is_absolute() and hybrid_header.is_file()
hybrid_journal = '--hybrid-journal' in args
if hybrid_journal:
    assert args.count('--hybrid-journal') == 1
    args.remove('--hybrid-journal')
bf16_deferred='--bf16-deferred-audit' in args
if bf16_deferred:
    assert args.count('--bf16-deferred-audit')==1
    args.remove('--bf16-deferred-audit')
bf16_paired='--bf16-pair-issue' in args
if bf16_paired:
    assert args.count('--bf16-pair-issue')==1
    args.remove('--bf16-pair-issue')
bf16_profile = '--bf16-profile' in args
if bf16_profile:
    assert args.count('--bf16-profile') == 1
    args.remove('--bf16-profile')
precise = '--precise-programs' in args
if precise:
    assert args.count('--precise-programs') == 1
    args.remove('--precise-programs')
pushed = '--pushed-programs' in args
if pushed:
    assert args.count('--pushed-programs') == 1 and not precise, 'distinct QTP3 cannot mix with QTP2 DATA'
    args.remove('--pushed-programs')
bf16 = '--bf16-programs' in args
if bf16:
    assert args.count('--bf16-programs') == 1 and not precise and not pushed
    args.remove('--bf16-programs')
assert not bf16_profile or bf16, 'timing journal requires its explicit QTP4 DATA parent'
assert not bf16_paired or (bf16_profile and bf16),'paired journal requires its explicit QTP4/timing DATA parents'
assert not bf16_deferred or (bf16_profile and bf16 and not bf16_paired),'deferred journal requires its distinct QTP4/timing DATA parents'
assert not hybrid_journal or (bf16 and not bf16_profile and not bf16_paired and not bf16_deferred), 'hybrid footprint fixture requires original QTP4 test DATA only'
assert hybrid_header is None or hybrid_journal, 'QGW5 DATA requires its distinct offline hybrid journal'
sdk = None
if len(args) >= 2 and args[-2] == '--sdk':
    sdk = pathlib.Path(args[-1])
    args = args[:-2]
assert args in ([], ['--positive'], ['--faults'], ['--capsule-faults'], ['--hybrid-faults'], ['--hybrid-manifest'])
assert args != ['--capsule-faults'] or precise or pushed or bf16, 'capsule faults require explicit code DATA'
assert not bf16 or sdk is None or (hybrid_journal and args==['--hybrid-manifest']), 'only the hybrid kernel template witness is allowed here'
assert not hybrid_journal or (args in (['--positive'], ['--hybrid-faults'], ['--hybrid-manifest'])
                              and (sdk is None or args==['--hybrid-manifest'])), 'hybrid journal is an offline fixture only'
assert args != ['--hybrid-faults'] or hybrid_journal, 'hybrid faults need their explicit offline footprint'
assert args != ['--hybrid-manifest'] or (hybrid_journal and hybrid_header is not None), 'hybrid manifest needs the exact QGW5 program and layout'
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
flags = re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (here/'run_pioneer_q38_ffn_persistent_image_tests.sh').read_text())
if bf16_profile or hybrid_journal:
    # Use non-Work dispatcher metadata,as in the BF16 native build,not the
    # unrelated legacy shader-test feature set. Do not bypass the DATA role.
    flags=['-DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR','-DPIONEER_Q38_SPAN_FAST']
profile = ['-DPIONEER_Q38_TOKEN_BF16_PROGRAMS=1'] if bf16 else ['-DPIONEER_Q38_TOKEN_PUSHED_PROGRAMS=1'] if pushed else ['-DPIONEER_Q38_TOKEN_PRECISE_PROGRAMS=1'] if precise else []
if bf16_profile:profile+=['-DPIONEER_Q38_TOKEN_BF16_PROFILE_PROGRAMS=1']
if bf16_paired:profile+=['-DPIONEER_Q38_TOKEN_BF16_PAIR_ISSUE_PROGRAMS=1']
if bf16_deferred:profile+=['-DPIONEER_Q38_TOKEN_BF16_DEFERRED_PROGRAMS=1']
if hybrid_journal:profile+=['-DPIONEER_Q38_HYBRID_JOURNAL_DIAGNOSTIC=1']
if hybrid_header is not None:
    profile+=['-DPIONEER_Q38_HYBRID_PROGRAMS_DIAGNOSTIC=1',
              '-DPIONEER_Q38_HYBRID_PROGRAMS_HEADER="'+str(hybrid_header)+'"',
              '-DPIONEER_Q38_HYBRID_LAYOUT_DIAGNOSTIC=1','-DPIONEER_Q38_HYBRID_JOB_DIAGNOSTIC=1',
              '-I'+str(here.parent)]
with tempfile.TemporaryDirectory(prefix='pioneer-token-memory-', dir='/private/tmp') as tmp:
    binary = pathlib.Path(tmp)/'test'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', *flags, *profile,
                    str(here/'pioneer_q38_token_memory_test.cpp'), '-o', str(binary)], check=True)
    if args == ['--hybrid-manifest']:
        # These methods stage a 1,344-word address table. Their scratch must
        # stay in the heap-only owner, not silently return to a 5 KiB frame.
        # Check an unsanitized no-inline HOST build separately: ASan marks
        # frames dynamic and cannot establish this limited static property.
        stack_binary=pathlib.Path(tmp)/'stack-check'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-O2', '-fno-inline', '-fstack-usage',
                        '-Wall', '-Wextra', '-Werror', *flags, *profile,
                        str(here/'pioneer_q38_token_memory_test.cpp'), '-o', str(stack_binary)], check=True)
        usage=list(pathlib.Path(tmp).glob('*.su'));assert len(usage)==1,usage
        rows=[line.split('\t') for line in usage[0].read_text().splitlines()]
        for name in ('InstallHybridManifestHeld', 'VerifyHybridManifestHeld'):
            matching=[row for row in rows if name in row[0]]
            assert len(matching)==1 and len(matching[0])==3 and matching[0][2]=='static' and int(matching[0][1])<=2048,(name,matching)
        print('PASS hybrid manifest install and verification frames <=2048 bytes in host diagnostic build',flush=True)
    for extra in ([[]] if args == ['--positive'] else [args] if args in (['--faults'], ['--capsule-faults'], ['--hybrid-faults'], ['--hybrid-manifest']) else [[], ['--faults']]):
        subprocess.run([str(binary), *extra], check=True, env={**os.environ, 'ASAN_OPTIONS':'quarantine_size_mb=32', 'UBSAN_OPTIONS':'halt_on_error=1'})
    if sdk:
        headers = sdk/'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop/headers'
        obj = pathlib.Path(tmp)/'kernel.o'
        tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku')
        subprocess.run([tool+'-g++', '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-Wno-misleading-indentation',
                        '-fno-exceptions', '-fno-rtti', '-ffreestanding', '-fstack-usage', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE',
                        '-I'+str(headers), '-idirafter', str(headers/'posix'),
                        *(['-DPIONEER_Q38_HYBRID_KERNEL_COMPILE_ONLY=1'] if hybrid_journal else []),
                        *profile, '-c',
                        str(here/'pioneer_q38_token_memory_kernel_compile.cpp'), '-o', str(obj)], check=True)
        rows = [line.split('\t') for line in obj.with_suffix('.su').read_text().splitlines()]
        assert rows and all(len(row) == 3 and row[2] == 'static' and int(row[1]) <= 2048 for row in rows)
        defined = subprocess.check_output([tool+'-nm', '--defined-only', str(obj)], text=True)
        assert all(name in defined for name in ('pioneer_q38_token_memory_template_prepare', 'pioneer_q38_token_memory_template_close'))
        if hybrid_journal:
            assert all(name in defined for name in ('pioneer_q38_hybrid_layout_template',
                       'pioneer_q38_hybrid_manifest_install_template','pioneer_q38_hybrid_manifest_verify_template'))
        assert not any(line.split()[-1] in ('api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device') for line in defined.splitlines() if line.split())
        unresolved = subprocess.check_output([tool+'-nm', '-u', str(obj)], text=True)
        assert 'TokenMemoryKernelCompileIO' in unresolved and 'PrefixMemoryKernelCompileIO' not in unresolved
        print(f'PASS real RISC-V SDK '+('hybrid layout/manifest and ' if hybrid_journal else '')+f'full-token journal TEMPLATE/maxStaticFrame{max(int(row[1]) for row in rows)}/separate undefined TOKEN IO hooks/no conventional ABI;NOT genuine native context or loadable driver')
