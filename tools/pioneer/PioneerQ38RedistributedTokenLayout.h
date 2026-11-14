/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_REDISTRIBUTED_TOKEN_LAYOUT_H
#define PIONEER_Q38_REDISTRIBUTED_TOKEN_LAYOUT_H
#include "PioneerQ38RecurrentPrefixLayout.h"
#include "PioneerQ38TokenStateLayout.h"
// Separate immutable token-package extension. This occupies existing zeroed
// recurrent-prefix slack and leaves the 512-KiB stride and state table intact.
namespace PioneerQ38RedistributedTokenLayout {
namespace P=PioneerQ38RecurrentPrefixLayout;namespace S=PioneerQ38TokenStateLayout;
enum {Magic=0x31525451,Version=1,HeaderWords=32,RecordWords=192,
	Workers=119,LogicalTasks=96,Offset=P::Bytes,
	TableWords=Workers*RecordWords,Words=HeaderWords+TableWords,Bytes=Words*4,
	End=Offset+Bytes};
static_assert(!(Offset&63) && !(Bytes&63) && Offset>=P::GdnOffset+P::GdnBytes
	&& End<=S::Offset,"redistributed GDN table fits existing package slack before current-state spans");
static_assert(S::Offset+S::Bytes<=S::C::Stride,"candidate table does not move the immutable token state record");
}
#endif
