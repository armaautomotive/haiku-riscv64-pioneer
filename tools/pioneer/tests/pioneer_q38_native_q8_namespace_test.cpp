/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstdio>
#define B_DEVICE_OP_CODES_END 0x1000
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38WorkProtocol.h"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38SpanProtocol.h"
int main() {
	static_assert(TT_PCI_PROBE_Q38_LEGACY_WORK_SUPPORT==0x70f2 && TT_PCI_PROBE_Q38_LEGACY_WORK_RUN==0x70f3,"legacy operations stable");
#ifdef PIONEER_Q38_EXPECT_ROOT_LIFETIME_NAMESPACE
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==7 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x7290 && TT_PCI_PROBE_Q38_WORK_RUN==0x7291,"new ROOT execution namespace bound");
#elif defined(PIONEER_Q38_EXPECT_PEER_LIFETIME_NAMESPACE)
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==6 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x7280 && TT_PCI_PROBE_Q38_WORK_RUN==0x7281,"new worker execution namespace bound");
#elif defined(PIONEER_Q38_EXPECT_CODE_NAMESPACE)
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==5 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x7270 && TT_PCI_PROBE_Q38_WORK_RUN==0x7271,"held-only code namespace bound");
#elif defined(PIONEER_Q38_EXPECT_PHASE_NAMESPACE)
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==4 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x7260 && TT_PCI_PROBE_Q38_WORK_RUN==0x7261,"private timing DATA namespace bound");
#elif defined(PIONEER_Q38_EXPECT_SM7_NAMESPACE)
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==3 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x7250 && TT_PCI_PROBE_Q38_WORK_RUN==0x7251,"SM7 namespace bound");
#else
	static_assert(TT_Q38_WORK_SUPPORT_VERSION==2 && TT_PCI_PROBE_Q38_WORK_SUPPORT==0x70f2 && TT_PCI_PROBE_Q38_WORK_RUN==0x70f3,"ordinary Work remains TC");
#endif
	static_assert(TT_PCI_PROBE_Q38_WORK_SUPPORT!=TT_PCI_PROBE_Q38_SPAN_SUPPORT && TT_PCI_PROBE_Q38_WORK_RUN!=TT_PCI_PROBE_Q38_SPAN_UPLOAD,"no existing span collision");
	puts("PASS ACTUAL Work protocol namespace:ordinary TC ABI2 unchanged,SM7 ABI3/private timing DATA ABI4/held-only code ABI5 use DISTINCT support/RUN operations,span namespace unchanged;NO IO");
}
