#!/usr/bin/env python3
"""Strict POSTHOC original CPU vs distinct EOS output DATA; no execution grant."""
import argparse
import array
import math
import os
import pathlib
import stat
import sys
import compare_pioneer_q38_observed_trace as baseline

require=baseline.require
WORDS=baseline.HIDDEN_WORDS+baseline.VOCAB_WORDS
def stable(path,size):
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
    with os.fdopen(fd,'rb') as source:
        before=os.fstat(source.fileno())
        require(stat.S_ISREG(before.st_mode) and before.st_size==size,'EOS file type/length')
        data=source.read(size+1);after=os.fstat(source.fileno())
        identity=lambda s:(s.st_dev,s.st_ino,s.st_size,s.st_mtime_ns,s.st_ctime_ns)
        require(identity(before)==identity(after) and len(data)==size,'EOS file changed during read')
        return data
def words(data):
    result=array.array('I');require(result.itemsize==4 and len(data)%4==0,'EOS word format');result.frombytes(data)
    if sys.byteorder!='little':result.byteswap()
    return result
def decode_eos(data,position):
    require(position in (0,1) and len(data)==(16+WORDS+64+1)*4,'EOS token length/position')
    w=words(data);h=w[:16]
    require(list(h[:4])==[0x31454f51,1,position,248319 if not position else 17],'EOS family/version/prompt')
    require(list(h[8:])==[65,5120,248320,64,2,2,70253,17] and w[-1]==0x454e4445,'EOS dimensions/config/footer')
    require(h[4]==(70253 if not position else 17) and h[5]<248320 and h[4]!=h[5],'EOS configured real prediction')
    floats=array.array('f');require(floats.itemsize==4,'EOS binary32 host');floats.frombytes(data[64:(16+WORDS)*4])
    if sys.byteorder!='little':floats.byteswap()
    require(all(math.isfinite(x) for x in floats),'EOS nonfinite payload')
    logits=floats[baseline.HIDDEN_WORDS:];ranked=sorted(range(248320),key=lambda i:(-logits[i],i))[:2]
    require(list(h[4:6])==ranked,'EOS greedy/runner-up vs actual vocabulary')
    require(h[6]==w[16+baseline.HIDDEN_WORDS+h[4]] and h[7]==w[16+baseline.HIDDEN_WORDS+h[5]],'EOS top-logit bits')
    record=w[16+WORDS:-1]
    expected=[baseline.TOKEN_MAGIC,1,position,h[3],h[4],h[6],position+1,(position+1)&1,int(not position),int(not position),64,973]
    require(list(record[:12])==expected and list(record[44:47])==[position+1,48,119] and not any(record[47:]),'EOS actual committed record')
    return {'header':h,'bits':w[16:16+WORDS],'floats':floats,'record':record}
def decode_terminal(data,traces):
    require(len(data)==(16+64+1)*4 and len(traces)==2,'EOS terminal length/coverage')
    w=words(data)
    require(list(w[:16])==[0x31544551,1,2,3,2,70253,17,119,64,973,0,0,0,0,0,0] and w[-1]==0x454e4445,'EOS terminal family/config/footer')
    term=w[16:-1]
    require(list(term[:10])==[0x31545451,1,2,2,1,119,64,973,0,1] and not any(term[10:12]) and not any(term[44:]),'EOS terminal fields/padding')
    for t in traces:require(t['record'][12:36]==term[12:36],'EOS token/terminal epoch+source identity')
    return term
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('cpu',type=pathlib.Path);parser.add_argument('observed',type=pathlib.Path);args=parser.parse_args()
    require(args.cpu.is_absolute() and args.observed.is_absolute() and not args.observed.is_symlink(),'absolute non-symlink EOS directory required')
    require(not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ),'unexpected role overrides')
    require(sorted(p.name for p in args.observed.iterdir())==['eos-terminal.bin','eos-token-0.bin','eos-token-1.bin'],'EOS complete exact file set')
    cpu=baseline.load_cpu(args.cpu);actual=[]
    for p in range(2):actual.append(decode_eos(stable(args.observed/f'eos-token-{p}.bin',(16+WORDS+64+1)*4),p))
    decode_terminal(stable(args.observed/'eos-terminal.bin',(16+64+1)*4),actual)
    exact=True
    for p in range(2):
        require(list(cpu[p]['header'][2:8])==list(actual[p]['header'][2:8]),'EOS source path/prediction differs from original CPU control')
        for layer in range(65):
            diff,maximum,relative=baseline.difference(cpu[p],actual[p],layer*5120,5120);exact=exact and diff==0
            print(f'COMPARE_EOS_HIDDEN position{p}/embeddingOrLayer{layer}/words5120/bitDifferences{diff}/maxAbs{maximum:.12g}/relativeL2{relative:.12g}')
        diff,maximum,relative=baseline.difference(cpu[p],actual[p],baseline.HIDDEN_WORDS,248320);exact=exact and diff==0
        print(f'COMPARE_EOS_VOCAB position{p}/words248320/bitDifferences{diff}/maxAbs{maximum:.12g}/relativeL2{relative:.12g}')
    print(f'EOS_STRUCTURE_VALID=1 ORIGINAL_PREFIX_NUMERICAL_BITEXACT={int(exact)} NO_TOLERANCE_USED;DATA ONLY,NOT native END/physical/Work/TPS qualification')
    return 0 if exact else 1
if __name__=='__main__':
    try:sys.exit(main())
    except (ValueError,OSError) as error:print(f'EOS_TRACE_REFUSAL {error}',file=sys.stderr);sys.exit(2)
