#!/usr/bin/env python3
"""Independent frozen85 CHILD parser;not parent/native/hardware closure authority."""
import hashlib
import pathlib
import re
import sys
assert __debug__
REFERENCE=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-q4-fpu-reference-20261104.log'
BANNERS=('AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_Q4_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
         'AUTHENTICATED_FIXED_Q4_MODEL c600de03 tensor7 matrix_width5120 block_width256 rows32 samples3 payload_and_reference_exact;DEBUG_NOT_WORK')
FINAL='PASS_NATIVE_Q4_FPU_BLOCKS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
SUFFIX=' generation7 saved_l112372;EXPLICIT_END_L1_NIU_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM'
def reference():
    raw=REFERENCE.read_bytes()
    if hashlib.sha256(raw).hexdigest()!='5e8b9fde8759b3c992745b21b191a8e0d5ab6b854fbeedace7af641b7285b893':
        raise ValueError('Independent85 reference changed')
    expected={}
    for line in raw.decode('ascii').splitlines():
        m=re.fullmatch(r'Q4_FRACTION_REFERENCE sample([012]) first(\d+) kblock(\d+) row(\d+) word([0-9a-f]{8})',line)
        if not m:raise ValueError('Reference framing')
        sample,first,block,row=map(int,m.group(1,2,3,4))
        if first!=(0,8704,17376)[sample] or block!=(0,10,19)[sample] or row>=32 or (sample,row) in expected:raise ValueError('Reference geometry/duplicate')
        expected[sample,row]=m[5]
    if len(expected)!=96:raise ValueError('Reference incomplete')
    return expected
def verify(text):
    if not isinstance(text,str) or not text.endswith('\n') or len(text)>262144 or '\r' in text or '\0' in text:
        raise ValueError('Child framing/size')
    lines=text.splitlines()
    if len(lines)!=198 or tuple(lines[:2])!=BANNERS or lines[-1]!=FINAL:raise ValueError('Child banners/count/final')
    expected=reference();index=2;polls=[]
    for sample in range(3):
        first,block=(0,8704,17376)[sample],(0,10,19)[sample]
        for job in range(2):
            for row in range(32):
                word=expected[sample,row]
                wanted=f'NATIVE_Q4_FPU_BLOCK_RETURNED_WORD sample{sample} first{first} kblock{block} pass{job} row{row} got{word} expected{word}'
                if lines[index]!=wanted:raise ValueError(f'Child order/reference at sample{sample}/pass{job}/row{row}')
                index+=1
        prefix=f'PASS_NATIVE_Q4_FPU_BLOCK sample{sample} tensor7 first{first} kblock{block} width256 phases4 dual_jobs2 exact_words64 polls'
        m=re.fullmatch(re.escape(prefix)+r'([1-9][0-9]*)'+re.escape(SUFFIX),lines[index])
        if not m or int(m[1])>20000:raise ValueError('Child epoch/poll/geometry/journal/restore')
        polls.append(int(m[1]));index+=1
    if index!=len(lines)-1:raise ValueError('Child extra records')
    return {'samples':3,'words':192,'ends':3,'generation':7,'polls':polls}
def fixture():
    words=reference();lines=list(BANNERS)
    for sample in range(3):
        first,block=(0,8704,17376)[sample],(0,10,19)[sample]
        for job in range(2):
            for row in range(32):
                word=words[sample,row];lines.append(f'NATIVE_Q4_FPU_BLOCK_RETURNED_WORD sample{sample} first{first} kblock{block} pass{job} row{row} got{word} expected{word}')
        lines.append(f'PASS_NATIVE_Q4_FPU_BLOCK sample{sample} tensor7 first{first} kblock{block} width256 phases4 dual_jobs2 exact_words64 polls2'+SUFFIX)
    return '\n'.join(lines+[FINAL])+'\n'
def self_test():
    good=fixture();lines=good.splitlines();assert verify(good)['polls']==[2,2,2]
    bad=[good[:-1],good+'NOISE\n',good.replace('\n','\r\n'),good+'\0']
    for i,line in enumerate(lines):
        bad.append('\n'.join(lines[:i]+lines[i+1:])+'\n')
        bad.append('\n'.join(lines[:i]+[line]+lines[i:])+'\n')
        if line.startswith('NATIVE_Q4_FPU_BLOCK_RETURNED_WORD'):
            m=re.search(r'got([0-9a-f]{8}) expected\1$',line);assert m
            word='%08x'%(int(m[1],16)^1);changed=list(lines);changed[i]=line[:m.start()]+'got'+word+' expected'+word
            bad.append('\n'.join(changed)+'\n')
    for sample in range(3):
        index=66+sample*65
        for old,new in [('generation7','generation8'),('polls2','polls0'),('polls2','polls20001'),('phases4','phases1'),
                        ('width256','width5120'),('saved_l112372','saved_l155372'),('CLOCK_PCI_RESTORED','PCI_RESTORED'),
                        (f'kblock{(0,10,19)[sample]}','kblock20')]:
            changed=list(lines);assert old in changed[index];changed[index]=changed[index].replace(old,new);bad.append('\n'.join(changed)+'\n')
    old=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-dram-band64-observed-20261104/gddr64.stdout'
    bad.append(old.read_text())
    for text in bad:
        try:verify(text)
        except ValueError:continue
        raise AssertionError('Corrupted/old child fixture accepted')
    print(f'PASS independent85 CHILD parser:1 valid/{len(bad)} refused;all192 Fraction words/order/epoch/poll/journal/restore;old84 not promoted;NO parent/native/device/full-model/speed proof')
if __name__=='__main__':
    if sys.argv[1:]==['--self-test']:self_test()
    elif len(sys.argv)==3 and sys.argv[1]=='--check-child-output':
        print('PASS exact frozen85 CHILD output ONLY:',verify(pathlib.Path(sys.argv[2]).read_text()),';parent/native/physical closure must be independently observed')
    else:raise SystemExit('Use --self-test or --check-child-output PATH;CPU read-only,NO GPU')
