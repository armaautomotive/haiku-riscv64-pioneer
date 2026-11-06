#!/usr/bin/env python3
"""Verify saved closed110 evidence; never grants live execution/cleanup authority."""
import hashlib
import json
from pathlib import Path
import re
import sys
from decimal import Decimal

if not __debug__:
    raise SystemExit('Checks required')
here = Path(__file__).resolve().parent
root = here.parents[3]
files = here / 'remote'
manifest = json.loads((here / 'manifest.json').read_text())
assert manifest['actual_exits'] == {
    'primary_waiter': 0, 'independent_postflight': 0, 'whole_archive_copy': 0,
    'reader_main': 0, 'reader_clean': 0,
}
assert manifest['source_commit'] == 'f934d9f7afa93aeb88ec73c689b26b487485a7e7'
assert manifest['package_commit'] == 'cd8c379cd962a4dbba3e5cbaf021bcad838deee6'
for name, receipt in manifest['saved_records'].items():
    data = (here / name).read_bytes()
    assert len(data) == receipt['bytes']
    assert hashlib.sha256(data).hexdigest() == receipt['sha256'], name
assert (here / 'primary.stdout').read_text() == 'DIRECT_DEFERRED_FULL_110_PARENT_ACTUAL_EXIT0\n'
for name in ('primary.stderr', 'postflight.stderr', 'reader-main.stderr', 'reader-clean.stderr'):
    assert not (here / name).read_bytes(), name
post = (here / 'postflight.stdout').read_text()
assert 'ALL110_TRIAL_PIDS_ABSENT_NO_MATCHING_OWNER\n' in post
assert post.endswith('PASS_INDEPENDENT_DEFERRED_110_CLOSED_NORMAL041_GEN50_64CPU_FULLRAM_ASSISTANT_RUNTIME_PRESERVED\n')
expected = {name: digest for digest, name in re.findall(
    r'^([0-9a-f]{64})  /boot/home/pioneer-q38-bf16-deferred-110-a/([^/\n]+)$', post, re.M)}
assert len(expected) == 14
names = {
    'pioneer_q38_bf16_deferred_full_offload_physical.sh', 'direct-parent.stdout',
    'direct-parent.stderr', 'direct-parent.exit', 'rate.stdout', 'rate.stderr',
    'rate.exit', 'state', 'parent.pid', 'direct-launcher.pid',
}
assert {p.name for p in files.iterdir()} == names
for name in names:
    assert hashlib.sha256((files / name).read_bytes()).hexdigest() == expected[name], name
for name in ('direct-parent.exit', 'rate.exit'):
    assert (files / name).read_text() == '0\n'
for name in ('direct-parent.stderr', 'rate.stderr'):
    assert not (files / name).read_bytes()
assert (files / 'state').read_text() == 'FULL_CARD_RATE32_CLOSED_STABLE_RESTORED\n'
assert (files / 'parent.pid').read_text() == '41593\n'
assert (files / 'direct-launcher.pid').read_text() == '41588\n'
script = 'pioneer_q38_bf16_deferred_full_offload_physical.sh'
assert expected[script] == 'b1a7c8ac9d830fe78bc3b1174e48bd694499fc49d16dcd61aa118f2b8459e1dc'
assert (files / script).read_bytes() == (root / 'tools/pioneer' / script).read_bytes()
parent = (files / 'direct-parent.stdout').read_text()
assert parent.endswith('PASS_FULL_CARD_RATE32_STABLE_RESTORED_NO_ASSISTANT_INSTALL_OR_SPEED_CLAIM\n')
assert parent.count('HOT_RELOAD_VERIFIED;NO_REBOOT_NO_GPU_EXECUTION\n') == 2
statuses = re.findall(r'^P100A_RELOAD driver55 tag([0-9]{16}) generation([0-9]+) quiesced[01] cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY$', parent, re.M)
assert len(statuses) == 12 and {int(g) for _, g in statuses} == {48, 49, 50}
for tag, generation in statuses:
    assert tag == ('2026110600010110' if generation == '49' else '2026102100010012')
