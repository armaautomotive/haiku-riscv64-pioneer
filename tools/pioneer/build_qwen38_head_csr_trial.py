#!/usr/bin/env python3
"""Bind a one-shot raw-Q4 head A/B runner locally. No SSH or device I/O."""
import hashlib
from pathlib import Path
import re
import sys


TEMPLATE = Path(__file__).with_name('pioneer_q38_tag206_phase_rate_once.sh')
TEMPLATE_SHA = '4509d788d8cc1db951fec20d0acefcdd8d2651ace833726801d1066e79b15416'
PACKAGES = {
    'serial': ('2026111700010207',
               'e9129fec2fe15ad7158c1712cd4f10d8ef0c61b5ac247ed1c48ca4ad55274439',
               '9b20a1c1da09f4af7bf68fec3ab63a3f4a693ca28faee12905a24a99f7cb1f10',
               '8df6d5ea75bce8cff68fc7bc5b318b8f2cf4389498af6e6d8c10cbef2ae9e0f8'),
    'candidate': ('2026111700010208',
                  '05e2e997791ea3e4d9de50e3efbc4f5ec58b424c657a746d4017f56f52105bdf',
                  '4b8c839c84a947a965d17301f5a293d88e4af9b0763f14c132b820a3d7114f58',
                  'b611e035f1a9d6d4897843d06c4d7463b7a7ac6699218087308ebd480e5d5f51'),
}


def bind(variant, start):
    return bind_raw_pair(variant, start, PACKAGES, 'head-csr', 'head CSR', 'HEAD_CSR')


