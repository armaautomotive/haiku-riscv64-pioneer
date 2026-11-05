#!/usr/bin/env python3
"""Audit complete unchanged non-math ROM, including the OLD service prefix.

Only the named new math functions may follow that prefix. This is source/
relocation evidence, NOT native permission to map or execute the extension.
"""
import hashlib
import pathlib
import re
import struct
import subprocess
from pioneer_q38_vector_q4_relocations import BASES

TAIL = {
    '_ZL7Counterj', '_ZL7IdleAllv', '_ZL5Routej', '_ZL13SourceMatchesRA16_Kj',
    '_ZL21PioneerQ38ExactQ4WorkPVKj',
    '_ZN26PioneerQ38ExactQ4Corrected3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_',
}


def audit(control, image, tool):
    directories = tuple(map(pathlib.Path, (control, image)))
    assert hashlib.sha256((directories[0]/'program.elf').read_bytes()).hexdigest() == \
        '4b3dc0c96d3888b9c43af683f58678cc91cade7c67e955413bad8c5e16d52870'
    raw = [{s:(d/(s+'.bin')).read_bytes() for s in BASES} for d in directories]
    maps = []
    for directory in directories:
        symbols = {}
        for row in subprocess.check_output([tool+'readelf','-sW',str(directory/'program.elf')],text=True).splitlines():
            p = row.split()
            if len(p)==8 and p[0].endswith(':') and p[3] in ('FUNC','OBJECT') and int(p[2]):
                symbols[int(p[1],16)] = (p[7],int(p[2]),p[3])
        maps.append(symbols)

    def contents(side, at, size):
        for section,base in BASES.items():
            if base<=at and at+size<=base+len(raw[side][section]):
                return raw[side][section][at-base:at-base+size]
        raise AssertionError(('whole symbol unmapped',side,hex(at),size))

    def same_targets(addresses, entry=False):
        result=[]
        for side,at in enumerate(addresses):
            name,size,kind=maps[side][at]
            result.append((re.sub(r'^CSWTCH\.\d+$','CSWTCH',name),size,kind,contents(side,at,size)))
        if entry:
            assert all(r[0]=='q38_compute_once' and r[2]=='FUNC' for r in result)
        else:
            assert result[0]==result[1],('entire target changed',result[0][:3],result[1][:3])
        return result[0][0]

    # No wholesale service exemption: compare EVERY old byte below, then
    # account for EVERY appended byte with an exact named FUNC. Only small
    # zero alignment gaps are allowed;no anonymous payload/control growth.
    end=BASES['service']+len(raw[0]['service'])
    limit=BASES['service']+len(raw[1]['service'])
    assert end<limit<=0x6e000
    tails=sorted((at,*item) for at,item in maps[1].items() if end<=at<limit)
    assert {r[1] for r in tails}==TAIL and len(tails)==len(TAIL)
    cursor=end
    for at,name,size,kind in tails:
        assert kind=='FUNC' and size>0 and cursor<=at and at-cursor<=12 and at+size<=limit
        assert contents(1,cursor,at-cursor)==bytes(at-cursor)
        cursor=at+size
    assert cursor==limit

    relocations=0
    for section,base in BASES.items():
        if section in ('text','affine'):
            continue  # deliberately changed arithmetic,executed separately
        size=len(raw[0][section])
        assert size==len(raw[1][section]) or section=='service'
        words=[struct.unpack('<'+'I'*(size//4),r[section][:size]) for r in raw]
        changed={i for i,(a,b) in enumerate(zip(*words)) if a!=b};covered=set()
        for i in range(len(words[0])-1):
            if not changed.intersection((i,i+1)):
                continue
            if not all(w[i]&0xfff==0x097 and w[i+1]&0xfffff==0x080e7 for w in words):
                continue
            addresses=[]
            for w in words:
                hi=w[i]&0xfffff000;hi-=(1<<32) if hi>>31 else 0
                lo=w[i+1]>>20;lo-=4096 if lo>>11 else 0
                addresses.append(base+i*4+hi+lo)
            entry=section=='globalworker' and base+i*4==0x21e28
            name=same_targets(addresses,entry)
            assert entry or all(maps[side][at][2]=='FUNC' for side,at in enumerate(addresses))
            covered.update(changed.intersection((i,i+1)));relocations+=1
            print('AUDITED_EXACT_Q4_WORK_CALL',section,hex(base+i*4),name,flush=True)
        for i in range(len(words[0])):
            if not all(w[i]&0x7f==0x37 for w in words) or words[0][i]&0xfff!=words[1][i]&0xfff:
                continue
            reg=(words[0][i]>>7)&31
            for j in range(i+1,min(i+17,len(words[0]))):
                if any((w[k]&0x7f) in (0x63,0x67,0x6f) for w in words for k in range(i+1,j)):
                    break
                if any((w[k]&0x7f) in (0x03,0x13,0x17,0x33,0x37,0x67,0x6f)
                       and (w[k]>>7)&31==reg for w in words for k in range(i+1,j)):
                    break
                if not changed.intersection((i,j)):
                    continue
                if not all(w[j]&0x707f==0x13 and (w[j]>>15)&31==reg for w in words):
                    continue
                if words[0][j]&0xfffff!=words[1][j]&0xfffff:
                    continue
                addresses=[]
                for w in words:
                    lo=w[j]>>20;lo-=4096 if lo>>11 else 0
                    addresses.append(((w[i]&0xfffff000)+lo)&0xffffffff)
                if not all(at in maps[side] for side,at in enumerate(addresses)):
                    continue
                name=same_targets(addresses)
                assert all(maps[side][at][2]=='OBJECT' for side,at in enumerate(addresses))
                covered.update(changed.intersection((i,j)));relocations+=1
                print('AUDITED_EXACT_Q4_WORK_DATA',section,hex(base+i*4),hex(base+j*4),name,flush=True)
        assert changed==covered,('unexplained non-matrix changes',section,sorted(changed-covered))
    assert relocations
    print('PASS_EXACT_Q4_WORK_NONMATH oldServiceBytes'+str(len(raw[0]['service'])),
          'explicitTailFunctions'+str(len(tails)),'NO_NATIVE_EXTENT_AUTHORITY',flush=True)
    return relocations