assert 'generation50 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY' in post
assert len(re.findall(r'^64 .*running at', post, re.M)) == 1 and '/ 137034936320)' in post
for digest in (
    '041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403',
    'b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f',
    'd1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229',
    '861e5287e6f417c28968422d01721c95bc2c73ed17e9111806dfa76477984a74',
    'c509da438d06f7e0dbcb23f4ef0fec2b000bfab923f5c7e86836127736d29e18',
    '0de9141eb3955cf1c993a67c80ef648b3538ce7155c17677458487ee083a4933',
    '0ae23a2c211af9fbafb89c3d93bdd136a4bdd574baeeaf2d622f582efcf426af',
    'a3b81c133641bbdacde8dd1d0e0ffa7a0a1ec0c33e95c426df1db59735379c16',
    '9cb45a36558bd78aa4d4b7ed5385cbf9df32de208f9e6c9b6d064e8527dca389',
    '69e9ec2c43a123afbb256c3feb6313cdcebdf1b2a87278ac3cdac8a1a65273d2',
):
    assert re.search(r'^' + digest + r'  /boot/home/[^\n]+$', post, re.M), digest
assert (here / 'reader-main.stdout').read_bytes() == (here / 'reader-clean.stdout').read_bytes()
sys.path.insert(0, str(root / 'tools/pioneer/tests'))
from parse_pioneer_q38_full_offload_rate_log import parse_complete, stable_text
parsed = parse_complete(stable_text(files / 'rate.stdout'), stable_text(files / 'rate.stderr'),
                        phase_profile=True, bf16_profile=True, startup_timing=True)
assert parsed == json.loads((here / 'reader-main.stdout').read_text().splitlines()[0]) == manifest['parsed']
assert (parsed['prompts'], parsed['output_tokens'], parsed['upload_rows'], parsed['uploaded_bytes']) == (31, 32, 2298, 19399024640)
assert parsed['answer_utf8_preview'] == '\n'.join(map(str, range(1, 15)))
rate = parsed['observed_post_prompt_count_rate_bounds']
assert (rate['lower_tps'], rate['upper_tps'], rate['sampled_commit_steps']) == ('0.082246701', '0.082247442', 31)
profile = parsed['on_card_phase_profile']
assert profile['valid'] and profile['committed'] == 62 and profile['total_cycles'] == 1017855297132
assert len(profile['buckets']) == 14 and not profile['physical_clock_frequency_assumed']
assert not parsed['broad_model_quality'] and not parsed['execution_or_cleanup_authority_from_log']
startup = parsed['host_startup_observations']
assert startup['available'] and startup['first_commit_available'] and not startup['first_commit_is_streamed_text']
assert (startup['vocabulary_authentication_us'], startup['request_setup_us'], startup['upload_calls'], startup['upload_window_us'], startup['upload_ioctl_sum_us']) == (314627910, 1585526808, 20367, 1244364788, 1080931219)
assert (startup['first_commit_minimum_us'], startup['first_commit_maximum_us'], startup['answer_ready_us']) == (1962511042, 1962616995, 2562911990)
assert manifest['pids_absent'] == [41588, 41593, 41677, 41678]
assert manifest['normal_restored_generation'] == 50
assert not manifest['measured_20tokens_per_second_in_this_short_run']
assert manifest['assistant_promotion'] == 'REJECTED_SLOWER_THAN_EARLIER92_AND103'
# Read CLOSED earlier results,not new hardware trials or a claim of repeatability.
previous = json.loads((here.parent / 'qwen38-bf16-pair-issue-full-physical-20261106/manifest.json').read_text())['parsed']
vector = here.parent / 'qwen38-vector-q4-full-card-20261104'
vector_parsed = parse_complete(stable_text(vector / 'rate.stdout'), stable_text(vector / 'rate.stderr'))
assert vector_parsed['observed_post_prompt_count_rate_bounds']['lower_tps'] == '0.172813448'
assert vector_parsed['answer_hex'] == parsed['answer_hex'] == previous['answer_hex']
assert Decimal(rate['upper_tps']) < Decimal(vector_parsed['observed_post_prompt_count_rate_bounds']['lower_tps'])
old_rate = previous['observed_post_prompt_count_rate_bounds']
low_gain = (Decimal(rate['lower_tps']) / Decimal(old_rate['upper_tps']) - 1) * 100
high_gain = (Decimal(rate['upper_tps']) / Decimal(old_rate['lower_tps']) - 1) * 100
assert Decimal('4.5695') < low_gain < high_gain < Decimal('4.5715')
assert Decimal('4.38') < (1 - Decimal(profile['total_cycles']) / previous['on_card_phase_profile']['total_cycles']) * 100 < Decimal('4.39')
print('PASS SAVED closed110/real27B/32outputs/0.082246701..0.082247442tok-s/'
      'whole hashes+actual exits0+independent normal041-gen50/64CPU-fullRAM-Assistant preserved;'
      'REJECT slower-than-earlier92/103;NOT repeatability,broad-quality,live authority or20TPS')
