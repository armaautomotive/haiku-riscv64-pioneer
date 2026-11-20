/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_ACTIVATION_PLAN_H
#define PIONEER_QWEN35_ACTIVATION_PLAN_H

#include <stdint.h>

enum {
	PioneerQ35ActivationBankCount=7,
	PioneerQ35ActivationRowGroup=16,
	PioneerQ35ActivationMaxBuffers=16
};

struct PioneerQ35ActivationWorkspace {
	unsigned bank;
	uint32_t address,bytes;
};

struct PioneerQ35ActivationFragment {
	unsigned bank,firstRow,rows;
	uint32_t address,bytes;
};

struct PioneerQ35ActivationBuffer {
	unsigned slot,rows,rowBytes,fragmentCount;
	PioneerQ35ActivationFragment fragments[PioneerQ35ActivationBankCount];
};

struct PioneerQ35ActivationRange {
	unsigned bank,firstRow,rows;
	uint32_t address,bytes;
};

// Offline layout for the two ping-pong activation tensors and other bounded
// row buffers. It consumes only the workspace already reserved by the packed
// model plan. A caller still has to allocate, initialize, and prove each
// buffer on the card before any device kernel can use it.
class PioneerQwen35ActivationPlan {
	struct Arena {
		uint32_t base,bytes,cursor;
	};
	Arena arenas[PioneerQ35ActivationBankCount]={};
	PioneerQ35ActivationBuffer allocated[PioneerQ35ActivationMaxBuffers]={};
	unsigned allocationCount=0;
	bool ready=false;

	static uint64_t AlignPage(uint64_t bytes)
	{
		return (bytes+4095)&~UINT64_C(4095);
	}
	static bool Same(const PioneerQ35ActivationBuffer& a,
		const PioneerQ35ActivationBuffer& b)
	{
		if (a.slot!=b.slot || a.rows!=b.rows || a.rowBytes!=b.rowBytes
			|| a.fragmentCount!=b.fragmentCount) return false;
		for (unsigned i=0;i<a.fragmentCount;i++) {
			const auto& x=a.fragments[i]; const auto& y=b.fragments[i];
			if (x.bank!=y.bank || x.firstRow!=y.firstRow || x.rows!=y.rows
				|| x.address!=y.address || x.bytes!=y.bytes) return false;
		}
		return true;
	}
	bool ShapeValid(const PioneerQ35ActivationBuffer& buffer) const
	{
		if (!ready || !buffer.rows || (buffer.rows&(PioneerQ35ActivationRowGroup-1))
			|| !buffer.rowBytes || !buffer.fragmentCount
			|| buffer.fragmentCount>PioneerQ35ActivationBankCount) return false;
		unsigned first=0,seen=0;
		for (unsigned i=0;i<buffer.fragmentCount;i++) {
			const auto& fragment=buffer.fragments[i];
			if (fragment.bank>=PioneerQ35ActivationBankCount
				|| (seen&(1U<<fragment.bank)) || !fragment.rows
				|| (fragment.rows&(PioneerQ35ActivationRowGroup-1))
				|| fragment.firstRow!=first || fragment.rows>buffer.rows-first
				|| fragment.rows>UINT32_MAX/buffer.rowBytes
				|| fragment.bytes!=fragment.rows*buffer.rowBytes
				|| (fragment.address&4095)) return false;
			const Arena& arena=arenas[fragment.bank];
			const uint64_t end=uint64_t(fragment.address)+AlignPage(fragment.bytes);
			if (fragment.address<arena.base || end>uint64_t(arena.base)+arena.bytes)
				return false;
			first+=fragment.rows; seen|=1U<<fragment.bank;
		}
		for (unsigned i=buffer.fragmentCount;i<PioneerQ35ActivationBankCount;i++)
			if (buffer.fragments[i].bank || buffer.fragments[i].firstRow
				|| buffer.fragments[i].rows || buffer.fragments[i].address
				|| buffer.fragments[i].bytes) return false;
		return first==buffer.rows;
	}

public:
	static constexpr uint32_t BankBase=0x04000000;
	static constexpr uint32_t BankLimit=0xc4000000;

	template<class PackedPlan>
	bool InitializeFromPacked(const PackedPlan& packed,uint32_t reserveBank0Bytes=0)
	{
		PioneerQ35ActivationWorkspace workspace[PioneerQ35ActivationBankCount]={};
		for (unsigned bank=0;bank<PioneerQ35ActivationBankCount;bank++) {
			workspace[bank].bank=bank;
			if (!packed.Workspace(bank,workspace[bank].address,workspace[bank].bytes)) {
				ready=false; return false;
			}
		}
		return Initialize(workspace,PioneerQ35ActivationBankCount,reserveBank0Bytes);
	}

