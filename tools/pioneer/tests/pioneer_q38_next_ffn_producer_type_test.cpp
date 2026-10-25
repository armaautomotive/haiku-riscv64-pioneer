/* SPDX-License-Identifier: MIT */
#include <type_traits>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38NextFfnProducer.h"
static_assert(!std::is_default_constructible<PioneerQ38NextFfnProducer>::value,"only SAME actual owner constructs");
static_assert(!std::is_copy_constructible<PioneerQ38NextFfnProducer>::value,"no detached copy");
static_assert(!std::is_move_constructible<PioneerQ38NextFfnProducer>::value,"no moved producer");
static_assert(!std::is_aggregate<PioneerQ38NextFfnProducer>::value,"no aggregate injection");
static_assert(std::is_same<decltype(std::declval<const PioneerQ38NextFfnProducer&>().Current()),bool>::value,"CPU relation query only");
int main(){}
