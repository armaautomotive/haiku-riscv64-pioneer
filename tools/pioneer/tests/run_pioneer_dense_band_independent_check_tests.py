#!/usr/bin/env python3
"""Synthetic parser/arithmetic refusal checks; no model or device."""
import os
from pathlib import Path
import subprocess
import tempfile
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
here=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='pioneer-dense-integer-check-',dir='/private/tmp') as tmp:
    directory=Path(tmp);binary=directory/'check';samples=directory/'samples';samples.mkdir()
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined',str(here/'pioneer_dense_band_independent_check.cpp'),'-o',str(binary)],check=True)
    env={**os.environ,'ASAN_OPTIONS':'quarantine_size_mb=32','UBSAN_OPTIONS':'halt_on_error=1'}
    def array(name,count,first=0):
        return 'const uint32_t '+name+'['+str(count)+']={'+','.join(['0x%08x'%first]+['0x00000000']*(count-1))+'};\n'
    data=''.join(array('w'+str(i),44160) for i in range(3))+array('input',3200)
    header=samples/'PioneerNativeBfpModelBandData.h';metadata=samples/'PioneerNativeBfpModelBandMetadata.h'
    prefix='Width=5120,Rows=1024,Tensor=45,Samples=3\nconst unsigned rows[3]={0,512,992}\n'
    header.write_text(data);metadata.write_text(prefix+''.join(array('g'+str(i),32) for i in range(3)))
    result=subprocess.run([str(binary),str(samples)],capture_output=True,text=True,env=env)
    assert result.returncode==0 and 'samples=3 rows=96' in result.stdout,(result.stdout,result.stderr)
    result=subprocess.run([str(binary),str(samples),'--qgwd-reference'],capture_output=True,text=True,env=env)
    assert result.returncode==0 and result.stdout.count('DENSE_INTEGER_REFERENCE ')==96 and 'sample2 first992 row31 word00000000' in result.stdout
    assert subprocess.run([str(binary),str(samples),'--recurrent-reference'],capture_output=True,env=env).returncode==1
    recurrent='Width=5120,Rows=6144,Tensor=3,Samples=3\nconst unsigned rows[3]={0,3072,6112}\n'
    metadata.write_text(recurrent+''.join(array('g'+str(i),32) for i in range(3)))
    result=subprocess.run([str(binary),str(samples),'--recurrent-reference'],capture_output=True,text=True,env=env)
    assert result.returncode==0 and result.stdout.count('RECURRENT_INTEGER_REFERENCE ')==96 and 'sample2 first6112 row31 word00000000' in result.stdout
    assert subprocess.run([str(binary),str(samples),'--qgwd-reference'],capture_output=True,env=env).returncode==1
    output_data=''.join(array('w'+str(i),52992) for i in range(3))+array('input',3840)
    out_meta='Width=6144,Rows=5120,Tensor=16,Samples=3\nconst unsigned rows[3]={0,2560,5088}\n'
    header.write_text(output_data);metadata.write_text(out_meta+''.join(array('g'+str(i),32) for i in range(3)))
    result=subprocess.run([str(binary),str(samples),'--recurrent-out-reference'],capture_output=True,text=True,env=env)
    assert result.returncode==0 and result.stdout.count('RECURRENT_OUT_INTEGER_REFERENCE ')==96 and 'sample2 first5088 row31 word00000000' in result.stdout
    assert subprocess.run([str(binary),str(samples),'--recurrent-reference'],capture_output=True,env=env).returncode==1
    header.write_text(data)
    qkv_meta='Width=5120,Rows=10240,Tensor=5,Samples=3\nconst unsigned rows[3]={0,5120,10208}\n'
    metadata.write_text(qkv_meta+''.join(array('g'+str(i),32) for i in range(3)))
    result=subprocess.run([str(binary),str(samples),'--recurrent-qkv-reference'],capture_output=True,text=True,env=env)
    assert result.returncode==0 and result.stdout.count('RECURRENT_QKV_INTEGER_REFERENCE ')==96 and 'sample2 first10208 row31 word00000000' in result.stdout
    assert subprocess.run([str(binary),str(samples),'--recurrent-reference'],capture_output=True,env=env).returncode==1
    metadata.write_text(array('g0',32)+array('g1',32,1)+array('g2',32))
    result=subprocess.run([str(binary),str(samples)],capture_output=True,text=True,env=env)
    assert result.returncode==1 and 'mismatch sample=1 row=0' in result.stderr
    metadata.write_text(prefix+''.join(array('g'+str(i),32) for i in range(3)))
    header.write_text(data.replace('0x00000000','0x00000001',1))
    assert subprocess.run([str(binary),str(samples)],capture_output=True,env=env).returncode==1
    assert subprocess.run([str(binary),'relative'],capture_output=True,env=env).returncode==2
print('PASS independent dense-band integer checker: synthetic zero, mismatch, corrupt header and path refusal; NO DEVICE')
