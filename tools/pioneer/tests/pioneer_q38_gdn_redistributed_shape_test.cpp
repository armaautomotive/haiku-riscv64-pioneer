/* SPDX-License-Identifier: MIT */
// Candidate descriptor geometry only. No launch, ownership, initialization,
// dispatch, native capability, emitted-image, or performance qualification.
#include <assert.h>
#include <stdint.h>
#define PIONEER_ATTENTION_FREESTANDING
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38ResidentGdnRedistributedServiceShape.h"
#include "../PioneerQ38MixedRecurrentDispatch.h"

#ifndef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
static void FillInput(uint32_t* d, uint32_t layer, uint32_t logical, uint32_t peer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_GDN_INPUT_MARKER; d[1] = 1; d[2] = 67108864;
	d[3] = 1; d[4] = 0x04000000; d[5] = layer; d[6] = 1;
	d[7] = logical; d[8] = PioneerQ38ProjectionRoute(d[3]); d[9] = peer;
	d[10] = logical + 1; d[11] = 2; d[12] = 0x0c000000;
	d[13] = PioneerQ38ProjectionRoute(d[11]); d[14] = 3; d[15] = 0x10000000;
	d[16] = 1; d[40] = 0; d[41] = PioneerQ38ProjectionRoute(d[14]);
	const unsigned h = logical / 2, k = h & 15, block = (h >> 4) * 64;
	d[42] = TT_Q38_CONV_OUTPUT + k * 512;
	d[43] = TT_Q38_CONV_OUTPUT + 8192 + k * 512;
	d[44] = TT_Q38_CONV_OUTPUT + 16384 + h * 512 + (logical & 1) * 256;
	d[45] = PioneerQ38ProjectionResultOffset(10) + block;
	d[46] = PioneerQ38ProjectionResultOffset(11) + block;
	d[47] = h & 15; d[48] = 512; d[49] = 512; d[50] = 256; d[51] = 64;
	d[52] = 7; d[53] = 24; d[54] = 1536; d[55] = 322;
	d[32] = d[34] = d[36] = d[38] = 1;
}

static void FillState(uint32_t* d, uint32_t layer, uint32_t logical, uint32_t peer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_RGDN_MARKER; d[1] = 1; d[2] = 128; d[3] = 64;
	d[4] = 0; d[5] = 0x04000000; d[6] = 6; d[7] = 0xc3ff8000;
	d[8] = 1; d[9] = logical; d[10] = layer; d[11] = 0;
	d[12] = PioneerQ38SpanRoute(d[4]); d[13] = PioneerQ38SpanRoute(d[6]);
	d[14] = TT_Q38_RGDN_STATE_BYTES; d[15] = peer; d[16] = 1;
	d[32] = d[34] = d[36] = d[38] = 1;
}

static void FillGated(uint32_t* d, const uint32_t* input, const uint32_t* state,
	uint32_t logical, uint32_t peer, uint32_t siblingPeer)
{
	for (unsigned i = 0; i < 64; ++i) d[i] = 0;
	d[0] = TT_Q38_GDN_GATED_MARKER; d[1] = 1;
	for (unsigned i = 2; i <= 10; ++i) d[i] = input[i];
	d[9] = peer; d[11] = 4; d[12] = 0x14000000;
	d[13] = PioneerQ38ProjectionRoute(d[11]); d[14] = (logical ^ 1) + 1;
	d[15] = siblingPeer; d[40] = input[40]; d[41] = state[11];
	d[42] = PioneerQ38ProjectionResultOffset(5) + (logical / 2) * 512;
	d[43] = TT_Q38_GDN_GATED_OUTPUT + logical * 256;
	d[44] = TT_Q38_GDN_GATED_READY; d[45] = 0x8e800;
	d[46] = 128; d[47] = 64; d[48] = d[49] = 512;
	d[50] = 256; d[51] = TT_Q38_GDN_GATED_POLLS;
	for (unsigned i = 16; i < 40; ++i) d[i] = input[i];
}

