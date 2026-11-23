/* Distributed under the MIT License. */
#include <cassert>
#include <cstdio>
using status_t=int;constexpr int B_OK=0,B_NOT_ALLOWED=-1,B_ERROR=-2;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerPciRootLeaseScope.h"
struct Owner {unsigned cold=0;void Quarantine(){++cold;}};
using Operation=status_t (*)(void*);
struct Service {status_t (*run)(void*,Operation,void*,status_t*);};
static int mode=0,calls=0;
static status_t Run(void* cookie,Operation operation,void* arg,status_t* result) {
	assert(cookie==(void*)1);++calls;
	if (mode==1) return B_NOT_ALLOWED;
	if (mode!=2) *result=operation(arg);
	if (mode==3) return B_ERROR;
	if (mode==4) (void)operation(arg);
	return B_OK;
}
static status_t OperationFails(void* arg) {++*static_cast<int*>(arg);return B_ERROR;}
int main() {
	Service service{Run};Owner owner;int count=0;status_t result=0;
	{PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(scope.Run(nullptr,(void*)1,OperationFails,&count,&result)==B_NOT_ALLOWED);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_OK);
		assert(result==B_ERROR && count==1 && owner.cold==0);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_NOT_ALLOWED);}
	mode=1;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_NOT_ALLOWED);}
	assert(owner.cold==0 && count==1);
	mode=2;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_ERROR);}
	assert(owner.cold==1 && count==1);
	mode=3;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_ERROR);}
	assert(owner.cold==2 && count==2);
	mode=4;{PioneerPciRootLeaseScope<Owner,Service> scope(owner);
		assert(scope.Run(&service,(void*)1,OperationFails,&count,&result)==B_ERROR);}
	assert(owner.cold==3 && count==3 && calls==5);
	std::puts("PASS synchronous PCI recovery scope: separate operation status and ambiguous release quarantine; SIMULATED ONLY");
}
