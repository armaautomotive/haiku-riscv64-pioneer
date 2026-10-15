/* SPDX-License-Identifier: MIT */
#ifndef TENSTORRENT_GGUF_TENSOR_H
#define TENSTORRENT_GGUF_TENSOR_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
// Read-only GGUFv3 subset, matching tenstorrent_gguf_preflight.py. No
// chat-template evaluation, executable formats or unrestricted allocation.
struct PioneerGgufTensor { uint64_t inputs,outputs,offset,bytes; uint32_t type; };
struct PioneerGgufReader {
	FILE* file; uint64_t size,position=0; bool good=true;
	PioneerGgufReader(FILE* input,uint64_t length):file(input),size(length) {}
	bool Take(void* out,uint64_t bytes) {
		if (!good || bytes>size-position || bytes>32*1024*1024-position) return good=false;
		if (fread(out,1,size_t(bytes),file)!=bytes) return good=false;
		position+=bytes; return true;
	}
	uint64_t Number(unsigned bytes) {
		uint8_t b[8]={}; if (bytes>8 || !Take(b,bytes)) return 0;
		uint64_t value=0; for (unsigned i=0;i<bytes;i++) value|=uint64_t(b[i])<<(8*i); return value;
	}
	bool Skip(uint64_t bytes) {
		if (!good || bytes>size-position || bytes>32*1024*1024-position || fseeko(file,off_t(bytes),SEEK_CUR)) return good=false;
		position+=bytes; return true;
	}
	bool String(char* buffer=NULL,size_t capacity=0) {
		uint64_t bytes=Number(8);
		if (!good || bytes>32*1024*1024) return good=false;
		if (!buffer) return Skip(bytes);
		if (bytes>=capacity || !Take(buffer,bytes)) return good=false;
		buffer[bytes]=0; return true;
	}
	unsigned Scalar(uint32_t type) {
		const unsigned bytes[]={1,1,2,2,4,4,4,1,0,0,8,8,8}; return type<13?bytes[type]:0;
	}
	bool Value(uint32_t type) {
		unsigned scalar=Scalar(type); if (scalar) return Skip(scalar);
		if (type==8) return String();
		if (type!=9) return good=false;
		uint32_t element=uint32_t(Number(4)); uint64_t count=Number(8);
		if (!good || count>1000000 || element==9) return good=false;
		scalar=Scalar(element); if (scalar) return Skip(count*scalar);
		if (element!=8) return good=false;
		for (uint64_t i=0;i<count;i++) if (!String()) return false;
		return true;
	}
};
static inline bool PioneerFindGgufTensor(FILE* file,const char* wanted,PioneerGgufTensor& tensor)
{
	struct stat st={}; if (fstat(fileno(file),&st) || st.st_size<24 || fseeko(file,0,SEEK_SET)) return false;
	PioneerGgufReader r(file,uint64_t(st.st_size));
	if (r.Number(4)!=0x46554747 || r.Number(4)!=3) return false;
	uint64_t tensors=r.Number(8),metadata=r.Number(8); uint32_t alignment=32;
	if (!r.good || tensors>100000 || metadata>100000) return false;
	for (uint64_t i=0;i<metadata;i++) {
		char key[512]; if (!r.String(key,sizeof(key))) return false;
		uint32_t type=uint32_t(r.Number(4));
		if (!strcmp(key,"general.alignment")) { if (type!=4) return false; alignment=uint32_t(r.Number(4)); }
		else if (!r.Value(type)) return false;
	}
	if (!alignment || alignment>4096 || (alignment&(alignment-1))) return false;
	bool found=false;
	for (uint64_t i=0;i<tensors;i++) {
		char name[1025]; if (!r.String(name,sizeof(name))) return false;
		uint32_t rank=uint32_t(r.Number(4)); if (!rank || rank>4) return false;
		uint64_t dims[4]={};
		for (unsigned d=0;d<rank;d++) { dims[d]=r.Number(8); if (!dims[d] || dims[d]>=UINT64_C(0x100000000)) return false; }
		uint32_t type=uint32_t(r.Number(4)); uint64_t offset=r.Number(8);
		if (!r.good || offset%alignment) return false;
		if (!strcmp(name,wanted)) {
			if (found || rank!=2 || type!=8 || dims[0]%32 || dims[1]>UINT64_MAX/(dims[0]/32)/34) return false;
			tensor={dims[0],dims[1],offset,(dims[0]/32)*34*dims[1],type}; found=true;
		}
	}
	uint64_t start=(r.position+alignment-1)&~uint64_t(alignment-1);
	if (!r.good || !found || start>r.size || tensor.offset>r.size-start || tensor.bytes>r.size-start-tensor.offset) return false;
	tensor.offset+=start; return true;
}
#endif
