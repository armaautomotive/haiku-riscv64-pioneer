/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_MODEL_WEIGHT_CACHE_H
#define PIONEER_MODEL_WEIGHT_CACHE_H
#if defined(PIONEER_ATTENTION_FREESTANDING) || defined(_KERNEL_MODE)
#error Disk cache is CPU/userspace only;not a shader or kernel capability
#endif
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// Derived WEIGHTS only. Callers must authenticate the original model on its
// own open FD and bind that source through Ready. A cache header/digest is
// integrity/versioning DATA,NOT proof of numerical quality or device authority.
// Conversion identifies the complete recipe:precision,tile order,padding,
// numerical rules and version. Entries from different recipes never alias.
class PioneerModelWeightCache {
public:
	using Digest=std::array<uint8_t,32>;
	struct Key {
		Digest sourceSHA256{},conversionSHA256{};
		uint64_t entry=0,bytes=0;
	};
	static constexpr unsigned ChunkBytes=65536,HeaderBytes=256;
	static constexpr uint64_t MaxEntryBytes=UINT64_C(1)<<36;
	using Ready=std::function<bool()>;
	using Produce=std::function<bool(uint64_t,unsigned,uint8_t*,std::string&)>;
	using Progress=std::function<void(uint64_t,uint64_t,bool)>;
	class Reader {
		friend class PioneerModelWeightCache;
		struct Data;
		std::unique_ptr<Data> data;
	public:
		Reader();~Reader();
		Reader(const Reader&)=delete;Reader& operator=(const Reader&)=delete;
		bool IsOpen() const;
		uint64_t Size() const;
		// At most64KiB,possibly spanning TWO chunks. Rehash each actual read
		// before atomic publication;changes/corruption never expose partial data.
		bool Read(uint64_t offset,unsigned bytes,void* output,unsigned capacity,std::string& error);
	};
	PioneerModelWeightCache();~PioneerModelWeightCache();
	PioneerModelWeightCache(const PioneerModelWeightCache&)=delete;
	PioneerModelWeightCache& operator=(const PioneerModelWeightCache&)=delete;
	// Creates only the exact dedicated leaf directory. Existing directories
	// require our marker;never takes over an arbitrary existing user folder.
	bool Open(const char* directory,std::string& error);
	// Open only an already-existing marked cache directory. Never mkdir or
	// create its marker, even if the directory disappears during lookup.
	bool OpenExistingDirectory(const char* directory,std::string& error);
	// A verified hit performs ZERO Produce calls. Miss/corrupt owned entry
	// builds privately and publishes by atomic rename only after complete
	// payload/chunk SHA,source Ready,fsync and read-only mode. Source failure
	// leaves an older entry unchanged. Unsafe links/permissions refuse.
	bool GetOrCreate(const Key&,const Ready&,const Produce&,Reader&,bool& hit,
		std::string& error,const Progress& progress=Progress());
	// Read-only phase for loaders AFTER preparation. Missing/corrupt entries
	// fail; this method never converts, creates, repairs or publishes files.
	bool OpenExisting(const Key&,const Ready&,Reader&,std::string& error,
		const Progress& progress=Progress());
	static Digest Hash(const void*,size_t);
	// SAME open regular file;pread does not disturb the converter's FILE
	// position. Caller checks this digest against its trusted model identity.
	static bool HashFile(int fd,uint64_t expectedBytes,Digest&,std::string& error,const Progress& progress=Progress());
	static std::string Hex(const Digest&);
	static std::string EntryName(const Key&);
private:
	int directoryFD=-1;
	bool OpenDirectory(const char*,std::string&,bool create);
	bool Load(const Key&,Reader&,std::string&,const Progress&,bool& missing,bool& unsafe);
};
#endif
