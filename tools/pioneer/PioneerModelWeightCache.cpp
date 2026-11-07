/* SPDX-License-Identifier: MIT */
#include "PioneerModelWeightCache.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#if defined(__HAIKU__)
#include <SHA256.h>
namespace { struct HashState {
	SHA256 value;
	void Update(const void* p,size_t n) { value.Update(p,n); }
	PioneerModelWeightCache::Digest Final() { PioneerModelWeightCache::Digest d;std::memcpy(d.data(),value.Digest(),32);return d; }
}; }
#elif defined(__APPLE__)
#include <CommonCrypto/CommonDigest.h>
namespace { struct HashState {
	CC_SHA256_CTX value;
	HashState() { CC_SHA256_Init(&value); }
	void Update(const void* p,size_t n) {
		const auto* at=static_cast<const uint8_t*>(p);
		while(n) { unsigned count=unsigned(std::min(n,size_t(1)<<30));CC_SHA256_Update(&value,at,count);at+=count;n-=count; }
	}
	PioneerModelWeightCache::Digest Final() { PioneerModelWeightCache::Digest d;CC_SHA256_Final(d.data(),&value);return d; }
}; }
#else
extern "C" {
#include "../../src/libs/libsolv/solv/sha2.h"
}
namespace { struct HashState {
	SHA256_CTX value;
	HashState() { solv_SHA256_Init(&value); }
	void Update(const void* p,size_t n) { solv_SHA256_Update(&value,static_cast<const uint8_t*>(p),n); }
	PioneerModelWeightCache::Digest Final() { PioneerModelWeightCache::Digest d;solv_SHA256_Final(d.data(),&value);return d; }
}; }
#endif
namespace {
using Cache=PioneerModelWeightCache;
using Digest=Cache::Digest;
constexpr char Marker[]="Pioneer derived model weights cache v1\n";
bool Fail(std::string& error,const char* text) { error=text;return false; }
bool Nonzero(const Digest& d) { return std::any_of(d.begin(),d.end(),[](uint8_t v){return v!=0;}); }
bool Valid(const Cache::Key& k) { return k.bytes && k.bytes<=Cache::MaxEntryBytes && Nonzero(k.sourceSHA256) && Nonzero(k.conversionSHA256); }
uint64_t Chunks(const Cache::Key& k) { return (k.bytes+Cache::ChunkBytes-1)/Cache::ChunkBytes; }
uint64_t PayloadOffset(const Cache::Key& k) { return Cache::HeaderBytes+Chunks(k)*32; }
void Put(uint8_t* p,uint64_t value,unsigned bytes) { for(unsigned i=0;i<bytes;i++)p[i]=uint8_t(value>>(i*8)); }
bool IO(int fd,void* data,size_t bytes,uint64_t offset,bool writing) {
	auto* at=static_cast<uint8_t*>(data);
	while(bytes) {
		ssize_t n=writing?pwrite(fd,at,bytes,off_t(offset)):pread(fd,at,bytes,off_t(offset));
		if(n<0 && errno==EINTR)continue;
		if(n<=0)return false;
		at+=n;bytes-=size_t(n);offset+=uint64_t(n);
	}
	return true;
}
bool Same(const struct stat& a,const struct stat& b) {
	if(a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime
		|| a.st_mode!=b.st_mode || a.st_uid!=b.st_uid || a.st_nlink!=b.st_nlink)return false;
#if defined(__APPLE__)
	return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
	return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}
bool SafeFile(const struct stat& s) { return S_ISREG(s.st_mode) && s.st_uid==geteuid() && s.st_nlink==1 && (s.st_mode&07777)==0400; }
std::array<uint8_t,Cache::HeaderBytes> Header(const Cache::Key& k,const Digest& payload,const Digest& table) {
	std::array<uint8_t,Cache::HeaderBytes> h{};
	std::memcpy(h.data(),"P100AWC1",8);Put(h.data()+8,1,4);Put(h.data()+12,Cache::HeaderBytes,4);
	Put(h.data()+16,Cache::ChunkBytes,4);Put(h.data()+24,k.bytes,8);Put(h.data()+32,Chunks(k),8);Put(h.data()+40,k.entry,8);
	std::memcpy(h.data()+48,k.sourceSHA256.data(),32);std::memcpy(h.data()+80,k.conversionSHA256.data(),32);
	std::memcpy(h.data()+112,payload.data(),32);std::memcpy(h.data()+144,table.data(),32);
	return h;
}
struct Temporary {
	int directory=-1,fd=-1;std::string name;
	~Temporary() {
		struct stat actual{},named{};
		if(fd>=0 && !name.empty() && !fstat(fd,&actual) && !fstatat(directory,name.c_str(),&named,AT_SYMLINK_NOFOLLOW)
			&& actual.st_dev==named.st_dev && actual.st_ino==named.st_ino)unlinkat(directory,name.c_str(),0);
		if(fd>=0)close(fd);
	}
};
}
struct PioneerModelWeightCache::Reader::Data {
	int fd=-1;Key key;struct stat identity{};Digest payload;
	std::vector<uint8_t> chunks;
	~Data() { if(fd>=0)close(fd); }
	bool Stable() const { struct stat s{};return fd>=0 && !fstat(fd,&s) && SafeFile(s) && Same(identity,s); }
};
PioneerModelWeightCache::Reader::Reader()=default;
PioneerModelWeightCache::Reader::~Reader()=default;
bool PioneerModelWeightCache::Reader::IsOpen() const { return data && data->Stable(); }
uint64_t PioneerModelWeightCache::Reader::Size() const { return IsOpen()?data->key.bytes:0; }
bool PioneerModelWeightCache::Reader::Read(uint64_t offset,unsigned bytes,void* output,unsigned capacity,std::string& error) {
	if(!IsOpen() || !output || !bytes || bytes>ChunkBytes || capacity<bytes
		|| offset>data->key.bytes || bytes>data->key.bytes-offset)return Fail(error,"Invalid or changed converted-weight cache read.");
	std::vector<uint8_t> result(bytes),chunk(ChunkBytes);unsigned copied=0;
	while(copied<bytes) {
		const uint64_t position=offset+copied,index=position/ChunkBytes,first=index*ChunkBytes;
		const unsigned count=unsigned(std::min(uint64_t(ChunkBytes),data->key.bytes-first));
		Digest expected;std::memcpy(expected.data(),data->chunks.data()+index*32,32);
		if(!IO(data->fd,chunk.data(),count,PayloadOffset(data->key)+first,false)
			|| Hash(chunk.data(),count)!=expected
			|| !data->Stable())return Fail(error,"Converted-weight cache chunk failed integrity verification.");
		const unsigned within=unsigned(position-first),take=std::min(bytes-copied,count-within);
		std::memcpy(result.data()+copied,chunk.data()+within,take);copied+=take;
	}
	if(!data->Stable())return Fail(error,"Converted-weight cache changed while reading.");
	std::memcpy(output,result.data(),bytes);return true;
}
PioneerModelWeightCache::PioneerModelWeightCache()=default;
PioneerModelWeightCache::~PioneerModelWeightCache() { if(directoryFD>=0)close(directoryFD); }
PioneerModelWeightCache::Digest PioneerModelWeightCache::Hash(const void* p,size_t bytes) { HashState h;h.Update(p,bytes);return h.Final(); }
bool PioneerModelWeightCache::HashFile(int fd,uint64_t bytes,Digest& result,std::string& error,const Progress& progress) {
	struct stat before{},after{};
	if(fd<0 || !bytes || bytes>(UINT64_C(1)<<42) || fstat(fd,&before) || !S_ISREG(before.st_mode)
		|| before.st_size!=off_t(bytes))return Fail(error,"Model hashing requires the expected same-open regular file.");
	std::vector<uint8_t> buffer(1024*1024);HashState hash;
	for(uint64_t offset=0;offset<bytes;) {
		const unsigned count=unsigned(std::min(uint64_t(buffer.size()),bytes-offset));
		if(!IO(fd,buffer.data(),count,offset,false))return Fail(error,"Cannot read original model for SHA-256.");
		hash.Update(buffer.data(),count);offset+=count;if(progress)progress(offset,bytes,false);
	}
	if(fstat(fd,&after) || !Same(before,after))return Fail(error,"Original model changed during authentication.");
	result=hash.Final();return true;
}
std::string PioneerModelWeightCache::Hex(const Digest& d) {
	std::string s;for(uint8_t b:d) { s.push_back("0123456789abcdef"[b>>4]);s.push_back("0123456789abcdef"[b&15]); }return s;
}
std::string PioneerModelWeightCache::EntryName(const Key& k) {
	const auto h=Header(k,Digest{},Digest{});return "p100a-weight-v1-"+Hex(Hash(h.data(),h.size()))+".cache";
}
bool PioneerModelWeightCache::Open(const char* path,std::string& error) {return OpenDirectory(path,error,true);}
bool PioneerModelWeightCache::OpenExistingDirectory(const char* path,std::string& error) {return OpenDirectory(path,error,false);}
bool PioneerModelWeightCache::OpenDirectory(const char* path,std::string& error,bool create) {
	if(directoryFD>=0 || !path || path[0]!='/')return Fail(error,"A dedicated absolute converted-weight cache directory is required.");
	bool created=false;
	if(create) {
		created=!mkdir(path,0700);
		if(!created && errno!=EEXIST)return Fail(error,"Cannot create converted-weight cache directory.");
	}
	int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);struct stat st{};
	if(fd<0 || fstat(fd,&st) || !S_ISDIR(st.st_mode) || st.st_uid!=geteuid() || (st.st_mode&07777)!=0700) {
		if(fd>=0)close(fd);
		return Fail(error,"Converted-weight cache directory is unsafe.");
	}
	int marker=openat(fd,".p100a-weights-v1",(created?O_WRONLY|O_CREAT|O_EXCL:O_RDONLY)|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK,0600);
	bool okay=marker>=0;
	if(okay && created)okay=IO(marker,const_cast<char*>(Marker),sizeof(Marker)-1,0,true) && !fchmod(marker,0400) && !fsync(marker);
	if(okay) {
		struct stat identity{};char data[sizeof(Marker)]={};
		// A newly created marker was write-only;reopen it read-only BELOW.
		if(created) { close(marker);marker=openat(fd,".p100a-weights-v1",O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK); }
		okay=marker>=0 && !fstat(marker,&identity) && SafeFile(identity) && identity.st_size==off_t(sizeof(Marker)-1)
			&& IO(marker,data,sizeof(Marker)-1,0,false) && !std::memcmp(data,Marker,sizeof(Marker)-1);
	}
	if(marker>=0)close(marker);
	if(!okay) { close(fd);return Fail(error,"Cache directory marker is absent,corrupt or unsafe;existing folder left unchanged."); }
	directoryFD=fd;return true;
}
bool PioneerModelWeightCache::Load(const Key& key,Reader& reader,std::string& error,const Progress& progress,bool& missing,bool& unsafe) {
	missing=false;unsafe=false;
	const auto name=EntryName(key);auto next=std::make_unique<Reader::Data>();next->key=key;
	next->fd=openat(directoryFD,name.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
	if(next->fd<0) { missing=errno==ENOENT;unsafe=!missing;return Fail(error,missing?"Converted-weight cache miss.":"Cannot safely open converted-weight cache entry."); }
	if(fstat(next->fd,&next->identity) || !SafeFile(next->identity)) { unsafe=true;return Fail(error,"Cache entry is not a private read-only single-link regular file."); }
	if(next->identity.st_size!=off_t(PayloadOffset(key)+key.bytes))return Fail(error,"Converted-weight cache length mismatch.");
	std::array<uint8_t,HeaderBytes> raw{};Digest payload{},table{};
	if(!IO(next->fd,raw.data(),raw.size(),0,false))return Fail(error,"Cannot read converted-weight cache header.");
	std::memcpy(payload.data(),raw.data()+112,32);std::memcpy(table.data(),raw.data()+144,32);
	if(raw!=Header(key,payload,table))return Fail(error,"Converted-weight cache version/source/recipe header mismatch.");
	next->payload=payload;next->chunks.resize(size_t(Chunks(key)*32));
	if(!IO(next->fd,next->chunks.data(),next->chunks.size(),HeaderBytes,false) || Hash(next->chunks.data(),next->chunks.size())!=table)
		return Fail(error,"Converted-weight cache chunk index checksum mismatch.");
	std::vector<uint8_t> buffer(ChunkBytes);HashState whole;
	for(uint64_t offset=0,index=0;offset<key.bytes;index++) {
		const unsigned count=unsigned(std::min(uint64_t(ChunkBytes),key.bytes-offset));Digest expected;
		std::memcpy(expected.data(),next->chunks.data()+index*32,32);
		if(!IO(next->fd,buffer.data(),count,PayloadOffset(key)+offset,false) || Hash(buffer.data(),count)!=expected || !next->Stable())
			return Fail(error,"Converted-weight cache payload checksum mismatch.");
		whole.Update(buffer.data(),count);offset+=count;if(progress)progress(offset,key.bytes,true);
	}
	if(whole.Final()!=payload || !next->Stable())return Fail(error,"Converted-weight cache whole checksum mismatch.");
	reader.data=std::move(next);return true;
}
bool PioneerModelWeightCache::GetOrCreate(const Key& key,const Ready& ready,const Produce& produce,Reader& reader,bool& hit,
	std::string& error,const Progress& progress) {
	hit=false;
	if(directoryFD<0 || !Valid(key) || !ready || !produce || reader.data || !ready())return Fail(error,"Cache requires a valid key and independently ready model source.");
	bool missing=false,unsafe=false;
	if(Load(key,reader,error,progress,missing,unsafe)) {
		if(!ready()) { reader.data.reset();return Fail(error,"Model source changed during cache verification."); }
		hit=true;error.clear();return true;
	}
	if(unsafe)return false;
	// No eviction or unrelated deletion. Refuse before conversion if space
	// cannot hold the complete new entry PLUS2GiB operating headroom.
	struct statvfs space{};const uint64_t required=PayloadOffset(key)+key.bytes+(UINT64_C(2)<<30);
	if(fstatvfs(directoryFD,&space) || space.f_frsize<=0 || space.f_bavail<=0)
		return Fail(error,"Cannot confirm free disk space for converted weights;no cache entry changed.");
	const uint64_t unit=uint64_t(space.f_frsize);
	if(uint64_t(space.f_bavail)<required/unit+uint64_t(required%unit!=0))
		return Fail(error,"Not enough disk space for converted weights plus2GiB headroom;no cache entry changed.");
	Temporary file;file.directory=directoryFD;static std::atomic<uint64_t> sequence{0};
	for(unsigned attempt=0;attempt<32 && file.fd<0;attempt++) {
		timespec stamp{};(void)clock_gettime(CLOCK_MONOTONIC,&stamp);
		char name[128];snprintf(name,sizeof(name),".p100a-building-%llu-%llu-%llu",(unsigned long long)getpid(),
			(unsigned long long)(uint64_t(stamp.tv_sec)*1000000000+uint64_t(stamp.tv_nsec)),(unsigned long long)sequence++);
		file.name=name;file.fd=openat(directoryFD,name,O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
		if(file.fd<0 && errno!=EEXIST) { file.name.clear();return Fail(error,"Cannot create private cache staging file."); }
		if(file.fd<0)file.name.clear();
	}
	if(file.fd<0)return Fail(error,"Cannot reserve a private cache staging name.");
	if(ftruncate(file.fd,off_t(PayloadOffset(key)+key.bytes)))return Fail(error,"Cannot size converted-weight cache staging file.");
	std::vector<uint8_t> chunks(size_t(Chunks(key)*32)),buffer(ChunkBytes);HashState whole;
	for(uint64_t offset=0,index=0;offset<key.bytes;index++) {
		const unsigned count=unsigned(std::min(uint64_t(ChunkBytes),key.bytes-offset));
		if(!ready() || !produce(offset,count,buffer.data(),error) || !ready())return Fail(error,"Weight conversion cancelled or source changed;previous cache entry unchanged.");
		const auto digest=Hash(buffer.data(),count);std::memcpy(chunks.data()+index*32,digest.data(),32);whole.Update(buffer.data(),count);
		if(!IO(file.fd,buffer.data(),count,PayloadOffset(key)+offset,true))return Fail(error,"Cannot write converted weights;previous cache entry unchanged.");
		offset+=count;if(progress)progress(offset,key.bytes,false);
	}
	const auto header=Header(key,whole.Final(),Hash(chunks.data(),chunks.size()));
	if(!IO(file.fd,chunks.data(),chunks.size(),HeaderBytes,true) || !IO(file.fd,const_cast<uint8_t*>(header.data()),header.size(),0,true)
		|| !ready() || fchmod(file.fd,0400) || fsync(file.fd))return Fail(error,"Cannot finalize converted weights;previous cache entry unchanged.");
	const auto name=EntryName(key);struct stat previous{};
	if(!fstatat(directoryFD,name.c_str(),&previous,AT_SYMLINK_NOFOLLOW)) {
		if(!SafeFile(previous))return Fail(error,"Unsafe cache entry appeared before publication;not replaced.");
	} else if(errno!=ENOENT)return Fail(error,"Cannot recheck cache publication target.");
	if(renameat(directoryFD,file.name.c_str(),directoryFD,name.c_str()))return Fail(error,"Cannot atomically publish converted weights.");
	file.name.clear();
	// Some filesystems do not implement directory fsync. The complete file
	// itself was fsynced;every future open still validates ALL bytes.
	(void)fsync(directoryFD);
	if(!ready())return Fail(error,"Original model source changed after cache publication.");
	if(!Load(key,reader,error,progress,missing,unsafe))return false;
	if(!ready()) { reader.data.reset();return Fail(error,"Model source changed during new cache verification."); }
	error.clear();return true;
}
bool PioneerModelWeightCache::OpenExisting(const Key& key,const Ready& ready,Reader& reader,
	std::string& error,const Progress& progress) {
	if(directoryFD<0 || !Valid(key) || !ready || reader.data || !ready())
		return Fail(error,"Read-only cache open requires a valid key and independently ready model source.");
	bool missing=false,unsafe=false;
	if(!Load(key,reader,error,progress,missing,unsafe))return false;
	if(!ready()) { reader.data.reset();return Fail(error,"Model source changed during read-only cache verification."); }
	error.clear();return true;
}
