/* SPDX-License-Identifier: MIT */
#include <Drivers.h>
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentOutputValidationProtocol.h"
extern "C" bool pioneer_output_validation_request_shape(const tt_q38_resident_output_validation& heap){
 return PioneerQ38ResidentOutputValidationRequest(heap);
}
