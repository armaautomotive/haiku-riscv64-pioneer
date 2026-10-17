/* SPDX-License-Identifier: MIT */
#include "../PioneerQwen38Activation.h"
#include "../PioneerQwen38PackedDot.h"
#include "../TenstorrentGgufQuant.h"
#include "pioneer_qwen38_activation_reference.h"
#include <cfenv>
#include <random>
#include <limits>
static void Compare(const float* input)
{
	PioneerQ38ActivationK actual; uint32_t wanted[73];
	assert(PioneerQ38QuantizeK(input,256,&actual)); Q38ActivationReference(input,wanted);
	assert(!std::memcmp(actual.words,wanted,sizeof(wanted)));
}
int main()
{
	assert(std::fegetround()==FE_TONEAREST); std::mt19937 random(0x38ac71);
	float input[256]={}; Compare(input);
	for (auto& x:input) x=-0.0f; Compare(input);
	for (unsigned trial=0;trial<4096;trial++) {
		float scale=ldexpf(1.0f,int(trial%201)-100);
		for (auto& x:input) x=float(int(random()%2000001)-1000000)*scale;
		Compare(input);
	}
	// The first equal absolute maximum controls scale sign. Exact half ties
	// distinguish nearest-even from the old Q8_0 ties-away quantizer.
	for (unsigned negative=0;negative<2;negative++) {
		for (unsigned i=0;i<256;i++) input[i]=(float(int(i%255)-127)+.5f)*.5f;
		input[0]=negative?-127:127; input[1]=-input[0]; Compare(input);
		for (int i=-253;i<=253;i++) {
			float x=float(i)*.5f;
			assert(PioneerQ38NearestEven(x)==int(nearbyintf(x)));
			for (float y:{nextafterf(x,-INFINITY),nextafterf(x,INFINITY)})
				assert(PioneerQ38NearestEven(y)==int(nearbyintf(y)));
		}
	}
	for (float bad:{INFINITY,-INFINITY,std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::denorm_min(),-std::numeric_limits<float>::denorm_min()})
		for (unsigned at:{0U,127U,255U}) {
			for (auto& x:input) x=1;
			input[at]=bad; PioneerQ38ActivationK output,before;
			std::memset(&output,0xa5,sizeof(output)); before=output;
			assert(!PioneerQ38QuantizeK(input,256,&output) && !std::memcmp(&output,&before,sizeof(output)));
		}
	for (auto& x:input) x=std::numeric_limits<float>::min();
	PioneerQ38ActivationK output,before; std::memset(&output,0xa5,sizeof(output)); before=output;
	assert(!PioneerQ38QuantizeK(input,256,&output) && !std::memcmp(&output,&before,sizeof(output)));
	assert(!PioneerQ38QuantizeK(input,255,&output) && !std::memcmp(&output,&before,sizeof(output)));
	assert(!PioneerQ38QuantizeK(input,256,reinterpret_cast<PioneerQ38ActivationK*>(input)));
	assert(!PioneerQ38QuantizeK(nullptr,256,&output) && !PioneerQ38QuantizeK(input,256,nullptr));
	// Composition uses an INDEPENDENT activation oracle and independently
	// expanded packed weights. The model weights remain packed in the API.
	for (unsigned type:{12U,14U}) for (unsigned trial=0;trial<128;trial++) {
		for (auto& x:input) x=float(int(random()%200001)-100000)*.001f;
		PioneerQ38ActivationK activation; assert(PioneerQ38QuantizeK(input,256,&activation));
		uint32_t oracle[73]; Q38ActivationReference(input,oracle);
		uint8_t packed[210]; for (auto& b:packed) b=uint8_t(random());
		unsigned bytes=type==12?144:210,d=type==12?0:208;
		packed[d]=0; packed[d+1]=0x3c;
		if (type==12) { packed[2]=0; packed[3]=0x38; }
		float decoded[256]; assert(type==12?PioneerDecodeQ4K(packed,decoded):PioneerDecodeQ6K(packed,decoded));
		float actual=0,scale=PioneerAttentionFloat(activation.words[0]);
		assert(PioneerQ38PackedDot(type,packed,bytes,reinterpret_cast<const PioneerQ38Int8*>(activation.words+1),256,scale,&actual));
		double wanted=0,absolute=0;
		const uint8_t* qs=reinterpret_cast<const uint8_t*>(oracle+1);
		for (unsigned i=0;i<256;i++) { double p=double(decoded[i])*(int(qs[i])-int((qs[i]&128)*2))*PioneerAttentionFloat(oracle[0]); wanted+=p; absolute+=fabs(p); }
		assert(fabs(actual-wanted)<=1e-6+absolute*5e-7);
	}
	puts("PASS OFFLINE Q8_K activation4096 random blocks/exact signed F32 scales/ALL bytes+16 sums/first-max sign/nearest-even half boundaries/zero canonicalization/nonfinite+subnormal+alias refusals/256 composed Q4_K+Q6_K dots;NOT installed");
}
