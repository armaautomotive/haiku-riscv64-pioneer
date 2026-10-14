/* Distributed under the MIT License. */
#ifndef SG2042_ROOT_POLICY_H
#define SG2042_ROOT_POLICY_H

#include <stddef.h>
#include <stdint.h>

// Only a private FDT can opt the recorded Blackhole slot into enumeration.
// This is not permission to assign its BARs or enable endpoint DMA/decoding.
static inline bool
Sg2042BlackholeRootRequested(uint32_t firstBus, uint32_t lastBus,
	const void* property, size_t length)
{
	if (firstBus != 0x80 || lastBus != 0xbf || property == NULL || length != 4)
		return false;
	const uint8_t* bytes = (const uint8_t*)property;
	return bytes[0] == 0x1e && bytes[1] == 0x52
		&& bytes[2] == 0xb1 && bytes[3] == 0x40;
}

static inline bool
Sg2042BlackholeRootRegistersMatch(uint64_t controller, uint64_t config,
	uint64_t configLength)
{
	return controller == UINT64_C(0x7062000000)
		&& config == UINT64_C(0x4800000000) && configLength == 0x1000;
}

#endif
