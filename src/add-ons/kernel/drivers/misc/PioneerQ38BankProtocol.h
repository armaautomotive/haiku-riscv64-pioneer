/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BANK_PROTOCOL_H
#define PIONEER_Q38_BANK_PROTOCOL_H
#include "PioneerResidentStore.h"
// Separate opt-in experimental ABI; an old driver cannot accidentally
// accept a seven-bank operation as a legacy bank-zero upload/decoder job.
#define TT_PCI_PROBE_Q38_BANK_SUPPORT (B_DEVICE_OP_CODES_END + 0x54e5)
#define TT_PCI_PROBE_Q38_BANK_BEGIN (B_DEVICE_OP_CODES_END + 0x54e6)
#define TT_PCI_PROBE_Q38_BANK_END (B_DEVICE_OP_CODES_END + 0x54e7)
#define TT_PCI_PROBE_Q38_BANK_STATUS (B_DEVICE_OP_CODES_END + 0x54e8)
#define TT_PCI_PROBE_Q38_BANK_UPLOAD (B_DEVICE_OP_CODES_END + 0x54e9)
#define TT_PCI_PROBE_Q38_BANK_READ (B_DEVICE_OP_CODES_END + 0x54ea)
enum { TT_Q38_BANK_DRIVER=44,TT_Q38_BANK_PROTOCOL=1 };
struct tt_q38_bank_support {
	uint32_t driver,version,banks,page_bytes,max_pages,storage_only;
	uint64_t bytes_per_bank;
};
struct tt_q38_bank_session {
	uint32_t version,reserved;
	tt_resident_session_result result;
};
struct tt_q38_bank_page {
	uint32_t version,bank;
	uint64_t offset;
	tt_resident_store_result result;
	uint32_t data[1024];
};
#endif
