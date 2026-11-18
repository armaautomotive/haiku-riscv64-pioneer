/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_GEOMETRY_H
#define PIONEER_QWEN35_GEOMETRY_H

namespace PioneerQwen35 {
enum : unsigned {
	Layers=32, Hidden=2560, Ffn=9216, Vocabulary=248320,
	DenseHeads=16, DenseKeyHeads=4, DenseHeadWidth=256, Rotary=64,
	DeltaHeads=32, DeltaKeyHeads=16, DeltaWidth=128,
	DeltaInner=DeltaHeads*DeltaWidth, ConvChannels=(2*DeltaKeyHeads+DeltaHeads)*DeltaWidth,
	DeltaStateWords=DeltaWidth*DeltaWidth, MaxPositions=8192
};
enum : unsigned {
	AttentionNorm=0, PostAttentionNorm=1, Gate=2, Up=3, Down=4,
	RecurrentGate=5, RecurrentQkv=6, NegativeA=7, DeltaBias=8, DeltaNorm=9,
	Alpha=10, Beta=11, ConvWeights=12, RecurrentOut=13,
	QueryAndGate=5, Key=6, Value=7, QueryNorm=8, KeyNorm=9, AttentionOut=10,
	TiedOutput=0, FinalNorm=1
};
inline bool Dense(unsigned layer) { return layer<Layers && (layer&3)==3; }
}
#endif
