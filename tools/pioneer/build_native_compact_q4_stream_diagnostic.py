#!/usr/bin/env python3
"""Build ONLY diagnostic104/native methods. PRIVATE weights/binaries MUST NOT be published."""
import argparse
import hashlib
import os
import pathlib
import re
import stat
import subprocess
import sys
if not __debug__ or os.environ.get('PYTHONOPTIMIZE'):
    raise SystemExit('Assertions required')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('haiku_build', type=pathlib.Path)
parser.add_argument('private_export', type=pathlib.Path)
parser.add_argument('original', type=pathlib.Path)
parser.add_argument('output', type=pathlib.Path)
selection=parser.add_mutually_exclusive_group()
selection.add_argument('--timing',action='store_true',help='Distinct107 timed kernel diagnostic;NEVER104 deployment')
selection.add_argument('--pair-issue',action='store_true',help='Distinct108 paired timed kernel;NEVER107 replay')
selection.add_argument('--factored-compare',action='store_true',help='Distinct111 old/new arithmetic comparison;NEW L1/code scope')
selection.add_argument('--factored-trace',action='store_true',help='Distinct112 fixed sample0 observations;NOT111 replay or speed test')
args = parser.parse_args()
if args.pair_issue or args.factored_compare or args.factored_trace:args.timing=True
assert args.original.is_absolute() and args.original.is_file()
build, private, output = args.haiku_build, args.private_export, args.output
assert build.is_absolute() and build.is_dir() and output.is_absolute() and not output.exists() and not output.is_symlink()
assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here = pathlib.Path(__file__).resolve().parent
root = here.parent.parent
pins = {'PioneerNativeCompactQ4StreamData.h': '4db508f0cecda10062b7373d43f4b6e7308ad8cdcff03c361759ac0523a896b1',
        'PioneerNativeCompactQ4StreamMetadata.h': 'c856c1747045a50c43e5b1f47a7e75999526bbc61960eed075246423392872de'}
def private_unchanged():
    assert private.is_absolute() and not private.is_symlink() and private.stat().st_uid == os.geteuid()
    assert stat.S_IMODE(private.stat().st_mode) == 0o700
    for name, digest in pins.items():
        info = (private / name).lstat()
        assert stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and info.st_uid == os.geteuid()
        assert stat.S_IMODE(info.st_mode) == 0o600
        assert hashlib.sha256((private / name).read_bytes()).hexdigest() == digest
private_unchanged()
subprocess.run([sys.executable, str(here / 'tests/pioneer_native_compact_q4_stream_export_test.py'), str(private), str(args.original)], check=True)
output.mkdir(mode=0o700)
images = {}
role=112 if args.factored_trace else 111 if args.factored_compare else 108 if args.pair_issue else 107 if args.timing else 104
image_bytes=12104 if args.factored_trace else 11792 if args.factored_compare else 7820 if args.pair_issue else 7972 if args.timing else 7644
image_words=image_bytes//4
widths=((5120,''),) if args.factored_trace else ((5120, 'f67b7b5bc07bddcf5f3ddfccfbe1013c2680650ea1994231565f050d1f0d89c5'),
                      (17408, 'c95e0cc41971b37dd89786a2c3747b6d8ef6ebbde3a2ec837c63198982373b92'))
