/* SPDX-License-Identifier: MIT */
// Actual complete driver/context/cookie/source/maps/retention/END code.
// Compile-only isolated tag43,NO public selector/install/hardware grant.
#define PIONEER_Q38_RESIDENT_DECODER_NATIVE 1
#define PIONEER_Q38_RESIDENT_DECODER_OWNER 1
#define PIONEER_Q38_DECODER_RUNTIME 1
#define PIONEER_Q38_PROJECTION_RESULT_STASH 1
#define PIONEER_Q38_PROJECTION_EXECUTION_OWNER 1
#define PIONEER_Q38_LEARNED_RMS_NATIVE 1
#define PIONEER_Q38_EMBEDDING_NATIVE 1
#include "../../../src/add-ons/kernel/drivers/misc/tenstorrent_pci_probe.cpp"
extern "C" bool pioneer_decoder_native_bind(PioneerStoreContext& c,unsigned token,unsigned bank){return BindQ38EmbeddingModel(c,token,0,bank);}
extern "C" bool pioneer_decoder_native_history(PioneerStoreContext& c,PioneerQ38ConvHistoryZeroResult& r){return InitializeQ38ConvHistoryModel(c,r);}
extern "C" bool pioneer_decoder_native_embedding(PioneerStoreContext& c,PioneerQ38ProjectionZeroResult& z,PioneerQ38EmbeddingResult& r){return InitializeQ38EmbeddingModel(c,z) && RunQ38EmbeddingModel(c,r);}
extern "C" bool pioneer_decoder_native_rms(PioneerStoreContext& c,PioneerQ38LearnedRmsResult& r){return RunQ38LearnedRmsModel(c,r);}
extern "C" bool pioneer_decoder_native_project(PioneerStoreContext& c,PioneerQ38ProjectionGraphResult& r){return PrepareQ38ProjectionExecution(c,r)
 && RunQ38ProjectionExecution(c,10,r) && RunQ38ProjectionExecution(c,11,r) && RunQ38ProjectionExecution(c,5,r) && RunQ38ProjectionExecution(c,6,r);}
extern "C" bool pioneer_decoder_native_run(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return RunQ38ResidentDecoderModel(c,r);}
extern "C" bool pioneer_decoder_native_close(PioneerStoreContext& c,PioneerQ38ResidentDecoderResult& r){return CloseQ38ResidentDecoderModel(c,r);}
