/* SPDX-License-Identifier: MIT */
#pragma once
#include <cstdint>
// HOST observation sink only. No reset,total/ready return,processor/native
// owner input,product,clock or execution control is exposed to the fixture.
void Q38NativeRootProfileRetired(uint32_t);
void Q38NativeRootProfileReport(uint64_t);