for width,digest in widths:
    directory = output / ('image' + str(width))
    if args.factored_trace:
        digest='f281b1419c35b1cb818e7c0278adef916ef6d19e1a2932453ee460510f560dad'
        command=[sys.executable,str(here/'build_q38_factored_q4_trace.py'),str(build),str(directory)]
    elif args.factored_compare:
        digest='1e2cc3d675161216bcf5593a79ca483e37c384dab815b34009161cbf36dce280'
        command=[sys.executable,str(here/'build_q38_factored_q4_compare.py'),str(build),str(directory)]
    elif args.pair_issue:
        from build_q38_q4_pair_issue import INSPECT_PINS
        digest=INSPECT_PINS[width][0]
        command=[sys.executable,str(here/'build_q38_q4_pair_issue.py'),str(build),str(directory),str(width)]
    elif args.timing:
        from build_q38_q4_kernel_timing import INSPECT_PINS
        digest=INSPECT_PINS[width][0]
        command=[sys.executable,str(here/'build_q38_q4_kernel_timing.py'),str(build),str(directory),str(width)]
    else:
        command=[sys.executable,str(here/'build_q38_q4_fpu_compact_stream.py'),str(build),str(directory),str(width),'--inspect-candidate']
    subprocess.run(command,check=True)
    code, boot = (directory / 'text.bin').read_bytes(), (directory / 'boot.bin').read_bytes()
    assert len(code) == image_bytes and hashlib.sha256(code).hexdigest() == digest
    assert len(boot) == 4 and hashlib.sha256(boot).hexdigest() == 'd47a1c29c265663f5bd4bd49aaf28248b7345d7feb5a4f770caa27315deaa4dd'
    images[width] = code
header = ('#include <stdint.h>\nstruct PioneerNativeCompactQ4StreamGeneratedImage {\n'
          f' static constexpr unsigned Samples={1 if args.factored_trace else 9},Rows=32,Phases=4,LayoutRole={role},Bytes={image_bytes},MetadataFirst=128,MetadataRows=128,DstLast=255;\n'
          ' static constexpr uint32_t Boot=0x%08x;\n' % int.from_bytes(boot, 'little') +
          ' static unsigned Width(unsigned s){return s<6?5120:s<9?17408:0;}\n'
          ' static uint32_t Word(unsigned s,unsigned i){\n')
if args.factored_trace:
    header=header.replace(' static unsigned Width(unsigned s){return s<6?5120:s<9?17408:0;}', ' static unsigned Width(unsigned s){return s==0?5120:0;}')
    header=header.replace(' static constexpr uint32_t Boot=', ' static constexpr unsigned FactoredTrace=1,TraceWords=64,TraceBlocks=20,NewPhases=2,Accumulator=80;\n static constexpr uint32_t Boot=')
elif args.factored_compare:
    header=header.replace(' static constexpr uint32_t Boot=', ' static constexpr unsigned FactoredComparison=1,NewPhases=2,Accumulator=80;\n static constexpr uint32_t Boot=')
for width, code in images.items():
    words = [int.from_bytes(code[i:i+4], 'little') for i in range(0,len(code),4)]
    header += (' static const uint32_t code%d[%d]={\n' % (width,image_words))
    header += ',\n'.join(','.join('0x%08x' % w for w in words[i:i+8]) for i in range(0,len(words),8)) + '\n };\n'
header += (f' return s!=0 || i>={image_words}?0:code5120[i];}}\n}};\n' if args.factored_trace else
           f' return s>=9 || i>={image_words}?0:s<6?code5120[i]:code17408[i];}}\n}};\n')
