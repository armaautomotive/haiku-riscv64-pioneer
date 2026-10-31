#!/usr/bin/env python3
"""Map coarse native ROOT instruction samples,NOT physical cycles or TPS."""
import argparse
import hashlib
import pathlib
import re
import subprocess
import tempfile

TEXT_BYTES=130888
TEXT_SHA='8be33008ec9aa1c3930b48825c43e795e50178292b44df9ab0bd7b4459f6c13a'
ELF_SHA='917972d54277d95b18ed7e2fc346feba3d1a60857b64ca366fec7b7bd131c60c'

def functions(table):
    result=[]
    for line in table.splitlines():
        row=line.split()
        if len(row)>=6 and row[2]=='F' and row[3]=='.text':
            start,size=int(row[0],16),int(row[4],16)
            if not size:continue
            if (start|size)&3 or start<0x40000 or start+size>0x40000+TEXT_BYTES:
                raise ValueError('invalid ROOT text function interval')
            result.append((start,start+size,' '.join(row[5:])))
    if not result:raise ValueError('missing ROOT function intervals')
    return result

def samples(text,intervals):
    result=[];previous=0
    for line in text.splitlines():
        if not line.startswith('OBSERVED native layer0 ROOTsteps'):continue
        match=re.fullmatch(r'OBSERVED native layer0 ROOTsteps([0-9]+) pc([0-9a-f]+) packets([0-9]+) newDoors([0-9]+) phase([0-9a-f]+) stateR([0-9]+) stateW([0-9]+) historyR([0-9]+);NOT layer/token qualification',line)
        if not match:raise ValueError('malformed ROOT sample')
        steps,pc=int(match[1]),int(match[2],16)
        if steps<=previous or pc&3:raise ValueError('unordered/misaligned ROOT samples')
        names=sorted({name for begin,end,name in intervals if begin<=pc<end})
        if not names:raise ValueError('sample is not inside an ELF ROOT function')
        # Preserve aliases/overlaps,not an arbitrary first symbol selection.
        result.append((steps,pc,match[5],names));previous=steps
    if not result:raise ValueError('no ROOT samples')
    return result

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sdk',type=pathlib.Path)
    parser.add_argument('elf',type=pathlib.Path)
    parser.add_argument('log',type=pathlib.Path)
    args=parser.parse_args(argv)
    if not all(p.is_absolute() for p in (args.sdk,args.elf,args.log)):
        raise ValueError('absolute paths required')
    tool=str(args.sdk/'cross-tools-riscv64/bin/riscv64-unknown-haiku-')
    elf=args.elf.read_bytes()
    if hashlib.sha256(elf).hexdigest()!=ELF_SHA:
        raise ValueError('ELF/symbol table is not the fixed G3 ROOT artifact')
    with tempfile.TemporaryDirectory(prefix='pioneer-root-pc-audit-',dir='/private/tmp') as name:
        snapshot=pathlib.Path(name)/'authenticated-root.elf'
        snapshot.write_bytes(elf)
        text=pathlib.Path(name)/'elf-text.bin'
        subprocess.run([tool+'objcopy','-O','binary','-j','.text',str(snapshot),str(text)],check=True)
        data=text.read_bytes()
        if len(data)!=TEXT_BYTES or hashlib.sha256(data).hexdigest()!=TEXT_SHA:
            raise ValueError('ELF .text is not the fixed installed G3 ROOT bytes')
        table=subprocess.check_output([tool+'objdump','-t','-C',str(snapshot)],text=True)
    log=args.log.read_bytes()
    rows=samples(log.decode('utf-8'),functions(table))
    print('ROOT_TEXT_SHA256 '+TEXT_SHA+';COARSE HOST INSTRUCTION SAMPLES ONLY')
    print('ROOT_ELF_SHA256 '+ELF_SHA+';symbol table and code read from SAME authenticated byte snapshot')
    print('LOG_SHA256 '+hashlib.sha256(log).hexdigest())
    for steps,pc,phase,names in rows:
        print(f'ROOT_PC_SAMPLE steps{steps} pc{pc:x} phase{phase} aliases{len(names)} '+ ' | '.join(names))
    print(f'MAPPED_ROOT_SAMPLES {len(rows)};NO physical cycles,wall-time percentage,steady inference rate,Work or TPS qualification')

if __name__=='__main__':
    main()
