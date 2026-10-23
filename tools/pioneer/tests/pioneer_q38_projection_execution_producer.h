/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PROJECTION_EXECUTION_PRODUCER_H
#define PIONEER_Q38_PROJECTION_EXECUTION_PRODUCER_H
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38EmbeddingNativeState.h"
#include <vector>
// TEST ONLY separate-TU bridge retaining the ACTUAL native factory/state
// and actual emitted zero/QPE1/QRN1. Never manufactures a completion seal.
class ProjectionExecutionProducer {
 struct Impl;Impl* implementation;
public:
 ProjectionExecutionProducer(const char* zero,const char* embedding,const char* rms);
 ~ProjectionExecutionProducer();
 ProjectionExecutionProducer(const ProjectionExecutionProducer&)=delete;
 PioneerQ38EmbeddingNativeState& Model();
 const tt_dram_samples_result& Proof();
 PioneerResidentBanks& Banks();
 const uint8_t* Ledger();
 std::vector<uint32_t>& DeviceWords();
 bool Valid()const;void Retain();
 void RootControls(uint32_t* source,uint32_t* command,uint32_t* counts);
};
#endif
