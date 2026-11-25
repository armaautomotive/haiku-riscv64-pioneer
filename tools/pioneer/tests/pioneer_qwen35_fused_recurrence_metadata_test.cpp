/* SPDX-License-Identifier: MIT */
#define PIONEER_QWEN35_PAIRED_RECURRENCE 1
#define PIONEER_QWEN35_FUSED_RECURRENCE 1
#define main q35_fused_metadata_fixture_main
#include "pioneer_qwen35_dataflow_stream_recorder_test.cpp"
#undef main
#include "../PioneerQwen35DataflowGraphMetadata.h"

// Intentionally omit pair/triple methods: the same decoder executes the
// original four projections sequentially as an independent metadata oracle.
struct SequentialMetadata {
    DataflowGraphMetadataService service;
    explicit SequentialMetadata(DataflowProjectionAuthority& a):service(a) {}
    template<class IO> int EmbedToken(const DataflowEmbeddingProgram& p,IO& io) { return service.EmbedToken(p,io); }
    template<class IO> int RunProjection(const DataflowProjectionProgram& p,IO& io) { return service.RunProjection(p,io); }
    template<class IO> int RunRms(const DataflowRmsProgram& p,IO& io) { return service.RunRms(p,io); }
    template<class IO> int RunSilu(const DataflowSiluProgram& p,IO& io) { return service.RunSilu(p,io); }
    template<class IO> int RunResidual(const DataflowResidualProgram& p,IO& io) { return service.RunResidual(p,io); }
    template<class IO> int RunConv(const DataflowConvProgram& p,IO& io) { return service.RunConv(p,io); }
    template<class IO> int RunDelta(const DataflowDeltaProgram& p,IO& io) { return service.RunDelta(p,io); }
    template<class IO> int RunDenseAttention(const DataflowDenseAttentionProgram& p,IO& io) { return service.RunDenseAttention(p,io); }
    template<class IO> int RunArgmax(const DataflowArgmaxProgram& p,unsigned& token,IO& io) { return service.RunArgmax(p,token,io); }
};
int main() {
    (void)&q35_fused_metadata_fixture_main;
    auto fixture=std::make_unique<Fixture>();
    auto live=std::make_unique<DataflowProjectionAuthority>();
    auto scratch=std::make_unique<DataflowProjectionProgram>();
    assert(live->BindValidatedModel(fixture->manifest,fixture->workspace,fixture->routes,fixture->rope));
    for (unsigned position=0;position<3;position++) {
        const bool decode=position!=1;const unsigned token=42+position;
        auto expected=std::make_unique<DataflowProjectionAuthority>();
        auto fused=std::make_unique<DataflowProjectionAuthority>();
        assert(live->CloneForGraphPlanning(*expected));
        SequentialMetadata sequential(*expected);struct NoIO {} io;
        unsigned ignored=Vocabulary,stage=0;
        assert(DataflowDecoderSequence::Run(position,token,0,*expected,sequential,io,ignored,stage,*scratch,decode)
            ==DataflowDecoderSequenceComplete);
        assert(!sequential.service.Failed() && sequential.service.Steps()==(decode?548U:545U));
        assert(DataflowGraphMetadata::Prepare(*live,*fused,position,token,0,decode,*scratch,stage));
        DataflowGraphAuthorityState state{};
        assert(fused->CaptureGraphState(state) && expected->GraphStateMatches(state));
        live=std::move(fused);
    }
    std::puts("PASS fused recurrent metadata matches original sequential decoder for three positions; SIMULATED ONLY");
}
