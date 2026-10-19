/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_CPU_VALIDATION_H
#define PIONEER_Q38_CPU_VALIDATION_H
#include "PioneerQwen38Panel.h"
#include "PioneerQwen38CohortHost.h"
// Independent CPU learned-matrix reference,DIAGNOSTIC builds ONLY.
// Reads original authenticated GGUF rows,not resident card/output/cache.
// Reproduces vector55's quantized-input FP32 operation order exactly.
// Never writes/replaces a P100A output. Any mismatch is CLOSE ONLY.
template<class Source> bool PioneerQ38CpuValidate(Source& source,unsigned tensor,const float* x,unsigned k,const float* actual,unsigned n)
{
	if (!source.Ready() || tensor>=source.Model().Tensors().size() || !x || !actual) return false;
	const auto& t=source.Model().Tensors()[tensor];
	if (t.inputs!=k || t.outputs!=n || !n || n%16) return false;
	PioneerQ38CohortInput input;
	if (!PioneerQ38CohortQuantize(t.type,k,x,input)) return false;
	unsigned blockInputs=PioneerQ38CohortBlockInputs(t.type),blocks=k/blockInputs;
	unsigned blockBytes=t.type==8?34:t.type==12?144:t.type==14?210:0;
	if (!blockBytes || k%blockInputs) return false;
	unsigned rowBytes=blocks*blockBytes;
	if (rowBytes>65536) return false;
	std::vector<uint8_t> packed(size_t(rowBytes)*16),tile(PioneerQ38PanelTileBytes(t.type),0);
	for (unsigned first=0;first<n;first+=16) {
		if (!source.ReadReferencePacked(tensor,uint64_t(first)*rowBytes,unsigned(packed.size()),packed.data(),unsigned(packed.size()))) return false;
		float expected[16]={};
		for (unsigned b=0;b<blocks;b++) {
			for (unsigned byte=0;byte<blockBytes;byte++) for (unsigned r=0;r<16;r++)
				tile[byte*16+r]=packed[size_t(r)*rowBytes+b*blockBytes+byte];
			for (unsigned p=0;p<blockInputs/16;p++) {
				PioneerQ38Panel panel;
				if (!PioneerQ38UnpackPanel(t.type,tile.data(),unsigned(tile.size()),p,&panel)) return false;
				int activation[16],sum=0;
				for (unsigned lane=0;lane<16;lane++) {
					unsigned index=b*blockInputs+p*16+lane;
					uint16_t v=uint16_t(input.activation[index/2]>>(16*(index%2)));
					activation[lane]=(v&0x8000)?-int(v&0x3ff):int(v&0x3ff); sum+=activation[lane];
				}
				float scale=PioneerAttentionFloat(input.scales[b]);
				for (unsigned r=0;r<16;r++) {
					int dot=0;
					for (unsigned lane=0;lane<16;lane++) {
						unsigned index=lane*16+r;
						uint16_t v=uint16_t(panel.operand[index/2]>>(16*(index%2)));
						int weight=(v&0x8000)?-int(v&0x3ff):int(v&0x3ff);
						dot+=weight*activation[lane];
					}
					float product=(PioneerAttentionFloat(panel.coefficient[r])*scale)*float(dot);
					float correction=(PioneerAttentionFloat(panel.minimum[r])*scale)*float(sum);
					expected[r]+=product+correction;
				}
			}
		}
		if (memcmp(expected,actual+first,sizeof(expected))) return false;
	}
	return source.Ready();
}
#endif
