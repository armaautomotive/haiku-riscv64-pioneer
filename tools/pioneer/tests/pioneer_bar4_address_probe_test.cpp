/* Distributed under the MIT License. */
#include <cassert>
#include <cstring>
#include <initializer_list>
#include "../../../src/add-ons/kernel/drivers/misc/BlackholeBar4Probe.h"

struct MockConfig {
	uint32_t id = 0xb1401e52;
	uint32_t subsystem = 0x00431e52;
	uint32_t command = 0;
	uint32_t bars[6] = {0x0c, 0, 0x0c, 0, 0x0c, 0};
	uint64_t sizes[3] = {0x20000000, 0x100000, UINT64_C(0x800000000)};
	uint32_t highMask = 0xfffffff8;
	unsigned writes = 0;
	unsigned commandReads = 0;
	bool interrupt = false;
	bool inaccessible = false;
	bool ignoreRestore = false;

	uint32_t Read(unsigned offset, unsigned width)
	{
		if (offset == 0) { assert(width == 4); return id; }
		if (offset == 0x2c) { assert(width == 4); return subsystem; }
		if (offset == 4) {
			assert(width == 2);
			if (++commandReads == 3 && interrupt)
				command = inaccessible ? 0xffff : 2;
			return command;
		}
		assert(width == 4 && offset >= 0x10 && offset <= 0x24);
		assert(offset % 4 == 0);
		return bars[(offset - 0x10) / 4];
	}
	void Write(unsigned offset, unsigned width, uint32_t value)
	{
		writes++;
		if (offset == 4) {
			assert(width == 2 && value == 0); // No enabling command exists.
			if (!inaccessible)
				command = 0;
			return;
		}
		assert(command == 0); // No BAR writes with any decoding enabled.
		assert(width == 4 && (offset == 0x20 || offset == 0x24));
		if (offset == 0x24 && value == 0 && ignoreRestore)
			return;
		bars[(offset - 0x10) / 4] = offset == 0x24 ? value & highMask : value;
	}
	uint64_t Size(unsigned bar) { assert(bar == 0 || bar == 2 || bar == 4); return sizes[bar / 2]; }
};

static void
ExpectRefusal(MockConfig io)
{
	tt_bar4_probe_result result;
	BlackholeBar4AddressProbe(io, result);
	assert(result.version == 1 && result.state == TT_BAR4_REFUSED);
	assert(io.writes == 0);
}

int main()
{
	for (uint32_t mask : {0U, 0x78U, 0xf8U, 0xfff8U, 0xfffffff8U}) {
		MockConfig io;
		io.highMask = mask;
		for (unsigned repetition = 0; repetition < 3; repetition++) {
			tt_bar4_probe_result result;
			BlackholeBar4AddressProbe(io, result);
			assert(result.state == TT_BAR4_RESTORED && io.command == 0);
			assert(result.trial[0] == 0x0c && result.trial[1] == mask);
			assert(memcmp(result.before, result.after, sizeof(result.before)) == 0);
		}
		assert(io.writes == 12);
	}
	MockConfig bad;
	bad.id ^= 1; ExpectRefusal(bad);
	bad = {}; bad.subsystem ^= 1; ExpectRefusal(bad);
	for (uint32_t command : {1U, 2U, 4U, 6U, 0x400U, 0xffffU}) {
		bad = {}; bad.command = command; ExpectRefusal(bad);
	}
	for (unsigned bar = 0; bar < 6; bar++) {
		bad = {}; bad.bars[bar] ^= 0x10; ExpectRefusal(bad);
	}
	for (unsigned i = 0; i < 3; i++) {
		bad = {}; bad.sizes[i] = 0; ExpectRefusal(bad);
	}
	bad = {}; bad.interrupt = true;
	tt_bar4_probe_result result;
	BlackholeBar4AddressProbe(bad, result);
	assert(result.state == TT_BAR4_INTERRUPTED && bad.command == 0);
	assert(memcmp(result.before, result.after, sizeof(result.before)) == 0);
	bad = {}; bad.interrupt = true; bad.inaccessible = true;
	BlackholeBar4AddressProbe(bad, result);
	assert(result.state == TT_BAR4_RESTORE_FAILED && bad.writes == 3);
	bad = {}; bad.ignoreRestore = true;
	BlackholeBar4AddressProbe(bad, result);
	assert(result.state == TT_BAR4_RESTORE_FAILED && bad.command == 0);
}
