#!/usr/bin/env python3
"""Independent CLOSED115-a receipt arithmetic; no device or inference calls."""
import ast
from fractions import Fraction
import json
from pathlib import Path
import re
import sys

METADATA='P100A_RELOAD driver55 tag2026102100010012 generation8 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY'
FOOTER='NATIVE115_INDEPENDENT_POSTFLIGHT_CLOSED_NORMAL8_IDLE64_FULLRAM_APPS_PRESERVED'
def inspect(text,postflight):
 assert METADATA in postflight.splitlines() and FOOTER in postflight.splitlines()
 lines=text.splitlines();index=0
 def take():
  nonlocal index
  assert index<len(lines);line=lines[index];index+=1;return line
 assert take()=='PREPARED_BF16_COMPARE_AUTHENTICATING_ORIGINAL_AND_BOTH_CPU_REFERENCES;NO_DEVICE_OPEN'
 assert take()=='PREPARED_BF16_COMPARE_AUTHENTICATED c600de03 all9_originalAndPreparedPayloads_bothArithmeticReferences;CPU_DIAGNOSTIC_NOT_WORK'
 samples=[]
 for sample,width in ((0,5120),(6,17408)):
  data=[]
  for word in range(64):
   m=re.fullmatch(r'PREPARED_BF16_COMPARE_DATA_WORD sample'+str(sample)+' kind'+str(word//32)+' row'+str(word%32)+r' got([0-9a-f]{8}) expected([0-9a-f]{8}) exact1',take());assert m
   assert m[1]==m[2];bits=int(m[1],16);exp=bits&0x7f800000
   assert exp!=0x7f800000 and (exp or not(bits&0x7fffffff));data.append(m[1])
  assert take()==f'PREPARED_BF16_COMPARE_DATA_DONE sample{sample} words64 mismatches0;DATA_ONLY_NO_END_OR_SPEED_AUTHORITY'
  for word in range(64):
   assert take()==f'PREPARED_BF16_COMPARE_WORD sample{sample} kind{word//32} row{word%32} got{data[word]} expected{data[word]}'
  blocks=width//256
  assert take()==f'PASS_NATIVE_PREPARED_BF16_COMPARE sample{sample} width{width} exact64 generation7 saved_l137136 saved_gddr{blocks*2688} requests{blocks*29} flits{blocks*176};EXPLICIT_END_L1_GDDR_NIU_TLB_FIFO_CLOCK_PCI_RESTORED_NOT_MODEL_TPS'
  timings=[]
  for kind in range(2):
   line=take();prefix=f'PREPARED_BF16_COMPARE_TIMING sample{sample} width{width} kind{kind} '
   assert line.startswith(prefix);body=line[len(prefix):]
   m=re.fullmatch(r'state1 start([1-9][0-9]*) end([1-9][0-9]*) cycles([1-9][0-9]*);0OLD_BF16_1NEW_PREPARED_BF16_NOT_MODEL_TPS',body)
   if m:
    start,end,cycles=map(int,m.groups());assert 0<start<end<2**64 and cycles==end-start
    timings.append({'state':1,'start':start,'end':end,'cycles':cycles})
   else:
    m=re.fullmatch(r'state([02]);UNAVAILABLE_NOT_TIMING_OR_CLEANUP_AUTHORITY',body);assert m
    timings.append({'state':int(m[1])})
  result={'sample':sample,'input_width':width,'output_rows':32,'exact_old_and_new_words':64,'timings':timings,'observed_pairs':1}
  if all(t['state']==1 for t in timings):
   ratio=Fraction(timings[0]['cycles'],timings[1]['cycles'])
   result.update(old_over_new_cycle_ratio=float(ratio),exact_ratio_numerator=ratio.numerator,exact_ratio_denominator=ratio.denominator)
  samples.append(result)
 assert take()=='PASS_NATIVE_PREPARED_BF16_COMPARE_ALL samples2 exact128 cleanEnds2;DIAGNOSTIC_NOT_WORK_NOT_MODEL_QUALITY_OR_TPS'
 assert index==len(lines)
 return {'schema':1,'scope':'isolated_on_card_load_compute_clock_windows_not_model_TPS_or_repeatability','samples':samples,'new_model_speed_measurement':False}

if sys.argv[1:]==['--self-test']:
 module=ast.parse(Path(__file__).with_name('pioneer_native_prepared_bf16_q4_parent_test.py').read_text())
 function=next(n for n in module.body if isinstance(n,ast.FunctionDef) and n.name=='receipt')
 space={'manual':True,'generation':7};exec(compile(ast.Module(body=[function],type_ignores=[]),'<fixture>','exec'),space)
 post=METADATA+'\n'+FOOTER+'\n';good=space['receipt']()
 for state in (0,1,2):assert len(inspect(space['receipt'](state),post)['samples'])==2
 bad=[good.replace('got00000000','got00000001',1),good.replace('exact1','exact0',1),
      good.replace('end2','end0',1),good.replace('cycles1;','cycles2;',1),
      good.replace('generation7','generation2'),good+'UNEXPECTED\n',
      '\n'.join(good.splitlines()[:-1])+'\n']
 for value in bad:
  try:inspect(value,post)
  except AssertionError:pass
  else:raise AssertionError('bad receipt accepted')
 try:inspect(good,post.replace('generation8','generation7'))
 except AssertionError:pass
 else:raise AssertionError('wrong restoration accepted')
 print('PASS_CLOSED_RECEIPT_READER positive3 refused8 exact128_DATA_matches_afterEND_integer64_cycle_differences_restoreRequired;NO_HARDWARE')
else:
 assert len(sys.argv)==3
 print(json.dumps(inspect(Path(sys.argv[1]).read_text(),Path(sys.argv[2]).read_text()),sort_keys=True,indent=2))
