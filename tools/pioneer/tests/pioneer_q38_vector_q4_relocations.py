#!/usr/bin/env python3
"""Every non-matrix instruction change must be an exact symbolic relocation."""
import pathlib
import re
import struct
import subprocess

BASES = {'boot':0x6000, 'globalworker':0x20000, 'precise':0x30000,
         'dense':0x40000, 'ffn':0x60000, 'service':0x68000, 'fetch':0x70000,
         'affine':0x70b00, 'text':0x72000, 'gdn':0xb8000}


def audit(control, image, tool):
    directories = tuple(map(pathlib.Path, (control, image)))
    raw = [{s:(d/(s+'.bin')).read_bytes() for s in BASES} for d in directories]
    maps = []
    for directory in directories:
        symbols = {}
        for row in subprocess.check_output([tool+'readelf', '-sW', str(directory/'program.elf')], text=True).splitlines():
            p = row.split()
            if len(p) == 8 and p[0].endswith(':') and p[3] in ('FUNC', 'OBJECT') and int(p[2]):
                symbols[int(p[1],16)] = (p[7],int(p[2]),p[3])
        maps.append(symbols)

    def contents(side, at, size):
        for s,b in BASES.items():
            if b <= at and at+size <= b+len(raw[side][s]):
                return raw[side][s][at-b:at-b+size]
        raise AssertionError(('whole symbol unmapped',side,hex(at),size))

    def same_targets(addresses, entry=False):
        result = []
        for side,at in enumerate(addresses):
            name,size,kind = maps[side][at]
            result.append((re.sub(r'^CSWTCH\.\d+$','CSWTCH',name),size,kind,contents(side,at,size)))
        if entry:
            assert all(r[0] == 'q38_compute_once' and r[2] == 'FUNC' for r in result)
        else:
            assert result[0] == result[1], ('entire target changed',result[0][:3],result[1][:3])
        return result[0][0]

    relocations = 0
    for section,base in BASES.items():
        # Only the two matrix code regions deliberately change. The new
        # helper happens to be emitted in .text by GCC's local clone pass;
        # do NOT pretend its source section attribute moved native authority.
        if section in ('text','affine'):
            continue
        assert len(raw[0][section]) == len(raw[1][section])
        words = [struct.unpack('<'+'I'*(len(r[section])//4),r[section]) for r in raw]
        changed = {i for i,(a,b) in enumerate(zip(*words)) if a != b}
        covered = set()
        for i in range(len(words[0])-1):
            if not changed.intersection((i,i+1)):
                continue
            if not all(w[i]&0xfff == 0x097 and w[i+1]&0xfffff == 0x080e7 for w in words):
                continue
            addresses = []
            for w in words:
                hi = w[i]&0xfffff000
                hi -= (1<<32) if hi>>31 else 0
                lo = w[i+1]>>20
                lo -= 4096 if lo>>11 else 0
                addresses.append(base+i*4+hi+lo)
            entry = section == 'globalworker' and base+i*4 == 0x21e28
            name = same_targets(addresses,entry)
            assert entry or all(maps[side][at][2]=='FUNC' for side,at in enumerate(addresses))
            covered.update(changed.intersection((i,i+1)))
            relocations += 1
            print('AUDITED_VECTOR_METADATA_CALL',section,hex(base+i*4),name,flush=True)
        for i in range(len(words[0])):
            if not all(w[i]&0x7f == 0x37 for w in words) or words[0][i]&0xfff != words[1][i]&0xfff:
                continue
            reg = (words[0][i]>>7)&31
            for j in range(i+1,min(i+17,len(words[0]))):
                # GCC may derive two pointers in different destination
                # registers from one live LUI. Never cross an intervening
                # write to that source register (or infer a new dataflow).
                if any((w[k]&0x7f) in (0x63,0x67,0x6f) for w in words for k in range(i+1,j)):
                    break
                if any((w[k]&0x7f) in (0x03,0x13,0x17,0x33,0x37,0x67,0x6f)
                       and (w[k]>>7)&31 == reg for w in words for k in range(i+1,j)):
                    break
                if not changed.intersection((i,j)):
                    continue
                if not all(w[j]&0x707f == 0x13 and (w[j]>>15)&31 == reg for w in words):
                    continue
                if words[0][j]&0xfffff != words[1][j]&0xfffff:
                    continue
                addresses = []
                for w in words:
                    lo = w[j]>>20
                    lo -= 4096 if lo>>11 else 0
                    addresses.append(((w[i]&0xfffff000)+lo)&0xffffffff)
                if not all(at in maps[side] for side,at in enumerate(addresses)):
                    continue
                name = same_targets(addresses)
                assert all(maps[side][at][2] == 'OBJECT' for side,at in enumerate(addresses))
                covered.update(changed.intersection((i,j)))
                relocations += 1
                print('AUDITED_VECTOR_METADATA_DATA',section,hex(base+i*4),hex(base+j*4),name,flush=True)
        assert changed == covered, ('unexplained non-matrix instruction changes',section,sorted(changed-covered))
    assert relocations
    return relocations
