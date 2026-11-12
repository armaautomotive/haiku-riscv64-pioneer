#!/usr/bin/env python3
"""Guard the native source ordering; this is not a driver or hardware test."""
from pathlib import Path

root = Path(__file__).resolve().parents[3]
native = (root / 'src/add-ons/kernel/drivers/misc/PioneerQ38QgwdResidentNative.inc').read_text()
driver = (root / 'src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp').read_text()
begin = driver.split('BeginQ38NormalClock(PioneerStoreContext& c)', 1)[1].split('auto& dram=', 1)[0]
assert '|| c.q38Span' in begin, 'revisit ordering: clock preconditions changed'
handoff = native.split('PioneerQ38TokenCallScope::NextQgwdResidentRequest', 1)[1]
handoff = handoff.split('static __attribute__((noinline)) status_t Q38QgwdResidentSupport', 1)[0]
allocation = handoff.index('auto* freshSpan=new(std::nothrow) PioneerQ38SpanLease;')
publish_pending = handoff.index('cookie->qgwdResidentPending=next;')
failure_capture = handoff.index('if(freshSpan){next->q38Span=freshSpan;freshSpan=NULL;}')
clock = handoff.index('if(!PioneerQ38QgwdResidentClockBeforeSpan(*next,freshSpan,')
prepare = handoff.index('if(!PioneerQ38SpanPrepareService(*next)')
assert allocation < publish_pending < failure_capture < clock < prepare
assert 'next->q38Span=' not in handoff[allocation:failure_capture]
assert 'next->q38Span=' not in handoff[failure_capture + len('if(freshSpan){next->q38Span=freshSpan;freshSpan=NULL;}'):clock]
print('PASS resident clock-before-span source ordering and failure capture; NO DRIVER OR DEVICE')
