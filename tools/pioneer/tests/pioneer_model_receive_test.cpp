/* SPDX-License-Identifier: MIT */
#define PIONEER_RECEIVE_HEADER_TEST 1
#include "../pioneer_model_receive.cpp"
#include <cassert>
#include <initializer_list>

static bool check(const char* text,uint64_t first=123,uint64_t last=456)
{
	char buffer[8192]; assert(strlen(text)<sizeof(buffer)); strcpy(buffer,text);
	return ReceiveHeaders(buffer,first,last);
}
int main()
{
	const char* valid="HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\n\r\n";
	assert(check(valid));
	assert(check("HTTP/1.1 206 Partial Content\r\ncontent-length:\t334\r\ncontent-range: bytes 123-456/18973870528\r\nServer: public file\r\n\r\n"));
	const char* invalid[]={"", "HTTP/1.0 200 OK\r\n\r\n", "HTTP/1.0 206 Partial Content\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 333\r\nContent-Range: bytes 123-456/18973870528\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 124-457/18973870528\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870529\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\nTransfer-Encoding: chunked\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\nContent-Range: bytes 123-456/18973870528\r\n\r\n",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\n\r\ngarbage",
		"HTTP/1.0 206 Partial Content\r\nContent-Length: 334\r\nContent-Range: bytes 123-456/18973870528\r\n"};
	for (unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) assert(!check(invalid[i]));
	assert(!check(valid,457,456)); assert(!check(valid,123,ReceiveModelBytes));
	uint64_t n=0; assert(ReceiveNumber("0",n) && n==0);
	assert(ReceiveNumber("18446744073709551615",n) && n==UINT64_MAX);
	for (const char* text: {"", "-1", "+1", " 1", "1\n", "1.0", "18446744073709551616"}) assert(!ReceiveNumber(text,n));
	assert(ReceiveEndpoint[0]=='/');
	puts("PASS: bounded public-model HTTP header/range/length parser; no network/model/device access");
}
