/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_PROJECTION_SHAPE_H
#define PIONEER_QWEN35_PROJECTION_SHAPE_H

namespace PioneerQwen35Projection {
enum : unsigned { Magic=0x31503551, Fault=0x46503551, MaxInputs=9216 };

inline bool Supports(unsigned type, unsigned inputs)
{
	switch (inputs) {
		case 2560: return type==8 || type==12 || type==13 || type==14;
		case 4096: return type==12 || type==13;
		case 9216: return type==12 || type==14;
		default: return false;
	}
}

inline unsigned TileBytes(unsigned type)
{
	switch (type) {
		case 8: return 576;
		case 12: return 2304;
		case 13: return 2816;
		case 14: return 3392;
		default: return 0;
	}
}
}
#endif
