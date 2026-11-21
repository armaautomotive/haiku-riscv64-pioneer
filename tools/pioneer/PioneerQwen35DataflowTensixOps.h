/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_QWEN35_DATAFLOW_TENSIX_OPS_H
#define PIONEER_QWEN35_DATAFLOW_TENSIX_OPS_H

#include "PioneerQ38SfpuAccumulator.h"
#include "PioneerQ38VectorDstInit.h"
#include "PioneerQwen35DataflowAffine.h"
#include "PioneerQwen35DataflowKernels.h"
#include "PioneerQwen35DataflowVectorUnpack.h"
#include "PioneerQ38ProjectionInput.h"

namespace PioneerQwen35 {

// Bounded command emitter for the rewrite's compute role. A failed config
// readback or Tensix drain latches failure; every later op becomes a no-op so
// the role can publish its queue error and return to the owner.
class DataflowBlackholeTensixEmitter {
	bool fGood=true;

	static volatile uint32_t* Words(uint32_t address)
	{
		return reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(address));
	}

	static void Fence()
	{
		asm volatile("fence rw,rw" ::: "memory");
	}

public:
	void Op(uint32_t instruction)
	{
		if (fGood) *Words(0xffe40000)=instruction;
	}

	void Config(unsigned index,uint32_t value)
	{
		if (!fGood) return;
		if (index>=224) { fGood=false; return; }
		volatile uint32_t* target=Words(0xffef0000+index*4);
		*target=value;
		if (*target!=value) fGood=false;
	}

	void Drain()
	{
		if (!fGood) return;
		volatile uint32_t* ack=Words(0xffe000fc);
		*ack=0; (void)*ack; Fence();
		Op(0xa2101816); Op(0x4500017e);
		for (unsigned attempt=0;attempt<1000000;attempt++) {
			if (*ack) { Fence(); return; }
		}
		fGood=false;
	}

	bool Good() const { return fGood; }
};

class DataflowBlackholeTensixOps {
	enum : unsigned {
		TensixDstInitSource=0xa0000,
		TensixDstInitSourceBytes=0x400,
		TensixActivationPanel=0x70920,
		TensixDotReadback=0xffbd8800,
		TensixOutputReadback=0xffbd9b00
	};

	const DataflowCoreInvocation* fProgram=nullptr;
	DataflowBlackholeTensixEmitter fEmitter;
	bool fBound=false,fSetup=false,fOutputActive=false;
	unsigned fNextBlock=0;

	static volatile uint32_t* Words(uint32_t address)
	{
		return reinterpret_cast<volatile uint32_t*>(static_cast<uintptr_t>(address));
	}

	static void Fence()
	{
		asm volatile("fence rw,rw" ::: "memory");
	}

	bool Setup()
	{
		fEmitter.Op(0xb2000000); fEmitter.Drain();
		for (unsigned i=1;i<68;i++) fEmitter.Op(0xb2000000|(i<<16));
		fEmitter.Op(0xb2020001); fEmitter.Op(0xb2030001);
		fEmitter.Op(0xb2050004); fEmitter.Drain();
		static const uint8_t indices[]={0,1,2,3,6,7,220,20,24,28,50,56,59,
			64,65,72,73,76,84,86,112,113,120,121,124};
		static const uint32_t values[]={0x4231,0xa0220000,0,0x10000,0,0,0,0,
			0xffff,0x100,0x100,0x100000,0x100,0x100011,16,1,3,0x7070,
			0x400040,0x100010,0x100011,8,1,3,0x7091};
		static_assert(sizeof(indices)==sizeof(values)/sizeof(values[0]),
			"one exact setup value for each Tensix config register");
		for (unsigned i=0;i<sizeof(indices);i++)
			fEmitter.Config(indices[i],values[i]);
		fEmitter.Op(0x5e23fc00); fEmitter.Op(0x5e41fc00);
		fEmitter.Drain();
		return fEmitter.Good();
	}

	bool InitOutputAccumulator()
	{
		// The Dst init helper uses the existing validated 0xa0000 source
		// window with AllDatumsAreZero set. Give that window an explicit local
		// lifetime in this rewrite image rather than borrowing model input.
		if (TensixDstInitSource+TensixDstInitSourceBytes>DataflowProgramAddress)
			return false;
		for (unsigned word=0;word<TensixDstInitSourceBytes/4;word++)
			Words(TensixDstInitSource)[word]=0;
		Fence();
		PioneerQ38VectorDstInit(fEmitter,false,true,true);
		return fEmitter.Good();
	}

	bool MatrixPanel()
	{
		fEmitter.Op(0x3700000f);
		fEmitter.Op(0x10080004); fEmitter.Op(0x10080005);
		fEmitter.Drain();
		fEmitter.Op(0x5160000a); fEmitter.Op(0x5460000f);
		fEmitter.Op(0x428000c1); fEmitter.Drain();
		for (unsigned phase=0;phase<4;phase++) {
			fEmitter.Op(0xb20b0000|phase); fEmitter.Drain();
			fEmitter.Op(phase==3?0x26c00020:0x26000020);
			fEmitter.Drain();
		}
		return fEmitter.Good();
	}

