#!/usr/bin/env python3
"""CPU-only layer-29 localization from pinned CPU and partial HOST-native DATA."""
import array
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import load_cpu, require, stable_bytes

def main():
    require(len(sys.argv)==4, 'MODEL CPU_TRACE_DIRECTORY PARTIAL_NATIVE_DIRECTORY')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ), 'role overrides')
    model,cpu_path,partial=map(pathlib.Path,sys.argv[1:])
    require(all(p.is_absolute() for p in (model,cpu_path,partial)), 'absolute inputs')
    cpu=load_cpu(cpu_path)[0]
    raw=stable_bytes(partial/'hidden.raw',65*5120*4)
    require(hashlib.sha256(raw).hexdigest()=='5a1c8478fea72b7a3d1acc63eecdebdb3fbdcf3c1f57166e04a3a19aa99ab784', 'original partial HOST-native token0 pin')
    native=array.array('I');native.frombytes(raw)
    require(sys.byteorder=='little' and native.itemsize==4, 'little-endian words')
    require(native[:30*5120]==cpu['bits'][:30*5120], 'earlier embedding+29 boundaries must match')
    require(native[30*5120:31*5120]!=cpu['bits'][30*5120:31*5120], 'known first differing boundary')
    words=array.array('I',[0x31524c51,1,29,5120,3]+[0]*11)
    words.extend(cpu['bits'][29*5120:31*5120]);words.extend(native[30*5120:31*5120])
    here=pathlib.Path(__file__).resolve().parent
    source=(here/'pioneer_q38_cpu_model_reference.cpp').read_text()
    start=source.index('struct Q38CpuModelReference::Impl {')
    end=source.index('\nQ38CpuModelReference::Q38CpuModelReference()',start)
    impl=source[start:end].replace('struct Q38CpuModelReference::Impl {','struct Q38LayerReplay {',1)
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-layer29-',dir='/private/tmp') as temporary:
        root=pathlib.Path(temporary)
        (root/'actual_cpu_replay_impl.inc').write_text(impl)
        snapshot=root/'CPU_LOCAL_INPUTS_NOT_PHYSICAL_OBSERVATIONS.bin'
        with snapshot.open('xb') as stream:stream.write(words.tobytes())
        snapshot.chmod(0o400)
        print('LAYER29_CPU_ONLY_INPUT',len(words)*4,hashlib.sha256(words.tobytes()).hexdigest(),flush=True)
        flags=['-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
        binary=root/'replay'
        subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(root),str(here/'pioneer_q38_layer29_replay.cpp'),
                        str(here/'pioneer_q38_layer29_math_bridge.cpp'),str(here/'pioneer_q38_layer29_precise_beta_bridge.cpp'),
                        str(here/'pioneer_q38_sigmoid_precision_host_bridge.cpp'),
                        str(here/'pioneer_q38_whole_original_bytes.cpp'),'-o',str(binary)],check=True)
        return subprocess.run([str(binary),str(model),str(snapshot)],env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'quarantine_size_mb=32'}).returncode
if __name__=='__main__':
    raise SystemExit(main())
