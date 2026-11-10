/* SPDX-License-Identifier: MIT */
// Missing/private-directory refusals only; no model or device access.
#define PIONEER_Q38_GDN_MUTABLE_STATE 1
#define PIONEER_BANKS_MOCK_ONLY 1
#include "../PioneerQ38HybridDenseQ8VerifiedModel.h"
#include <cassert>
#include <string>
int main(){
 PioneerQ38HybridDenseQ8VerifiedModel model;std::string error;
 assert(!model.Ready() && !model.PreparedPages() && !model.Canonical());
 assert(!model.Prepare("/not-a-cache-4","/not-a-cache-8","/not-a-cache-head","/not-a-cache-dense",error));
 assert(!error.empty() && !model.Ready() && !model.PreparedPages() && !model.Canonical());
 error.clear();assert(!model.Prepare("/not-a-cache-4","/not-a-cache-8","/not-a-cache-head","/not-a-cache-dense",error));
 assert(!error.empty());
}
