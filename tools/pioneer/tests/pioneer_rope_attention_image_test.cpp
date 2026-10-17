/* SPDX-License-Identifier: MIT */
#define PIONEER_COORDINATED_GLU_MACHINE_ONLY
#include "pioneer_coordinated_glu_image_test.cpp"
struct RopeGroupMachine:GroupMachine {
	std::vector<float> referenceQuery;
	std::vector<uint16_t> referenceKey,referenceValue;
	RopeGroupMachine(const Image& image,unsigned n,unsigned layer,unsigned group,unsigned position):GroupMachine(image,n,layer,group) {
		std::vector<float> query(256),key(128),qWeight(128),kWeight(128),rope(128);
		for (unsigned i=0;i<256;i++) query[i]=float(sin(i*.173+group*.137)*1.7);
		for (unsigned i=0;i<128;i++) { key[i]=float(cos(i*.317+layer*.171)*1.1); qWeight[i]=float(1+sin(i*.11)*.3); kWeight[i]=float(1+cos(i*.37)*.4); }
		for (unsigned i=0;i<64;i++) { float angle=float(position)*powf(1000000.0f,-float(i)/64); rope[i*2]=cosf(angle); rope[i*2+1]=sinf(angle); }
		referenceQuery=query; auto preparedKey=key;
		for (unsigned head=0;head<3;head++) {
			float* values=head<2?referenceQuery.data()+head*128:preparedKey.data(); const auto& weights=head<2?qWeight:kWeight;
			double sum=0; for (unsigned i=0;i<128;i++) sum+=double(values[i]*values[i]);
			float scale=1.0f/sqrtf(float(sum/128)+1e-6f);
			for (unsigned i=0;i<128;i++) values[i]=(values[i]*scale)*weights[i];
			for (unsigned i=0;i<64;i++) { float a=values[i],b=values[i+64]; values[i]=fmaf(a,rope[i*2],-(b*rope[i*2+1])); values[i+64]=fmaf(a,rope[i*2+1],b*rope[i*2]); }
		}
		referenceKey.resize(n*128); referenceValue.resize(n*128);
		for (unsigned i=0;i<n*128;i++) {
			_Float16 k=_Float16(sin(i*.371)*1.3),v=_Float16(cos(i*.217)*2); memcpy(&referenceKey[i],&k,2); memcpy(&referenceValue[i],&v,2);
		}
		for (unsigned i=0;i<128;i++) { _Float16 k=_Float16(preparedKey[i]); memcpy(&referenceKey[(n-1)*128+i],&k,2); }
		Inputs(query,referenceKey,referenceValue); mem[0x74000/4]=0x32575441;
		for (unsigned i=0;i<128;i++) {
			masterInput[0x86000+group*512+i*4]=B(key[i]); masterInput[0x147a00+i*4]=B(qWeight[i]);
			masterInput[0x147c00+i*4]=B(kWeight[i]); masterInput[0x147e00+i*4]=B(rope[i]);
		}
	}
	uint32_t Load(uint32_t a) override {
		if (Range(a,0x90000,0x1200)) return mem[a/4];
		return GroupMachine::Load(a);
	}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0x90000,0x600)) { mem[a/4]=value; return; }
		GroupMachine::Store(a,value);
	}
	void VerifyRope() {
		Verify(); std::vector<float> expected(256); Reference(referenceQuery,referenceKey,referenceValue,tokens,expected);
		assert(Error(expected)<.00002);
		for (unsigned i=0;i<256;i++) assert(mem[0x90000/4+i]==B(referenceQuery[i]));
		for (unsigned i=0;i<64;i++) assert(dram.at(Base+layer*0x100000+group*0x20000+(tokens-1)*256+i*4)
			==uint32_t(referenceKey[(tokens-1)*128+i*2])+(uint32_t(referenceKey[(tokens-1)*128+i*2+1])<<16));
	}
};
struct VectorGluFaultMachine:GluMachine {
	unsigned writesToDst=0,badDst=0;
	VectorGluFaultMachine(const Image& i):GluMachine(i,27,5) {}
	void Store(uint32_t a,uint32_t value) override {
		if (Range(a,0xffbd8000,8*64) && ++writesToDst==badDst) value^=1;
		GluMachine::Store(a,value);
	}
};
#ifndef PIONEER_ROPE_GROUP_MACHINE_ONLY
int main(int argc,char** argv)
{
	assert(argc==2); setvbuf(stdout,NULL,_IONBF,0); Image image(argv[1],false,true,true); assert(image.parallelRope);
	for (unsigned n:{1U,2U,17U,64U,65U,256U}) for (unsigned group=0;group<8;group++) {
		RopeGroupMachine m(image,n,n&1?27:0,group,n-1); m.Run(); m.VerifyRope();
		printf("OFFLINE actual RMS/RoPE/attention group%u n%u EXACT preparation/all KV bits/complete logits/proof PASS steps%u\n",group,n,m.steps);
	}
	for (unsigned packet=1;packet<=12;packet++) for (unsigned fault=0;fault<6;fault++) {
		RopeGroupMachine m(image,1,27,7,3);
		if (fault<4) { m.badCounter=packet; m.badCounterWord=fault+1; } else if (fault==4) m.corruptPull=packet; else m.failCommand=packet;
		m.Run(true); assert(!m.outputWrites && !m.writes && !m.mem[0x17ffc4/4]);
	}
	for (unsigned source:{0x140000U,0x86000U,0x147a00U,0x147c00U,0x147e00U,0x142100U}) {
		RopeGroupMachine m(image,1,0,0,2); m.masterInput[source]=0x7f800000; m.Run(source!=0x142100); assert(!m.outputWrites && !m.writes && !m.mem[0x17ffc4/4]);
	}
	for (unsigned layer:{0U,27U}) for (unsigned group=0;group<6;group++) { GluMachine m(image,layer,group); m.Run(); m.VerifyGlu(); }
	if (image.vectorGlu) {
		for (unsigned word=1;word<=128;word++) { VectorGluFaultMachine m(image); m.badDst=word; m.Run(true);
			assert(!m.operandWrites && !m.scaleWrites && !m.mem[0x17ffc4/4]); }
		for (unsigned word:{1921U,1984U,1985U,2048U}) { VectorGluFaultMachine m(image); m.badDst=word; m.Run(true);
			assert(!m.mem[0x17ffc4/4] && m.operandWrites==240 && m.scaleWrites==15); }
		VectorGluFaultMachine m(image); m.dropAck=true; m.Run(true); assert(!m.operandWrites && !m.scaleWrites && !m.mem[0x17ffc4/4]);
	}
	puts("PASS OFFLINE fixed parallel three-head RMS/exact fused RoPE/attention and exact GLU packing,all source bits/counters/guards/extension instruction bounds;NOT native qualification");
}
#endif