def bind_raw_pair(variant, start, packages, route, title, marker, *, resident=False):
    assert (route,title,marker,resident) in (('head-csr','head CSR','HEAD_CSR',False),
                                            ('ffn-retained','retained FFN','FFN_RETAINED',False),
                                            ('layer-split','layer split profile','LAYER_SPLIT',False),
                                            ('native-q8','native Q8','NATIVE_Q8',False),
                                            ('native-q8','native Q8','NATIVE_Q8_OBSERVATION',False),
                                            ('profile-head','v4 head batch','PROFILE_HEAD',False),
                                            ('resident','vector resident','VECTOR_RESIDENT',True))
    if variant not in packages or not re.fullmatch(r'[1-9][0-9]{0,5}', start):
        raise ValueError('supply an exact pair variant and the observed positive stable generation')
    tag, driver, worker, client = packages[variant]
    assert re.fullmatch(r'[0-9]{16}', tag) and all(re.fullmatch(r'[0-9a-f]{64}', h) for h in (driver,worker,client))
    g0 = int(start)
    g1, g2 = g0+1, g0+2
    raw = TEMPLATE.read_bytes()
    if hashlib.sha256(raw).hexdigest() != TEMPLATE_SHA:
        raise ValueError('restore template changed; review and reverify before rebinding')
    text = raw.decode('ascii')

    def change(old, new, count=1):
        nonlocal text
        if text.count(old) != count:
            raise ValueError('unexpected template binding: ' + old)
        text = text.replace(old, new)

    change('# One new, exact-generation all-card trial. Never run a spent stage.',
           f'# One fresh {variant} {title} raw-Q4 trial. Bound to stable generation {g0}.')
    change('stage=/boot/home/pioneer-q38-tag206-phase-rate-20261117-a',
           f'stage=/boot/home/pioneer-q38-{route}-{variant}-{tag[-3:]}\n'
           # Both experiments share the same lock, including live tag208.
           'pairLock=/boot/home/.pioneer-q38-head-csr-exclusive')
    change('candidate=/boot/home/pioneer-q38-gdn-l1-tag206-20261116-a/tenstorrent_pci_probe-gdn-l1-206',
           'candidate=$stage/tenstorrent_pci_probe-native')
    for old, new in (
            ('2026111500010206', tag),
            ('1c2ad62853c4617690956e083da8c364d27c5f1399fdc9509f8c6f781684e475', driver),
            ('30a0f35d85a7449a9b8aae93d19f9f0e0edc5047c8a8fa339c5f977f2a5521a4', worker),
            ('27c2d25a6e9d3fb74ff3a375c7a1438e2d7cac3f416a0e70ab3bf416badabcaa', client)):
        change(old, new)
    # Replace generation checks in one pass so consecutive generations are
    # never mistaken for an old placeholder during substitution.
    generations = (
        ('__STABLE_BEFORE__', 'metadata "$stableTag" 1', f'metadata "$stableTag" {g0}', 2),
        ('__CANDIDATE__', 'metadata "$candidateTag" 2', f'metadata "$candidateTag" {g1}', 2),
        ('__STABLE_AFTER__', 'metadata "$stableTag" 3', f'metadata "$stableTag" {g2}', 1),
        # The protocol's wait argument is the old generation in hex.
        ('__WAIT_INITIAL__', '0000000000000001', f'{g0:016x}', 1),
        ('__WAIT_RESTORE__', '0000000000000002', f'{g1:016x}', 1))
    for key, old, _, count in generations:
        change(old, key, count)
    for key, _, new, count in generations:
        change(key, new, count)
    change('STABLE041_GEN3_RESTORED', f'STABLE041_GEN{g2}_RESTORED')
    change('\tcommand -v cmp > /dev/null || fail missing_cmp',
           '\t[ "$(stat -c \'%d\' "$stage")" = "$(stat -c \'%d\' "$installed")" ] || fail different_volume\n'
           '\ttest ! -e "$pairLock" && test ! -L "$pairLock" || fail pair_lock_present')
    # Only the raw worker is bound here: both builds must have empty stderr.
    start_fn, end_fn = text.index('readiness_stderr(){'), text.index('restore(){')
    text = text[:start_fn] + text[end_fn:]
    change('readiness_stderr || fail unknown_worker_stderr',
           'test ! -s "$stage/rate.stderr" || fail nonempty_worker_stderr')
    if resident:
        change('client=$stage/pioneer-q38-full-offload-rate-smoke',
               'client=$stage/pioneer-q38-full-offload-resident-rate')
        change('"$client" --full-card-rate-32', '"$client" --full-card-resident-rate-32')
        change('^PASS_FULL_OFFLOAD_RATE_JOB clean_worker_exit1 rate_available[01] output_tokens32;'
               'CPU_INFERENCE_CONTEXTS0;POST_PROMPT_COUNT_WINDOW_NOT_BROAD_QUALITY_OR_ISOLATED_LATENCY$',
               '^PASS_RESIDENT_TWO_REQUEST one_upload1 model_end1 worker_exit0 '
               'CPU_INFERENCE_CONTEXTS0;NOT_BROAD_QUALITY$')
    else:
        change('rate_available[01]', 'rate_available1')
    if route in ('layer-split','native-q8','profile-head'):
        before='test ! -s "$stage/rate.stderr" || fail nonempty_worker_stderr'
        change(before, before+'\n'
               'awk \'/^FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile valid1 committed62 window_start31 last_position61 /{headers++}\n'
               ' /^FULL_OFFLOAD_PROGRESS id2 value4294967295 Full-card layer split profile bucket[0-9]+ /{buckets++}\n'
               ' END{if(headers!=1 || buckets!=14)exit 1}\' "$stage/rate.stdout" || fail missing_closed_v4_timing')
    # No device-open metadata probe before the process-clear check on restore.
    change(f'metadata "$candidateTag" {g1} && idle', f'idle && metadata "$candidateTag" {g1}', 2)
    change('mkdir "$stage/.trial-once" || fail lock\nexchange=0',
           'mkdir "$pairLock" || fail pair_lock\nexchange=0')
    change("trap 'exit 1' HUP INT TERM", "trap 'exit 1' HUP INT TERM\n"
           'mkdir "$stage/.trial-once" || fail lock')
    change('\tfi\n\texit "$result"\n}',
           '\tfi\n\trmdir "$pairLock" || fail pair_lock_release\n\texit "$result"\n}')
    change('# Always restore via cleanup. An apparent client success is insufficient to\n'
           '# report a pass until a fresh, idle generation3 and the stable hash agree.',
           '# A closed client is not a physical pass without stable-driver restoration\n'
           '# and independent validation of the entire token/count transcript.')
    text = text.replace('TAG206_PHASE_RATE', marker+'_RATE').replace('TAG206', marker)
    return text


def main():
    if len(sys.argv) != 4:
        raise SystemExit('use serial|candidate OBSERVED_STABLE_GENERATION NEW_SCRIPT')
    variant, generation, path = sys.argv[1:]
    bound = bind(variant, generation).encode('ascii')
    with Path(path).open('xb') as output:
        output.write(bound)
    print(hashlib.sha256(bound).hexdigest(), path)


if __name__ == '__main__':
    main()
