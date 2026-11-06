#!/usr/bin/env python3
"""Audit saved OFFBOARD timing receipts, not a new build or hardware result."""
import ast
import hashlib
import json
from pathlib import Path
import sys

assert __debug__
here=Path(__file__).resolve().parent;root=here.parents[3]
sys.path.insert(0,str(root/'tools/pioneer'))
from PioneerQ38Bf16ProfileProgramPins import ROOT_PINS
from PioneerQ38Bf16ProgramPins import ROOT_PINS as BASELINE_PINS
assert json.loads((here/'actual-exits.json').read_text())=={
 'main_final_root_build':0,'main_observer_tests':0,'main_exact_elf_stack':0,
 'main_unchanged_bf16_build':0,'clean_root_build':0,'clean_final_observer_and_stack':0,
 'legacy_phase_and_private_frontier_roles':0,'old_bf16_generator_expected_refusal':1}
def pins(path):
 rows=[ast.literal_eval(line) for line in path.read_text().splitlines() if line.startswith("('")]
 return tuple(rows[-6:])
assert pins(here/'main/root-build.stdout')==ROOT_PINS
assert pins(here/'clean/root-build.stdout')==ROOT_PINS
assert pins(here/'main/unchanged-build.stdout')==BASELINE_PINS
assert (here/'main/source-sha256.txt').read_bytes()==(here/'clean/source-sha256.txt').read_bytes()
sources=(here/'main/source-sha256.txt').read_text().splitlines();assert len(sources)==16
for line in sources:
 digest,name=line.split('  ',1);assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
for copy in ('main','clean'):
 directory=here/copy
 for name in ('root-build.stderr','observer-tests.stderr','stack.stderr'):assert not (directory/name).read_bytes()
 assert (directory/'observer-tests.stdout').read_text().endswith(
  'PASS_BF16_TIMING_OBSERVER children5_stderr0_sources15_EXACT_LINKED_HELPERS_ONLY_SYNTHETIC_CLOCKS_NOT_FULL_ROOT_NATIVE_PHYSICAL_CLOCK_OR_TPS\n')
 observer=directory/'observer'
 assert json.loads((observer/'actual-exits.json').read_text())=={
  'roles':0,'data-compile':0,'data':0,'helpers-compile':0,'helpers':0}
 for name in ('roles','data-compile','data','helpers-compile','helpers'):
  assert not (observer/(name+'.stderr')).read_bytes()
 assert 'positive6/refusals133;' in (observer/'roles.stdout').read_text()
 assert 'DATA:90 cases/14 exclusive buckets/496 matrix scopes per token/8192-token bounds/' in (observer/'data.stdout').read_text()
 assert 'RV32:21histories/1+2+34 positions/rawCSR stream exact per14buckets/' in (observer/'helpers.stdout').read_text()
 stack=json.loads((directory/'stack.stdout').read_text())
 assert (stack['conservativeBytes'],stack['stackBytes'],stack['functions'],stack['calls'])==(7936,8192,216,1051)
 assert 'no execution or model proof' in stack['scope']
assert (here/'main/stack.stdout').read_bytes()==(here/'clean/stack.stdout').read_bytes()
assert (here/'main/observer/source-sha256.txt').read_bytes()==(here/'clean/observer/source-sha256.txt').read_bytes()
for name in ('unchanged-build.stderr','legacy-phase-roles.stderr','legacy-frontier-roles.stderr','old-family-refusal.stdout'):
 assert not (here/'main'/name).read_bytes()
assert '3 positive/86 ' in (here/'main/legacy-phase-roles.stdout').read_text()
assert '3 syntax positives/303 ' in (here/'main/legacy-frontier-roles.stdout').read_text()
refusal=(here/'main/old-family-refusal.stderr').read_text()
assert refusal.endswith('ValueError: artifact text.bin: type/length\n') and 'FileNotFoundError' not in refusal
print('PASS SAVED BF16 timing BUILD/main-clean pins/16 scoped sources/linked helpers/14 buckets/7936B static bound/'
 'old BF16 pins unchanged;NOT full ROOT,model execution,native admission,physical timing or speed proof')
