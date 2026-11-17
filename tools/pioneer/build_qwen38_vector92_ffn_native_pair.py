#!/usr/bin/env python3
"""Package the pinned vector92 FFN pair locally; no accelerator or SSH access."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from build_qwen38_head_csr_native_pair import (ARRAY, DRIVER_DIR, ROOT_PAIR, TOOLS, build_variant, load_sections,
                                              pinned, prepare_control, root_sections)
from build_qwen38_vector92_ffn_root import MATCHED
from build_qwen38_vector92_ffn_sender import REUSE_ELF, SECTIONS

MARKER='PIONEER_Q38_FFN_RETAINED_NATIVE_PAIR'
TAGS={'refetch':'0x2026111700010211','reuse':'0x2026111700010212'}
FRONT=(9500,'197165c26e6720b54fc24f41a2935eaa7c3f553053e9273187ecc13a077ecf95')


def load_root(tool, root, name):
    text, conv, (_, elf_sha)=MATCHED[name]
    folder=root/name
    return load_sections(tool,folder/'program.elf',folder,elf_sha,root_sections(text,conv))


def check_journal(work, root, peers, tag):
    overlay=work/'source'
    role=(overlay/DRIVER_DIR/'PioneerQ38FullOffloadRole.h').read_text()
    required=set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b',role))
    assert len(required)==23
    defines=dict.fromkeys(required | {MARKER,'PIONEER_Q38_FULL_OFFLOAD_BATCHED',
                                    'PIONEER_Q38_FULL_OFFLOAD_METRICS','PIONEER_Q38_FULL_OFFLOAD_VECTOR_Q4'},'1')
    defines['PIONEER_DRIVER_BUILD_TAG']=tag+'ULL'
    (work/'full_offload_vector_q4_role_defines.inc').write_text(
        ''.join(f'#define {n} {v}\n' for n,v in sorted(defines.items())))
    filename='pioneer_q38_full_offload_vector_q4_data_test.cpp'
    test=overlay/TOOLS/'tests'/filename
    shutil.copyfile(Path(__file__).resolve().parent/'tests'/filename,test)
    binary=work/'journal-audit'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(work),test,'-o',binary],cwd=work,check=True)
    subprocess.run([binary,root,peers],cwd=work,check=True,timeout=60,
                   env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1'))


def main():
    assert __debug__ and not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
    if len(sys.argv)!=6:
        raise SystemExit('use SDK FROZEN_LLAMA_LIBRARIES ROOT_PAIR PINNED_REUSE_PEER NEW_OUTPUT')
    sdk,libraries,root,peer,out=map(Path,sys.argv[1:])
    assert all(p.is_absolute() for p in (sdk,libraries,root,peer,out))
    tool=str(sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    front=load_sections(tool,peer/'program.elf',peer,REUSE_ELF,(('PeerFront','globalworker',FRONT),))
    roots={n:load_root(tool,root,n) for n in TAGS}
    source,native,client,tool,original=prepare_control(sdk,libraries,out)
    old={label:digest for digest,label in ARRAY.findall(original)}
    assert len(old)==14
    # Snapshot the exact ELF for the independent all-word journal exercise.
    # The other nine peer sections must match the measured native arrays.
    peers=out/'peer-sections';peers.mkdir(mode=0o700)
    elf=peers/'program.elf';elf.write_bytes(pinned(peer/'program.elf',REUSE_ELF))
    for section,label in SECTIONS.items():
        dest=peers/(section+'.bin')
        subprocess.run([tool+'objcopy','-O','binary','-j','.'+section,elf,dest],check=True,cwd=peers)
        pinned(dest,FRONT[1] if label=='PeerFront' else old[label])
    result={}
    for n,tag in TAGS.items():
        other=next(v for k,v in TAGS.items() if k!=n)
        result[n]=build_variant(n,(tag,{**roots[n],**front}),out,source,native,client,tool,original,
                                marker=MARKER,pair_label='ffn-retained',other_tag=other,
                                replacements=ROOT_PAIR | {'PeerFront'})
        # Compare the actual executable selector/ordinal and every loaded
        # word against the authenticated ROM, over all 120 journal layouts.
        check_journal(out/n,root/n,peers,tag)
        assert {label for label,(_,digest) in result[n].items() if old[label]!=digest}==ROOT_PAIR|{'PeerFront'}
    assert {label for label in old if result['reuse'][label]!=result['refetch'][label]}==ROOT_PAIR
    print('PASS BUILD ONLY exact vector92 refetch/reuse packages; identical ten peer arrays; '
          'all 120 journal ranges checked. Not installed or a speed result.',flush=True)


if __name__=='__main__':
    main()
