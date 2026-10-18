/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_PUBLIC_FIXTURES_H
#define PIONEER_Q38_PUBLIC_FIXTURES_H
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
enum { PIONEER_Q38_PUBLIC_FIXTURE_BYTES=12544 };
static const char* const PioneerQ38PublicFixturePath=
	"/boot/home/pioneer-qwen38-affine-20261017-a/public-model-panels-eb62d234.bin";
// SIX public first/last model tiles,not an arbitrary model or executable.
// Source WHOLE GGUF c600de03 was authenticated before generation. Verify the
// EXACT memory buffer which is later consumed,not just its source path/FD.
// No accelerator FD exists until this child has exited and all pipes close.
static inline bool PioneerQ38LoadPublicFixtures(uint8_t* output,unsigned capacity)
{
	if (!output || capacity!=PIONEER_Q38_PUBLIC_FIXTURE_BYTES) return false;
	FILE* file=fopen(PioneerQ38PublicFixturePath,"rb"); if (!file) return false;
	struct stat identity={};
	bool okay=!fstat(fileno(file),&identity) && S_ISREG(identity.st_mode) && identity.st_size==capacity;
	if (okay) okay=fread(output,1,capacity,file)==capacity && fgetc(file)==EOF && !ferror(file);
	if (fclose(file)) okay=false;
	if (!okay) return false;
	int input[2],result[2];
	if (pipe(input)) return false;
	if (pipe(result)) { close(input[0]); close(input[1]); return false; }
	pid_t child=fork();
	if (!child) {
		if (dup2(input[0],0)<0 || dup2(result[1],1)<0) _exit(126);
		for (int fd:{input[0],input[1],result[0],result[1]}) if (fd>2) close(fd);
		char name[]="sha256sum"; char* argv[]={name,NULL}; execv("/bin/sha256sum",argv); _exit(127);
	}
	close(input[0]); close(result[1]);
	if (child<0) { close(input[1]); close(result[0]); return false; }
	unsigned sent=0;
	while (sent<capacity) {
		ssize_t count=write(input[1],output+sent,capacity-sent);
		if (count<0 && errno==EINTR) continue;
		if (count<=0) { okay=false; break; }
		sent+=unsigned(count);
	}
	close(input[1]);
	char digest[96]={}; unsigned used=0;
	for (;;) {
		char data[96]; ssize_t count=read(result[0],data,sizeof(data));
		if (count<0 && errno==EINTR) continue;
		if (count<=0) { if (count<0) okay=false; break; }
		if (unsigned(count)>sizeof(digest)-1-used) okay=false;
		else { memcpy(digest+used,data,size_t(count)); used+=unsigned(count); }
	}
	close(result[0]); int status=0; pid_t done;
	do { done=waitpid(child,&status,0); } while (done<0 && errno==EINTR);
	return okay && sent==capacity && done==child && WIFEXITED(status) && !WEXITSTATUS(status) && used==68
		&& !strcmp(digest,"eb62d234359d028a62e3dcf8ee1729c970cc2470e4999c6327ab3f01198a4f51  -\n");
}
#endif