(output / 'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header)
sdk = build / 'objects/haiku/riscv64/packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
release = build / 'objects/haiku/riscv64/release'
tool = str(build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
cxx = tool + 'g++'
includes = []
for path in ('kernel', 'kernel/arch/riscv64', 'kernel/arch/generic', '', 'drivers', 'system', 'system/arch/riscv64', 'shared'):
    includes += ['-isystem', str(root / 'headers/private' / path)]
includes += ['-isystem', str(root / 'build/config_headers'), '-I' + str(release / 'system/kernel'),
             '-I' + str(sdk / 'headers'), '-idirafter', str(sdk / 'headers/posix'), '-I' + str(output), '-I' + str(here), '-I' + str(private)]
for path in [sdk / 'headers/os', *sorted((sdk / 'headers/os').iterdir())]:
    if path.is_dir():
        includes += ['-isystem', str(path)]
common = [cxx, '-std=gnu++17', '-O2', '-Wall', '-Wextra', '-Werror', '-fno-exceptions', '-fno-rtti', '-fstack-usage',
          '-ffreestanding', '-finline', '-fno-semantic-interposition', '-fno-tree-vectorize', '-fno-use-cxa-atexit',
          '-fno-strict-aliasing', '-fno-delete-null-pointer-checks', '-mcmodel=medany', '-fpic', '-D_KERNEL_MODE']
roles = ['BANKS', 'PANEL', 'AFFINE', 'BLOCK', 'STREAM', 'ROWS', 'COHORT', 'COHORT_SEALED', 'COHORT_PAYLOAD', 'DISPATCH', 'DISPATCH_VECTOR']
native = ['-DTT_PIONEER_EXPERIMENTAL_Q38_' + name for name in roles]
tag='2026110600010112' if args.factored_trace else '2026110600010111' if args.factored_compare else '2026110600010108' if args.pair_issue else '2026110600010107' if args.timing else '2026110500010104'
native += ['-DTT_PIONEER_DRIVER_RELOAD', '-DPIONEER_DRIVER_BUILD_TAG=0x'+tag+'ULL', '-DPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC=1']
if args.timing:native.append('-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=1')
if args.pair_issue:native.append('-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=1')
if args.factored_compare:native.append('-DPIONEER_NATIVE_FACTORED_Q4_COMPARE=1')
if args.factored_trace:native.append('-DPIONEER_NATIVE_FACTORED_Q4_TRACE=1')
commands = []
def run(command):
    commands.append(command)
    subprocess.run(command, check=True, cwd=output)
driver = root / 'src/add-ons/kernel/drivers/misc'
run(common + includes + ['-c', str(driver / 'PioneerPciProfile.cpp'), '-o', str(output / 'profile.o')])
run(common + native + includes + ['-c', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp'), '-o', str(output / 'native.o')])
run(common + includes + ['-c', str(driver / 'tenstorrent_pci_probe.cpp'), '-o', str(output / 'control.o')])
cases = [('missing-native-role', ['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC']),
         ('wrong-native-role', ['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC', '-DPIONEER_NATIVE_COMPACT_Q4_STREAM_DIAGNOSTIC=2']),
         ('missing-reload', ['-UTT_PIONEER_DRIVER_RELOAD']),
         ('old-tag', ['-UPIONEER_DRIVER_BUILD_TAG', '-DPIONEER_DRIVER_BUILD_TAG=0x2026110400010086ULL']),
         ('fast-math', ['-ffast-math'])]
if args.timing:
    cases += [('missing-timing',['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING']),
              ('wrong-timing',['-UPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING','-DPIONEER_NATIVE_COMPACT_Q4_STREAM_TIMING=0']),
              ('untimed-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110500010104ULL'])]
if args.pair_issue:
    cases += [('missing-pair',['-UPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING']),
              ('wrong-pair',['-UPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING','-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=0']),
              ('old107-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010107ULL'])]
if args.factored_compare:
    cases += [('missing-comparison',['-UPIONEER_NATIVE_FACTORED_Q4_COMPARE']),
              ('wrong-comparison',['-UPIONEER_NATIVE_FACTORED_Q4_COMPARE','-DPIONEER_NATIVE_FACTORED_Q4_COMPARE=0']),
              ('old107-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010107ULL']),
              ('old108-alias',['-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=1'])]
if args.factored_trace:
    cases += [('missing-trace',['-UPIONEER_NATIVE_FACTORED_Q4_TRACE']),
              ('wrong-trace',['-UPIONEER_NATIVE_FACTORED_Q4_TRACE','-DPIONEER_NATIVE_FACTORED_Q4_TRACE=0']),
              ('old111-tag',['-UPIONEER_DRIVER_BUILD_TAG','-DPIONEER_DRIVER_BUILD_TAG=0x2026110600010111ULL']),
              ('old111-alias',['-DPIONEER_NATIVE_FACTORED_Q4_COMPARE=1']),
              ('old108-alias',['-DPIONEER_NATIVE_COMPACT_Q4_PAIR_ISSUE_TIMING=1'])]
for macro in ('PIONEER_NATIVE_Q4_FPU_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_MODEL_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_MODEL_BAND_ALIGNMENT64',
              'PIONEER_NATIVE_DRAM_LOAD_DIAGNOSTIC', 'PIONEER_NATIVE_DRAM_LOAD_ALIGNMENT64', 'PIONEER_NATIVE_BFP_DIAGNOSTIC',
              'PIONEER_NATIVE_BFP_BAND_DIAGNOSTIC', 'PIONEER_NATIVE_MODEL_BAND_DIAGNOSTIC',
              'PIONEER_DRAM_READ_SWEEP_NATIVE_CONTEXT', 'PIONEER_Q38_TOKEN_NATIVE_CONTEXT', 'TT_PIONEER_Q38_WORK_MODE',
              'PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'PIONEER_NATIVE_Q4_FPU_STREAM_DIAGNOSTIC',
              'PIONEER_NATIVE_MIXED_FPU_STREAM_DIAGNOSTIC', 'PIONEER_Q38_FULL_OFFLOAD_NATIVE', 'PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
    for value in (0, 1):
        cases.append((macro + '=' + str(value), ['-D' + macro + '=' + str(value)]))
for label, extra in cases:
    # Preprocessing stdout contains PRIVATE weights:NEVER retain or publish it.
    result = subprocess.run(common + native + extra + includes + ['-E', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and any(message in result.stderr for message in ('Build only the exact diagnostic104',
        'Diagnostic104 cannot alias','Exact separate timed compact Q4 diagnostic107 role required','Exact diagnostic107 kernel identity required',
        'Exact separate paired compact Q4 diagnostic108 role required','Exact diagnostic108 kernel identity required',
        'Exact separate factored-Q4 comparison111 role required','Exact comparison111 kernel identity required',
        'Exact separate factored-Q4 trace112 role required','Exact trace112 kernel identity required')),label
(output / 'driver-role-refusals.txt').write_text('\n'.join('PASS ' + label for label, _ in cases) + '\n')
wrong = output / 'wrong-image';wrong.mkdir(mode=0o700)
(wrong / 'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header.replace('LayoutRole='+str(role), 'LayoutRole=86'))
result = subprocess.run(common + native + ['-I' + str(wrong)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp')],
                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
assert result.returncode and 'exact compact QCS1 two-width image and Dst scope only' in result.stderr
for field, value in (('MetadataFirst=128', 'MetadataFirst=0'), ('MetadataRows=128', 'MetadataRows=64'), ('DstLast=255', 'DstLast=127')):
    (wrong / 'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header.replace(field, value))
    result = subprocess.run(common + native + ['-I' + str(wrong)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp')], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
    assert result.returncode and 'exact compact QCS1 two-width image and Dst scope only' in result.stderr
if args.factored_compare:
    for field,value in (('FactoredComparison=1','FactoredComparison=0'),('NewPhases=2','NewPhases=4'),('Accumulator=80','Accumulator=32')):
        (wrong / 'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header.replace(field,value))
        result=subprocess.run(common+native+['-I'+str(wrong)]+includes+['-fsyntax-only',str(driver/'pioneer_native_compact_q4_stream_driver.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=output)
        assert result.returncode and 'exact comparison111 changed arithmetic and ownership' in result.stderr
if args.factored_trace:
    for field,value in (('FactoredTrace=1','FactoredTrace=0'),('TraceWords=64','TraceWords=32'),('TraceBlocks=20','TraceBlocks=68'),('NewPhases=2','NewPhases=4'),('Accumulator=80','Accumulator=32')):
        (wrong / 'PioneerNativeCompactQ4StreamGeneratedImage.h').write_text(header.replace(field,value))
        result=subprocess.run(common+native+['-I'+str(wrong)]+includes+['-fsyntax-only',str(driver/'pioneer_native_compact_q4_stream_driver.cpp')],stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True,cwd=output)
        assert result.returncode and 'exact trace112 fixed observations and ownership' in result.stderr
wrong_data = output / 'wrong-data';wrong_data.mkdir(mode=0o700)
(wrong_data / 'PioneerNativeCompactQ4StreamData.h').write_text('#include <stdint.h>\nstruct PioneerNativeCompactQ4StreamData {static constexpr unsigned Version=1,Rows=32,Samples=9,Role=86,PreparedBytes=4608;static unsigned Width(unsigned s){return s<6?5120:17408;}static unsigned Words(unsigned s){return Width(s)/256*1280;}static uint32_t Word(unsigned,unsigned){return 0;}};\n')

result = subprocess.run(common + native + ['-I' + str(wrong_data)] + includes + ['-fsyntax-only', str(driver / 'pioneer_native_compact_q4_stream_driver.cpp')],
                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, cwd=output)
assert result.returncode and 'exact compiled diagnostic104 data only' in result.stderr
(output / 'placement-compile-refusals.txt').write_text('PASS wrong-image-role and three Dst-scope refusals actual full driver template instantiation\nPASS wrong-data-role actual full driver template instantiation\n')
def compiler_path(name):
    return subprocess.check_output([cxx, '-print-file-name=' + name], text=True).strip()
native_name='tenstorrent_pci_probe-native-q4-fpu-stream'+str(role)
for obj, name in [('native.o', native_name), ('control.o', 'tenstorrent_pci_probe-default43-control')]:
    run([cxx, '-shared', '-nostdlib', '-Wl,--no-undefined,-soname=tenstorrent_pci_probe', compiler_path('crtbeginS.o'),
         str(release / 'system/glue/haiku_version_glue.o'), str(output / obj), str(output / 'profile.o'),
         str(release / 'system/kernel/kernel.so'), compiler_path('libgcc.a'), compiler_path('crtendS.o'), '-o', str(output / name)])
binary = output / native_name
exports = subprocess.check_output([tool + 'nm', '-D', str(binary)], text=True)
(output / 'native-exports.txt').write_text(exports)
assert {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'} <= {line.split()[-1] for line in exports.splitlines() if line.split()}
legacy_name = 'PioneerNativeCompactQ4StreamLegacyFindDevice'
legacy = subprocess.check_output([tool + 'objdump', '-d', '--disassemble=_Z' + str(len(legacy_name)) + legacy_name + 'PKc', str(binary)], text=True)
(output / 'native-legacy-entry.disassembly').write_text(legacy)
instructions = []
for line in legacy.splitlines():
    match = re.match(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+([a-z.]+)\s*(.*)$', line)
    if match:
        instructions.append((match[1], match[2].strip()))
assert instructions == [('add', 'sp,sp,-16'), ('sd', 's0,8(sp)'), ('add', 's0,sp,16'), ('ld', 's0,8(sp)'), ('li', 'a0,0'), ('add', 'sp,sp,16'), ('ret', '')]
assert hashlib.sha256((output / 'tenstorrent_pci_probe-default43-control').read_bytes()).hexdigest() == '2df0e0f845ba5847cbf519916d58e40e7827dbeaf1fef7c7d9ccc6ff432df9cc'
(output / 'build-argv.txt').write_text('\n'.join('\t'.join(c) for c in commands) + '\n')
private_unchanged()
for name in (native_name, 'tenstorrent_pci_probe-default43-control'):
    print(hashlib.sha256((output / name).read_bytes()).hexdigest(), name)
print(f'PASS BUILD_ONLY actual native{role} driver/all methods LINKED,{len(cases)} role refusals,actual wrong-image/wrong-data refusals,legacy NULL/control unchanged;PRIVATE headers unchanged;NO stage/open/hardware/full-model/speed authority')
