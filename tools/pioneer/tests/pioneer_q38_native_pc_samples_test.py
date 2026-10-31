#!/usr/bin/env python3
"""Synthetic interval/log parser refusals only,NOT shader execution."""
import importlib.util
import pathlib
import tempfile

here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('pc_audit',here/'audit_pioneer_q38_native_pc_samples.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
table='00040000 l F .text 00000010 first()\n00040000 l F .text 00000010 alias()\n00040010 l F .text 00000004 second()\n'
intervals=m.functions(table)
line='OBSERVED native layer0 ROOTsteps40 pc40004 packets1 newDoors0 phase6 stateR0 stateW0 historyR0;NOT layer/token qualification'
rows=m.samples(line,intervals)
assert len(rows)==1 and rows[0]==(40,0x40004,'6',['alias()','first()'])
refused=0
for text in ('','ignored\n',line+'\n'+line,line.replace('steps40','steps0'),line.replace('40004','40002'),
             line.replace('40004','40014'),line.replace('packets1','packets-1'),line.replace('phase6','phase?')):
    try:m.samples(text,intervals)
    except ValueError:refused+=1
    else:raise AssertionError('bad sample accepted')
for text in ('',table.replace('00000010','00000011'),table.replace('00040000','0003fff0'),
             table.replace('00040010','00070000')):
    try:m.functions(text)
    except ValueError:refused+=1
    else:raise AssertionError('bad function interval accepted')
with tempfile.TemporaryDirectory(prefix='pioneer-root-pc-refusal-',dir='/private/tmp') as name:
    bad=pathlib.Path(name)/'NOT_A_ROOT.elf';bad.write_bytes(b'not an ELF or trusted symbol table')
    try:m.main(['/SDK_MUST_NOT_BE_ACCESSED',str(bad),'/LOG_MUST_NOT_BE_OPENED'])
    except ValueError as error:assert 'ELF/symbol table' in str(error);refused+=1
    else:raise AssertionError('untrusted ELF reached external tools/log access')
    try:m.main(['relative-sdk',str(bad),'/LOG_MUST_NOT_BE_OPENED'])
    except ValueError as error:assert 'absolute paths' in str(error);refused+=1
    else:raise AssertionError('relative SDK accepted')
print(f'PASS coarse ROOT PC parser/real symbol aliases preserved/{refused} missing+unordered+malformed+unknown+misaligned+range refusals;SYNTHETIC parser only,NO hardware/runtime/performance proof')
