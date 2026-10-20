/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CLOCK_CYCLE_H
#define PIONEER_Q38_CLOCK_CYCLE_H
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38BankProtocol.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerDriverReloadProtocol.h"
// Bounded qualification ONLY. Pinned normal-clock build,BEGIN+END with NO
// upload,RISC image/core start,matrix,reset,arbitrary message or retry.
// The kernel validates/reports the actual clocks. A failed reply is CLOSE
// ONLY:the caller cannot infer permission for END or another operation.
template<class IO> static bool PioneerQ38ClockCycle(IO& io)
{
	const int fd=io.Open(); if (fd<0) return false;
	auto stop=[&]() { io.Close(fd); return false; };
	tt_pioneer_driver_reload metadata={}; metadata.version=1;
	if (!io.Call(fd,TT_PCI_PROBE_RELOAD_STATUS,&metadata,sizeof(metadata))
		|| metadata.version!=1 || metadata.driver!=55 || metadata.guard_version!=1
		|| metadata.build_tag!=UINT64_C(0x2026102000010004) || !metadata.generation
		|| metadata.quiesced || metadata.cold || metadata.active_owner || metadata.open_count!=1) return stop();
	tt_q38_bank_support support={};
	if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_SUPPORT,&support,sizeof(support))
		|| support.driver!=45 || support.version!=1 || support.banks!=7 || support.page_bytes!=4096
		|| support.max_pages!=4 || support.storage_only!=1 || support.bytes_per_bank!=PioneerResidentBanks::Capacity) return stop();
	tt_q38_bank_session session={}; session.version=1;
	if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_BEGIN,&session,sizeof(session))) return stop();
	const auto& begin=session.result;
	if (begin.version!=1 || begin.state!=TT_RESIDENT_SESSION_READY || begin.active!=1 || begin.host_ready!=1
		|| begin.cold || begin.capabilities || begin.bytes!=PioneerResidentBanks::Total) return stop();
	session={}; session.version=1;
	if (!io.Call(fd,TT_PCI_PROBE_Q38_BANK_END,&session,sizeof(session))) return stop();
	const auto& end=session.result;
	if (end.version!=1 || end.state!=TT_RESIDENT_SESSION_CLOSED || end.active || end.cold
		|| end.pci_restored!=1 || end.host_released!=1) return stop();
	return io.Close(fd);
}
#endif
