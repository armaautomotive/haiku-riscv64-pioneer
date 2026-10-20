/* Known wire bytes catch incorrect byte-order detection in the bundled copy. */
#include <msgpack.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    static const unsigned char expected[] = {
        0x93, 0xce, 0x12, 0x34, 0x56, 0x78, 0xd1, 0xff, 0x7f,
        0xcb, 0x3f, 0xf8, 0, 0, 0, 0, 0, 0
    };
    msgpack_sbuffer buffer;
    msgpack_sbuffer_init(&buffer);
    msgpack_packer packer;
    msgpack_packer_init(&packer, &buffer, msgpack_sbuffer_write);
    if (msgpack_pack_array(&packer, 3) != 0
        || msgpack_pack_uint32(&packer, 0x12345678) != 0
        || msgpack_pack_int64(&packer, -129) != 0
        || msgpack_pack_double(&packer, 1.5) != 0)
        return 1;
    int failed = buffer.size != sizeof(expected)
        || memcmp(buffer.data, expected, sizeof(expected)) != 0;
    msgpack_sbuffer_destroy(&buffer);
    if (failed)
        return 2;
    puts("MSGPACK_INTEGER_FLOAT_WIRE_BYTES_PASS");
    return 0;
}