static void FillServiceEvidence(const uint32_t* d, const uint32_t* input,
	const uint32_t* state,const uint32_t* source,unsigned worker,unsigned self,
	uint32_t* before,uint32_t* after,uint32_t* first,uint32_t* recurrent,
	uint32_t* gated,uint32_t* clock,uint32_t* proof)
{
	for(unsigned i=0;i<8;i++)before[i]=0;
	const unsigned inputDelta[8]={7,24,7,7,0,0,7,0};
	const unsigned stateDelta[8]={2,512,4,2,2,512,4,2};
	const unsigned gatedDelta[8]={4,21,5,4,1,4,5,1};
	uint32_t afterInput[8],afterState[8];
	for(unsigned i=0;i<8;i++)afterInput[i]=inputDelta[i];
	for(unsigned i=0;i<8;i++)afterState[i]=afterInput[i]+stateDelta[i];
	for(unsigned i=0;i<8;i++)after[i]=afterState[i]+gatedDelta[i];
	for(unsigned i=0;i<32;i++)first[i]=recurrent[i]=gated[i]=0;
	first[0]=7;first[1]=24;first[2]=0;first[3]=0;first[4]=322;first[5]=1536;
	first[6]=input[6];first[7]=TT_Q38_GDN_INPUT_MARKER;
	for(unsigned i=0;i<8;i++){first[8+i]=before[i];first[16+i]=afterInput[i];}
	const unsigned inputTail[]={input[7],input[5],input[40],input[10],input[11],input[14],input[42],input[44]};
	for(unsigned i=0;i<8;i++)first[24+i]=inputTail[i];
	recurrent[0]=2;recurrent[1]=512;recurrent[2]=2;recurrent[3]=512;
	recurrent[4]=64;recurrent[5]=32768;recurrent[6]=state[8];recurrent[7]=TT_Q38_RGDN_MARKER;
	for(unsigned i=0;i<8;i++){recurrent[8+i]=afterInput[i];recurrent[16+i]=afterState[i];}
	const unsigned stateTail[]={state[9],state[10],state[11],state[11]^1,state[15],1,state[5],state[7]};
	for(unsigned i=0;i<8;i++)recurrent[24+i]=stateTail[i];
	gated[0]=4;gated[1]=21;gated[2]=1;gated[3]=4;gated[4]=64;gated[5]=1;
	gated[6]=d[6];gated[7]=TT_Q38_GDN_RENDEZVOUS_MARKER;
	for(unsigned i=0;i<8;i++){gated[8+i]=afterState[i];gated[16+i]=after[i];}
	const unsigned gatedTail[]={d[7],d[5],d[40],d[15],d[4]+d[43],d[3],d[11],d[12]};
	for(unsigned i=0;i<8;i++)gated[24+i]=gatedTail[i];
	for(unsigned i=0;i<16;i++)clock[i]=0;
	clock[0]=TT_Q38_GDN_RENDEZVOUS_MARKER;clock[1]=1;clock[3]=2;
	clock[5]=gated[5];clock[6]=d[6];clock[7]=d[7];
	for(unsigned i=0;i<8;i++)clock[8+i]=d[32+i];
	for(unsigned i=0;i<64;i++)proof[i]=0;
	for(unsigned i=0;i<8;i++){proof[i]=before[i];proof[8+i]=after[i];}
	proof[16]=64;proof[17]=1;proof[18]=worker;proof[19]=TT_Q38_FFN_PERSISTENT_MARKER;
	const unsigned command[]={0x8ea00,0,self,d[4]+d[43],0,d[8],0,0x212,256,0};
	for(unsigned i=0;i<10;i++)proof[20+i]=command[i];
	proof[30]=source[0];proof[31]=self;
	for(unsigned i=0;i<16;i++)proof[32+i]=source[i];
	proof[48]=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION;proof[49]=d[6];
	proof[50]=d[5];proof[51]=d[7];proof[52]=d[40];proof[53]=state[11];
	for(unsigned i=0;i<8;i++)proof[54+i]=d[32+i];
	proof[62]=TT_Q38_GDN_RENDEZVOUS_MARKER;proof[63]=gated[5];
}
#endif