	bool Initialize(const PioneerQ35ActivationWorkspace* workspace,unsigned count,
		uint32_t reserveBank0Bytes=0)
	{
		ready=false; allocationCount=0;
		for (auto& arena:arenas) arena={};
		for (auto& buffer:allocated) buffer={};
		if (!workspace || count!=PioneerQ35ActivationBankCount) return false;
		for (unsigned i=0;i<count;i++) {
			const auto& region=workspace[i];
			if (region.bank>=PioneerQ35ActivationBankCount || !region.address || !region.bytes
				|| (region.address&4095) || (region.bytes&4095)
				|| region.address<BankBase
				|| uint64_t(region.address)+region.bytes>BankLimit) return false;
			if (arenas[region.bank].base) return false;
			if (region.bank==0 && ((reserveBank0Bytes&4095)
				|| reserveBank0Bytes>region.bytes)) return false;
			const uint32_t reserved=region.bank==0?reserveBank0Bytes:0;
			arenas[region.bank]={region.address,region.bytes,region.address+reserved};
		}
		for (const auto& arena:arenas) if (!arena.base) return false;
		ready=true;
		return true;
	}

	bool AllocateRows(unsigned rows,unsigned rowBytes,PioneerQ35ActivationBuffer& output)
	{
		if (!ready || allocationCount>=PioneerQ35ActivationMaxBuffers || !rows
			|| (rows&(PioneerQ35ActivationRowGroup-1)) || !rowBytes) return false;
		PioneerQ35ActivationBuffer candidate{};
		candidate.slot=allocationCount; candidate.rows=rows; candidate.rowBytes=rowBytes;
		const unsigned groups=rows/PioneerQ35ActivationRowGroup;
		unsigned first=0;
		Arena updated[PioneerQ35ActivationBankCount];
		for (unsigned bank=0;bank<PioneerQ35ActivationBankCount;bank++) updated[bank]=arenas[bank];
		for (unsigned bank=0;bank<PioneerQ35ActivationBankCount;bank++) {
			const unsigned bankGroups=groups/PioneerQ35ActivationBankCount
				+unsigned(bank<groups%PioneerQ35ActivationBankCount);
			if (!bankGroups) continue;
			if (bankGroups>UINT32_MAX/PioneerQ35ActivationRowGroup) return false;
			const unsigned bankRows=bankGroups*PioneerQ35ActivationRowGroup;
			const uint64_t bytes=uint64_t(bankRows)*rowBytes,allocation=AlignPage(bytes);
			const uint64_t end=uint64_t(updated[bank].cursor)+allocation;
			if (bytes>UINT32_MAX || allocation>UINT32_MAX
				|| end>uint64_t(updated[bank].base)+updated[bank].bytes) return false;
			candidate.fragments[candidate.fragmentCount++]={bank,first,bankRows,
				updated[bank].cursor,uint32_t(bytes)};
			updated[bank].cursor=uint32_t(end);
			first+=bankRows;
		}
		if (first!=rows || !ShapeValid(candidate)) return false;
		for (unsigned i=0;i<allocationCount;i++) for (unsigned a=0;a<allocated[i].fragmentCount;a++)
			for (unsigned b=0;b<candidate.fragmentCount;b++) {
				const auto& prior=allocated[i].fragments[a];
				const auto& next=candidate.fragments[b];
				if (prior.bank==next.bank
					&& prior.address<uint64_t(next.address)+AlignPage(next.bytes)
					&& next.address<uint64_t(prior.address)+AlignPage(prior.bytes)) return false;
			}
		for (unsigned bank=0;bank<PioneerQ35ActivationBankCount;bank++) arenas[bank]=updated[bank];
		allocated[allocationCount++]=candidate; output=candidate;
		return true;
	}

	bool Valid(const PioneerQ35ActivationBuffer& buffer) const
	{
		return ready && buffer.slot<allocationCount && ShapeValid(buffer)
			&& Same(buffer,allocated[buffer.slot]);
	}

	bool ResolveRows(const PioneerQ35ActivationBuffer& buffer,unsigned firstRow,unsigned rows,
		PioneerQ35ActivationRange& range) const
	{
		if (!Valid(buffer) || !rows || ((firstRow|rows)&(PioneerQ35ActivationRowGroup-1))
			|| firstRow>=buffer.rows || rows>buffer.rows-firstRow) return false;
		for (unsigned i=0;i<buffer.fragmentCount;i++) {
			const auto& fragment=buffer.fragments[i];
			if (firstRow<fragment.firstRow || firstRow-fragment.firstRow>=fragment.rows) continue;
			const unsigned offsetRows=firstRow-fragment.firstRow;
			if (rows>fragment.rows-offsetRows) return false;
			const uint64_t offset=uint64_t(offsetRows)*buffer.rowBytes;
			const uint64_t bytes=uint64_t(rows)*buffer.rowBytes;
			const uint64_t address=uint64_t(fragment.address)+offset;
			if (bytes>UINT32_MAX || address>UINT32_MAX || offset>fragment.bytes
				|| bytes>fragment.bytes-offset) return false;
			range={fragment.bank,firstRow,rows,uint32_t(address),uint32_t(bytes)};
			return true;
		}
		return false;
	}
};

#endif