	bool LoadAffine(unsigned type,const uint8_t* weightPage,unsigned bytes,
		unsigned panel)
	{
		auto* metadata=reinterpret_cast<PioneerQ38Panel*>(
			static_cast<uintptr_t>(DataflowQ5PanelScratchAddress));
		if (!PioneerQwen35DataflowAffineOnly(type,weightPage,bytes,panel,
			metadata)) return false;
		volatile uint32_t* coefficient=Words(DataflowAffineCoefficientAddress);
		volatile uint32_t* minimum=Words(DataflowAffineMinimumAddress);
		for (unsigned row=0;row<16;row++) {
			coefficient[row]=metadata->coefficient[row];
			minimum[row]=metadata->minimum[row];
		}
		Fence();
		return true;
	}

	bool LoadActivationPanel(unsigned type,unsigned block,unsigned panel,
		const PioneerQ38ProjectionInput& input,uint32_t& scale,int& sum)
	{
		const unsigned wordBase=type==8?(block%8)*16+panel*8:panel*8;
		const unsigned scaleIndex=type==8?block%8:0;
		if (wordBase>128-8 || scaleIndex>=16) return false;
		volatile uint32_t* destination=Words(TensixActivationPanel);
		sum=0;
		for (unsigned word=0;word<8;word++) {
			const uint32_t value=input.operand[wordBase+word];
			destination[word]=value;
			for (unsigned lane=0;lane<2;lane++) {
				const uint16_t encoded=uint16_t(value>>(lane*16));
				if (!PioneerInt8OperandValid(encoded)) return false;
				const int magnitude=int(encoded&1023);
				sum+=(encoded&0x8000)?-magnitude:magnitude;
			}
		}
		(void)destination[7]; Fence();
		scale=input.scale[scaleIndex];
		return sum>=-2048 && sum<=2048;
	}

public:
	enum { TensixMatrixEngine=1 };

	bool Bind(const DataflowCoreInvocation& program)
	{
		if (fBound || !program.Valid()) return false;
		fProgram=&program; fBound=true;
		return true;
	}

	bool TensixBeginOutput(unsigned type)
	{
		if (!fBound || !fProgram || fOutputActive || type!=fProgram->type
			|| !fEmitter.Good()) return false;
		if (!fSetup) {
			if (!Setup()) return false;
			fSetup=true;
		}
		if (!InitOutputAccumulator()) return false;
		fNextBlock=0; fOutputActive=true;
		return true;
	}

	bool TensixMatmul(unsigned type,unsigned block,const uint8_t* weightPage,
		const uint8_t* inputPage)
	{
		if (!fOutputActive || !fProgram || type!=fProgram->type
			|| block!=fNextBlock || block>=fProgram->blocks || !weightPage
			|| !inputPage || !fEmitter.Good()) return false;
		const uintptr_t weightAddress=reinterpret_cast<uintptr_t>(weightPage);
		const uintptr_t inputAddress=reinterpret_cast<uintptr_t>(inputPage);
		if ((weightAddress!=DataflowWeightBufferAddress
			&& weightAddress!=DataflowWeightBufferAddress+DataflowWeightBufferPageStride)
			|| (inputAddress!=DataflowInputBufferAddress
			&& inputAddress!=DataflowInputBufferAddress+DataflowInputPageBytes))
			return false;
		const unsigned weightBytes=fProgram->circularBuffer[0].pageBytes;
		const unsigned panels=type==8?2:16;
		const unsigned affineType=type==13?12:type;
		const auto& input=*reinterpret_cast<const PioneerQ38ProjectionInput*>(inputPage);
		for (unsigned panel=0;panel<panels;panel++) {
			const bool loadMetadata=type==14 || !(panel&1);
			if (loadMetadata && !LoadAffine(type,weightPage,weightBytes,panel))
				return false;
			uint32_t scale=0; int sum=0;
			if (!LoadActivationPanel(type,block,panel,input,scale,sum)) return false;
			if (!PioneerQwen35DataflowVectorUnpack(fEmitter,type,panel,
				unsigned(weightAddress)) || !fEmitter.Good()) return false;
			if (!MatrixPanel()) return false;
			volatile uint32_t* dots=Words(TensixDotReadback);
			for (unsigned row=0;row<16;row++) {
				const int32_t dot=int32_t(dots[row]);
				if (dot<-262144 || dot>262144) return false;
			}
			if (!PioneerQ38SfpuAccumulator<DataflowBlackholeTensixEmitter,true>(
				fEmitter,affineType,block==0 && panel==0,loadMetadata,scale,sum)
				|| !fEmitter.Good()) return false;
		}
		fNextBlock++;
		return true;
	}

	bool TensixFinishOutput(uint8_t* outputPage,unsigned bytes)
	{
		if (!fOutputActive || !fProgram || !outputPage
			|| bytes!=DataflowOutputPageBytes || fNextBlock!=fProgram->blocks
			|| !fEmitter.Good()) return false;
		volatile uint32_t* values=Words(TensixOutputReadback);
		auto* output=reinterpret_cast<volatile uint32_t*>(outputPage);
		for (unsigned row=0;row<16;row++) {
			const uint32_t value=values[row];
			if ((value&0x7f800000)==0x7f800000) return false;
			output[row]=value;
		}
		Fence();
		fOutputActive=false;
		return fEmitter.Good();
	}
};

} // namespace PioneerQwen35

#endif
