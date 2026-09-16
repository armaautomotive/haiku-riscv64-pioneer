/* Distributed under the terms of the MIT License. */
// Offline migration helper. Only copy into a freshly initialized, empty BFS
// image. The source is mounted read-only; any failed check invalidates the copy.
#include "fssh.h"
#include "command_cp.h"
#include "fssh_dirent.h"
#include "fssh_errors.h"
#include "fssh_fcntl.h"
#include "fssh_fs_volume.h"
#include "fssh_node_monitor.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "syscalls.h"
#include <stdio.h>
#include <string.h>
#include <set>
#include <string>

namespace FSShell {
namespace {

class Descriptor {
public:
	explicit Descriptor(int value) : fd(value) {}
	~Descriptor() { if (fd >= 0) _kern_close(fd); }
	int fd;
};

static fssh_status_t
Names(int fd, std::set<std::string>& names)
{
	if (fd < 0) return fd;
	alignas(fssh_dirent) char buffer[sizeof(fssh_dirent) + 1024];
	fssh_dirent* entry = (fssh_dirent*)buffer;
	int count;
	while ((count = _kern_read_dir(fd, entry, sizeof(buffer), 1)) == 1) {
		if (strcmp(entry->d_name, ".") != 0
			&& strcmp(entry->d_name, "..") != 0)
			names.insert(entry->d_name);
	}
	return count < 0 ? count : FSSH_B_OK;
}

static fssh_status_t
EqualData(int a, int b, fssh_off_t size)
{
	if (a < 0 || b < 0) return FSSH_B_ERROR;
	char left[32768], right[32768];
	for (fssh_off_t offset = 0; offset < size;) {
		size_t bytes = size - offset < (fssh_off_t)sizeof(left)
			? (size_t)(size - offset) : sizeof(left);
		fssh_ssize_t n = _kern_read(a, offset, left, bytes);
		fssh_ssize_t m = _kern_read(b, offset, right, bytes);
		if (n != (fssh_ssize_t)bytes || m != n
			|| memcmp(left, right, bytes) != 0) return FSSH_B_ERROR;
		offset += bytes;
	}
	return FSSH_B_OK;
}

static fssh_status_t
EqualAttributes(int a, int b)
{
	Descriptor ad(_kern_open_attr_dir(a, NULL));
	Descriptor bd(_kern_open_attr_dir(b, NULL));
	std::set<std::string> left, right;
	if (Names(ad.fd, left) != FSSH_B_OK || Names(bd.fd, right) != FSSH_B_OK
		|| left != right) return FSSH_B_ERROR;
	for (std::set<std::string>::const_iterator i = left.begin();
		i != left.end(); ++i) {
		Descriptor af(_kern_open_attr(a, i->c_str(), FSSH_O_RDONLY));
		Descriptor bf(_kern_open_attr(b, i->c_str(), FSSH_O_RDONLY));
		struct fssh_stat as, bs;
		if (af.fd < 0 || bf.fd < 0
			|| _kern_read_stat(af.fd, NULL, false, &as, sizeof(as)) != FSSH_B_OK
			|| _kern_read_stat(bf.fd, NULL, false, &bs, sizeof(bs)) != FSSH_B_OK
			|| as.fssh_st_type != bs.fssh_st_type
			|| as.fssh_st_size != bs.fssh_st_size
			|| EqualData(af.fd, bf.fd, as.fssh_st_size) != FSSH_B_OK)
			return FSSH_B_ERROR;
	}
	return FSSH_B_OK;
}

static fssh_status_t
Walk(const std::string& source, const std::string& target, bool verify)
{
	struct fssh_stat st;
	fssh_status_t status = _kern_read_stat(-1, source.c_str(), false,
		&st, sizeof(st));
	if (status != FSSH_B_OK) return status;
	bool directory = FSSH_S_ISDIR(st.fssh_st_mode);
	bool regular = FSSH_S_ISREG(st.fssh_st_mode);
	bool link = FSSH_S_ISLNK(st.fssh_st_mode);
	if ((!directory && !regular && !link)
		|| (!directory && st.fssh_st_nlink > 1)) {
		fprintf(stderr, "clonefs: unsupported node or hard links: %s\n",
			source.c_str());
		return FSSH_B_NOT_SUPPORTED;
	}
	if (directory) {
		Descriptor dir(_kern_open_dir(-1, source.c_str()));
		std::set<std::string> names;
		status = Names(dir.fd, names);
		if (status != FSSH_B_OK) return status;
		if (verify) {
			Descriptor other(_kern_open_dir(-1, target.c_str()));
			std::set<std::string> targetNames;
			if (Names(other.fd, targetNames) != FSSH_B_OK || names != targetNames)
				return FSSH_B_ERROR;
		}
		for (std::set<std::string>::const_iterator i = names.begin();
			i != names.end(); ++i) {
			status = Walk(source + "/" + *i, target + "/" + *i, verify);
			if (status != FSSH_B_OK) return status;
		}
	}
	if (!verify) return FSSH_B_OK;
	struct fssh_stat dest;
	if (_kern_read_stat(-1, target.c_str(), false, &dest, sizeof(dest))
			!= FSSH_B_OK
		|| (st.fssh_st_mode & FSSH_S_IFMT) != (dest.fssh_st_mode & FSSH_S_IFMT))
		return FSSH_B_ERROR;
	Descriptor a(_kern_open(-1, source.c_str(), FSSH_O_RDONLY | FSSH_O_NOTRAVERSE, 0));
	Descriptor b(_kern_open(-1, target.c_str(), FSSH_O_RDONLY | FSSH_O_NOTRAVERSE, 0));
	if (a.fd < 0 || b.fd < 0 || EqualAttributes(a.fd, b.fd) != FSSH_B_OK)
		return FSSH_B_ERROR;
	if (regular && (st.fssh_st_size != dest.fssh_st_size
		|| EqualData(a.fd, b.fd, st.fssh_st_size) != FSSH_B_OK))
		return FSSH_B_ERROR;
	if (link) {
		char left[4096], right[4096];
		fssh_size_t n = sizeof(left), m = sizeof(right);
		if (_kern_read_link(-1, source.c_str(), left, &n) != FSSH_B_OK
			|| _kern_read_link(-1, target.c_str(), right, &m) != FSSH_B_OK
			|| n >= sizeof(left) || n != m || memcmp(left, right, n) != 0)
			return FSSH_B_ERROR;
	}
	// Apply metadata after all content/attribute writes and directory traversal.
	int mask = FSSH_B_STAT_MODE | FSSH_B_STAT_UID | FSSH_B_STAT_GID
		| FSSH_B_STAT_ACCESS_TIME | FSSH_B_STAT_MODIFICATION_TIME
		| FSSH_B_STAT_CREATION_TIME;
	status = _kern_write_stat(-1, target.c_str(), false, &st, sizeof(st), mask);
	if (status != FSSH_B_OK) return status;
	if (_kern_read_stat(-1, target.c_str(), false, &dest, sizeof(dest)) != FSSH_B_OK
		|| st.fssh_st_mode != dest.fssh_st_mode
		|| st.fssh_st_uid != dest.fssh_st_uid || st.fssh_st_gid != dest.fssh_st_gid
		|| st.fssh_st_mtim.tv_sec != dest.fssh_st_mtim.tv_sec
		|| st.fssh_st_mtim.tv_nsec != dest.fssh_st_mtim.tv_nsec
		|| st.fssh_st_crtim.tv_sec != dest.fssh_st_crtim.tv_sec
		|| st.fssh_st_crtim.tv_nsec != dest.fssh_st_crtim.tv_nsec)
		return FSSH_B_ERROR;
	return FSSH_B_OK;
}

} // namespace

fssh_status_t
command_clonefs(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: clonefs <source-image> (destination /myfs must be empty)\n");
		return FSSH_B_BAD_VALUE;
	}
	Descriptor target(_kern_open_dir(-1, "/myfs"));
	std::set<std::string> names;
	if (Names(target.fd, names) != FSSH_B_OK || !names.empty())
		return FSSH_B_NOT_ALLOWED;
	fssh_status_t status = _kern_create_dir(-1, "/clone-source", 0755);
	if (status != FSSH_B_OK) return status;
	fssh_dev_t device = _kern_mount("/clone-source", argv[1], "bfs",
		FSSH_B_MOUNT_READ_ONLY, NULL, 0);
	if (device < 0) return device;
	status = Walk("/clone-source", "/myfs", false);
	struct fssh_stat targetStat;
	if (status == FSSH_B_OK)
		status = _kern_read_stat(target.fd, NULL, false, &targetStat, sizeof(targetStat));
	// Create the source's typed indices before copying attribute-bearing files.
	if (status == FSSH_B_OK) {
		Descriptor indices(_kern_open_index_dir(device));
		std::set<std::string> indexNames;
		status = Names(indices.fd, indexNames);
		for (std::set<std::string>::const_iterator i = indexNames.begin();
			status == FSSH_B_OK && i != indexNames.end(); ++i) {
			struct fssh_stat index, existing;
			status = _kern_read_index_stat(device, i->c_str(), &index);
			if (status != FSSH_B_OK) break;
			if (_kern_read_index_stat(targetStat.fssh_st_dev, i->c_str(), &existing)
					== FSSH_B_OK) {
				if (index.fssh_st_type != existing.fssh_st_type) status = FSSH_B_ERROR;
			} else
				status = _kern_create_index(targetStat.fssh_st_dev, i->c_str(),
					index.fssh_st_type, 0);
		}
	}
	if (status == FSSH_B_OK) {
		const char* args[] = {"cp", "-rd", "/clone-source/.", "/myfs"};
		status = command_cp(4, args);
	}
	if (status == FSSH_B_OK) {
		const char* args[] = {"cp", "-a", "/clone-source", "/myfs"};
		status = command_cp(4, args);
	}
	if (status == FSSH_B_OK) status = Walk("/clone-source", "/myfs", true);
	fssh_status_t unmountStatus = _kern_unmount("/clone-source", 0);
	if (status == FSSH_B_OK) status = unmountStatus;
	if (status == FSSH_B_OK) status = _kern_sync();
	fprintf(stderr, "clonefs: %s (%" FSSH_B_PRId32 ")\n",
		status == FSSH_B_OK ? "copy and verification passed" : "FAILED; discard destination",
		status);
	return status;
}

} // namespace FSShell
