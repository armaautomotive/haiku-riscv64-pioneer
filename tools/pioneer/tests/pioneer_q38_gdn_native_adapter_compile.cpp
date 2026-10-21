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
