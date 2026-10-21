/* SPDX-License-Identifier: MIT */
// DEBUG qualification of WORK ioctl ONLY. This client deliberately runs
// CPU references to qualify the work path;it is NOT the Work inference
// executable. Production Work never includes this file/reference code.
#include <Drivers.h>
#include "PioneerQ38WorkSession.h"
#ifdef PIONEER_Q38_ROOT_CYCLE_CLIENT
#include "../../src/add-ons/kernel/drivers/misc/PioneerQ38RootCycleData.h"
#endif
#define PIONEER_Q38_DISPATCH_CLIENT
#define PIONEER_Q38_DISPATCH_SUPPORT_VERSION TT_Q38_WORK_SUPPORT_VERSION
#undef TT_PCI_PROBE_Q38_DISPATCH_SUPPORT
#undef TT_PCI_PROBE_Q38_DISPATCH_RUN
#define TT_PCI_PROBE_Q38_DISPATCH_SUPPORT TT_PCI_PROBE_Q38_WORK_SUPPORT
#define TT_PCI_PROBE_Q38_DISPATCH_RUN TT_PCI_PROBE_Q38_WORK_RUN
#define PioneerQ38DispatchHistory PioneerQ38WorkHistory
#ifndef PIONEER_Q38_WORK_PROBE_MAIN
#define PIONEER_Q38_WORK_PROBE_MAIN main
#endif
#define main WorkQualificationMain
#include "pioneer_q38_cohort_probe.cpp"
#undef main
#undef PioneerQ38DispatchHistory
int PIONEER_Q38_WORK_PROBE_MAIN(int argc,char** argv) {
	puts("WORK_IOCTL_DEBUG_QUALIFICATION;CPU_REFERENCE_IN_THIS_TEST_CLIENT_ONLY;NOT_PRODUCTION_WORK_MODE");
	return WorkQualificationMain(argc,argv);
}
