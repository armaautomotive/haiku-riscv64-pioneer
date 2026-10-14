#include <cassert>
#include "../../../src/add-ons/kernel/busses/pci/ecam/Sg2042RootPolicy.h"

int main()
{
	const uint8_t magic[] = {0x1e, 0x52, 0xb1, 0x40};
	const uint8_t swapped[] = {0x40, 0xb1, 0x52, 0x1e};
	assert(Sg2042BlackholeRootRequested(0x80, 0xbf, magic, sizeof(magic)));
	assert(!Sg2042BlackholeRootRequested(0x80, 0xbf, NULL, 4));
	for (unsigned length = 0; length < 9; length++) {
		if (length != 4)
			assert(!Sg2042BlackholeRootRequested(0x80, 0xbf, magic, length));
	}
	assert(!Sg2042BlackholeRootRequested(0x80, 0xbf, swapped, 4));
	for (unsigned bus = 0; bus < 256; bus++) {
		if (bus != 0x80)
			assert(!Sg2042BlackholeRootRequested(bus, 0xbf, magic, 4));
	}
	assert(!Sg2042BlackholeRootRequested(0x80, 0xff, magic, 4));
	assert(!Sg2042BlackholeRootRequested(0x180, 0xbf, magic, 4));
	assert(!Sg2042BlackholeRootRequested(0x80, 0x1bf, magic, 4));
	assert(Sg2042BlackholeRootRegistersMatch(0x7062000000, 0x4800000000, 0x1000));
	assert(!Sg2042BlackholeRootRegistersMatch(0x7062800000, 0x4800000000, 0x1000));
	assert(!Sg2042BlackholeRootRegistersMatch(0x7062000000, 0x4c00000000, 0x1000));
	assert(!Sg2042BlackholeRootRegistersMatch(0x7062000000, 0x4800000000, 0x2000));
}
