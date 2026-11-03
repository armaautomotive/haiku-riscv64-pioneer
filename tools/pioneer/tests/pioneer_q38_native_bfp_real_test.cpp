/* SPDX-License-Identifier: MIT */
// OFFBOARD ONLY:learned weight tiles with SYNTHETIC BF16 activations,not a
// full projection,prompt,generation,model-quality acceptance or device run.
#define PIONEER_NATIVE_BFP_IMAGE_ONLY
#include "pioneer_q38_native_bfp_image_test.cpp"
#include "../PioneerQ38NativeWeightTiles.h"
#include <limits>
#include <memory>
#include <sys/stat.h>
static bool SameFile(const struct stat& a,const struct stat& b) {
	if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime)return false;
#if defined(__APPLE__)
	return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
	return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
int main(int argc,char** argv) {
	assert(argc==5);setvbuf(stdout,nullptr,_IONBF,0);
	std::unique_ptr<FILE,decltype(&fclose)> file(fopen(argv[3],"rb"),&fclose);assert(file);
	struct stat identity{};assert(!fstat(fileno(file.get()),&identity) && S_ISREG(identity.st_mode));
	PioneerQwen38Gguf source;assert(source.Open(file.get()));const auto epoch=source.Epoch();
	auto ready=[&] { struct stat now{};return source.Ready() && source.Stable() && source.Epoch()==epoch
		&& !fstat(fileno(file.get()),&now) && SameFile(identity,now); };
	using Cache=PioneerModelWeightCache;Cache::Digest digest{};std::string error;
	puts("AUTHENTICATING_SAME_OPEN_851_TENSOR_ORIGINAL_NO_ACCELERATOR_ACCESS");
	assert(Cache::HashFile(fileno(file.get()),PioneerQwen38Gguf::FullBytes,digest,error) && ready());
	assert(Cache::Hex(digest)=="c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747");
	Cache cache;assert(cache.Open(argv[4],error));
	const char recipe[]="OFFBOARD_LEARNED_TILE_NATIVE_BFP_MATVEC_FIXTURE_v1_NOT_A_RUNTIME_CACHE_KEY";
	for(unsigned type:{8U,12U,14U}) {
		unsigned index=0;PioneerQ38NativeWeightTiles::Layout layout;
		for(;index<source.Tensors().size();index++)if(source.Tensors()[index].type==type
			&& PioneerQ38NativeWeightTiles::Describe(source.Tensors()[index],layout))break;
		assert(index<source.Tensors().size());const unsigned bits=type==12?4:8,bytes=Kernel::WeightBytes(bits);
		const uint64_t tiles=layout.bytes/bytes;
		for(uint64_t tile:{UINT64_C(0),tiles/2,tiles-1}) {
			assert(tile<=UINT32_MAX);Cache::Key key;key.sourceSHA256=digest;key.conversionSHA256=Cache::Hash(recipe,sizeof(recipe)-1);
			key.entry=(uint64_t(index)<<32)|tile;key.bytes=bytes;
			unsigned conversions=0;auto produce=[&](uint64_t offset,unsigned count,uint8_t* out,std::string&) {
				conversions++;return ready() && PioneerQ38NativeWeightTiles::ReadRange(source,index,tile*bytes+offset,count,out,count) && ready();
			};
			std::array<float,1024> original;assert(PioneerQ38NativeWeightTiles::ReadFloatTile(source,index,tile,original.data(),original.size()) && ready());
			std::array<uint16_t,32> input;
			for(unsigned c=0;c<32;c++)input[c]=uint16_t(0x3f00+((c*13+21)%256)+((c&3)==2?0x8000:0));
			for(unsigned pass=0;pass<2;pass++) {
				Cache::Reader reader;bool hit=false;assert(cache.GetOrCreate(key,ready,produce,reader,hit,error) && hit==bool(pass));
				assert(conversions==1);std::array<uint8_t,1088> packed;assert(reader.Read(0,bytes,packed.data(),packed.size(),error));
				std::array<float,1024> decoded;assert(Codec::UnpackTile(Codec::Format(bits),packed.data(),bytes,decoded.data(),decoded.size()));
				NativeBfpRV32 image(argv[bits==4?1:2]);image.native.Stage(packed.data(),bytes,input);assert(image.Execute());
				long double originalNorm=0,quantError=0,maxModelDelta=0,maxOracleDelta=0;
				for(unsigned row=0;row<32;row++) {
					long double cached=0,learned=0,sumAbs=0;
					for(unsigned c=0;c<32;c++) { const long double v=NativeBfpMachine::Float(uint32_t(input[c])<<16);
						const long double product=decoded[row*32+c]*v;cached+=product;sumAbs+=std::fabs(product);learned+=original[row*32+c]*v; }
					const float actual=Float(image.l1[image.Index(0x92000+row*4)]);
					assert(std::isfinite(actual) && image.l1[image.Index(0x92080+row*4)]==Bits(actual));
					// Separate,PREDECLARED floating accumulation bound for this NEW
					// representation/observer;NOT the original model's quality gate.
					const long double tolerance=8*std::numeric_limits<float>::epsilon()*sumAbs+std::numeric_limits<float>::min();
					assert(std::fabs(static_cast<long double>(actual)-cached)<=tolerance);
					maxOracleDelta=std::max(maxOracleDelta,std::fabs(static_cast<long double>(actual)-cached));
					const long double delta=cached-learned;quantError+=delta*delta;originalNorm+=learned*learned;maxModelDelta=std::max(maxModelDelta,std::fabs(delta));
				}
				printf("OFFBOARD_LEARNED_TILE type%u tensor%u %s tile%llu bits%u cache%s conversions%u vs_cached_oracle_maxabs%.9Lg vs_original_Q_weight_L2%.9Lg maxabs%.9Lg;SYNTHETIC_BF16_INPUT_NOT_MODEL_QUALITY_OR_HARDWARE\n",
					type,index,source.Tensors()[index].name.c_str(),(unsigned long long)tile,bits,hit?"HIT":"CREATE",conversions,maxOracleDelta,
					std::sqrt(quantError/(originalNorm?originalNorm:1)),maxModelDelta);checked++;
			}
		}
	}
	assert(ready());printf("PASS_OFFBOARD_REAL_CACHED_WEIGHTS_TO_ACTUAL_RV32_BFP_IMAGES %u tile/load checks;all3 original quantizations/sameFD c600 SHA/851layout/first-middle-last tiles/cachehit ZERO callback conversion;NOT fullK,model quality,device ownership,hardware inference or timing\n",checked);
}
