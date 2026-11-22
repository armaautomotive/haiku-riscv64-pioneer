/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen35DataflowAffine.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <new>

static uint32_t bits(float value)
{
	uint32_t result;
	std::memcpy(&result,&value,sizeof(result));
	return result;
}

int main()
{
	using namespace PioneerQwen35;
	uint8_t tile[16*176]{};
	for (unsigned row=0;row<16;row++) {
		tile[16+row]=0x3c; // d = 1
		tile[48+row]=0x40; // dmin = 2
		tile[64+row]=uint8_t(row+1);
		tile[128+row]=uint8_t(row+17);
	}
	alignas(PioneerQ38Panel) uint8_t scratch[sizeof(PioneerQ38Panel)]{};
	auto* panel=new (scratch) PioneerQ38Panel{};
	assert(PioneerQwen35DataflowAffineOnly(13,tile,sizeof(tile),0,panel));
	const auto* coefficients=reinterpret_cast<const uint32_t*>(scratch
		+(DataflowAffineCoefficientAddress-DataflowAffinePanelScratchAddress));
	const auto* minima=reinterpret_cast<const uint32_t*>(scratch
		+(DataflowAffineMinimumAddress-DataflowAffinePanelScratchAddress));
	for (unsigned row=0;row<16;row++) {
		assert(coefficients[row]==bits(float(row+1)));
		assert(minima[row]==bits(-2.0f*float(row+17)));
	}
}
