/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_H
#define PIONEER_QWEN35_PROJECTION_H

#include "PioneerQwen35Q5Panel.h"
#include "../../src/add-ons/kernel/drivers/misc/PioneerQwen35ProjectionShape.h"

namespace PioneerQwen35Projection {
inline bool Unpack(unsigned type, const uint8_t* tile, unsigned bytes,
	unsigned panel, PioneerQ38Panel* output)
{
	return type==13 ? PioneerQwen35Q5Panel(tile, bytes, panel, output)
		: PioneerQ38UnpackPanel(type, tile, bytes, panel, output);
}
}
#endif
