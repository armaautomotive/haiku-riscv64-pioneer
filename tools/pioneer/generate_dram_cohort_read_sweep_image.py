#!/usr/bin/env python3
"""Authenticate ONE complete offboard ELF and emit immutable snapshot DATA.

No native owner or driver includes/selects this candidate. A snapshot address
map is not proof that any hardware was saved, held, restored or safe to launch.
"""
import hashlib
import os
import pathlib
import stat
import struct
import sys

ELF_SHA='0f697dde151a85b1a7a2808581e842de0a8139f7282fdd6c1bcdd87035791abc'
TEXT_SHA='bdfca6485d8f2c20f14bec8e0315b780987784a0027e8dacede82729acedb33b'
BOOT_SHA='d0565e4ef0f59aa08ad0be91ffae61ceb85aedff5f36f346c95312b0f9407ac6'

def render(path):
    assert path.is_absolute() and not path.is_symlink()
    with path.open('rb') as f:
        before=os.fstat(f.fileno())
        assert stat.S_ISREG(before.st_mode) and 0<before.st_size<1024*1024
        data=f.read(1024*1024+1);after=os.fstat(f.fileno())
        identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
        assert identity(before)==identity(after) and len(data)==before.st_size
    assert hashlib.sha256(data).hexdigest()==ELF_SHA
    h=struct.unpack_from('<16sHHIIIIIHHHHHH',data)
    assert h[0][:7]==b'\x7fELF\x01\x01\x01' and h[2]==243 and h[3]==1 and h[4]==0x6000 and h[8]==52 and h[11]==40
    assert h[6]+h[11]*h[12]<=len(data) and h[13]<h[12]
    sections=[struct.unpack_from('<IIIIIIIIII',data,h[6]+i*40) for i in range(h[12])]
    strings=sections[h[13]];names=data[strings[4]:strings[4]+strings[5]];found={}
    for s in sections:
        assert not (s[2]&1 and s[5]), 'writable ELF section'
        assert s[0]<len(names)
        end=names.index(0,s[0]);name=names[s[0]:end].decode('ascii')
        if not name:continue
        assert name not in found and s[4]+s[5]<=len(data)
        found[name]=(s,data[s[4]:s[4]+s[5]])
    boot=found['.boot'];text=found['.text']
    assert boot[0][3]==0x6000 and len(boot[1])==4 and hashlib.sha256(boot[1]).hexdigest()==BOOT_SHA
    assert text[0][3]==0x64000 and len(text[1])==8760 and hashlib.sha256(text[1]).hexdigest()==TEXT_SHA
    words=struct.unpack('<2190I',text[1]);first=struct.unpack('<I',boot[1])[0]
    rows=['  '+','.join('0x%08xU'%w for w in words[i:i+6])+',' for i in range(0,len(words),6)]
    return '''/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_DRAM_COHORT_READ_SWEEP_IMAGE_H
#define BLACKHOLE_DRAM_COHORT_READ_SWEEP_IMAGE_H
#include <stdint.h>
// Generated from complete ELF '''+ELF_SHA+'''.
// OFFBOARD DATA ONLY. No driver/native owner selects these bytes. All eight
// complete snapshots,exclusive ownership,clock,STOP/HOLD/restore still owed.
struct BlackholeDramCohortReadSweepImage {
 enum : uint32_t {TextWords=2190,CodeWords=2191,StackWords=2048,
  FrameWords=32,ReportWords=32,RawWords=65536,RootSavedWords=4563,
  ReaderSavedWords=69842,CohortSavedWords=493457,
  Entry=0x64000,Park=0x6400c,FaultPark=0x64030};
 static uint32_t Code(unsigned i){
  static const uint32_t text[TextWords]={
'''+ '\n'.join(rows)+'''
  };
  return i==0?0x%08xU:i<CodeWords?text[i-1]:0;
 }
 static unsigned SavedWords(bool coordinator){return coordinator?RootSavedWords:ReaderSavedWords;}
 static uint32_t Address(bool coordinator,unsigned i){
  if(i<CodeWords)return i?Entry+(i-1)*4:0x6000;
  i-=CodeWords;if(i<StackWords)return 0x6e000+i*4;
  i-=StackWords;if(i<FrameWords)return 0x74000+i*4;
  i-=FrameWords;if(i<ReportWords)return 0x74100+i*4;
  i-=ReportWords;if(i<3)return 0x74200+i*32;
  i-=3;
  if(!coordinator)return i<RawWords?0x90000+i*4:UINT32_MAX;
  if(!i)return 0x74300;
  i--;if(i<32)return 0x74400+i*4;
  i-=32;return i<224?0x74500+i*4:UINT32_MAX;
 }
};
#endif
'''%first

def main():
    assert len(sys.argv)==3 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
    source,out=map(pathlib.Path,sys.argv[1:])
    assert out.is_absolute() and not out.exists() and not out.is_symlink()
    data=render(source).encode('ascii') # authenticate BEFORE creating output
    with out.open('xb') as f:f.write(data)
    print('COHORT_IMMUTABLE_DATA '+hashlib.sha256(data).hexdigest()+
          ' all8SnapshotWords493457;NO native ownership/driver/install/hardware/Work grant')

if __name__=='__main__':main()
