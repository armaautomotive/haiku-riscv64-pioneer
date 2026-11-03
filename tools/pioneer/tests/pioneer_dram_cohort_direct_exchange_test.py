#!/usr/bin/env python3
"""Actual direct-revision helper in unchanged synthetic exchange fault suite."""
import pathlib
here=pathlib.Path(__file__).resolve().parent
source=(here/'pioneer_dram_sweep_exchange_test.py').read_text()
rebindings={
 'pioneer_dram_sweep_exchange.sh':'pioneer_dram_cohort_direct_exchange.sh',
 '83029ee4d91ffd7df4aebfb4839d041903d1bc2dc0ba98acfb0f0a85c5b7e5c8':'f35559c24c60a36dfdd94cb6f965502bedcbd38be7f6ffe66ca317a5a872e195',
 '2026110200050076':'2026110300020077',
 'tenstorrent_pci_probe-dram-sweep-diagnostic76':'tenstorrent_pci_probe-dram-cohort-direct77',
 'synthetic-diagnostic76':'synthetic-direct77',
}
for old,new in rebindings.items():
    assert old in source
    source=source.replace(old,new)
exec(compile(source,str(here/'pioneer_dram_sweep_exchange_test.py'),'exec'),{'__file__':__file__,'__name__':'__main__'})
