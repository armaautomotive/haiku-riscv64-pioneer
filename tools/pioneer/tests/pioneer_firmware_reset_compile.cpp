/* SPDX-License-Identifier: MIT */
// Compile the complete experimental template against a transport-shaped
// declaration. This is not a transport, a linked driver or a hardware test.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerFirmwareReset.h"
struct FirmwareResetCompileIO {
	bool RecoveryPermitted();bool PciReady();bool NotificationReady();
	uint32_t TlbRead(unsigned);void TlbWrite(unsigned,uint32_t);
	uint32_t CsmRead(uint32_t);void SetQueueBounds(uint32_t,uint32_t,uint32_t);
	void QueueWrite(uint32_t,uint32_t);uint32_t MsiStatusRead();void NotifyArc();
	void NotifyPreparedArc();void Pause();void RecoveryDelay();
	uint32_t CardRead(uint16_t,uint8_t);uint32_t RootRead(uint16_t,uint8_t);
	void CardWrite(uint16_t,uint8_t,uint32_t);
};
template class PioneerFirmwareReset<FirmwareResetCompileIO>;
