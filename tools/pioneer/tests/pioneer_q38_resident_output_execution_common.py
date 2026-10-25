"""Pinned actual predecessor compilation for new-controller TEST endpoints only."""
import hashlib
import os
import pathlib
import re
import runpy
import struct
import subprocess

HERE = pathlib.Path(__file__).resolve().parent


def compile_test(directory, sources, fixtures, *, producer=True, native_projection=False, extra_flags=(), extra_objects=()):
    assert len(fixtures) == 5
    for name, (section, size, digest, _) in runpy.run_path(str(HERE.parent/'generate_qwen38_projection_stash_images.py'))['PINS'].items():
        data = (pathlib.Path(fixtures[0 if name.startswith('root-') else 1])/(section+'.bin')).read_bytes()
        assert len(data) == size and hashlib.sha256(data).hexdigest() == digest
    for index in (0, 1, 2):
        assert (pathlib.Path(fixtures[index])/'boot.bin').read_bytes() == struct.pack('<I', 0x0006c06f)
    zero = pathlib.Path(fixtures[2]);assert len((zero/'text.bin').read_bytes()) == 2652
    assert hashlib.sha256((zero/'text.bin').read_bytes()).hexdigest() == '39817b679f4358e47c0e6602fb5ae184831206c11dab0b63fe9b7173c410df8b'
    for index, digest in ((3,'c754b24f767f33a00e61d2789cea3bda8ae656f6d58a2d20517311673a5aaaa1'),(4,'f13d8676f97920c182fa47d9f555453d7855dd5ac394919a577d58c83abe523a')):
        path = pathlib.Path(fixtures[index]);assert hashlib.sha256((path/'code.bin').read_bytes()).hexdigest() == digest
        assert hashlib.sha256((path/'boot.bin').read_bytes()).hexdigest() == 'd0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'
    flags = sorted(set(re.findall(r'-D[A-Z][A-Z0-9_]*(?:=[0-9]+)?', (HERE/'run_pioneer_q38_projection_graph_cooperative_tests.sh').read_text())))
    flags.remove('-DPIONEER_Q38_TEST_PROJECTION_FOOTPRINT')
    flags += ['-DPIONEER_Q38_RESIDENT_OUTPUT_OWNER=1','-DPIONEER_Q38_PROJECTION_RESULT_STASH=1','-DPIONEER_Q38_TEST_CLOSED_INPUT_POSITION_ZERO=1',
              '-DPIONEER_Q38_EMBEDDING_NATIVE=1','-DPIONEER_Q38_LEARNED_RMS_NATIVE=1','-DPIONEER_Q38_PROJECTION_EXECUTION_OWNER=1',
              '-DPIONEER_Q38_NORMAL_CLOCK=1','-DPIONEER_Q38_GDN_MUTABLE_STATE=','-DPIONEER_Q38_DECODER_RUNTIME=1',
              '-DPIONEER_Q38_TEST_RESIDENT_OUTPUT_EXECUTION=1']
    for flag in extra_flags:
        assert re.fullmatch(r'-D[A-Z][A-Z0-9_]*(?:=[A-Za-z0-9_]+)?',flag),flag
        key=flag.split('=',1)[0]
        flags=[old for old in flags if old!=key and not old.startswith(key+'=')]
        flags.append(flag)
    native = (HERE.parents[2]/'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
    for prefix, names in [('embedding',('scope','unsafe','mapping','factory','end')),('rms',('scope','factory'))]:
        for name in names:
            marker = 'PIONEER_Q38_'+('EMBEDDING' if prefix == 'embedding' else 'LEARNED_RMS')+'_NATIVE_'+name.upper()
            begin,end = '// '+marker+'_BEGIN','// '+marker+'_END';assert native.count(begin) == native.count(end) == 1
            (directory/(prefix+'_factory_'+name+'.inc')).write_text(native.split(begin,1)[1].split(end,1)[0])
    (directory/'embedding_factory_retain.inc').write_text(native.split('// PIONEER_Q38_RETAIN_MEMORY_BEGIN',1)[1].split('// PIONEER_Q38_RETAIN_MEMORY_END',1)[0])
    if native_projection:
        assert not producer
        for name in ('scope','unsafe','factory','end'):
            marker='PIONEER_Q38_PROJECTION_EXECUTION_NATIVE_'+name.upper()
            (directory/('projection_factory_'+name+'.inc')).write_text(native.split('// '+marker+'_BEGIN',1)[1].split('// '+marker+'_END',1)[0])
        marker='PIONEER_Q38_PROJECTION_MEMORY_NATIVE_ADAPTER'
        (directory/'projection_factory_mapping.inc').write_text(native.split('// '+marker+'_BEGIN',1)[1].split('// '+marker+'_END',1)[0])
        start='static bool\nEndQ38DispatchOwnership('
        (directory/'projection_factory_complete_end.inc').write_text(start+native.split(start,1)[1].split('// PIONEER_Q38_DISPATCH_RETAIN_END',1)[0])
    base = (['pioneer_q38_projection_execution_producer','pioneer_q38_projection_execution_machine'] if producer else [])
    base += ['pioneer_q38_embedding_execution_bridge','pioneer_q38_learned_rms_execution_bridge']
    if native_projection:
        base += ['pioneer_q38_projection_native_transport','pioneer_q38_projection_execution_machine']
    include_flags=['-I'+str(directory)]
    if native_projection:
        include_flags += ['-I'+str(HERE.parents[2]/'src/add-ons/kernel/drivers/misc')]
    binary = directory/'test'
    for obj in extra_objects:
        assert pathlib.Path(obj).is_absolute() and pathlib.Path(obj).is_file() and not pathlib.Path(obj).is_symlink() and pathlib.Path(obj).suffix=='.o'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',*flags,
                    *include_flags,*[str(HERE/(name+'.cpp')) for name in sources+base],*[str(obj) for obj in extra_objects],'-o',str(binary)],check=True)
    return binary


def run_test(binary, arguments):
    subprocess.run([str(binary),*arguments],check=True,env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'})
