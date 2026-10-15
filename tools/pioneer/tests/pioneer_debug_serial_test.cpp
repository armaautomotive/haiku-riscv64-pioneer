/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstddef>
#include <cstring>

struct FakeUART {
	int result = -1;
	bool expectedWait = false;
	unsigned calls = 0;
	int GetChar(bool wait)
	{
		assert(wait == expectedWait);
		calls++;
		return result;
	}
};
static FakeUART* sArchDebugUART;
#include "pioneer_debug_serial.inc"

enum { INIT, CMDREAD, CKSUM1 };
static char sCommand[512];
static int sCommandIndex;
static unsigned naks;
static void gdb_nak() { naks++; }
#include "pioneer_gdb_buffer.inc"

int main()
{
	assert(arch_debug_serial_try_getchar() == -1);
	FakeUART uart;
	sArchDebugUART = &uart;
	assert(arch_debug_serial_try_getchar() == -1);
	uart.result = 255;
	assert(arch_debug_serial_try_getchar() == 255);
	uart.result = 'x';
	uart.expectedWait = true;
	assert(arch_debug_serial_getchar() == 'x');
	assert(uart.calls == 3);

	assert(gdb_init_handler('$') == CMDREAD);
	for (int i = 0; i < 511; i++)
		assert(gdb_cmdread_handler('m') == CMDREAD);
	assert(sCommandIndex == 511 && sCommand[511] == 0);
	assert(gdb_cmdread_handler('#') == CKSUM1);
	assert(gdb_cmdread_handler('x') == INIT);
	assert(sCommandIndex == 511 && sCommand[511] == 0 && naks == 1);
	assert(gdb_init_handler('$') == CMDREAD);
	assert(sCommandIndex == 0 && sCommand[0] == 0);
	for (const char* p = "mffffffc000898f80,80"; *p; p++)
		assert(gdb_cmdread_handler(*p) == CMDREAD);
	assert(!strcmp(sCommand, "mffffffc000898f80,80"));
	assert(gdb_cmdread_handler('#') == CKSUM1);
}
