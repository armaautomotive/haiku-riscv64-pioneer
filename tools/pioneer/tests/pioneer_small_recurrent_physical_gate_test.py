#!/usr/bin/env python3
"""Exercise the actual physical wrapper's output gate; no device access."""
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
wrapper=(here.parent/'pioneer_small_recurrent_band_physical_20261113.sh').read_text()
start="awk -v tensor=\"$tensor\" -v generation=\"$candidate_generation\" -v matrix_rows=\"$matrix_rows\" -v first1=\"$first1\" -v first2=\"$first2\" -v reference_kind=\"$reference_kind\" -v tail_padding=\"$tail_padding\" '\n"
end="}' \"$reference\" \"$stage/trial.stdout\" || exit 1"
assert wrapper.count(start)==wrapper.count(end)==1
program=wrapper.split(start,1)[1].split(end,1)[0]+'}'
assert wrapper.index('SMALL_RECURRENT_OUTPUT_GATE_END')<wrapper.index('KNOWN_CLOSED_EXACT192')<wrapper.index('exchange "$candidate_hash" "$stable_hash"')
assert 'UNPROVEN_NO_END_RESET_RETRY_RESTORE' in wrapper and '--check-only' in wrapper
first=(0,32,0)
def reference(qkv=False):
 starts=(0,5120,10208) if qkv else first;kind='QKV_INTEGER_REFERENCE' if qkv else 'SMALL_INTEGER_REFERENCE'
 return [f'{kind} sample{s} first{starts[s]} row{r} word00000000' for s in range(3) for r in range(32)]
def output(tensor,gen,qkv=False):
 starts=(0,5120,10208) if qkv else first;rows=10240 if qkv else 48
 lines=['AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET',
        f'AUTHENTICATED_FIXED_MODEL c600de03 tensor{tensor} width5120 rows{rows} samples3 payload_and_reference_exact;DEBUG_NOT_WORK']
 for s in range(3):
  lines += [f'NATIVE_LEARNED_BAND_RETURNED_WORD sample{s} first{starts[s]} pass{p} row{r} got00000000 expected00000000' for p in range(2) for r in range(32)]
  lines.append(f'PASS_NATIVE_LEARNED_BAND sample{s} tensor{tensor} first{starts[s]} width5120 phases4 dual_jobs2 exact_words64 polls1 generation{gen};EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
 lines.append('PASS_NATIVE_LEARNED_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM')
 return lines
with tempfile.TemporaryDirectory(prefix='small-physical-gate-',dir='/private/tmp') as d:
 ref=Path(d)/'reference';out=Path(d)/'output'
 def run(r,o,tensor=11,gen=43,qkv=False):
  ref.write_text('\n'.join(r)+'\n');out.write_text('\n'.join(o)+'\n')
  args=['matrix_rows=10240','first1=5120','first2=10208','reference_kind=QKV_INTEGER_REFERENCE','tail_padding=0'] if qkv else ['matrix_rows=48','first1=32','first2=0','reference_kind=SMALL_INTEGER_REFERENCE','tail_padding=1']
  return subprocess.run(['awk','-v',f'tensor={tensor}','-v',f'generation={gen}',*[item for a in args for item in ('-v',a)],program,str(ref),str(out)],capture_output=True).returncode
 for tensor in (11,12):
  good=output(tensor,43);r=reference();assert run(r,good,tensor)==0
  assert run(r[:-1],good,tensor)!=0 and run(r+[r[0]],good,tensor)!=0
  altered=r.copy();altered[48]=altered[48].replace('word00000000','word00000001');assert run(altered,good,tensor)!=0
  assert run(r,good[:-1],tensor)!=0 and run(r,good,12 if tensor==11 else 11)!=0 and run(r,good,tensor,44)!=0
  altered=good.copy();altered[3]=altered[3].replace('got00000000','got00000001');assert run(r,altered,tensor)!=0
  altered=good.copy();altered.insert(4,altered[3]);assert run(r,altered,tensor)!=0
  altered=good.copy();altered[2],altered[66]=altered[66],altered[2];assert run(r,altered,tensor)!=0
 qref=reference(True);qout=output(5,43,True);assert run(qref,qout,5,43,True)==0
 assert run(qref[:-1],qout,5,43,True)!=0 and run(reference(),qout,5,43,True)!=0
print('PASS physical output gate valid alpha/beta/QKV, missing/duplicate/mismatch/tail/order/tag/generation refusals; NO DEVICE')
