/* Distributed under the MIT License. */
#include <cassert>
#include <cstdint>
#include <cstdio>
using status_t=int;constexpr int B_OK=0,B_NOT_ALLOWED=-1,B_ERROR=-2;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciRootLeaseScope.h"
struct Owner {unsigned cold=0;void Quarantine(){++cold;}};
static unsigned starts=0,ends=0;static int beginMode=0,endMode=0;
static status_t Begin(void* cookie,uint64_t* ticket) {
	assert(cookie==(void*)1);++starts;*ticket=beginMode==1?0:17;
	return beginMode==2?B_ERROR:B_OK;
}
static status_t End(void* cookie,uint64_t ticket) {
	assert(cookie==(void*)1 && ticket==17);++ends;return endMode?B_ERROR:B_OK;
}
struct Service {status_t (*begin)(void*,uint64_t*);status_t (*end)(void*,uint64_t);};
static void EarlyReturn(Owner& owner,Service& service) {
	PioneerPciRootLeaseScope<Owner,Service> scope(owner);
	assert(scope.Begin(&service,(void*)1));
}
int main() {
	Service service{Begin,End};Owner owner;
	EarlyReturn(owner,service);assert(starts==1 && ends==1 && !owner.cold);
	{
		PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(!scope.Begin(nullptr,(void*)1) && scope.Begin(&service,(void*)1));
		assert(scope.Active() && !scope.Begin(&service,(void*)1));
		assert(scope.Close()==B_OK && !scope.Active() && scope.Close()==B_NOT_ALLOWED);
	}
	assert(starts==2 && ends==2 && !owner.cold);
	beginMode=1;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);assert(!scope.Begin(&service,(void*)1));}
	assert(owner.cold==1 && ends==2);
	beginMode=2;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);assert(!scope.Begin(&service,(void*)1));}
	assert(owner.cold==2 && ends==2);
	beginMode=0;endMode=1;
	{PioneerPciRootLeaseScope<Owner,Service> scope(owner);assert(scope.Begin(&service,(void*)1));
		assert(scope.Close()==B_ERROR && scope.Close()==B_NOT_ALLOWED);}
	assert(owner.cold==3 && ends==3);
	{PioneerPciRootLeaseScope<Owner,Service> scope(owner);assert(scope.Begin(&service,(void*)1));}
	assert(owner.cold==4 && ends==4);
	std::puts("PASS root PCI lease scope: all exits release once and ambiguous results quarantine; SIMULATED ONLY");
}
