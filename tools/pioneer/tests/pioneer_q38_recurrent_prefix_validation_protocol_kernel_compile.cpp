/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixValidationProtocol.h"
extern "C" bool pioneer_prefix_validation_request_data(const tt_q38_recurrent_prefix_validation& value){
 return PioneerQ38RecurrentPrefixValidationRequest(value);
} // DATA ABI only;NO source,context,driver control,device operations or launch