int main()
{
#ifdef PIONEER_Q38_RGDN_ROOT_QUALIFICATION
	uint32_t rootDescriptor[64] = {};
	assert(!PioneerQ38ResidentGdnRedistributedShape(rootDescriptor));
	assert(!PioneerQ38GdnRedistributedInputShape(rootDescriptor));
	assert(!PioneerQ38GdnRedistributedPairing(rootDescriptor, rootDescriptor));
	assert(!PioneerQ38GdnRedistributedGatedShape(rootDescriptor, rootDescriptor, rootDescriptor));
	assert(!PioneerQ38GdnRedistributedRendezvousShape(rootDescriptor, rootDescriptor, rootDescriptor));
	assert(!PioneerQ38ResidentGdnRedistributedServiceJob(rootDescriptor, rootDescriptor,
		rootDescriptor, rootDescriptor, rootDescriptor, 1, 1));
	assert(!PioneerQ38ResidentGdnRedistributedServiceSummaries(rootDescriptor,rootDescriptor,
		rootDescriptor,rootDescriptor,rootDescriptor,rootDescriptor,rootDescriptor,
		rootDescriptor,rootDescriptor));
	assert(!PioneerQ38ResidentGdnRedistributedServiceProof(rootDescriptor,rootDescriptor,
		rootDescriptor,rootDescriptor,rootDescriptor,rootDescriptor,1,1,rootDescriptor,
		rootDescriptor,rootDescriptor,rootDescriptor,rootDescriptor,rootDescriptor));
	return 0;
#else
	unsigned stateUse[PioneerQ38GdnTaskPlacement::PhysicalWorkers] = {};
	for (uint32_t layer = 0; layer < PioneerQ38GdnTaskPlacement::RecurrentLayers; ++layer) {
		uint32_t peers[PioneerQ38GdnTaskPlacement::LogicalWorkers] = {};
		for (uint32_t logical = 0; logical < PioneerQ38GdnTaskPlacement::LogicalWorkers; ++logical) {
			PioneerQ38GdnTaskPlacement::Route route = {};
			assert(PioneerQ38GdnTaskPlacement::Map(layer, logical, route));
			assert(route.layerOrdinal == layer && route.actualLayer == PioneerQ38GdnTaskPlacement::ActualLayer(layer));
			assert(route.logicalWorker == logical && route.physicalWorker >= 1);
			assert(route.physicalWorker <= PioneerQ38GdnTaskPlacement::PhysicalWorkers);
			assert(route.stateIndex < PioneerQ38GdnTaskPlacement::TotalTasks);
			++stateUse[route.physicalWorker - 1];
			for (uint32_t previous = 0; previous < logical; ++previous)
				assert(peers[previous] != route.peer);
			peers[logical] = route.peer;
			uint32_t recovered = ~uint32_t(0);
			assert(PioneerQ38GdnTaskPlacement::LogicalWorker(route.actualLayer,
				route.physicalWorker, recovered));
			assert(recovered == logical);
			uint32_t input[64], state[64];
			FillInput(input, route.actualLayer, logical, route.peer);
			FillState(state, route.actualLayer, logical, route.peer);
			assert(PioneerQ38GdnRedistributedInputShape(input));
			assert(PioneerQ38ResidentGdnRedistributedShape(state));
			assert(PioneerQ38GdnRedistributedPairing(input, state));
			uint32_t siblingRouteOrdinal = 0;
			assert(PioneerQ38GdnTaskPlacement::LayerOrdinal(route.actualLayer, siblingRouteOrdinal));
			PioneerQ38GdnTaskPlacement::Route sibling = {};
			assert(PioneerQ38GdnTaskPlacement::Map(siblingRouteOrdinal, logical ^ 1, sibling));
			uint32_t gated[64]; FillGated(gated, input, state, logical, route.peer, sibling.peer);
			assert(PioneerQ38GdnRedistributedGatedShape(gated, input, state));
			gated[0] = TT_Q38_GDN_RENDEZVOUS_MARKER;
			gated[51] = TT_Q38_GDN_RENDEZVOUS_POLLS;
			gated[52] = uint32_t(PioneerQ38GdnRendezvousBudget());
			gated[53] = uint32_t(PioneerQ38GdnRendezvousBudget() >> 32);
			gated[54] = 4; gated[55] = TT_Q38_GDN_RENDEZVOUS_CLOCK;
			assert(PioneerQ38GdnRedistributedRendezvousShape(gated, input, state));
			uint32_t source[16], job[16] = {};
			for(unsigned i=0;i<16;i++)source[i]=input[16+i];
			job[0]=TT_Q38_RESIDENT_GDN_SERVICE_JOB;job[1]=64;job[2]=768;job[3]=gated[6];
			job[10]=TT_Q38_FFN_PERSISTENT_MARKER;job[11]=route.physicalWorker;job[12]=1;
			job[13]=TT_Q38_RESIDENT_GDN_SERVICE_OPERATION;
			assert(PioneerQ38ResidentGdnRedistributedServiceJob(job,gated,input,state,
				source,route.physicalWorker,route.peer));
			assert(!PioneerQ38ResidentGdnRedistributedServiceJob(job,gated,input,state,
				source,route.physicalWorker,route.peer+0x10000));
			uint32_t boot[64]={},dispatch[64]={};
			boot[0]=PioneerQ38MixedWorkerLayout::Magic;boot[1]=1;boot[2]=64;
			boot[3]=route.physicalWorker;boot[4]=route.peer;boot[5]=input[16];
			boot[6]=8192;boot[7]=64;
			for(unsigned i=8;i<16;i++)boot[i]=input[24+i];
			boot[16]=input[16];
			for(unsigned i=17;i<32;i++)boot[i]=input[i];
			assert(PioneerQ38MixedRecurrentDispatch::GdnRedistributed(boot,input,state,
				gated,0,dispatch));
			assert(dispatch[0]==PioneerQ38MixedWorkerLayout::Magic
				&& dispatch[3]==route.physicalWorker && dispatch[4]==route.peer
				&& dispatch[5]==route.actualLayer);
			boot[3]=route.physicalWorker==119?118:route.physicalWorker+1;
			boot[4]=PioneerQ38MixedWorkerLayout::Peer(boot[3]);
			assert(!PioneerQ38MixedRecurrentDispatch::GdnRedistributed(boot,input,state,
				gated,0,dispatch));
			uint32_t legacyJob[16];for(unsigned i=0;i<16;i++)legacyJob[i]=job[i];
			legacyJob[11]=logical+1;
			assert(PioneerQ38ResidentGdnServiceJob(legacyJob,gated,input,state,source,
				logical+1,route.peer)==(route.physicalWorker==logical+1));
			uint32_t before[8],after[8],first[32],recurrent[32],gatedSummary[32],clock[16],proof[64];
			FillServiceEvidence(gated,input,state,source,route.physicalWorker,route.peer,
				before,after,first,recurrent,gatedSummary,clock,proof);
			uint32_t legacyD[64],legacyInput[64],legacyState[64],legacyRecurrent[32],legacyGated[32];
			for(unsigned i=0;i<64;i++){legacyD[i]=gated[i];legacyInput[i]=input[i];legacyState[i]=state[i];}
			legacyD[9]=PioneerQ38ConvPeer(legacyD[10]);
			legacyD[15]=PioneerQ38ConvPeer(legacyD[14]);
			legacyInput[9]=PioneerQ38ConvPeer(legacyInput[10]);
			legacyState[15]=PioneerQ38ResidentGdnPeer(legacyState[9]);
			for(unsigned i=0;i<32;i++){legacyRecurrent[i]=recurrent[i];legacyGated[i]=gatedSummary[i];}
			legacyRecurrent[28]=legacyState[15];legacyGated[27]=legacyD[15];
			assert(PioneerQ38GdnInputSummary(first,legacyInput,before,first+16));
			assert(PioneerQ38ResidentGdnSummary(legacyRecurrent,legacyState,first+16,recurrent+16));
			assert(PioneerQ38GdnRendezvousSummary(legacyGated,legacyD,legacyInput,legacyState,recurrent+16,after));
			assert(PioneerQ38GdnRendezvousClockReceipt(clock,legacyD,gatedSummary[5]));
			assert(PioneerQ38ResidentGdnRedistributedServiceSummaries(gated,input,state,
				first,recurrent,gatedSummary,clock,before,after));
			assert(PioneerQ38ResidentGdnRedistributedServiceProof(proof,job,gated,input,state,
				source,route.physicalWorker,route.peer,before,after,first,recurrent,gatedSummary,clock));
			job[11]^=1;
			assert(!PioneerQ38ResidentGdnRedistributedServiceJob(job,gated,input,state,
				source,route.physicalWorker,route.peer));
			job[11]^=1;
			recurrent[28]^=1;
			assert(!PioneerQ38ResidentGdnRedistributedServiceSummaries(gated,input,state,
				first,recurrent,gatedSummary,clock,before,after));
			recurrent[28]^=1;gatedSummary[27]^=1;
			assert(!PioneerQ38ResidentGdnRedistributedServiceSummaries(gated,input,state,
				first,recurrent,gatedSummary,clock,before,after));
			gatedSummary[27]^=1;proof[22]^=1;
			assert(!PioneerQ38ResidentGdnRedistributedServiceProof(proof,job,gated,input,state,
				source,route.physicalWorker,route.peer,before,after,first,recurrent,gatedSummary,clock));
			proof[22]^=1;
			gated[15] ^= 1;
			assert(!PioneerQ38GdnRedistributedGatedShape(gated, input, state));
			gated[15] ^= 1;
			input[9] ^= 1;
			assert(!PioneerQ38GdnRedistributedInputShape(input));
			assert(!PioneerQ38GdnRedistributedPairing(input, state));
			input[9] ^= 1;

			uint32_t d[64] = {};
			d[0] = TT_Q38_RGDN_MARKER; d[1] = 1; d[2] = 128; d[3] = 64;
			d[4] = 0; d[5] = 0x04000000; d[6] = 6;
			d[7] = 0xc4000000 - TT_Q38_RGDN_STATE_BYTES;
			d[8] = 1; d[9] = logical; d[10] = route.actualLayer;
			d[12] = PioneerQ38SpanRoute(d[4]); d[13] = PioneerQ38SpanRoute(d[6]);
			d[14] = TT_Q38_RGDN_STATE_BYTES; d[15] = route.peer; d[16] = 1;
			for (unsigned epoch = 32; epoch < 40; epoch += 2)
				d[epoch] = 1;
			assert(PioneerQ38ResidentGdnRedistributedShape(d));

			uint32_t savedPeer = d[15];
			d[15] = PioneerQ38ResidentGdnPeer(logical);
			assert(PioneerQ38ResidentGdnRedistributedShape(d)
				== (route.physicalWorker == logical + 1));
			d[15] = savedPeer;
			d[10] = route.actualLayer + 1;
			assert(!PioneerQ38ResidentGdnRedistributedShape(d));
			d[10] = route.actualLayer;
			d[15] ^= 1;
			assert(!PioneerQ38ResidentGdnRedistributedShape(d));
		}
	}
	for (unsigned peer = 0; peer < PioneerQ38GdnTaskPlacement::PhysicalWorkers; ++peer)
		assert(stateUse[peer] == PioneerQ38GdnTaskPlacement::StateSlots(peer + 1));
	assert(PioneerQ38GdnTaskPlacement::Peer(0) == ~uint32_t(0));
	assert(PioneerQ38GdnTaskPlacement::Peer(120) == ~uint32_t(0));
	PioneerQ38GdnTaskPlacement::Route invalid = {};
	assert(!PioneerQ38GdnTaskPlacement::Map(48, 0, invalid));
	assert(!PioneerQ38GdnTaskPlacement::Map(0, 96, invalid));
	return 0;
#endif
}
