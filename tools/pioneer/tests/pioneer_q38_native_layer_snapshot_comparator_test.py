#!/usr/bin/env python3
"""CPU-derived SYNTHETIC snapshot controls,NEVER accelerator observations."""
import array
import contextlib
import io
import os
import pathlib
import shutil
import struct
import sys
import tempfile
import compare_pioneer_q38_native_layer_snapshot as compare

assert len(sys.argv) == 2 and not any(key.startswith(('PIONEER_', 'TT_PIONEER_')) for key in os.environ)
cpu = pathlib.Path(sys.argv[1])
assert cpu.is_absolute()
reference = compare.load_cpu(cpu)
positive = negative = different = 0
def refuse(action):
    global negative
    try:
        action()
    except (ValueError, OSError):
        negative += 1
    else:
        raise AssertionError('invalid incomplete-layer comparison accepted')

with tempfile.TemporaryDirectory(prefix='pioneer-q38-SYNTHETIC-native-layer-', dir='/private/tmp') as temporary:
    temporary = pathlib.Path(temporary)
    for position in range(3):
        for layers in (1, 4, 64):
            words = [0x314c4e51, 1, position, reference[position]['header'][3], layers, 5120, *([0]*10)]
            words += reference[position]['bits'][layers*5120:(layers+1)*5120]
            words += [0x454e444c]
            data = struct.pack('<'+str(len(words))+'I', *words)
            path = temporary/f'INCOMPLETE-native-position-{position}-layers-{layers}.bin'
            path.write_bytes(data)
            with contextlib.redirect_stdout(io.StringIO()):
                assert compare.main([str(cpu), str(path)]) == 0
            positive += 1
            changed = words[:]
            changed[16] ^= 1  # finite least-significant fraction change MUST remain numerical failure
            path.write_bytes(struct.pack('<'+str(len(changed))+'I', *changed))
            with contextlib.redirect_stdout(io.StringIO()):
                assert compare.main([str(cpu), str(path)]) == 1
            different += 1
    # Last intact fixture is position2/layer64. All controls below are DATA,
    # not claimed execution histories or physical files.
    path.write_bytes(data)
    assert compare.decode(data)['header'][2:5] == array.array('I', words[2:5])
    for field, value in [(0, 0x31424851), (1, 0), (2, 3), (3, 248320), (4, 0), (4, 65), (5, 5119),
                         *((i, 1) for i in range(6, 16)), (len(words)-1, 0)]:
        wrong = words[:]
        wrong[field] = value
        refuse(lambda: compare.decode(struct.pack('<'+str(len(wrong))+'I', *wrong)))
    for length in (0, 4, len(data)-1):
        refuse(lambda: compare.decode(data[:length]))
    refuse(lambda: compare.decode(data+b'\0'))
    nonfinite = words[:]
    nonfinite[16] = 0x7f800000
    refuse(lambda: compare.decode(struct.pack('<'+str(len(nonfinite))+'I', *nonfinite)))
    wrong_input = words[:]
    wrong_input[3] ^= 1
    path.write_bytes(struct.pack('<'+str(len(wrong_input))+'I', *wrong_input))
    refuse(lambda: compare.main([str(cpu), str(path)]))
    path.write_bytes(data)
    renamed = temporary/'NOT-AN-INCOMPLETE-SNAPSHOT.bin'
    renamed.write_bytes(data)
    refuse(lambda: compare.main([str(cpu), str(renamed)]))
    link = temporary/'symlink.bin'
    link.symlink_to(path)
    refuse(lambda: compare.main([str(cpu), str(link)]))
    refuse(lambda: compare.main([str(cpu), 'RELATIVE-NOT-OPENED.bin']))
    for role in ('PIONEER_Q38_WORK_INFERENCE', 'PIONEER_Q38_FULL_MODEL_CPU_REFERENCE', 'TT_PIONEER_Q38_WORK_MODE'):
        for value in ('0', '1'):
            os.environ[role] = value
            try:
                refuse(lambda: compare.main([str(cpu), str(path)]))
            finally:
                del os.environ[role]
    corrupt_cpu = temporary/'corrupt-cpu'
    corrupt_cpu.mkdir(mode=0o700)
    for position in range(3):
        shutil.copyfile(cpu/f'token-{position}.bin', corrupt_cpu/f'token-{position}.bin')
    corrupted = bytearray((corrupt_cpu/'token-0.bin').read_bytes())
    corrupted[100] ^= 1
    (corrupt_cpu/'token-0.bin').write_bytes(corrupted)
    refuse(lambda: compare.main([str(corrupt_cpu), str(path)]))
    with contextlib.redirect_stderr(io.StringIO()):
        try:
            compare.main([str(cpu), str(path), '--diagnostic-only'])
        except SystemExit as error:
            assert error.code == 2
            negative += 1
        else:
            raise AssertionError('diagnostic-only numerical acceptance was added')
assert positive == different == 9 and negative == 35, (positive, different, negative)
print(f'PASS incomplete native-layer comparator/{positive} CPU-derived SYNTHETIC positives/'
      f'{different} finite one-bit NUMERICAL_FAILURES/{negative} family,header,length,finite,path,role,pin+CLI refusals;'
      'ALL3 original CPU pins+full-model comparator unchanged;NO observed model,terminal,END,physical,Work or TPS proof')
