/* SPDX-License-Identifier: MIT */
#ifdef PIONEER_Q38_TOKEN_NATIVE_CONTEXT
#include "PioneerQ38TokenNativeRole.h"
private:
 friend class PioneerQ38TokenPreparationAudit;
 struct TokenPreparationAuditRecord {
  const void *context=nullptr,*state=nullptr,*dispatch=nullptr,*fd=nullptr,*hashes=nullptr;
  const uint8_t* ledger=nullptr;PioneerQ38GdnIdentity identity={};
  uint64_t generation=0,key=0;int64_t thread=-1,team=-1;unsigned worker=120;
 };
 TokenPreparationAuditRecord q38TokenPreparationAudit;
public:
#endif
