/* SPDX-License-Identifier: MIT */
// Exercise the actual production field helpers with/without optional output.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
using uint8 = uint8_t;
using int16 = int16_t;
using int32 = int32_t;
using type_code = uint32_t;
static void* _Align(void* address, int32 offset = 0)
{
	return (void*)((uintptr_t(address) + offset + 3) & ~uintptr_t(3));
}
struct KMessage { struct FieldValueHeader; struct FieldHeader; };
#include "kmessage_fields.inc"

int main()
{
	alignas(8) uint8 bytes[256] = {};
	auto* field = reinterpret_cast<KMessage::FieldHeader*>(bytes);
	field->headerSize = 24;
	field->elementSize = 4;
	field->elementCount = 2;
	int32 size = -1;
	assert(field->ElementAt(0, nullptr) == bytes + 24);
	assert(field->ElementAt(1, &size) == bytes + 28 && size == 4);
	assert(field->ElementAt(-1, nullptr) == nullptr);
	assert(field->ElementAt(2, nullptr) == nullptr);
	field->elementSize = -1;
	auto* first = reinterpret_cast<KMessage::FieldValueHeader*>(bytes + 24);
	first->size = 3;
	auto* second = first->NextFieldValueHeader();
	second->size = 5;
	assert(field->ElementAt(0, nullptr) == first->Data());
	assert(field->ElementAt(1, nullptr) == second->Data());
	assert(field->ElementAt(0, &size) == first->Data() && size == 3);
	assert(field->ElementAt(1, &size) == second->Data() && size == 5);
	std::puts("production KMessage optional element-size output: PASS");
}
