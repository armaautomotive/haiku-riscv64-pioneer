/* Distributed under the terms of the MIT License. */
#ifndef _KERNEL_UTIL_BOOT_STAGE_LOG_H
#define _KERNEL_UTIL_BOOT_STAGE_LOG_H

#include <stddef.h>
#include <stdint.h>

// Single-writer, append-only boot log. Readers must wait for the writer to
// finish (or stop it in the debugger). Strings must have static lifetime.
// No allocation, locking, clock access, or output occurs in this container.
template<size_t Capacity>
class BootStageLog {
public:
	struct Record {
		const char* code;
		const char* description;
		int64_t time;
		int64_t delta;
	};

	void Reset(int64_t start)
	{
		fCount = 0;
		fDropped = 0;
		fStart = fLast = start;
	}

	bool Append(const char* code, const char* description, int64_t now)
	{
		const int64_t delta = now - fLast;
		fLast = now;
		if (fCount == Capacity) {
			++fDropped;
			return false;
		}
		Record& record = fRecords[fCount];
		record.code = code;
		record.description = description;
		record.time = now;
		record.delta = delta;
		// Publish only a completely initialized entry to debugger readers.
		__atomic_store_n(&fCount, fCount + 1, __ATOMIC_RELEASE);
		return true;
	}

	size_t Count() const { return __atomic_load_n(&fCount, __ATOMIC_ACQUIRE); }
	uint64_t Dropped() const { return fDropped; }
	int64_t Start() const { return fStart; }
	int64_t Last() const { return fLast; }
	const Record& At(size_t index) const { return fRecords[index]; }

private:
	static_assert(Capacity > 0, "boot log must have storage");
	Record fRecords[Capacity];
	size_t fCount;
	uint64_t fDropped;
	int64_t fStart;
	int64_t fLast;
};

#endif
