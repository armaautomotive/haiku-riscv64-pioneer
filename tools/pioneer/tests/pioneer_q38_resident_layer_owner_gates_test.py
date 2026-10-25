#!/usr/bin/env python3
"""The full-layer controller is a distinct private consumer, not old native68."""
import os
import pathlib
import subprocess

here = pathlib.Path(__file__).resolve().parent
kernel = here.parents[2]/'src/add-ons/kernel/drivers/misc'
valid = {'PIONEER_Q38_RESIDENT_LAYER_NATIVE_COMPONENT':'1', 'PIONEER_Q38_RESIDENT_LAYER_OWNER':'1',
         'PIONEER_Q38_RESIDENT_OUTPUT_OWNER':'1', 'PIONEER_Q38_PROJECTION_EXECUTION_OWNER':'1',
         'PIONEER_Q38_PROJECTION_RESULT_STASH':'1', 'PIONEER_Q38_LEARNED_RMS_NATIVE':'1',
         'PIONEER_Q38_EMBEDDING_NATIVE':'1', 'PIONEER_Q38_DECODER_RUNTIME':'1', 'PIONEER_Q38_GDN_MUTABLE_STATE':'1'}
role = '#include "'+str(kernel/'PioneerQ38ResidentLayerOwnerRole.h')+'"\n'
clean = {k:v for k,v in os.environ.items() if not k.startswith('PIONEER_')}
count = 0


def check(text, definitions, expected, syntax=False):
    global count
    result = subprocess.run([os.environ.get('CXX','c++'), '-std=c++17', '-x', 'c++',
                             '-fsyntax-only' if syntax else '-E',
                             *['-D'+k+'='+v for k,v in definitions.items()], '-'],
                            input=text, text=True, capture_output=True, env=clean, timeout=60)
    assert (result.returncode == 0) == expected, (definitions, result.stderr)
    count += 1
    return result


check(role, valid, True)
check(role, {**valid, '_KERNEL_MODE':'1'}, True)
for name in valid:
    for value in (None, '', '0', '2', '-1'):
        definitions = dict(valid)
        if value is None:
            del definitions[name]
        else:
            definitions[name] = value
        check(role, definitions, False)
    check(role+'#undef '+name+'\n'+role, valid, False)
for name in ('PIONEER_Q38_CPU_VALIDATION', 'PIONEER_Q38_WORK_INFERENCE',
             'PIONEER_Q38_RESIDENT_OUTPUT_VALIDATION_NATIVE', 'PIONEER_Q38_RESIDENT_OUTPUT_CONTEXT_COMPILE_ONLY',
             'PIONEER_Q38_RESIDENT_DECODER_OWNER', 'PIONEER_Q38_GDN_RECURRENT_NATIVE', 'PIONEER_Q38_GDN_INPUT_OWNER',
             'PIONEER_Q38_RGDN_ROOT_QUALIFICATION', 'PIONEER_Q38_NATIVE_Q8_LAYOUT', 'PIONEER_Q38_GDN_GATED_NORM_LOCAL',
             'PIONEER_Q38_RESIDENT_LAYER_ROOT_IMAGE', 'PIONEER_Q38_RESIDENT_LAYER_NORM_IMAGE',
             'PIONEER_Q38_RESIDENT_LAYER_FFN_IMAGE', '__FAST_MATH__'):
    for value in ('0','1'):
        check(role, {**valid, name:value}, False)
        check(role+'#define '+name+' '+value+'\n'+role, valid, False)
header = '#include "'+str(kernel/'PioneerQ38ResidentLayerOwner.h')+'"\n'
check(header+'#include <type_traits>\n'
      'static_assert(!std::is_copy_constructible<PioneerQ38ResidentLayerOwner>::value);\n'
      'static_assert(!std::is_copy_assignable<PioneerQ38ResidentLayerOwner>::value);\n'
      'static_assert(!std::is_default_constructible<PioneerQ38ResidentProjectionInput>::value);\n', valid, True, True)
for member in ('layerConsumer', 'owner', 'token', 'phase', 'source'):
    result = check(header+'void forbidden(PioneerQ38ResidentProjectionInput& input){ (void)input.'+member+'; }\n', valid, False, True)
    assert 'private' in result.stderr
for bank in ('PioneerQ38ConvHistoryBanks', 'PioneerQ38GdnBanks'):
    result = check(header+'void forbidden('+bank+'& state){ state.residentLayerOwner=nullptr; }\n', valid, False, True)
    assert 'private' in result.stderr
# Old IO cannot acquire the new controller merely because it has the old
# output Scope/map/journal callbacks. Instantiate the REAL complete body.
old = '#define PIONEER_Q38_PROJECTION_EXECUTION_FIXTURE_ONLY\n'
old += '#include "'+str(here/'pioneer_q38_resident_output_owner_execution_test.cpp')+'"\n'+header
old += 'bool rejected(ResidentExecutionIO& io,PioneerQ38ProjectionGraphOwner& input,PioneerQ38ResidentLayerOwner& owner,PioneerQ38ResidentLayerMemory& memory){PioneerQ38ResidentLayerResult result;return owner.Prepare(io,input,memory,result)&&owner.Run(io,result)&&owner.Close(io,result);}\n'
result = check(old, valid, False, True)
assert 'ResidentLayerExecutionScope' in result.stderr and 'ResidentLayerRetainHostOwners' in result.stderr
print(f'PASS new full-layer owner gates{count}:exact roles/reinclusion/defined0+old68/CPU/Work/image refusals,private noncopyable input and separate state claims,OLD IO cannot instantiate NEW lifetime;NO native factory/driver or physical execution qualification')
