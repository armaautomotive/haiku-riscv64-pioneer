/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_FREESTANDING_MEMCPY_H
#define PIONEER_Q38_FREESTANDING_MEMCPY_H

/* Used only by the inspection-only -Os ROOT TU; avoid a libc dependency and
 * prevent the compiler from folding this byte loop into recursive memcpy. */

extern "C" __attribute__((weak)) void* memcpy(void* destination, const void* source,
	__SIZE_TYPE__ bytes)
{
	volatile unsigned char* out = static_cast<volatile unsigned char*>(destination);
	const volatile unsigned char* in = static_cast<const volatile unsigned char*>(source);
	while (bytes--)
		*out++ = *in++;
	return destination;
}

#endif
