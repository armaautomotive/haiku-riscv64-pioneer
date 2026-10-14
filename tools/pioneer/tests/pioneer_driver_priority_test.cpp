#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
using int32 = int32_t;
using uint32 = uint32_t;
using directory_which = int;
enum { B_SYSTEM_DIRECTORY, B_SYSTEM_NONPACKAGED_DIRECTORY, B_USER_DIRECTORY };
static const int B_OK = 0;
static int gBootDevice = 1;
static bool missingPaths;
#define B_COUNT_OF(array) (sizeof(array) / sizeof((array)[0]))
class KPath {
	char fBuffer[256] = {};
public:
	char* LockBuffer() { return fBuffer; }
	size_t BufferSize() { return sizeof(fBuffer); }
	void UnlockBuffer() {}
	size_t Length() const { return strlen(fBuffer); }
	const char* Path() const { return fBuffer; }
};
static int __find_directory(int which, int device, bool create, char* buffer,
	size_t size)
{
	assert(device == 1 && !create);
	const char* paths[] = {"/boot/system", "/boot/system/non-packaged", "/boot/home"};
	if (missingPaths) return -1;
	snprintf(buffer, size, "%s", paths[which]);
	return B_OK;
}
#include "production_priority.inc"
int main()
{
	assert(get_priority("/boot/system/add-ons/kernel/drivers/bin/rtl8125") == 0);
	assert(get_priority("/boot/system/non-packaged/add-ons/kernel/drivers/bin/rtl8125") == 1);
	assert(get_priority("/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/rtl8125") == 2);
	assert(get_priority("/boot/system/non-packaged") == 1);
	assert(get_priority("/boot/system/non-packaged-extra/driver") == 0);
	assert(get_priority("/boot/systematic/driver") == -1);
	assert(get_priority("/") == -1);
	assert(get_priority("") == -1);
	missingPaths = true;
	assert(get_priority("/boot/system/non-packaged/driver") == -1);
}
