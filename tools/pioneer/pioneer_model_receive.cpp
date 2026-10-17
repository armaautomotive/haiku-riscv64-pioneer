/* SPDX-License-Identifier: MIT */
// PUBLIC pinned model only, fresh output only, no model/GPU execution.
// Temporary LAN sender authenticates source; full receiver SHA is mandatory.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

static const uint64_t ReceiveModelBytes=UINT64_C(18973870528);
static const char* ReceiveEndpoint="/qwen38-c600de0300ae8a0e.gguf";

static bool ReceiveNumber(const char* text,uint64_t& number)
{
	if (!text || !*text) return false;
	for (const char* p=text;*p;p++) if (*p<'0' || *p>'9') return false;
	errno=0; char* end=NULL; unsigned long long value=strtoull(text,&end,10);
	if (errno || !end || *end) return false;
	number=value; return true;
}

static bool ReceiveHeaders(char* header,uint64_t first,uint64_t last)
{
	if (first>last || last>=ReceiveModelBytes) return false;
	char* line=strstr(header,"\r\n"); if (!line) return false;
	*line=0;
	if (strcmp(header,"HTTP/1.0 206 Partial Content") && strcmp(header,"HTTP/1.1 206 Partial Content")) return false;
	char expectedLength[32],expectedRange[128];
	snprintf(expectedLength,sizeof(expectedLength),"%llu",(unsigned long long)(last-first+1));
	snprintf(expectedRange,sizeof(expectedRange),"bytes %llu-%llu/%llu",(unsigned long long)first,
		(unsigned long long)last,(unsigned long long)ReceiveModelBytes);
	bool length=false,range=false;
	for (char* p=line+2;;p=line+2) {
		line=strstr(p,"\r\n"); if (!line) return false;
		*line=0;
		if (!*p) return length && range && !line[2];
		char* colon=strchr(p,':'); if (!colon) return false;
		*colon=0; char* value=colon+1; while (*value==' ' || *value=='\t') value++;
		if (!strcasecmp(p,"Transfer-Encoding")) return false;
		if (!strcasecmp(p,"Content-Length")) {
			if (length || strcmp(value,expectedLength)) return false;
			length=true;
		} else if (!strcasecmp(p,"Content-Range")) {
			if (range || strcmp(value,expectedRange)) return false;
			range=true;
		}
	}
}

#ifndef PIONEER_RECEIVE_HEADER_TEST
int main(int argc,char** argv)
{
	uint64_t port,first,last;
	if (argc!=5 || !ReceiveNumber(argv[1],port) || !port || port>65535
		|| !ReceiveNumber(argv[2],first) || !ReceiveNumber(argv[3],last)
		|| first>last || last>=ReceiveModelBytes) {
		fprintf(stderr,"Usage: %s PORT FIRST LAST FRESH_OUTPUT; public Mac10.0.0.211 only; full SHA required\n",argv[0]); return 2;
	}
	int connection=socket(AF_INET,SOCK_STREAM,0); if (connection<0) return 1;
	int requested=4*1024*1024,actual=0; socklen_t optionSize=sizeof(actual);
	if (setsockopt(connection,SOL_SOCKET,SO_RCVBUF,&requested,sizeof(requested))) return 1;
	struct timeval timeout={60,0};
	if (setsockopt(connection,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout))
		|| setsockopt(connection,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout))
		|| getsockopt(connection,SOL_SOCKET,SO_RCVBUF,&actual,&optionSize)) return 1;
	printf("PUBLIC_RECEIVE_SOCKET_BUFFER=%d\n",actual); fflush(stdout);
	struct sockaddr_in address; memset(&address,0,sizeof(address));
	address.sin_family=AF_INET; address.sin_port=htons(uint16_t(port));
	if (inet_pton(AF_INET,"10.0.0.211",&address.sin_addr)!=1
		|| connect(connection,(struct sockaddr*)&address,sizeof(address))) return 1;
	char request[512]; int requestBytes=snprintf(request,sizeof(request),
		"GET %s HTTP/1.0\r\nHost: 10.0.0.211\r\nRange: bytes=%llu-%llu\r\nConnection: close\r\n\r\n",
		ReceiveEndpoint,(unsigned long long)first,(unsigned long long)last);
	if (requestBytes<=0 || size_t(requestBytes)>=sizeof(request)) return 1;
	for (int sent=0;sent<requestBytes;) {
		ssize_t n=send(connection,request+sent,size_t(requestBytes-sent),0);
		if (n<0 && errno==EINTR) continue;
		if (n<=0) return 1;
		sent+=int(n);
	}
	char header[8192]; size_t headerBytes=0;
	while (headerBytes<sizeof(header)-1) {
		ssize_t n=recv(connection,header+headerBytes,1,0);
		if (n<0 && errno==EINTR) continue;
		if (n!=1) return 1;
		header[++headerBytes]=0;
		if (headerBytes>=4 && !memcmp(header+headerBytes-4,"\r\n\r\n",4)) break;
	}
	if (!ReceiveHeaders(header,first,last)) { fprintf(stderr,"STOP:exact206/range/length required\n"); return 1; }
	int output=open(argv[4],O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
	if (output<0) { fprintf(stderr,"STOP:fresh output required\n"); return 1; }
	char* buffer=static_cast<char*>(malloc(1024*1024)); if (!buffer) return 1;
	uint64_t received=0,bytes=last-first+1,next=UINT64_C(1)<<28;
	while (received<bytes) {
		size_t wanted=bytes-received>1024*1024?1024*1024:size_t(bytes-received);
		ssize_t n=recv(connection,buffer,wanted,MSG_WAITALL);
		if (n<0 && errno==EINTR) continue;
		if (n<=0) { fprintf(stderr,"STOP:incomplete body at%llu\n",(unsigned long long)received); return 1; }
		for (size_t done=0;done<size_t(n);) {
			ssize_t wrote=write(output,buffer+done,size_t(n)-done);
			if (wrote<0 && errno==EINTR) continue;
			if (wrote<=0) return 1;
			done+=size_t(wrote);
		}
		received+=uint64_t(n);
		if (received>=next) { printf("PUBLIC_RECEIVE %llu/%llu\n",(unsigned long long)received,(unsigned long long)bytes); fflush(stdout); next+=UINT64_C(1)<<28; }
	}
	char extra;
	if (recv(connection,&extra,1,0)!=0 || fsync(output) || close(output)) return 1;
	close(connection); free(buffer);
	printf("PUBLIC_RECEIVE_LENGTH_COMPLETE_FULL_SHA256_STILL_REQUIRED\n"); return 0;
}
#endif
