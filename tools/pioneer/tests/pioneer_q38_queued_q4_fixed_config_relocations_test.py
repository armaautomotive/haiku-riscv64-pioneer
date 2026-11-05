#!/usr/bin/env python3
"""Mutation-test whole-image boundary,not arithmetic/native/hardware proof."""
import copy,sys
from pioneer_q38_queued_q4_fixed_config_relocations import audit,audit_data,load_image
assert __debug__ and len(sys.argv)==3
tool='/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-'
control,image=sys.argv[1:]
audit(control,image,tool)
old,old_symbols=load_image(control,tool);new,new_symbols=load_image(image,tool)
refused=0
def reject(a,b,c,d):
 global refused
 try:audit_data(a,b,c,d)
 except ValueError:refused+=1;return
 raise AssertionError('mutation escaped whole-image boundary')
for i in range(len(new['service'])):
 mutated=dict(new);data=bytearray(new['service']);data[i]^=1;mutated['service']=bytes(data)
 reject(old,mutated,old_symbols,new_symbols)
for section,data in new.items():
 if section=='service':continue
 for i in {0,len(data)//2,len(data)-1}:
  mutated=dict(new);changed=bytearray(data);changed[i]^=1;mutated[section]=bytes(changed)
  reject(old,mutated,old_symbols,new_symbols)
  other=dict(old);other[section]=mutated[section]
  reject(other,mutated,old_symbols,new_symbols)
symbol_refusals=0
for side,symbols in enumerate((old_symbols,new_symbols)):
 limit=0x68000+len((old,new)[side]['service'])
 for at,(name,size,kind) in symbols.items():
  if not 0x6a6a0<=at<limit:continue
  for mode in range(5):
   altered=copy.copy(symbols)
   if mode==0:del altered[at]
   elif mode==1:altered[at]=(name+'X',size,kind)
   elif mode==2:altered[at]=(name,size+4,kind)
   elif mode==3:altered[at]=(name,size,'OBJECT')
   else:altered[at+1]=altered.pop(at)
   reject(old,new,altered if not side else old_symbols,altered if side else new_symbols);symbol_refusals+=1
print(f'PASS_FIXED_CONFIG_WHOLE_IMAGE_BOUNDARY newServiceByteMutations{len(new["service"])} otherSectionMutations{refused-len(new["service"])-symbol_refusals} symbolMutations{symbol_refusals} NO_MODEL_NATIVE_DEVICE_OR_TPS',flush=True)
