/*
 * Copyright 2021, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <sys/cdefs.h>

#include <OS.h>
#include <SupportDefs.h>

#include <util/Random.h>


extern "C" {

long __stack_chk_guard;


void
__stack_chk_fail()
{
	panic("stack smashing detected\n");
}


}


void
stack_protector_init()
{
#if defined(__riscv)
	// secure_random_value() begins with an AMO.  The Pioneer reaches this
	// point before its scheduler is running, while bootstrap AMOs are not yet
	// reliable.  Mix the clocks and the relocated guard address for an early
	// boot canary without touching the shared random generator.
	uint64 entropy = (uint64)system_time();
	entropy ^= (uint64)real_time_clock() << 32;
	entropy ^= (uint64)(addr_t)&__stack_chk_guard;
	entropy ^= entropy << 13;
	entropy ^= entropy >> 7;
	entropy ^= entropy << 17;
	if (entropy == 0)
		entropy = 0x6a09e667f3bcc909ULL;
	__stack_chk_guard = (long)entropy;
#else
	__stack_chk_guard = secure_get_random<long>();
#endif
}
