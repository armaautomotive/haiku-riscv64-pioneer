/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdint>
#include <cstring>
#include <initializer_list>
using int32 = int32_t;
using status_t = int;
constexpr int B_OK = 0, B_HANDLED_INTERRUPT = 1, B_ONE_SHOT_RELATIVE_TIMER = 2;
struct timer {};
struct { unsigned num_cpus; struct { struct { uint64_t size; } physMap; } arch_args; } sKernelArgs;
static unsigned armed, debugCalls;
static unsigned readyMarkers, callbackMarkers;
static int addStatus;
static int32 (*savedHook)(timer*);
static int add_timer(timer*, int32 (*hook)(timer*), int64_t delay, int flags)
{
	assert(delay == 120000000 && flags == B_ONE_SHOT_RELATIVE_TIMER);
	armed++; savedHook = hook; return addStatus;
}
static void kernel_debugger(const char* message)
{
	assert(!strcmp(message, "Pioneer opt-in diagnostic: 120-second AP timer"));
	debugCalls++;
}
static void debug_early_boot_checkpoint(const char* message)
{
	if (!strcmp(message, "PIONEER-KDL-TIMER: AP1 armed before scheduler release\n"))
		readyMarkers++;
	else {
		assert(!strcmp(message, "PIONEER-KDL-TIMER: callback entered\n"));
		callbackMarkers++;
	}
}
struct Stop {};
[[noreturn]] static void panic(const char*) { throw Stop{}; }
#include "pioneer_kdl_timer.inc"
int main()
{
	sKernelArgs.num_cpus = 64;
	sKernelArgs.arch_args.physMap.size = UINT64_C(137000000000);
	for (int cpu = 0; cpu < 64; cpu++) arm_pioneer_diagnostic_kdl_timer(cpu);
	assert(armed == 1 && debugCalls == 0 && savedHook != nullptr);
	verify_pioneer_diagnostic_kdl_timer();
	assert(readyMarkers == 1 && callbackMarkers == 0);
	assert(savedHook(nullptr) == B_HANDLED_INTERRUPT && debugCalls == 1);
	assert(callbackMarkers == 1);
	for (unsigned count : {1u, 2u, 4u, 8u, 16u, 32u}) {
		sKernelArgs.num_cpus = count; arm_pioneer_diagnostic_kdl_timer(1);
	}
	assert(armed == 1);
	sKernelArgs.num_cpus = 64;
	for (uint64_t size : {UINT64_C(0), UINT64_C(8500535296), UINT64_C(128849018880)}) {
		sKernelArgs.arch_args.physMap.size = size; arm_pioneer_diagnostic_kdl_timer(1);
	}
	assert(armed == 1);
	sKernelArgs.arch_args.physMap.size = UINT64_C(137000000000);
	addStatus = -1;
	sPioneerDiagnosticKDLTimerArmed = false;
	try { arm_pioneer_diagnostic_kdl_timer(1); assert(false); }
	catch (const Stop&) {}
	assert(armed == 2 && debugCalls == 1);
	try { verify_pioneer_diagnostic_kdl_timer(); assert(false); }
	catch (const Stop&) {}
	assert(readyMarkers == 1 && callbackMarkers == 1);
}
