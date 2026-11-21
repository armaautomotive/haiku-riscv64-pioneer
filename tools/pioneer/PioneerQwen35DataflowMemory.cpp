/* SPDX-License-Identifier: MIT */
#include <stddef.h>
#include <stdint.h>

using PioneerDataflowWord __attribute__((__may_alias__))=uint32_t;

extern "C" __attribute__((noinline)) void* memcpy(void* destination,
	const void* source,size_t bytes) noexcept
{
	volatile uint8_t* output=static_cast<volatile uint8_t*>(destination);
	const volatile uint8_t* input=static_cast<const volatile uint8_t*>(source);
	if (!((reinterpret_cast<uintptr_t>(output)
		|reinterpret_cast<uintptr_t>(input))&3)) {
		volatile PioneerDataflowWord* wordOutput=
			reinterpret_cast<volatile PioneerDataflowWord*>(output);
		const volatile PioneerDataflowWord* wordInput=
			reinterpret_cast<const volatile PioneerDataflowWord*>(input);
		while (bytes>=4) { *wordOutput++=*wordInput++; bytes-=4; }
		output=reinterpret_cast<volatile uint8_t*>(wordOutput);
		input=reinterpret_cast<const volatile uint8_t*>(wordInput);
	}
	while (bytes--) *output++=*input++;
	return destination;
}

extern "C" __attribute__((noinline)) void* memset(void* destination,
	int value,size_t bytes) noexcept
{
	volatile uint8_t* output=static_cast<volatile uint8_t*>(destination);
	const uint8_t byte=uint8_t(value);
	while (bytes && (reinterpret_cast<uintptr_t>(output)&3)) {
		*output++=byte;
		bytes--;
	}
	volatile PioneerDataflowWord* wordOutput=
		reinterpret_cast<volatile PioneerDataflowWord*>(output);
	const PioneerDataflowWord word=PioneerDataflowWord(byte)*0x01010101U;
	while (bytes>=4) { *wordOutput++=word; bytes-=4; }
	output=reinterpret_cast<volatile uint8_t*>(wordOutput);
	while (bytes--) *output++=byte;
	return destination;
}
