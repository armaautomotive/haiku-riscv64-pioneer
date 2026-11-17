#!/usr/bin/env python3
"""Build a matched Qwen head A/B pair from the measured vector92 stack.

This writes local build artifacts only. It never stages a benchmark, opens
the card, or modifies the installed driver, Assistant, model, or cache.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory


SEED = '113fdbd404b1a5e55d160c9b8ca5ecf15c57208e'
OLD_TAG = '0x2026110400010092'
DRIVER_DIR = Path('src/add-ons/kernel/drivers/misc')
TOOLS = Path('tools/pioneer')
DRIVER = 'tenstorrent_pci_probe-native'
WORKER = 'HaikuAssistantQwen27FullOffload'
RATE = 'pioneer-q38-full-offload-rate-smoke'
CONTROL = {
    DRIVER: 'c21b9bb80bfe7bb6d62540c267254fea33505842c2421cfd2732a389053be014',
    WORKER: '1ea9f6ac9c2fc5022bcce92be8949b6432827e76c984835935db3bdc9d2301a9',
    RATE: '9aa2ceea44c0143e6af53f5ad8050c50e2101826dc892505b42c60eb392558af',
    'PioneerQ38FullOffloadVectorQ4Programs.h':
        '3ec53793460dbb937368f952da6a5d19f83e82e27d1a027e416952a309bec389',
}
# Experiment-specific tags and distinct, fresh remote path names. The build
# never creates these remote paths or gives either binary run authority.
VARIANTS = {
    'serial': ('0x2026111700010207',
               'a4cbb1ee3da4f2996c53bb4a1c2b7a98126e46e51a278f91aa67599e917b36b9',
               (129144, 'd4f8c23111b60b0ae0f6d641cd3c48194052350979b699287a996ae0a98d59e1'),
               (32072, '6bc19c8e08e98bf5e9e6220b2bbeef0081663933f407403f077d968c86a13122')),
    'candidate': ('0x2026111700010208',
                  '0c78b9ef5035016ed1fd18d5eadf9f479ab728cdd7e57182c0046060a47ed962',
                  (130972, 'f79a41a69946be350906acf7251ec7cab9340dd3b4c6b22af0597572f1946ef1'),
                  (32320, '10f8f6c2744f66c28e6d90e38ef2ec4626c4615fac56b7d36878701e159cefc4')),
}
ARRAY = re.compile(r'// SHA256 ([0-9a-f]{64})\nstatic const uint32_t ([A-Za-z]+)\[\]=\{'
                   r'.*?\n\};\nstatic_assert\(sizeof\(\2\)==[0-9]+,"exact complete \2 DATA"\);', re.S)
MARKER = 'PIONEER_Q38_HEAD_CSR_NATIVE_PAIR'
ROOT_PAIR = frozenset({'RootText', 'RootAux'})


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pinned(path, digest):
    assert path.is_file() and not path.is_symlink(), path
    data = path.read_bytes()
    assert sha(data) == digest, path
    return data


def root_sections(text_pin, conv_pin):
    return (
        ('RootBoot', 'boot', (4, '32791432e2d260817d9007281f3719631163b7e9cd5c44dd4c059eaefca668ca')),
        ('RootText', 'text', text_pin),
        ('RootTerminal', 'terminal', (8, '468eb84a74c067cc328b3fb3e51b768a8440cd67d292add55eef6861c9b24de2')),
        ('RootAux', 'conv', conv_pin))


def load_sections(tool, image_path, folder, elf_sha, sections):
    image = pinned(image_path, elf_sha)
    arrays = {}
    with TemporaryDirectory(prefix='q38-pair-sections-', dir='/private/tmp') as stage:
        elf = Path(stage) / 'checked.elf'
        elf.write_bytes(image)
        for label, section, (size, digest) in sections:
            data = pinned(folder / (section + '.bin'), digest)
            assert len(data) == size and not size % 4
            dest = Path(stage) / (section + '.bin')
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', '.' + section, elf, dest], check=True)
            assert data == dest.read_bytes(), (image_path, section)
            arrays[label] = data
    return arrays


def load_variant(pair, name, tool):
    tag, elf_sha, text_pin, conv_pin = VARIANTS[name]
    folder = pair if name == 'candidate' else pair / 'serial'
    arrays = load_sections(tool, pair / (name + '.elf'), folder, elf_sha, root_sections(text_pin, conv_pin))
    return tag, arrays


def new_programs(original, arrays, replacements=ROOT_PAIR):
    assert replacements in (ROOT_PAIR, ROOT_PAIR | {'PeerFront'})
    assert replacements <= set(arrays) <= replacements | {'RootBoot', 'RootTerminal'}
    replaced = set()

    def replace(match):
        label = match[2]
        if label not in replacements:
            if label in arrays:
                assert sha(arrays[label]) == match[1], label
            return match[0]
        assert label not in replaced
        replaced.add(label)
        data = arrays[label]
        words = [f'0x{int.from_bytes(data[i:i+4], "little"):08x}U' for i in range(0, len(data), 4)]
        rows = '\n'.join(' ' + ','.join(words[i:i+8]) + ',' for i in range(0, len(words), 8))
        return (f'// SHA256 {sha(data)}\nstatic const uint32_t {label}[]={{\n{rows}\n}};'
                f'\nstatic_assert(sizeof({label})=={len(data)},"exact complete {label} DATA");')

    assert len(ARRAY.findall(original)) == 14
    result = ARRAY.sub(replace, original)
    assert replaced == replacements and len(ARRAY.findall(result)) == 14
    return result


def verify_linked_arrays(tool, work, expected, *, namespace='PioneerQ38FullOffloadVectorQ4Programs'):
    # Measure the packaged driver, not just the generated C++ source/object.
    driver = work / DRIVER
    raw = subprocess.check_output([tool + 'nm', '-S', '--defined-only', driver], text=True)
    entries = {r[-1]: (int(r[0], 16), int(r[1], 16))
               for r in map(str.split, raw.splitlines()) if len(r) == 4}
    rows = subprocess.check_output([tool + 'objdump', '-t', driver], text=True)
    sections = {r[-1]: r[-3] for r in map(str.split, rows.splitlines()) if len(r) >= 6 and r[-1] in entries}
    headers = subprocess.check_output([tool + 'objdump', '-h', driver], text=True)
    origins = {r[1]: int(r[3], 16) for r in map(str.split, headers.splitlines())
               if len(r) >= 7 and r[0].isdigit()}
    section_data = {}
    verified = {}
    for label, digest in expected.items():
        matches = [symbol for symbol in entries if namespace in symbol
                   and symbol.endswith(f'{len(label)}{label}E')]
        assert len(matches) == 1, label
        symbol = matches[0]
        section = sections[symbol]
        if section not in section_data:
            dest = work / ('linked-' + section.removeprefix('.') + '.raw')
            subprocess.run([tool + 'objcopy', '-O', 'binary', '-j', section, driver, dest], check=True)
            section_data[section] = dest.read_bytes()
        vma, size = entries[symbol]
        start = vma - origins[section]
        data = section_data[section][start:start+size]
        assert start >= 0 and len(data) == size and sha(data) == digest, label
        verified[label] = [size, digest]
    return verified


def commands(directory):
    return [line.split('\t') for line in (directory/'build-argv.txt').read_text().splitlines()]


def build_variant(name, variant, out, source, native, client, tool, original, *,
                  marker=MARKER, pair_label='head-csr', other_tag=None, replacements=ROOT_PAIR):
    tag, arrays = variant
    assert pair_label in ('head-csr', 'ffn-retained') and marker in (MARKER, 'PIONEER_Q38_FFN_RETAINED_NATIVE_PAIR')
    if other_tag is None:
        other_tag = next(v[0] for n, v in VARIANTS.items() if n != name)
    refusal = ('Qwen head A/B build role required' if marker == MARKER
               else 'Qwen retained FFN A/B build role required')
    work = out / name
    work.mkdir(mode=0o700)
    overlay = work / 'source'
    shutil.copytree(source, overlay, symlinks=True)
    for filename in ('PioneerQ38FullOffloadVectorQ4Role.h', 'PioneerQ38FullOffloadProtocol.h',
                     'tenstorrent_pci_probe.cpp'):
        path = overlay / DRIVER_DIR / filename
        text = path.read_text()
        assert text.count(OLD_TAG) == 1
        text = text.replace(OLD_TAG, tag)
        if filename.endswith('Role.h'):
            text = f'#if !defined({marker}) || {marker} != 1\n#error {refusal}\n#endif\n' + text
        path.write_text(text)
    programs = new_programs(original, arrays, replacements)
    (overlay / TOOLS / 'PioneerQ38FullOffloadVectorQ4Programs.h').write_text(programs)
    nc = next(c for c in commands(native) if str(native/'native.o') in c and '-c' in c)
    nl = next(c for c in commands(native) if str(native/DRIVER) in c and '-shared' in c)
    uc, ul, _, rate = commands(client)
    assert all(c[0] == tool + 'g++' for c in (nc, nl, uc, ul, rate))
    for command in (nc, uc):
        command[:] = [a.replace(str(source), str(overlay)) for a in command]
        command.insert(1, '-D' + marker + '=1')
    old_tag = '-DPIONEER_DRIVER_BUILD_TAG=' + OLD_TAG + 'ULL'
    nc[nc.index(old_tag)] = '-DPIONEER_DRIVER_BUILD_TAG=' + tag + 'ULL'
    nc[nc.index('-MF')+1] = str(work/'native.d')
    nc[nc.index('-o')+1] = str(work/'native.o')
    uc[uc.index('-o')+1] = str(work/'worker.o')
    nl = [str(work/'native.o') if a == str(native/'native.o') else a for a in nl]
    nl[nl.index('-o')+1] = str(work/DRIVER)
    ul = [str(work/'worker.o') if a == str(client/'worker.o') else a for a in ul]
    ul[ul.index('-o')+1] = str(work/WORKER)
    worker_path = f'/boot/home/pioneer-q38-{pair_label}-{name}-{tag[-3:]}/{WORKER}'
    i = next(i for i, a in enumerate(rate) if a.startswith('-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER='))
    rate[i] = '-DPIONEER_FULL_OFFLOAD_SMOKE_WORKER="' + worker_path + '"'
    rate[rate.index('-o')+1] = str(work/RATE)
    argv = (nc, nl, uc, ul, rate)
    for command in argv:
        subprocess.run(command, check=True, cwd=work)
    for command in (nc, uc):
        refused = [('-U'+marker,), ('-U'+marker, '-D'+marker+'=0'),
                   ('-DPIONEER_Q38_CPU_VALIDATION=0',), ('-DPIONEER_Q38_WORK_INFERENCE=0',)]
        if command is nc:
            refused += [('-UPIONEER_DRIVER_BUILD_TAG', '-DPIONEER_DRIVER_BUILD_TAG='+t+'ULL')
                        for t in (OLD_TAG, other_tag)]
        for flags in refused:
            prefix, source_arg = command[:command.index('-c')], command[command.index('-c')+1]
            result = subprocess.run([*prefix, *flags, '-fsyntax-only', source_arg], cwd=work,
                                    capture_output=True, text=True)
            assert result.returncode and any(s in result.stderr for s in (
                refusal, 'Full-card vector Q4',
                'Full offload cannot alias', 'Full-offload client is separate')), flags
    for binary, baseline in ((DRIVER, native), (WORKER, client), (RATE, client)):
        undefined = lambda p: subprocess.check_output([tool+'nm', '-u', p])
        imports = undefined(work/binary)
        assert imports == undefined(baseline/binary), binary
    frames = [row.split('\t') for row in (work/'native.su').read_text().splitlines()]
    assert frames and all(len(r) == 3 and r[2] == 'static' for r in frames)
    assert max(int(r[1]) for r in frames if 'Token' in r[0] or 'FullOffload' in r[0]) <= 2048
    expected = {label: digest for digest, label in ARRAY.findall(programs)}
    measured = verify_linked_arrays(tool, work, expected)
    result = {'tag': tag, 'worker_path': worker_path, 'arrays': measured,
              'bins': {n: sha((work/n).read_bytes()) for n in (DRIVER, WORKER, RATE)}}
    (work/'build-argv.json').write_text(json.dumps(argv, indent=2)+'\n')
    (work/'manifest.json').write_text(json.dumps(result, indent=2, sort_keys=True)+'\n')
    print(name, tag, result['bins'], flush=True)
    return measured


def prepare_control(sdk, libraries, out):
    assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    assert all(p.is_absolute() for p in (sdk, libraries, out))
    assert not out.exists() and not out.is_symlink()
    tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    repo = Path(__file__).resolve().parents[2]
    out.mkdir(mode=0o700)
    source, native, client = (out/d for d in ('source', 'native-control', 'client-control'))
    source.mkdir(mode=0o700)
    with subprocess.Popen(['git', '-C', repo, 'archive', SEED, TOOLS, DRIVER_DIR,
                           'src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h',
                           'src/apps/assistant', 'headers', 'build/config_headers'], stdout=subprocess.PIPE) as proc:
        with proc.stdout:
            subprocess.run(['tar', '-x', '-C', source], stdin=proc.stdout, check=True)
        assert proc.wait() == 0
    (source/'src/libs').mkdir()
    (source/'src/libs/llama.cpp').symlink_to(repo/'src/libs/llama.cpp', target_is_directory=True)
    common = ('--batched', '--metrics', '--vector-q4')
    subprocess.run([sys.executable, source/TOOLS/'build_qwen38_full_offload_native.py', sdk, native, *common],
                   cwd=out, check=True)
    subprocess.run([sys.executable, source/TOOLS/'build_assistant_qwen27_full_offload.py', sdk, libraries, client, *common],
                   cwd=out, check=True)
    for path in (native/DRIVER, client/WORKER, client/RATE):
        pinned(path, CONTROL[path.name])
    path = source/TOOLS/'PioneerQ38FullOffloadVectorQ4Programs.h'
    original = pinned(path, CONTROL[path.name]).decode()
    return source, native, client, tool, original


def main():
    if not __debug__:
        raise SystemExit('do not disable paired-build identity checks')
    if len(sys.argv) != 5:
        raise SystemExit('use SDK FROZEN_LLAMA_LIBRARIES PAIRED_ROOTS NEW_OUTPUT')
    sdk, libraries, pair, out = map(Path, sys.argv[1:])
    assert pair.is_absolute()
    tool = str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    inputs = {name: load_variant(pair, name, tool) for name in VARIANTS}
    source, native, client, tool, original = prepare_control(sdk, libraries, out)
    verified = {n: build_variant(n, v, out, source, native, client, tool, original) for n, v in inputs.items()}
    assert {label for label in verified['serial'] if verified['serial'][label] != verified['candidate'][label]} == {
        'RootText', 'RootAux'}
    print('PASS BUILD ONLY: exact vector92 pins, unchanged imports, 14 linked arrays per variant; '
          'only ROOT text/conv differ. No hardware, correctness or speed qualification.')


if __name__ == '__main__':
    main()
