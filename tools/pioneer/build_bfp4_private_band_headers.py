#!/usr/bin/env python3
"""Generate PRIVATE fixed BFP4 diagnostic headers; no device or model read."""
import argparse
import hashlib
import os
from pathlib import Path
import stat
import struct

assert __debug__ and not os.environ.get('PYTHONOPTIMIZE')
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('samples',type=Path)
parser.add_argument('output',type=Path)
args=parser.parse_args()
assert args.samples.is_absolute() and not args.samples.is_symlink()
directory=args.samples.stat()
assert stat.S_ISDIR(directory.st_mode) and directory.st_uid==os.geteuid() and stat.S_IMODE(directory.st_mode)==0o700
assert args.output.is_absolute() and not args.output.exists() and not args.output.is_symlink()
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
pins=('edbc6d7cadcd85d0114ff16a5c44afb3627296497a7a0e03f76efa41da5ac7dd',
      '59f05959ef8486021eefd0b7e02e083b6f18a7eef747f15d5e24e6e87c587695',
      '036bcee72fc9bb84fedb9c6f93a6d22358f9d7c90c610cc5c19159f002a587e3')
samples=[]
for i,digest in enumerate(pins):
    path=args.samples/('band'+str(i)+'.rbp4')
    info=path.lstat()
    assert stat.S_ISREG(info.st_mode) and info.st_uid==os.geteuid() and info.st_nlink==1 and stat.S_IMODE(info.st_mode)==0o600
    data=path.read_bytes()
    assert len(data)==102544 and hashlib.sha256(data).hexdigest()==digest
    assert struct.unpack_from('<4I',data)==(0x34504252,1,5120,(0,8704,17376)[i])
    samples.append(data)
weights=[struct.unpack_from('<23040I',data,16) for data in samples]
inputs=[struct.unpack_from('<2560I',data,16+160*576) for data in samples]
gold=[struct.unpack_from('<32I',data,16+160*576+5120*2) for data in samples]
assert inputs[0]==inputs[1]==inputs[2]
args.output.mkdir(mode=0o700)
def write_private(name,lines):
    fd=os.open(args.output/name,os.O_WRONLY|os.O_CREAT|os.O_EXCL|os.O_CLOEXEC,0o600)
    with os.fdopen(fd,'w') as file:
        file.write('\n'.join(lines)+'\n')
        file.flush()
        os.fsync(file.fileno())
def array(name,values):
    result=['  static const uint32_t '+name+'['+str(len(values))+']={']
    for i in range(0,len(values),8):
        result.append('   '+','.join('0x%08xU'%v for v in values[i:i+8])+',')
    return result+['  };']
data=['/* PRIVATE MODEL-DERIVED DATA: DO NOT COMMIT OR REDISTRIBUTE. */',
      '#ifndef PIONEER_NATIVE_BFP4_DRAM_MODEL_DATA_H','#define PIONEER_NATIVE_BFP4_DRAM_MODEL_DATA_H',
      '#include <stdint.h>','struct PioneerNativeBfp4DramModelData {',
      ' static uint32_t Weight(unsigned sample,unsigned word){', '  if(sample>=3 || word>=23040)return 0;']
for i,values in enumerate(weights):data+=array('w'+str(i),values)
data+=['  return sample==0?w0[word]:sample==1?w1[word]:w2[word];',' }',
       ' static uint32_t Input(unsigned sample,unsigned word){','  if(sample>=3 || word>=2560)return 0;']
data+=array('input',inputs[0])
data+=['  return input[word];',' }','};','#endif']
write_private('PioneerNativeBfp4DramModelData.h',data)
metadata=['/* PRIVATE DERIVED MODEL PREDICTIONS: DO NOT COMMIT. */',
          '#ifndef PIONEER_NATIVE_BFP4_DRAM_MODEL_METADATA_H','#define PIONEER_NATIVE_BFP4_DRAM_MODEL_METADATA_H',
          '#include <stdint.h>','struct PioneerNativeBfp4DramModelMetadata {',
          ' static const uint32_t* Golden(unsigned sample){']
for i,values in enumerate(gold):metadata+=array('g'+str(i),values)
metadata+=['  return sample==0?g0:sample==1?g1:sample==2?g2:nullptr;',' }','};','#endif']
write_private('PioneerNativeBfp4DramModelMetadata.h',metadata)
for i,digest in enumerate(pins):
    assert hashlib.sha256((args.samples/('band'+str(i)+'.rbp4')).read_bytes()).hexdigest()==digest
for name in ('PioneerNativeBfp4DramModelData.h','PioneerNativeBfp4DramModelMetadata.h'):
    path=args.output/name
    assert stat.S_IMODE(path.stat().st_mode)==0o600
    print(hashlib.sha256(path.read_bytes()).hexdigest(),name)
print('PASS PRIVATE fixed BFP4 header generation; NO device or Work access')
