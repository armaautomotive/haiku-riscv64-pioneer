/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_RMS_ACTUAL_TEST_SOURCE_H
#define PIONEER_Q38_RMS_ACTUAL_TEST_SOURCE_H
#ifdef _KERNEL_MODE
#error Offline original-model test oracle only;never driver code
#endif
#include <array>
#include <cstdint>
// Test-only opaque host oracle in a SEPARATE translation unit. Native test
// receives ORIGINAL weight bytes only;CPU reference vectors only observe.
struct RmsActualSource {
 struct Original;Original* original=nullptr;
 std::array<uint8_t,46080> embedding;
 std::array<uint32_t,5120> norm,rawReference,normalizedReference;
 RmsActualSource();~RmsActualSource();
 RmsActualSource(const RmsActualSource&)=delete;RmsActualSource& operator=(const RmsActualSource&)=delete;
 bool Open(const char*);bool Ready()const;bool Token(unsigned);
};
#endif
