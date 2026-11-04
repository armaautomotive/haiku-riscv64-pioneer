#!/usr/bin/env python3
"""Independent frozen86 CHILD parser;NOT parent/native/hardware authority."""
import hashlib
import pathlib
import re
import sys
assert __debug__
REFERENCE=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-q4-fpu-stream-reference-20261104.log'
BANNERS=('AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_Q4_STREAM_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
         'AUTHENTICATED_FIXED_Q4_STREAM_MODEL c600de03 samples9 widths5120_17408 rows32 full_k_payload_and_reference_exact;DEBUG_NOT_WORK')
FINAL='PASS_NATIVE_Q4_FPU_STREAM samples9 exact_words576 clean_ends9;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
RESTORE=';EXPLICIT_END_L1_GDDR_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
def geometry(sample):
    return (7 if sample<3 else 8 if sample<6 else 6,
            (0,8704,17376)[sample%3] if sample<6 else (0,2560,5088)[sample%3],
            5120 if sample<6 else 17408)
def reference():
    raw=REFERENCE.read_bytes()
    if hashlib.sha256(raw).hexdigest()!='56ec253884deb7ba3f8a72dc09202f0b2a687752ebb15274843cbf14483008e9':
        raise ValueError('Independent86 reference changed')
    expected={}
    for line in raw.decode('ascii').splitlines():
        m=re.fullmatch(r'Q4_STREAM_FRACTION_REFERENCE sample([0-8]) tensor(\d+) first(\d+) width(\d+) row(\d+) word([0-9a-f]{8})',line)
        if not m:raise ValueError('Reference framing')
        sample,tensor,first,width,row=map(int,m.group(1,2,3,4,5))
        if (tensor,first,width)!=geometry(sample) or row>=32 or (sample,row) in expected:raise ValueError('Reference geometry/duplicate')
        expected[sample,row]=m[6]
    if len(expected)!=288:raise ValueError('Reference incomplete')
    return expected
def summary(sample,polls):
    tensor,first,width=geometry(sample);blocks=width//256
    return (f'PASS_NATIVE_Q4_FPU_STREAM_SAMPLE sample{sample} tensor{tensor} first{first} width{width} phases4 dual_jobs2 exact_words64 polls{polls}'
            f' generation9 saved_l126908 saved_gddr{blocks*1664} requests{2*blocks*56} flits{2*blocks*104}'+RESTORE)
def returned(sample,job,row,word):
    tensor,first,width=geometry(sample)
    return f'NATIVE_Q4_FPU_STREAM_RETURNED_WORD sample{sample} tensor{tensor} first{first} width{width} pass{job} row{row} got{word} expected{word}'
def verify(text):
    if not isinstance(text,str) or not text.endswith('\n') or len(text)>262144 or '\r' in text or '\0' in text:raise ValueError('Child framing/size')
    lines=text.splitlines()
    if len(lines)!=588 or tuple(lines[:2])!=BANNERS or lines[-1]!=FINAL:raise ValueError('Child banners/count/final')
    expected=reference();index=2;polls=[]
    for sample in range(9):
        for job in range(2):
            for row in range(32):
                if lines[index]!=returned(sample,job,row,expected[sample,row]):raise ValueError('Child order/reference')
                index+=1
        m=re.search(r' polls([1-9][0-9]*) generation',lines[index])
        if not m or int(m[1])>20000 or lines[index]!=summary(sample,int(m[1])):raise ValueError('Child epoch/poll/geometry/journals/traffic/restore')
        polls.append(int(m[1]));index+=1
    if index!=len(lines)-1:raise ValueError('Child extra records')
    return {'samples':9,'words':576,'ends':9,'generation':9,'polls':polls}
def fixture():
    words=reference();lines=list(BANNERS)
    for sample in range(9):
        for job in range(2):
            for row in range(32):lines.append(returned(sample,job,row,words[sample,row]))
        lines.append(summary(sample,2))
    return '\n'.join(lines+[FINAL])+'\n'
def self_test():
    good=fixture();lines=good.splitlines();assert verify(good)['polls']==[2]*9
    bad=[good[:-1],good+'NOISE\n',good.replace('\n','\r\n'),good+'\0']
    for i,line in enumerate(lines):
        bad.append('\n'.join(lines[:i]+lines[i+1:])+'\n')
        bad.append('\n'.join(lines[:i]+[line]+lines[i:])+'\n')
        if line.startswith('NATIVE_Q4_FPU_STREAM_RETURNED_WORD'):
            m=re.search(r'got([0-9a-f]{8}) expected\1$',line);assert m
            word='%08x'%(int(m[1],16)^1);changed=list(lines);changed[i]=line[:m.start()]+'got'+word+' expected'+word;bad.append('\n'.join(changed)+'\n')
    for sample in range(9):
        index=66+sample*65;tensor,first,width=geometry(sample);blocks=width//256
        for old,new in [('generation9','generation10'),('polls2','polls0'),('polls2','polls20001'),('phases4','phases1'),
                        (f'width{width}','width256'),('saved_l126908','saved_l112372'),(f'saved_gddr{blocks*1664}','saved_gddr0'),
                        (f'requests{2*blocks*56}','requests320'),(f'flits{2*blocks*104}','flits2880'),
                        ('GDDR_NIU_FIFO_TLB_CLOCK_PCI_RESTORED','PCI_RESTORED'),(f'tensor{tensor}','tensor0'),(f'first{first}','first1')]:
            changed=list(lines);assert old in changed[index];changed[index]=changed[index].replace(old,new);bad.append('\n'.join(changed)+'\n')
    for name in ('qwen38-native-dram-band64-observed-20261104/gddr64.stdout','qwen38-native-q4-fpu-physical-20261104/q4.stdout'):
        p=REFERENCE.parent/name
        if p.is_file():bad.append(p.read_text())
    for text in bad:
        try:verify(text)
        except ValueError:continue
        raise AssertionError('Corrupted/old child fixture accepted')
    print(f'PASS independent86 CHILD parser:1 valid/{len(bad)} refused;all576 Fraction words/order/epoch/poll/full-K journals/traffic/restore;NO parent/native/device/full-model/speed proof')
if __name__=='__main__':
    if sys.argv[1:]==['--self-test']:self_test()
    elif len(sys.argv)==3 and sys.argv[1]=='--check-child-output':
        print('PASS exact frozen86 CHILD output ONLY:',verify(pathlib.Path(sys.argv[2]).read_text()),';parent/native/physical closure must be independently observed')
    else:raise SystemExit('Use --self-test or --check-child-output PATH;CPU read-only,NO GPU')
