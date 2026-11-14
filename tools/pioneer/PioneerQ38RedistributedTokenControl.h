/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_REDISTRIBUTED_TOKEN_CONTROL_H
#define PIONEER_Q38_REDISTRIBUTED_TOKEN_CONTROL_H
#include <stdint.h>
namespace PioneerQ38RedistributedTokenControl {
enum {Magic=0x31434752,Words=16,Bytes=Words*4};
// Binds the mode selector to the active recurrent frame and token identity.
// This is metadata validation only; it does not authorize dispatch or Work.
static inline bool Shape(const uint32_t* control,const uint32_t* outer){
	if(!control || !outer || control[0]!=Magic || control[1]!=1 || control[2]>1
		|| control[3]!=outer[3] || control[4]!=outer[4] || control[5]!=outer[7]
		|| control[6]!=outer[6]+5 || control[7]!=outer[48])return false;
	for(unsigned i=0;i<8;i++)if(control[8+i]!=outer[32+i])return false;
	return true;
}
}
#endif
