#!/usr/bin/env python3
"""Read-only FULL frozen104 parent transcript parser,NOT execution/admission proof."""
import pathlib
import re
import sys
from pioneer_native_compact_q4_stream_output import fixture, verify
assert __debug__
NORMAL='2026102100010012'
NATIVE='2026110500010104'
QUIESCED='QUIESCED_CLOSED;NEW_HARDWARE_WORK_BLOCKED_UNTIL_RELOAD'
RELOADED='HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION'
START='COMPACT_Q4_STREAM_SOURCE_AUTH_AND_NINE_FULL_K_SAMPLES_STARTED_WAIT_SAME_PARENT'
FINAL='NATIVE_COMPACT_Q4_STREAM_KNOWN_CLEAN_NO_FULL_MODEL_SPEED_CLAIM'
def metadata(tag,generation,quiesced=0):
    return (f'P100A_RELOAD driver55 tag{tag} generation{generation} quiesced{quiesced} cold0 active_owner0 '
            'open_count1 guard1;CPU_METADATA_ONLY')
def health(lines):
    if len(lines)!=9 or not lines[0].startswith('Kernel name: kernel_riscv64 built on: ') or lines[1]!='64 (Unknown), revision 0000 running at 1000MHz' or lines[2]:
        raise ValueError('CPU/kernel/health framing')
    memory=re.fullmatch(r'\s*(\d+) bytes free\s+\(used/max (\d+) / 137034936320\)',lines[3])
    cached=re.fullmatch(r'\s+\(cached\s+(\d+)\)',lines[4])
    if not memory or not cached or sum(map(int,memory.group(1,2)))!=137034936320 or int(cached[1])>137034936320:
        raise ValueError('RAM totals/cache')
    for i,(name,total) in enumerate((('semaphores',65536),('ports',4096),('threads',4096),('teams',2048)),5):
        m=re.fullmatch(r'\s*(\d+) '+name+r' free\s+\(used/max\s+(\d+) /\s+'+str(total)+r'\)',lines[i])
        if not m or sum(map(int,m.group(1,2)))!=total:raise ValueError('Resource totals')
def verify_parent(text):
    if not isinstance(text,str) or not text.endswith('\n') or len(text)>300000 or '\r' in text or '\0' in text:raise ValueError('Outer framing/size')
    lines=text.splitlines()
    if len(lines)!=624:raise ValueError('Outer record count')
    prefix,suffix=lines[:18],lines[-18:]
    health(prefix[1:10]);health(suffix[8:17])
    if prefix[1]!=suffix[8]:raise ValueError('Kernel changed')
    if prefix[0]!=metadata(NORMAL,36) or prefix[10:]!=[
        metadata(NORMAL,36),metadata(NORMAL,36,1),QUIESCED,metadata(NATIVE,37),RELOADED,
        metadata(NATIVE,37),metadata(NATIVE,37),START]:raise ValueError('Admission/swap/start ordering')
    if suffix[:8]!=[metadata(NATIVE,37),metadata(NATIVE,37),metadata(NATIVE,37,1),QUIESCED,
                     metadata(NORMAL,38),RELOADED,metadata(NORMAL,38),metadata(NORMAL,38)] or suffix[-1]!=FINAL:
        raise ValueError('Known completion/restoration ordering')
    result=verify('\n'.join(lines[18:-18])+'\n')
    return dict(result,restored_generation=38)
def example():
    h=['Kernel name: kernel_riscv64 built on: Sep  8 2026 15:18:31 version 0xa01',
       '64 (Unknown), revision 0000 running at 1000MHz','',
       '133772189696 bytes free      (used/max 3262746624 / 137034936320)',
       '                           (cached   19174223872)',
       '     64488 semaphores free (used/max       1048 /      65536)',
       '      3945 ports free      (used/max        151 /       4096)',
       '      3901 threads free    (used/max        195 /       4096)',
       '      2025 teams free      (used/max         23 /       2048)']
    prefix=[metadata(NORMAL,36),*h,metadata(NORMAL,36),metadata(NORMAL,36,1),QUIESCED,
            metadata(NATIVE,37),RELOADED,metadata(NATIVE,37),metadata(NATIVE,37),START]
    suffix=[metadata(NATIVE,37),metadata(NATIVE,37),metadata(NATIVE,37,1),QUIESCED,
            metadata(NORMAL,38),RELOADED,metadata(NORMAL,38),metadata(NORMAL,38),*h,FINAL]
    return '\n'.join(prefix)+'\n'+fixture()+'\n'.join(suffix)+'\n'
def self_test():
    text=example();lines=text.splitlines();assert verify_parent(text)['restored_generation']==38
    bad=[text[:-1],text+'NOISE\n',text.replace('\n','\r\n'),text+'\0',
         text.replace('generation38','generation36'),text.replace('cold0','cold1'),
         text.replace('active_owner0','active_owner1'),text.replace('64 (Unknown)','1 (Unknown)'),
         text.replace('137034936320','3185577984'),text.replace('133772189696','133772189695'),
         text.replace('65536','65535'),text.replace('NO_REBOOT_NO_GPU_EXECUTION','REBOOTED')]
    for i,line in enumerate(lines):
        bad.append('\n'.join(lines[:i]+lines[i+1:])+'\n')
        bad.append('\n'.join(lines[:i]+[line]+lines[i:])+'\n')
        if line:bad.append('\n'.join(lines[:i]+[line+'X']+lines[i+1:])+'\n')
    old=pathlib.Path(__file__).resolve().parents[1]/'benchmarks/qwen38-native-q4-fpu-physical-20261104/parent.stdout'
    assert old.is_file();bad.append(old.read_text())
    for candidate in bad:
        try:verify_parent(candidate)
        except ValueError:continue
        raise AssertionError('Corrupt/old FULL parent accepted')
    print(f'PASS FULL104 parent transcript parser:1 valid/{len(bad)} refused,624 exact ordered records/'
          '576 Fraction words/nine END summaries/36->37->38/two64-CPU/full-RAM checks;'
          'NOT actual process exit/remote/physical/hardware/admission/model/speed proof')
if __name__=='__main__':
    if sys.argv[1:]==['--self-test']:self_test()
    elif len(sys.argv)==3 and sys.argv[1]=='--check-parent-output':
        print('PASS frozen104 FULL parent transcript ONLY:',verify_parent(pathlib.Path(sys.argv[2]).read_text()),
              ';original process exit0,stderr0,remote records and independent postflight still required')
    else:raise SystemExit('Use --self-test or --check-parent-output PATH;READ ONLY,NO GPU')
