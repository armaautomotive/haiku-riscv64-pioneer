/* SPDX-License-Identifier: MIT */
// CPU-only fixed PUBLIC model inventory. NEVER opens an accelerator FD,
// imports driver protocol,uploads,executes learned products or mutates model.
#define PIONEER_Q38_Q8_DOMAIN_DIAGNOSTIC
#include "PioneerQ38PublicModelSource.h"
#include <cstdio>
#include <cstring>
int main(int argc,char** argv) {
	if(argc!=2 || strcmp(argv[1],"--public-q8-byte-domain"))return 2;
	setvbuf(stdout,nullptr,_IOLBF,0);
	puts("PUBLIC_Q8_DOMAIN_AUTH_BEGIN;FIXED_PUBLIC_SOURCE_FULL_SHA_REQUIRED;CPU_FILE_READ_ONLY_NO_ACCELERATOR_OR_LEARNED_PRODUCT");
	PioneerQ38PublicModelSource source;if(!source.Open()){puts("STOP_PUBLIC_SOURCE_AUTH;NO_ACCELERATOR_FD_EVER");return 1;}
	puts("PUBLIC_Q8_DOMAIN_AUTHENTICATED_C600DE03;SAME_FILE_DESCRIPTION_IDENTITY_AND_EPOCH_RECHECKED_EACH_CHUNK");
	std::vector<uint8_t> chunk(34*16384);uint64_t counts[256]={},bytesRead=0,blocks=0,nonfinite=0;
	unsigned tensors=0;uint64_t nextProgress=UINT64_C(512)*1024*1024;
	const auto& model=source.Model();
	for(unsigned tensor=0;tensor<model.Tensors().size();tensor++) {
		const auto& t=model.Tensors()[tensor];if(t.type!=8 || t.rank!=2)continue;
		tensors++;
		for(uint64_t offset=0;offset<t.bytes;) {
			unsigned bytes=unsigned(t.bytes-offset<chunk.size()?t.bytes-offset:chunk.size());
			if(!source.ReadQ8DomainChunk(tensor,offset,bytes,chunk.data(),unsigned(chunk.size()))){puts("STOP_Q8_SOURCE_CHANGED_OR_READ_FAILED;NO_CARD_ACTIVITY");return 1;}
			for(unsigned at=0;at<bytes;at+=34) {
				unsigned half=unsigned(chunk[at])|(unsigned(chunk[at+1])<<8);nonfinite+=((half>>10)&31)==31;blocks++;
				for(unsigned k=0;k<32;k++)counts[chunk[at+2+k]]++;
			}
			offset+=bytes;bytesRead+=bytes;
			if(bytesRead>=nextProgress){printf("PUBLIC_Q8_DOMAIN_PROGRESS bytes%llu minus128%llu;READ_ONLY_NOT_MODEL_SPEED\n",(unsigned long long)bytesRead,(unsigned long long)counts[128]);nextProgress+=UINT64_C(512)*1024*1024;}
		}
	}
	uint64_t values=0;int minimum=127,maximum=-128;
	for(unsigned byte=0;byte<256;byte++)if(counts[byte]) {
		values+=counts[byte];int value=int(byte)-int((byte&128)*2);
		if(value<minimum)minimum=value;
		if(value>maximum)maximum=value;
	}
	if(!source.Ready() || !tensors || values!=blocks*32 || bytesRead!=blocks*34){puts("STOP_Q8_DOMAIN_INCOMPLETE;NO_CARD_ACTIVITY");return 1;}
	printf("PASS_PUBLIC_Q8_DOMAIN tensors%u original_bytes%llu blocks%llu quant_values%llu minimum%d maximum%d minus128%llu nonfinite_half_scales%llu sm7_domain_eligible%u;AUTHENTICATED_BYTE_INVENTORY_ONLY_NOT_FORMAT_IMAGE_NATIVE_INFERENCE_OR_SPEED_QUALIFICATION\n",tensors,(unsigned long long)bytesRead,(unsigned long long)blocks,(unsigned long long)values,minimum,maximum,(unsigned long long)counts[128],(unsigned long long)nonfinite,unsigned(!counts[128] && !nonfinite));
	return 0;
}
