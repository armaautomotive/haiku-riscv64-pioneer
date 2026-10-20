#include <nss.h>
#include <pk11pub.h>
#include <prerror.h>
#include <secoid.h>
#include <ssl.h>
#include <sslproto.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (NSS_NoDB_Init(NULL) != SECSuccess) {
        fprintf(stderr, "NSS init failed: %d\n", PR_GetError());
        return 1;
    }
    static const unsigned char expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    unsigned char digest[32], random_bytes[32];
    if (PK11_HashBuf(SEC_OID_SHA256, digest, (const unsigned char*)"abc", 3) != SECSuccess
        || memcmp(digest, expected, sizeof(digest)) != 0)
        return 2;
    puts("NSS SHA-256 known-answer test: PASS");
    if (PK11_GenerateRandom(random_bytes, sizeof(random_bytes)) != SECSuccess)
        return 3;
    SSLVersionRange versions;
    if (SSL_VersionRangeGetSupported(ssl_variant_stream, &versions) != SECSuccess
        || versions.max < SSL_LIBRARY_VERSION_TLS_1_3)
        return 4;
    puts("NSS random generation/TLS 1.3 availability: PASS");
    if (NSS_Shutdown() != SECSuccess)
        return 5;
    puts("NSS_SMOKE_PASS (not an HTTPS/browser test)");
    return 0;
}
