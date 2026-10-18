/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_BANK_PROTOCOL_H
#define PIONEER_Q38_BANK_PROTOCOL_H
#include "PioneerBulkUpload.h"
// Separate opt-in experimental ABI; an old driver cannot accidentally
// accept a seven-bank operation as a legacy bank-zero upload/decoder job.
#define TT_PCI_PROBE_Q38_BANK_SUPPORT (B_DEVICE_OP_CODES_END + 0x54e5)
#define TT_PCI_PROBE_Q38_BANK_BEGIN (B_DEVICE_OP_CODES_END + 0x54e6)
#define TT_PCI_PROBE_Q38_BANK_END (B_DEVICE_OP_CODES_END + 0x54e7)
#define TT_PCI_PROBE_Q38_BANK_STATUS (B_DEVICE_OP_CODES_END + 0x54e8)
#define TT_PCI_PROBE_Q38_BANK_UPLOAD (B_DEVICE_OP_CODES_END + 0x54e9)
#define TT_PCI_PROBE_Q38_BANK_READ (B_DEVICE_OP_CODES_END + 0x54ea)
#define TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD (B_DEVICE_OP_CODES_END + 0x54eb)
enum { TT_Q38_BANK_DRIVER=45,TT_Q38_BANK_PROTOCOL=1 };
// Shape only, NOT route/ownership authority. Keep the ordinary bank-zero
// <=2GiB capacity policy unchanged; the private banks owner authorizes3GiB.
static inline bool PioneerQ38BankBulkRange(unsigned bank,uint64_t offset,unsigned pages)
{
	return bank<7 && pages>=1 && pages<=4 && !(offset&4095)
		&& offset<PioneerResidentBanks::Capacity && uint64_t(pages)*4096<=PioneerResidentBanks::Capacity-offset;
}
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
// Inline ordinary bytes, NEVER a user/physical pointer. One owned 16KiB
// host staging buffer serves at most four contiguous pages in ONE bank.
struct tt_q38_bank_bulk {
	uint32_t version,bank,pages,reserved;
	uint64_t offset;
	tt_bulk_upload_result result;
	uint32_t data[4096];
};
#endif
