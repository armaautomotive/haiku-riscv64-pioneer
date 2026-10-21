/* SPDX-License-Identifier: MIT */
// OBJECT ONLY witness. Compile the COMPLETE actual driver and instantiate
// its unselected private adapters with the ACTUAL native IO type. This
// file is NEVER linked into a module or used by the ordinary driver build.
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_q38_gdn_native_adapter_compile(PioneerStoreContext& context,unsigned operation,
	unsigned region,unsigned first,unsigned pages,PioneerQ38StateZeroResult& result)
{
	if (!operation) return PrepareQ38MutableZero(context);
	if (operation==1) return RunQ38MutableZero(context,region,first,pages,result);
	if (operation==2) return CloseQ38MutableZero(context,true);
	if (operation==3) return CloseQ38MutableZero(context,false);
	return false;
}
extern "C" bool pioneer_q38_gdn_call_scope_compile(PioneerProbeCookie* cookie,unsigned operation,
	unsigned region,unsigned first,unsigned pages,PioneerQ38StateZeroResult& result)
{
	PioneerQ38GdnCallScope scope(cookie); auto* context=scope.Context();
	if (!context) return false;
	return pioneer_q38_gdn_native_adapter_compile(*context,operation,region,first,pages,result);
}
extern "C" bool pioneer_q38_gdn_bind_owner_compile(PioneerProbeCookie* cookie,const PioneerQ38GdnIdentity& identity)
{
	PioneerQ38GdnCallScope scope(cookie); auto* context=scope.Context();
	return context && BindQ38MutableOwner(*context,identity);
}
extern "C" bool pioneer_q38_gdn_initialize_compile(PioneerProbeCookie* cookie,PioneerQ38InitializeOwnedResult& result)
{
	PioneerQ38GdnCallScope scope(cookie); auto* context=scope.Context();
	return context && InitializeQ38MutableState(*context,result);
}
extern "C" bool pioneer_q38_gdn_qualify_compile(PioneerProbeCookie* cookie,PioneerQ38InitializeOwnedResult& result)
{
	PioneerQ38GdnCallScope scope(cookie); auto* context=scope.Context();
	return context && QualifyQ38MutableState(*context,result);
}
#ifdef PIONEER_Q38_RESIDENT_GDN_ROOT_DIAGNOSTIC
// Instantiate REAL native IO/controller functions in this OBJECT ONLY.
// No allocation/native entry/linked module or selection is provided here.
extern "C" bool pioneer_q38_rgdn_native_compile(PioneerProbeCookie* cookie,unsigned operation,tt_q38_rgdn_root_request& job)
{
	PioneerQ38GdnCallScope scope(cookie); auto* c=scope.Context();
	if (!c || !c->q38ResidentGdnRoot || !c->q38GdnBanks || !c->q38StateZero) return false;
	if (!operation) return PioneerQ38RgdnRootPrepare(c->io,c->baseline.baseline.baseline.baseline.dram,
		c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->q38GdnIdentity,c->host,c->lease,
		c->bulk,c->bulkHost,c->wideHost,*c->q38ResidentGdnRoot);
	if (operation==1) return PioneerQ38RgdnRootRun(c->io,c->baseline.baseline.baseline.baseline.dram,
		c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,
		c->wideHost,*c->q38ResidentGdnRoot,job);
	return PioneerQ38RgdnRootClose(c->io,c->baseline.baseline.baseline.baseline.dram,
		c->baseline.baseline.baseline.noc,*c->q38GdnBanks,c->banks,c->host,c->lease,c->bulk,c->bulkHost,
		c->wideHost,*c->q38ResidentGdnRoot,operation==2);
}
#endif
