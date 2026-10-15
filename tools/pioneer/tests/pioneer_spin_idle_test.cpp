/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstddef>
#include <initializer_list>
static unsigned cpus, paused, archIdle, moduleIdle;
static unsigned smp_get_num_cpus() { return cpus; }
static void cpu_pause() { paused++; }
static void arch_cpu_idle() { archIdle++; }
static void module_idle() { moduleIdle++; }
struct IdleModule { void (*cpuidle_idle)(); };
static IdleModule* sCPUIdleModule;
#include "pioneer_idle.inc"
int main()
{
	IdleModule module = {module_idle};
	for (unsigned count : {1u, 2u, 4u, 8u, 16u, 32u, 64u}) {
		cpus = count;
		for (bool withModule : {false, true}) {
			sCPUIdleModule = withModule ? &module : nullptr;
			paused = archIdle = moduleIdle = 0;
			cpu_idle();
#ifdef PIONEER_DIAGNOSTIC_SPIN_IDLE
			if (count == 64) {
				assert(paused == 1 && archIdle == 0 && moduleIdle == 0);
				continue;
			}
#endif
			assert(paused == 0);
			assert(archIdle == (withModule ? 0u : 1u));
			assert(moduleIdle == (withModule ? 1u : 0u));
		}
	}
}
