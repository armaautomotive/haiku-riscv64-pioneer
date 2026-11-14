/* SPDX-License-Identifier: MIT */
// Returning command DATA builder for a future on-card ROOT;not an image,
// native owner,physical observation or permission to submit these commands.
#define PIONEER_ATTENTION_FREESTANDING
#include "../PioneerAttentionMath.h"
#include "../PioneerQ38MixedRecurrentDispatch.h"
#include "../PioneerQ38GlobalRecurrentDispatch.h"
extern "C" void* memset(void* target,int value,__SIZE_TYPE__ bytes){
	volatile unsigned char* out=static_cast<volatile unsigned char*>(target);
	for(__SIZE_TYPE__ i=0;i<bytes;i++)out[i]=static_cast<unsigned char>(value);
	return target;
}
extern "C" bool q38_recurrent_conv_command(const uint32_t* boot,const uint32_t* table,unsigned previous,uint32_t* output){
 return PioneerQ38MixedRecurrentDispatch::Conv(boot,table,previous,output);
}
extern "C" bool q38_recurrent_gdn_command(const uint32_t* boot,const uint32_t* input,const uint32_t* state,
 const uint32_t* gated,unsigned previous,uint32_t* output){
 return PioneerQ38MixedRecurrentDispatch::Gdn(boot,input,state,gated,previous,output);
}
extern "C" bool q38_recurrent_gdn_redistributed_command(const uint32_t* boot,const uint32_t* input,
 const uint32_t* state,const uint32_t* gated,unsigned previous,uint32_t* output){
 return PioneerQ38MixedRecurrentDispatch::GdnRedistributed(boot,input,state,gated,previous,output);
}
extern "C" bool q38_global_recurrent_gdn_redistributed_command(const uint32_t* boot,const uint32_t* input,
 const uint32_t* state,const uint32_t* gated,unsigned previous,uint32_t* output){
 return PioneerQ38GlobalRecurrentDispatch::GdnRedistributed(boot,input,state,gated,previous,output);
}
