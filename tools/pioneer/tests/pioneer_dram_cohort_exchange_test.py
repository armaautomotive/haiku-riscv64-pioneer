#!/usr/bin/env python3
"""Reuse unchanged exchange endpoint fault suite with this exact subject.

Only subject basename,tag/hash and synthetic label change. The actual new
production script is read by that suite;no shell algorithm is replaced.
"""
import pathlib
here=pathlib.Path(__file__).resolve().parent
source=(here/'pioneer_dram_sweep_exchange_test.py').read_text()
rebindings={
 'pioneer_dram_sweep_exchange.sh':'pioneer_dram_cohort_exchange.sh',
 '83029ee4d91ffd7df4aebfb4839d041903d1bc2dc0ba98acfb0f0a85c5b7e5c8':'e50f1dfdb05827ee020a106aea474b3e926ad7a8f17de3decfbd71cd72b2e0e3',
 '2026110200050076':'2026110300010077',
 'tenstorrent_pci_probe-dram-sweep-diagnostic76':'tenstorrent_pci_probe-dram-cohort-diagnostic77',
 'synthetic-diagnostic76':'synthetic-cohort77',
}
for old,new in rebindings.items():
    assert old in source
    source=source.replace(old,new)
exec(compile(source,str(here/'pioneer_dram_sweep_exchange_test.py'),'exec'),{'__file__':__file__,'__name__':'__main__'})
