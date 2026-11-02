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

def main(emitted=None,wide=False):
    require(len(sys.argv)==4, 'MODEL CPU_TRACE_DIRECTORY PARTIAL_NATIVE_DIRECTORY')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ), 'role overrides')
    require(not (wide and emitted is not None), 'separate arithmetic-candidate and old emitted subjects')
    image_snapshots=[]
    if emitted is not None:
        require(emitted.is_absolute() and emitted.is_dir() and not emitted.is_symlink(), 'absolute emitted image directory')
        for name, size, digest in (
            ('boot.bin',4,'e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b'),
            ('code.bin',2864,'0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe')):
            raw_image=stable_bytes(emitted/name,size)
            require(hashlib.sha256(raw_image).hexdigest()==digest, 'unchanged post-dot-scale emitted '+name)
            image_snapshots.append((name,raw_image))
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
        if emitted is not None:
            # The engine opens ONLY private copies of the SAME authenticated
            # snapshots, never the caller's path again for each half-head.
            emitted=root/'AUTHENTICATED_LOCAL_IMAGE_NOT_CURRENT_LINKED_PEER'
            emitted.mkdir(mode=0o700)
            for name,data in image_snapshots:
                target=emitted/name
                with target.open('xb') as stream:stream.write(data)
                target.chmod(0o400)
        (root/'actual_cpu_replay_impl.inc').write_text(impl)
        snapshot=root/'CPU_LOCAL_INPUTS_NOT_PHYSICAL_OBSERVATIONS.bin'
        with snapshot.open('xb') as stream:stream.write(words.tobytes())
        snapshot.chmod(0o400)
        print('LAYER29_CPU_ONLY_INPUT',len(words)*4,hashlib.sha256(words.tobytes()).hexdigest(),flush=True)
        flags=['-std=c++17','-O1','-g','-ffp-contract=off','-Wall','-Wextra','-Werror','-fsanitize=address,undefined']
        if emitted is not None:
            flags.append('-DPIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC=1')
        if wide:
            flags.append('-DPIONEER_Q38_LAYER29_WIDE_REDUCTION_DIAGNOSTIC=1')
        binary=root/'replay'
        subprocess.run([os.environ.get('CXX','c++'),*flags,'-I'+str(root),str(here/'pioneer_q38_layer29_replay.cpp'),
                        str(here/'pioneer_q38_layer29_math_bridge.cpp'),str(here/'pioneer_q38_layer29_precise_beta_bridge.cpp'),
                        str(here/'pioneer_q38_sigmoid_precision_host_bridge.cpp'),
                        *([str(here/'pioneer_q38_layer29_emitted_delta_bridge.cpp')] if emitted is not None else []),
                        *([str(here/'pioneer_q38_layer29_wide_reduction_bridge.cpp')] if wide else []),
                        str(here/'pioneer_q38_whole_original_bytes.cpp'),'-o',str(binary)],check=True)
        return subprocess.run([str(binary),str(model),str(snapshot),*([str(emitted)] if emitted is not None else [])],env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1','ASAN_OPTIONS':'quarantine_size_mb=32'}).returncode
if __name__=='__main__':
    raise SystemExit(main())
