/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_DEFER_AFFINE_IMAGE_TEST_MODEL_ONLY
#include "pioneer_q38_defer_affine_image_test.cpp"
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38Group32PeerImage.h"
#ifndef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#define PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#define PIONEER_Q38_UNDEFINE_GROUP32_REFERENCE_CLIENT
#endif
#include "../PioneerQ38Group32Reference.h"
#ifdef PIONEER_Q38_UNDEFINE_GROUP32_REFERENCE_CLIENT
#undef PIONEER_Q38_GROUP32_REFERENCE_CLIENT
#undef PIONEER_Q38_UNDEFINE_GROUP32_REFERENCE_CLIENT
#endif
// Separate reference uses decoded input/tile DATA,not candidate instructions.
struct Group32Image:DeferredAffineImage {
	using DeferredAffineImage::DeferredAffineImage;
	bool paired=true,omitClear=false;
	unsigned pairClears=0,pairLoads=0;
	struct GroupHazard {};
	bool Active() const {return paired && BlockAffineActive();}
	int Input(unsigned k) const {auto x=vector.at(k);return int(x&1023)*((x&0x8000)?-1:1);}
	int Dot(unsigned group,unsigned block,unsigned first,unsigned row,int& sum) const {
		int dot=0;sum=0;
		for(unsigned p=first;p<first+2;p++) {
			PioneerQ38Panel panel;const auto& data=tiles.at(group*(inputs/256)+block);
			assert(PioneerQ38UnpackPanel(12,data.data(),unsigned(data.size()),p,&panel));
			for(unsigned k=0;k<16;k++) {unsigned e=k*16+row;uint16_t w=uint16_t(panel.operand[e/2]>>((e%2)*16));
				int x=Input(block*256+p*16+k);dot+=int(w&1023)*((w&0x8000)?-1:1)*x;sum+=x;}
		}
		return dot;
	}
	void Instruction(uint32_t op) override {
		if(Active() && (op==0x10080004 || op==0x10080005)) {
			unsigned p=matrixInstructions/MatrixPhasesPerPanel();assert(!(p&1));
			pairClears++;if(omitClear)return;
		}
		if(Active() && (op&~2U)==0x70040020) {
			unsigned p=matrixInstructions/MatrixPhasesPerPanel()-1;
			assert((p&1) && pairClears==((p+1)/2)*2);
			unsigned group=p/(inputs/16),within=p%(inputs/16),block=within/16,first=(within%16)&~1U;
			for(unsigned col=0;col<16;col++) {int sum=0,expected=Dot(group,block,first,col,sum);
				for(unsigned row=32;row<36;row++)if(dstInt[row][col]!=(row==32?expected:0)){
					if(!omitClear)fprintf(stderr,"GROUP32_DOT p%u group%u block%u first%u row%u col%u actual%d expected%d\n",p,group,block,first,row,col,dstInt[row][col],expected);
					throw GroupHazard{};}}
			pairLoads++;
		}
		DeferredAffineImage::Instruction(op);
	}
	bool CorrectRows() const override {
		if(!Active())return DeferredAffineImage::CorrectRows();
		assert(matrixInstructions==MatrixPhasesPerPanel()*groups*(inputs/16));
		for(unsigned group=0;group<groups;group++)for(unsigned row=0;row<16;row++) {
			float expected=0;double reference=0,expanded=0;
			for(unsigned b=0;b<inputs/256;b++)for(unsigned p=0;p<16;p+=2) {
				int sum=0,dot=Dot(group,b,p,row,sum);const auto& data=tiles.at(group*(inputs/256)+b);
				PioneerQ38Panel panel;assert(PioneerQ38UnpackPanel(12,data.data(),unsigned(data.size()),p,&panel));
				float c=Float(panel.coefficient[row]),m=Float(panel.minimum[row]),s=Float(scales[b]);
				volatile float cs=c*s,ms=m*s,product=cs*float(dot),correction=ms*float(sum),value=product+correction;
				expected+=value;
				// Independent FP64 expanded affine terms BEFORE cancellation.
				double a=double(c)*s*dot,z=double(m)*s*sum;reference+=a+z;expanded+=fabs(a)+fabs(z);
			}
			double nu=double(inputs/32+8)*0x1p-24,bound=1e-6+nu/(1-nu)*expanded;
			if(buffers[4096+group*16+row]!=Bits(expected) || fabs(double(expected)-reference)>bound)return false;
		}
		for(unsigned row=groups*16;row<4096;row++)if(buffers[4096+row]!=0x7fc00000)return false;
		// Cross-check the ACTUAL native debug client's separate DATA oracle.
		struct Request {unsigned type=12,peers=1,inputs;struct Part{unsigned groups;}part[1];std::vector<uint32_t> activation,scales;} request;
		request.inputs=inputs;request.part[0].groups=groups;request.scales=scales;request.activation.resize(inputs/2);
		for(unsigned k=0;k<inputs;k++)request.activation[k/2]|=uint32_t(vector[k])<<((k%2)*16);
		std::vector<uint8_t> packed;for(const auto& tile:tiles)packed.insert(packed.end(),tile.begin(),tile.end());
		float debug[256];if(!PioneerQ38Group32Reference(request,packed,0,debug))return false;
		for(unsigned row=0;row<groups*16;row++)if(buffers[4096+row]!=Bits(debug[row]))return false;
		return true;
	}
};
#ifndef PIONEER_Q38_GROUP32_IMAGE_MODEL_ONLY
int main(int argc,char** argv) {
	assert(argc==3 || (argc==4 && !strcmp(argv[3],"--q4-work-pilot")));setvbuf(stdout,nullptr,_IOLBF,0);
	const auto code=Read((std::string(argv[2])+"/text.bin").c_str()),prefix=Read((std::string(argv[2])+"/affine.bin").c_str());
	for(unsigned w=0;w<2048;w++) {
		assert(PioneerQ38Group32PeerWord(w)==(w*4<code.size()?Word(code,w*4):0));unsigned at=0x70000+w*4;
		assert(PioneerQ38Group32PeerPrefixWord(at)==(at>=0x70b00 && at-0x70b00<prefix.size()?Word(prefix,at-0x70b00):0));
	}
	assert(PioneerQ38Group32PeerWord(2048)==UINT32_MAX);
	for(unsigned at:{0x6fffcU,0x72000U,0x70001U})assert(PioneerQ38Group32PeerPrefixWord(at)==UINT32_MAX);
	for(unsigned type:{8U,12U,14U})for(unsigned k:{5120U,6144U,17408U})
	for(unsigned groups:{1U,3U,16U})for(bool work:{false,true}) {
		if(argc==4 && (type!=12 || k!=5120 || groups!=1 || !work))continue;
		printf("GROUP32_CASE type%u K%u groups%u work%u\n",type,k,groups,unsigned(work));
		Group32Image old(argv[1],type,k,groups),next(argv[2],type,k,groups);old.paired=false;
		assert(old.Boot() && next.Boot());old.memory[0x74038]=next.memory[0x74038]=unsigned(work);
		for(unsigned job=1;job<=2;job++) {
			if(job==2){old.NextPackets();next.NextPackets();FreshInputs(old);FreshInputs(next);next.pairClears=next.pairLoads=0;}
			old.Poison();next.Poison();unsigned before=old.steps,after=next.steps;
			assert(old.Job(job) && next.Job(job));
			assert(old.requests==next.requests && old.responses==next.responses && old.matrixInstructions==next.matrixInstructions
				&& old.sumWrites==next.sumWrites && old.sumReads==next.sumReads && old.dotProbeReads==next.dotProbeReads);
			if(type==12 && work) {assert(next.pairLoads==groups*(k/32)*2);assert(next.pairClears==next.pairLoads);
				if(groups==16 && job==2)printf("OFFLINE_GROUP32 K%u RV%u/%u vector_ops%u/%u drains%u/%u;NOT_NATIVE_OR_TOKEN_RATE\n",k,old.steps-before,next.steps-after,old.vectorOps,next.vectorOps,old.drains,next.drains);
			}else for(unsigned row=0;row<groups*16;row++)assert(old.buffers[4096+row]==next.buffers[4096+row]);
		}
		old.memory[0x74038]=next.memory[0x74038]=0;assert(old.Job(3,2) && next.Job(3,2));
	}
	Group32Image bad(argv[2],12,5120,1);bad.paired=true;assert(bad.Boot());bad.memory[0x74038]=1;bad.Poison();bad.omitClear=true;
	bool caught=false;try{(void)bad.Job(1);}catch(const Group32Image::GroupHazard&){caught=true;}
	assert(caught && !bad.outputReads && !bad.memory.at(0x74144));
	puts(argc==4?"PASS PARTIAL ONLY group32 actual-image pilot+missing-clear observer;FULL SUITE SKIPPED":"PASS OFFLINE ACTUAL group32:independent exact paired-FP32+FP64 references,all formats/widths/groups/modes/two changed inputs,unchanged NoC/matrix/source/proof/STOP,checked and non-Q4 exact to old,missing-clear observer before publication;NO_NATIVE_MODEL_QUALITY_OR_SPEED_QUALIFICATION");
}
#endif
