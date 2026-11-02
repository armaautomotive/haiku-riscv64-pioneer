#!/usr/bin/env python3
"""Actual replay CLI refusal tests. No model arithmetic or device access."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
from compare_pioneer_q38_observed_trace import require, stable_bytes

def main():
    wide='--wide-delta' in sys.argv
    if wide:
        require(sys.argv.count('--wide-delta')==1,'one explicit new wide-delta gate subject')
        sys.argv.remove('--wide-delta')
    require(len(sys.argv)==5, 'MODEL CPU_TRACE_DIRECTORY PARTIAL_NATIVE_DIRECTORY POST_SCALE_IMAGE_DIRECTORY')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ), 'no inherited role overrides')
    model,cpu,partial,image=map(pathlib.Path,sys.argv[1:])
    require(all(p.is_absolute() for p in (model,cpu,partial,image)), 'absolute input directories')
    code_size=3408 if wide else 2864
    boot=stable_bytes(image/'boot.bin',4);code=stable_bytes(image/'code.bin',code_size)
    require(hashlib.sha256(boot).hexdigest()=='e678476d9aa0bf79d0a982b2561908eb649740efa31b5131392d2e686a03d22b','original boot')
    require(hashlib.sha256(code).hexdigest()==('1ab17ccc83b072f6064fb13885ce4e3c4bdfc48ea9814440bf385a5694937979' if wide else '0c61dc435a3c2e00c8ae8a11319a307c86707b410100f7fbc85d15f8587bfcbe'),'selected code')
    runner=pathlib.Path(__file__).with_name('run_pioneer_q38_layer29_wide_emitted_replay.py' if wide else 'run_pioneer_q38_layer29_emitted_replay.py')
    refused=0
    with tempfile.TemporaryDirectory(prefix='pioneer-q38-layer29-emitted-gates-',dir='/private/tmp') as temporary:
        root=pathlib.Path(temporary);candidate=root/'image';candidate.mkdir()
        compiler=str(root/'COMPILER_MUST_NOT_RUN')
        env={**os.environ,'CXX':compiler}
        def reset():
            (candidate/'boot.bin').write_bytes(boot);(candidate/'code.bin').write_bytes(code)
        def run(tail=None,roles=None,positive=False):
            nonlocal refused
            args=[str(model),str(cpu),str(partial),str(candidate)] if tail is None else tail
            result=subprocess.run([sys.executable,str(runner),*args],env={**env,**(roles or {})},capture_output=True,text=True)
            require(result.returncode!=0,'no CLI case may execute inference')
            if positive:
                require('LAYER29_CPU_ONLY_INPUT 61504 bd3faafe' in result.stdout and compiler in result.stderr,'valid image reaches ONLY compile sentinel')
            else:
                require('LAYER29_CPU_ONLY_INPUT' not in result.stdout and compiler not in result.stderr,'invalid input reaches neither snapshot nor compiler')
                refused+=1
        reset();run(positive=True)
        for at in range(4):
            for bit in range(8):
                reset();changed=bytearray(boot);changed[at]^=1<<bit;(candidate/'boot.bin').write_bytes(changed);run()
        for at in (0,code_size//2,code_size-1):
            for bit in range(8):
                reset();changed=bytearray(code);changed[at]^=1<<bit;(candidate/'code.bin').write_bytes(changed);run()
        for name,data in (('boot.bin',boot),('code.bin',code)):
            for length in (0,len(data)-1,len(data)+1):
                reset();(candidate/name).write_bytes((data+b'\0')[:length]);run()
            reset();(candidate/name).unlink();run()
        reset();link=root/'linked-image';link.symlink_to(candidate,target_is_directory=True)
        run([str(model),str(cpu),str(partial),str(link)])
        run([str(model),str(cpu),str(partial),'relative-image'])
        run([str(model),str(cpu),str(partial),str(root/'missing-image')])
        run([str(model),str(cpu),str(partial)])
        run([str(model),str(cpu),str(partial),str(candidate),'extra-image'])
        for key in ('PIONEER_Q38_LAYER29_EMITTED_DELTA_DIAGNOSTIC','PIONEER_Q38_CPU_VALIDATION',
                    'PIONEER_Q38_WORK_INFERENCE','TT_PIONEER_Q38_WORK_MODE',
                    *(('PIONEER_Q38_LAYER29_WIDE_EMITTED_DIAGNOSTIC','PIONEER_Q38_WIDE_DELTA_EXPECTED_OBSERVER_TEST') if wide else ())):
            for value in ('0','1','unexpected'):run(roles={key:value})
        require((candidate/'boot.bin').read_bytes()==boot and (candidate/'code.bin').read_bytes()==code,'original candidate unchanged after routing checks')
    print(('PASS actual HOST wide-delta13 emitted layer29 CLI:' if wide else 'PASS actual HOST emitted layer29 CLI:')+'1 valid route stopped at compile sentinel/'+str(refused)+' pre-snapshot/pre-compiler refusals;all boot bits/first-middle-last code bits/type-length/path/arity/role cases;NO model arithmetic/device/native/Work authority')

if __name__=='__main__':
    main()
