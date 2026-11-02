#!/usr/bin/env python3
"""Audit every changed word OUTSIDE the deliberately changed matrix text.

Only an exact q38_compute_once entry call and references to complete,
byte-identical helpers/constants may move. This is not a general linker
relocation or changed-arithmetic waiver.
"""
import pathlib
import re
import struct
import subprocess

BASES = {'boot':0x6000, 'globalworker':0x20000, 'precise':0x30000,
         'dense':0x40000, 'ffn':0x60000, 'service':0x68000, 'fetch':0x70000,
         'affine':0x70b00, 'text':0x72000, 'gdn':0xb8000}


def audit(control, image, tool):
    directories = (pathlib.Path(control), pathlib.Path(image))
    raw = [{s:(d/(s+'.bin')).read_bytes() for s in BASES} for d in directories]
    maps = []
    for directory in directories:
        symbols = {}
        for row in subprocess.check_output([tool+'readelf', '-sW', str(directory/'program.elf')], text=True).splitlines():
            fields = row.split()
            # nm labels OBJECTs 't' when the linker places rodata in a
            # readonly executable output section. Use actual ELF types,
            # not that lossy section-derived label.
            if len(fields) == 8 and fields[0].endswith(':') and fields[3] in ('FUNC','OBJECT'):
                address,size,kind,name=fields[1],fields[2],fields[3],fields[7]
                if int(size):
                    symbols[int(address,16)] = (name,int(size),kind)
        maps.append(symbols)

    def contents(side, address, size):
        for section, base in BASES.items():
            if base <= address and address+size <= base+len(raw[side][section]):
                return raw[side][section][address-base:address-base+size]
        raise AssertionError(('unmapped complete symbol',side,hex(address),size))

    def targets(addresses, entry=False):
        records = []
        for side,address in enumerate(addresses):
            name,size,kind = maps[side][address]
            canonical = re.sub(r'^CSWTCH\.\d+$', 'CSWTCH', name)
            records.append((canonical,size,kind,contents(side,address,size)))
        if entry:
            assert all(r[0]=='q38_compute_once' and r[2]=='FUNC' for r in records)
        else:
            assert records[0] == records[1], ('entire referenced object changed',records[0][:3],records[1][:3])
        return records[0][0]

    relocations = 0
    for section, base in BASES.items():
        if section == 'text':
            continue
        assert len(raw[0][section]) == len(raw[1][section]) and not len(raw[0][section])%4
        words = [struct.unpack('<'+'I'*(len(r[section])//4), r[section]) for r in raw]
        changed = {i for i,(a,b) in enumerate(zip(*words)) if a != b}
        covered = set()
        for i in range(len(words[0])-1):
            if not changed.intersection((i,i+1)):
                continue
            if not all(w[i]&0xfff == 0x097 and w[i+1]&0xfffff == 0x080e7 for w in words):
                continue
            addresses = []
            for w in words:
                high=w[i]&0xfffff000
                high-=(1<<32) if high>>31 else 0
                low=w[i+1]>>20
                low-=4096 if low>>11 else 0
                addresses.append(base+i*4+high+low)
            entry=section=='globalworker' and base+i*4==0x21e28
            name=targets(addresses,entry)
            assert entry or name in (
                '_Z17PioneerSfpuLoad32I18VectorPanelEmitterEvRT_jj.isra.0',
                '_Z22PioneerQ38VectorAffineI18VectorPanelEmitterEvRT_.isra.0')
            covered.update(changed.intersection((i,i+1)))
            relocations+=1
            print('AUDITED_CONSTANT_Q4_ENTRY' if entry else 'AUDITED_IDENTICAL_Q4_HELPER',section,hex(base+i*4),name)
        # The compiler's immutable switch/setup tables move after new text.
        # Match only LUI + ADDI into the SAME register and exact old/new
        # complete named data bytes; no arbitrary address/value change.
        for i in range(len(words[0])):
            if not all(w[i]&0x7f==0x37 for w in words):
                continue
            if words[0][i]&0xfff != words[1][i]&0xfff:
                continue
            reg=(words[0][i]>>7)&31
            for j in range(i+1,min(i+7,len(words[0]))):
                if not changed.intersection((i,j)):
                    continue
                if not all(w[j]&0x707f==0x13 and (w[j]>>7)&31==reg and (w[j]>>15)&31==reg for w in words):
                    continue
                if words[0][j]&0xfffff != words[1][j]&0xfffff:
                    continue
                addresses=[]
                for w in words:
                    low=w[j]>>20
                    low-=4096 if low>>11 else 0
                    addresses.append(((w[i]&0xfffff000)+low)&0xffffffff)
                if not all(at in maps[side] for side,at in enumerate(addresses)):
                    continue
                name=targets(addresses)
                assert name in ('CSWTCH','_ZZL5SetupvE6values','_ZZL5SetupvE7indices')
                assert all(maps[side][at][2]=='OBJECT' for side,at in enumerate(addresses))
                covered.update(changed.intersection((i,j)))
                relocations+=1
                print('AUDITED_IDENTICAL_Q4_DATA',section,hex(base+i*4),hex(base+j*4),name)
        assert changed == covered, ('unexplained non-text instruction changes',section,sorted(changed-covered))
    assert relocations == 18, ('unexpected reference count',relocations)
    return relocations
