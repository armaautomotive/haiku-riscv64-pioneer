#!/usr/bin/env python3
"""Bound the deliberate prepared-Q4 changes;no execution/native authority.

All pre-existing convolution/FFN/GDN/fetch code remains exact. The global
main and its twelve-entry dispatch table deliberately change to account
for the new reads. Everything before main is exact except symbol-resolved
literal relocations. Every service-tail byte belongs to a named function.
"""
import hashlib
import pathlib
import re
import struct
import subprocess
from pioneer_q38_vector_q4_relocations import BASES

HELPERS = {'_ZL7Counterj', '_ZL7IdleAllv', '_ZL5Routej', '_ZL13SourceMatchesRA16_Kj'}
TAIL = HELPERS | {
    '_ZL21PioneerQ38ExactQ4WorkPVKj',
    '_ZL14PreparedQ4ReadjjjjRjS_', '_ZL16PreparedQ4Inputsj',
    '_ZL15PreparedQ4GroupPVKjjRjS1_',
    '_ZN26PioneerQ38PreparedQ4Layout11GroupWindowEPVKjjS1_RNS_6WindowE',
    '_ZN25PioneerQ38NativeBfpMatvec5SetupI28PioneerQ38ExactQ4WorkEmitterEEbRT_j',
    '_ZN26PioneerQ38ExactQ4Corrected3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_',
}


def audit(control, image, tool, *, two_product=False):
    assert type(two_product) is bool
    directories = tuple(map(pathlib.Path, (control, image)))
    assert hashlib.sha256((directories[0]/'program.elf').read_bytes()).hexdigest() == \
        '8a7c6543e59a4067cee72d67843ca2d33119c307a8277bd14a70cb80f2f41e2f'
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
        raise AssertionError(('unmapped whole symbol',side,hex(at),size))

    def same_targets(addresses):
        result=[]
        for side,at in enumerate(addresses):
            name,size,kind=maps[side][at]
            result.append((re.sub(r'^CSWTCH\.\d+$','CSWTCH',name),size,kind,contents(side,at,size)))
        assert result[0][:3]==result[1][:3],('relocation target identity changed',result[0][:3],result[1][:3])
        if result[0][3]!=result[1][3]:
            # Moving an unchanged helper changes its PC-relative calls.
            # Resolve the COMPLETE callee recursively;no opcode/body waiver.
            assert result[0][0] in HELPERS and result[0][2]=='FUNC'
            words=[struct.unpack('<'+'I'*(len(r[3])//4),r[3]) for r in result]
            changed={i for i,(a,b) in enumerate(zip(*words)) if a!=b};covered=set()
            for i in range(len(words[0])-1):
                if not changed.intersection((i,i+1)):
                    continue
                if not all(w[i]&0xfff==0x097 and w[i+1]&0xfffff==0x080e7 for w in words):
                    continue
                targets=[]
                for side,w in enumerate(words):
                    hi=w[i]&0xfffff000;hi-=(1<<32) if hi>>31 else 0
                    lo=w[i+1]>>20;lo-=4096 if lo>>11 else 0
                    targets.append(addresses[side]+i*4+hi+lo)
                assert all(maps[side][at][2]=='FUNC' for side,at in enumerate(targets))
                same_targets(targets);covered.update(changed.intersection((i,i+1)))
            assert changed==covered,('changed helper instructions',result[0][0],sorted(changed-covered))
        return result[0][0]

    for section in ('boot','precise','dense','ffn','fetch','gdn'):
        assert raw[0][section]==raw[1][section],section
    # The accepted original's first 9888 service bytes are the existing
    # convolution code and constants,not a blanket service exemption.
    assert raw[0]['service'][:9888]==raw[1]['service'][:9888]
    end=BASES['service']+9888
    limit=BASES['service']+len(raw[1]['service'])
    assert end<limit<=0x6e000
    tail=sorted((at,*item) for at,item in maps[1].items() if end<=at<limit)
    expected_tail=TAIL
    if two_product:
        original='_ZN26PioneerQ38ExactQ4Corrected3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_'
        candidate='_ZN30PioneerQ38ExactQ4TwoProductDot3RunIN23PioneerQ38ExactQ4Affine12PanelEmitterI28PioneerQ38ExactQ4WorkEmitterEEEEbRT_'
        # Candidate affine is forced inline into its one worker caller;
        # an unexpected out-of-line helper is NOT a blanket tail waiver.
        expected_tail=(TAIL-{original})|{candidate}
    assert {r[1] for r in tail}==expected_tail and len(tail)==len(expected_tail), ('explicit tail',sorted(r[1] for r in tail),sorted(expected_tail))
    cursor=end
    for at,name,size,kind in tail:
        assert kind=='FUNC' and size and cursor<=at and at-cursor<=12 and at+size<=limit
        assert contents(1,cursor,at-cursor)==bytes(at-cursor)
        cursor=at+size
    assert cursor==limit
    for name in HELPERS:
        addresses=[next(at for at,item in m.items() if item[0]==name) for m in maps]
        same_targets(addresses)

    # Main intentionally gains the private completed-read counter check.
    # Its associated compiler switch table has twelve bounded code targets;
    # all remaining global literal bytes retain their original values.
    main=[]
    for side,m in enumerate(maps):
        at=next(at for at,item in m.items() if item[0]=='q38_global_worker_main')
        name,size,kind=m[at]
        assert kind=='FUNC' and at==0x21128 and size>0
        after=at+size-BASES['globalworker']
        suffix=raw[side]['globalworker'][after:]
        assert len(suffix)>=48
        for target in struct.unpack('<12I',suffix[:48]):
            assert at<=target<at+size and not target%4
        main.append((at,size,suffix[48:]))
    assert main[0][2]==main[1][2]
    size=main[0][0]-BASES['globalworker']
    words=[struct.unpack('<'+'I'*(size//4),r['globalworker'][:size]) for r in raw]
    changed={i for i,(a,b) in enumerate(zip(*words)) if a!=b}
    covered=set();relocations=0;base=BASES['globalworker']
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
        assert all(maps[side][at][2]=='FUNC' for side,at in enumerate(addresses))
        same_targets(addresses);covered.update(changed.intersection((i,i+1)));relocations+=1
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
            if all(at==base+len(raw[side]['globalworker'])-28 for side,at in enumerate(addresses)):
                # GCC emits the dense route's second 28-byte literal
                # without a symbol. Account for that ENTIRE exact object.
                assert contents(0,addresses[0],28)==contents(1,addresses[1],28)
            else:
                if not all(at in maps[side] for side,at in enumerate(addresses)):
                    continue
                assert all(maps[side][at][2]=='OBJECT' for side,at in enumerate(addresses))
                same_targets(addresses)
            covered.update(changed.intersection((i,j)));relocations+=1
    assert changed==covered,('unexplained global-prefix changes',sorted(changed-covered))
    assert relocations
    print('PASS_PREPARED_Q4_NONMATH exactOldService9888 explicitTailFunctions'+str(len(tail)),
          'globalMainAnd12TargetDispatchDeliberatelyChanged prefixRelocations'+str(relocations),
          'NO_NATIVE_ROOT_OR_EXECUTION_AUTHORITY',flush=True)
    return relocations
