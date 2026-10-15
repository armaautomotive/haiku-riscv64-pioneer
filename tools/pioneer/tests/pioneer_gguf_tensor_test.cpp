/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <vector>
#include "../TenstorrentGgufTensor.h"
static void Number(std::vector<uint8_t>& b,uint64_t value,unsigned size) { for (unsigned i=0;i<size;i++) b.push_back(value>>(8*i)); }
static void String(std::vector<uint8_t>& b,const char* value) { Number(b,strlen(value),8); for (const char* p=value;*p;p++) b.push_back(*p); }
static std::vector<uint8_t> Model() {
	std::vector<uint8_t> b; Number(b,0x46554747,4); Number(b,3,4); Number(b,1,8); Number(b,1,8);
	String(b,"general.alignment"); Number(b,4,4); Number(b,32,4);
	String(b,"blk.0.attn_q.weight"); Number(b,2,4); Number(b,1024,8); Number(b,32,8); Number(b,8,4); Number(b,0,8);
	while (b.size()%32) b.push_back(0);
	b.resize(b.size()+32*32*34); return b;
}
static bool Parse(const std::vector<uint8_t>& b) {
	FILE* f=tmpfile(); assert(f); assert(fwrite(b.data(),1,b.size(),f)==b.size()); fflush(f);
	PioneerGgufTensor t={}; bool valid=PioneerFindGgufTensor(f,"blk.0.attn_q.weight",t);
	if (valid) assert(t.inputs==1024 && t.outputs==32 && t.type==8 && t.bytes==32*32*34);
	fclose(f); return valid;
}
int main() {
	auto b=Model(); assert(Parse(b));
	for (unsigned bytes:{0U,8U,23U,24U,50U,100U}) { auto shortFile=b; shortFile.resize(bytes); assert(!Parse(shortFile)); }
	for (unsigned offset:{0U,4U,8U,16U}) { auto bad=b; for (unsigned i=0;i<8 && offset+i<bad.size();i++) bad[offset+i]=255; assert(!Parse(bad)); }
	auto truncated=b; truncated.pop_back(); assert(!Parse(truncated));
	const size_t metadataValue=24+8+strlen("general.alignment")+4;
	const size_t rank=metadataValue+4+8+strlen("blk.0.attn_q.weight");
	for (size_t offset:{size_t(24),metadataValue,rank,rank+4,rank+20,rank+24}) {
		auto bad=b; bad[offset]=offset==metadataValue?3:offset==rank?5:255;
		assert(!Parse(bad));
	}
	FILE* f=tmpfile(); assert(f); assert(fwrite(b.data(),1,b.size(),f)==b.size()); fflush(f);
	PioneerGgufTensor t={}; assert(!PioneerFindGgufTensor(f,"missing.weight",t)); fclose(f);
}
