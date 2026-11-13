/* SPDX-License-Identifier: MIT */
#include "../PioneerModelWeightCache.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
using Cache=PioneerModelWeightCache;
static unsigned cases=0;
static void Exact(Cache::Reader& r,const std::vector<uint8_t>& expected) {
	assert(r.IsOpen() && r.Size()==expected.size());
	for(uint64_t offset=0;offset<expected.size();) {
		unsigned count=unsigned(std::min(uint64_t(17011),expected.size()-offset));
		std::vector<uint8_t> output(count+8,0xd3);std::string error;
		assert(r.Read(offset,count,output.data()+3,count,error));
		assert(output[0]==0xd3 && output[2]==0xd3 && output[count+3]==0xd3);
		assert(!memcmp(output.data()+3,expected.data()+offset,count));offset+=count;
	}
	cases++;
}
static unsigned Staging(const std::string& directory) {
	DIR* d=opendir(directory.c_str());assert(d);unsigned count=0;
	while(auto* e=readdir(d))if(!strncmp(e->d_name,".p100a-building-",16))count++;
	assert(!closedir(d));return count;
}
static void Alter(const std::string& path,uint64_t offset,uint8_t delta) {
	assert(!chmod(path.c_str(),0600));int fd=open(path.c_str(),O_RDWR|O_NOFOLLOW);assert(fd>=0);uint8_t value;
	assert(pread(fd,&value,1,off_t(offset))==1);value^=delta;
	assert(pwrite(fd,&value,1,off_t(offset))==1 && !fsync(fd) && !close(fd) && !chmod(path.c_str(),0400));
}
int main(int argc,char** argv) {
	assert(argc==2);const std::string base=argv[1],directory=base+"/weights";std::string error;
	unsigned unsupported=0;
	assert(Cache::Hex(Cache::Hash(nullptr,0))=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	assert(Cache::Hex(Cache::Hash("abc",3))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	{
		Cache missing;const std::string name=base+"/absent";struct stat st{};
		assert(!missing.OpenExistingDirectory(name.c_str(),error) && lstat(name.c_str(),&st) && errno==ENOENT);
		const std::string unmarked=base+"/existing-only-unmarked";assert(!mkdir(unmarked.c_str(),0700));
		Cache unsafe;assert(!unsafe.OpenExistingDirectory(unmarked.c_str(),error));
		const std::string marker=unmarked+"/.p100a-weights-v1";
		assert(lstat(marker.c_str(),&st) && errno==ENOENT);
	}
	Cache cache;assert(cache.Open(directory.c_str(),error));
	{
		Cache existing;assert(existing.OpenExistingDirectory(directory.c_str(),error));
		const std::string linkpath=base+"/cache-link";assert(!symlink(directory.c_str(),linkpath.c_str()));
		Cache linked;assert(!linked.OpenExistingDirectory(linkpath.c_str(),error));
	}
	Cache::Key key;const char model[]="fully-authenticated-fixture-model",recipe[]="experimental-native-bfp4b-v1";
	key.sourceSHA256=Cache::Hash(model,sizeof(model)-1);
	key.conversionSHA256=Cache::Hash(recipe,sizeof(recipe)-1);key.entry=7;key.bytes=Cache::ChunkBytes*3+17;
	std::vector<uint8_t> original(size_t(key.bytes));
	for(size_t i=0;i<original.size();i++)original[i]=uint8_t((i*71)^(i>>8)^(i>>16));
	unsigned produced=0;bool ready=true,hit=true;
	auto produce=[&](uint64_t offset,unsigned count,uint8_t* out,std::string&) {
		assert(offset<original.size() && count<=original.size()-offset);produced++;
		memcpy(out,original.data()+offset,count);return true;
	};
	auto sourceReady=[&] { return ready; };
	const std::string path=directory+"/"+Cache::EntryName(key);
	{
		Cache::Reader r;assert(cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && !hit && produced==4);Exact(r,original);
		assert(!Staging(directory));struct stat st{};assert(!lstat(path.c_str(),&st) && (st.st_mode&0777)==0400 && st.st_nlink==1);
	}
	{
		Cache::Reader r;const unsigned before=produced;
		assert(cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && hit && produced==before);Exact(r,original);
		// Exercise the aligned whole-chunk fast path, including the short
		// final chunk, and an unaligned read spanning two chunks.
		for(uint64_t offset:{UINT64_C(0),uint64_t(Cache::ChunkBytes),uint64_t(Cache::ChunkBytes)*2,uint64_t(Cache::ChunkBytes)*3}) {
			const unsigned count=unsigned(std::min(uint64_t(Cache::ChunkBytes),key.bytes-offset));
			std::vector<uint8_t> exact(count+2,0xa7);
			assert(r.Read(offset,count,exact.data()+1,count,error));
			assert(exact.front()==0xa7 && exact.back()==0xa7 && !memcmp(exact.data()+1,original.data()+offset,count));
		}
		const uint64_t offset=Cache::ChunkBytes-7;std::vector<uint8_t> crossing(22,0xa7);
		assert(r.Read(offset,20,crossing.data()+1,20,error));
		assert(crossing.front()==0xa7 && crossing.back()==0xa7 && !memcmp(crossing.data()+1,original.data()+offset,20));
		std::vector<uint8_t> output(32,0xa7);const auto prior=output;
		assert(!r.Read(key.bytes-1,2,output.data(),32,error) && output==prior);
		assert(!r.Read(0,0,output.data(),32,error) && !r.Read(0,33,output.data(),32,error));cases++;
	}
	// The exact on-disk schema includes key/source/recipe/reserved bytes,
	// whole payload SHA and a SHA-authenticated per64KiB chunk table.
	for(uint64_t where:{UINT64_C(0),UINT64_C(8),UINT64_C(48),UINT64_C(80),UINT64_C(112),UINT64_C(144),UINT64_C(255),UINT64_C(256),UINT64_C(256+128),UINT64_C(256+128)+key.bytes-1}) {
		Alter(path,where,0x40);Cache::Reader r;const unsigned before=produced;
		assert(cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && !hit && produced==before+4);Exact(r,original);
		assert(!Staging(directory));cases++;
	}
	{
		Cache::Reader r;assert(cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && hit);
		Alter(path,256+128+Cache::ChunkBytes+5,1);
		std::vector<uint8_t> output(32,0x79);const auto prior=output;
		assert(!r.Read(Cache::ChunkBytes-8,32,output.data(),output.size(),error) && output==prior);cases++;
	}
	{
		// A failed rebuild MUST leave the previous (corrupt) file unchanged.
		struct stat before{},after{};assert(!lstat(path.c_str(),&before));
		unsigned calls=0;Cache::Reader r;
		auto fail=[&](uint64_t,unsigned,uint8_t*,std::string&) { return ++calls<3; };
		assert(!cache.GetOrCreate(key,sourceReady,fail,r,hit,error) && !r.IsOpen() && !Staging(directory));
		assert(!lstat(path.c_str(),&after) && before.st_ino==after.st_ino && before.st_size==after.st_size);cases++;
	}
	{
		Cache::Reader r;assert(cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && !hit);Exact(r,original);
	}
	for(unsigned change=0;change<3;change++) {
		auto other=key;
		if(!change)other.sourceSHA256[0]^=1;else if(change==1)other.conversionSHA256[31]^=1;else other.entry++;
		assert(Cache::EntryName(other)!=Cache::EntryName(key));Cache::Reader r;
		assert(cache.GetOrCreate(other,sourceReady,produce,r,hit,error) && !hit);Exact(r,original);cases++;
	}
	{
		ready=false;Cache::Reader r;const unsigned before=produced;
		assert(!cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && produced==before && !r.IsOpen());ready=true;cases++;
	}
	for(unsigned unsafe=0;unsafe<3;unsafe++) {
		auto other=key;other.entry=100+unsafe;const std::string target=directory+"/"+Cache::EntryName(other);
		if(!unsafe)assert(!symlink(path.c_str(),target.c_str()));
		else if(unsafe==1) {
			// BFS does not implement hard links. Do not pretend that its
			// inability to create this fixture exercises cache rejection.
			if(link(path.c_str(),target.c_str())) {
				assert(errno==EPERM || errno==EOPNOTSUPP || errno==ENOTSUP || errno==ENOSYS);
				puts("UNSUPPORTED_FIXTURE hard links unavailable on this filesystem;hard-link cache refusal NOT exercised here");
				unsupported++;continue;
			}
		}
		else assert(!mkfifo(target.c_str(),0600));
		Cache::Reader r;const unsigned before=produced;
		assert(!cache.GetOrCreate(other,sourceReady,produce,r,hit,error) && produced==before && !r.IsOpen());
		assert(!unlink(target.c_str()));cases++;
	}
	{
		assert(!chmod(path.c_str(),0644));Cache::Reader r;const unsigned before=produced;
		assert(!cache.GetOrCreate(key,sourceReady,produce,r,hit,error) && produced==before);
		assert(!chmod(path.c_str(),0400));cases++;
	}
	{
		auto other=key;other.entry=200;other.bytes=Cache::MaxEntryBytes+1;Cache::Reader r;
		assert(!cache.GetOrCreate(other,sourceReady,produce,r,hit,error) && !Staging(directory));cases++;
	}
	{
		auto other=key;other.entry=201;Cache::Reader r;
		auto cancelVerification=[&](uint64_t,uint64_t,bool verifying) { if(verifying)ready=false; };
		assert(!cache.GetOrCreate(other,sourceReady,produce,r,hit,error,cancelVerification) && !r.IsOpen());
		ready=true;assert(!Staging(directory));cases++;
	}
	{
		const std::string unmarked=base+"/unmarked";assert(!mkdir(unmarked.c_str(),0700));Cache other;
		assert(!other.Open(unmarked.c_str(),error));cases++;
	}
	{
		const std::string linked=base+"/linked";assert(!symlink(directory.c_str(),linked.c_str()));Cache other;
		assert(!other.Open(linked.c_str(),error));cases++;
	}
	{
		Cache reopened;assert(reopened.Open(directory.c_str(),error));Cache::Reader r;const unsigned before=produced;
		assert(reopened.GetOrCreate(key,sourceReady,produce,r,hit,error) && hit && produced==before);Exact(r,original);
	}
	{
		Cache::Reader r;const unsigned before=produced;
		assert(cache.OpenExisting(key,sourceReady,r,error) && produced==before);Exact(r,original);
		auto absent=key;absent.entry=999;Cache::Reader missing;
		assert(!cache.OpenExisting(absent,sourceReady,missing,error) && !missing.IsOpen() && !Staging(directory));
		ready=false;Cache::Reader revoked;assert(!cache.OpenExisting(key,sourceReady,revoked,error));ready=true;cases++;
	}
	{
		Cache::Reader fullyVerified;Cache::VerifiedEntry receipt;
		assert(cache.OpenExisting(key,sourceReady,fullyVerified,error));
		assert(cache.RememberVerified(fullyVerified,receipt,error));
		assert(!cache.RememberVerified(fullyVerified,receipt,error));
		Cache other;assert(other.OpenExistingDirectory(directory.c_str(),error));
		Cache::VerifiedEntry foreignReceipt;assert(!other.RememberVerified(fullyVerified,foreignReceipt,error));
		Cache::Reader foreign;assert(!other.OpenRemembered(receipt,sourceReady,foreign,error));
		Cache::Reader reopened;assert(cache.OpenRemembered(receipt,sourceReady,reopened,error));Exact(reopened,original);
		ready=false;Cache::Reader revoked;assert(!cache.OpenRemembered(receipt,sourceReady,revoked,error));ready=true;
		Alter(path,256+128+Cache::ChunkBytes+5,1);
		Cache::Reader changed;assert(!cache.OpenRemembered(receipt,sourceReady,changed,error) && !changed.IsOpen());
		std::vector<uint8_t> result(32,0x6d);const auto prior=result;
		assert(!reopened.Read(Cache::ChunkBytes,32,result.data(),result.size(),error) && result==prior);
		Cache::Reader rebuilt;assert(cache.GetOrCreate(key,sourceReady,produce,rebuilt,hit,error) && !hit);
		Cache::Reader replaced;assert(!cache.OpenRemembered(receipt,sourceReady,replaced,error));cases++;
	}
	{
		const std::string originalPath=base+"/original-fixture";
		int fd=open(originalPath.c_str(),O_RDWR|O_CREAT|O_EXCL,0600);assert(fd>=0);
		assert(write(fd,original.data(),original.size())==ssize_t(original.size()) && lseek(fd,123,SEEK_SET)==123);
		uint64_t observed=0;unsigned callbacks=0;
		auto progress=[&](uint64_t done,uint64_t total,bool verifying) {
			assert(!verifying && total==original.size() && done>observed && done<=total);
			observed=done;callbacks++;
		};
		Cache::Digest hashed{};assert(Cache::HashFile(fd,original.size(),hashed,error,progress) && hashed==Cache::Hash(original.data(),original.size()));
		assert(observed==original.size() && callbacks==1);
		assert(lseek(fd,0,SEEK_CUR)==123);const auto prior=hashed;
		assert(!Cache::HashFile(fd,original.size()-1,hashed,error) && hashed==prior);
		auto change=[&](uint64_t,uint64_t,bool) { uint8_t byte=17;assert(pwrite(fd,&byte,1,0)==1 && !fsync(fd)); };
		assert(!Cache::HashFile(fd,original.size(),hashed,error,change) && hashed==prior);
		assert(!close(fd));cases++;
	}
	{
		// A stream that ends after fstat must fail without publishing a hash
		// for the prefix. This models a short source read; it does not emulate
		// the Linux BeFS implementation or establish filesystem health.
		const std::string path=base+"/short-hash-fixture";
		int fd=open(path.c_str(),O_RDWR|O_CREAT|O_EXCL,0600);assert(fd>=0);
		const off_t size=off_t(1024*1024+32);assert(!ftruncate(fd,size));
		Cache::Digest hashed=Cache::Hash("unchanged",9);const auto before=hashed;unsigned callbacks=0;
		auto truncate=[&](uint64_t done,uint64_t total,bool verifying){
			assert(!verifying && done==1024*1024 && total==uint64_t(size));
			callbacks++;assert(!ftruncate(fd,off_t(done)));
		};
		assert(!Cache::HashFile(fd,uint64_t(size),hashed,error,truncate) && hashed==before && callbacks==1);
		assert(!close(fd));cases++;
	}
	printf("PASS CPU-only converted-weight disk cache:%u checks;%u unsupported filesystem fixtures;verified hits ZERO conversion,full/chunk SHA,source/recipe/version keys,atomic rebuild/cancellation,private marked directory,supported link/FIFO/permission refusals;NO GPU or model-speed claim\n",cases,unsupported);
}
