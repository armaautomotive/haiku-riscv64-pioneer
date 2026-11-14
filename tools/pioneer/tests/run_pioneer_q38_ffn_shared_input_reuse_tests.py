#!/usr/bin/env python3
import pathlib
import subprocess
import tempfile

here=pathlib.Path(__file__).resolve().parent
root=here.parent
with tempfile.TemporaryDirectory(prefix='q38-ffn-input-reuse-') as tmp:
 binary=pathlib.Path(tmp)/'contract'
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-Werror',
  '-DPIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE=1','-DPIONEER_Q38_GLOBAL_CHAIN_COMPONENT_IMAGE=1',
  str(here/'pioneer_q38_ffn_shared_input_reuse_test.cpp'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)

recurrent=(root/'blackhole_qwen38_mixed_recurrent_block_root.cpp').read_text()
recurrent_layout=(root/'PioneerQ38MixedRecurrentBlockRootLayout.h').read_text()
projection=(root/'blackhole_qwen38_mixed_projection_root.cpp').read_text()
worker=(root/'blackhole_qwen38_global_worker.cpp').read_text()
rows=(root/'blackhole_qwen38_rows.cpp').read_text()
components=(root/'build_qwen38_global_token_components.py').read_text()
worker_builder=(root/'build_qwen38_global_worker_image.py').read_text()
token_builder=(root/'build_qwen38_token_root.py').read_text()
full_builder=(root/'build_qwen38_full_offload_root.py').read_text()
pushed_worker_builder=(root/'build_qwen38_pushed_worker_image.py').read_text()
program_generator=(root/'generate_qwen38_token_programs.py').read_text()
program_role=(root/'PioneerQ38TokenFfnSharedInputReuseProgramsRole.h').read_text()
assert 'PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE' in recurrent
assert 'if(slot==1)' in recurrent
assert 'slot==5?2:slot==6?3' in recurrent_layout
assert 'Pair(previous,d)' in recurrent
reuse_header=(root/'PioneerQ38FfnSharedInputReuse.h').read_text()
assert 'same sealed recurrent request' in reuse_header
assert recurrent.index('RecipeSeal(count);unsigned from') < recurrent.index('Pair(previous,d)')
assert 'CaptureRecipes(&seal,5,3);Matrix(&seal,0,3,10);Matrix(&seal,1,3,11);' in recurrent
assert 'if(slot==1)' in recurrent and 'Projection::Run(d)!=n' in recurrent
assert 'd[43]==PioneerQ38FfnSharedInputReuse::UpRole?0U:1U' in projection
assert 'if(d[43]!=PioneerQ38FfnSharedInputReuse::UpRole)' in projection
assert 'InputReads(reuseSharedInput,k)' not in projection
assert 'InputFlits(reuseSharedInput,k,blocks)' not in projection
assert 'PIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE' in worker
assert 'InputReads(j[9]==0,j[5])' in worker
assert 'InputFlits(j[9]==0,j[5],blocks)' in worker
assert 'if (job[9]) SharedInputs(inputs,blocks);' in rows
assert "'--ffn-shared-input-reuse'" in components
assert "-DPIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE=1" in components
assert "'--ffn-shared-input-reuse'" in worker_builder
assert "-DPIONEER_Q38_FFN_SHARED_INPUT_REUSE_CANDIDATE=1" in worker_builder
assert "'--ffn-shared-input-reuse'" in token_builder
assert "'--ffn-shared-input-reuse'" in full_builder
assert "'--ffn-shared-input-reuse'" in pushed_worker_builder
assert "PINS_FFN_SHARED_INPUT_REUSE" in full_builder and "PINS_FFN_SHARED_INPUT_REUSE" in pushed_worker_builder
assert "PIONEER_Q38_TOKEN_FFN_SHARED_INPUT_REUSE_PROGRAMS" in program_generator
assert "PioneerQ38TokenFfnSharedInputReusePrograms" in program_generator
assert "PioneerQ38TokenFfnSharedInputReuseProgramsRole.h" in program_generator
assert "not a native, validation, kernel or Work selector" in program_role
print('PASS FFN gate->up candidate predicate, sender/worker NoC delta contract, and distinct component/worker builder switches; static/synthetic only, not full image, model correctness or speed qualification')
