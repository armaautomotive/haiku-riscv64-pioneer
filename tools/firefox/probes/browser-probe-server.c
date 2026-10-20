/* Serve only the browser fixture on loopback and wait for its JS result. */
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int send_all(int socket, const char* buffer, size_t length)
{
    while (length) {
        ssize_t written = send(socket, buffer, length, 0);
        if (written <= 0)
            return -1;
        buffer += written;
        length -= (size_t)written;
    }
    return 0;
}

static int respond(int socket, const char* type, const char* body, size_t length)
{
    char header[256];
    int size = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Cache-Control: no-store\r\nConnection: close\r\n\r\n", type, length);
    return send_all(socket, header, (size_t)size) || send_all(socket, body, length);
}

int main(int argc, char** argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc != 2)
        return 64;
    FILE* file = fopen(argv[1], "rb");
    if (!file || fseek(file, 0, SEEK_END) != 0)
        return 1;
    long length = ftell(file);
    if (length <= 0 || length > 1024 * 1024 || fseek(file, 0, SEEK_SET) != 0)
        return 1;
    char* page = malloc((size_t)length);
    if (!page || fread(page, 1, (size_t)length, file) != (size_t)length)
        return 1;
    fclose(file);
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
#ifdef __HAIKU__
    address.sin_len = sizeof(address);
#endif
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (listener < 0 || bind(listener, (struct sockaddr*)&address, sizeof(address)) != 0
        || listen(listener, 4) != 0)
        return 2;
    socklen_t address_size = sizeof(address);
    if (getsockname(listener, (struct sockaddr*)&address, &address_size) != 0)
        return 2;
    printf("PROBE_URL=http://127.0.0.1:%u/\n", (unsigned)ntohs(address.sin_port));
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int result = 3;
    for (int request_number = 0; request_number < 32; request_number++) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (now.tv_sec - start.tv_sec >= 120)
            break;
        struct pollfd descriptor = {listener, POLLIN, 0};
        int ready = poll(&descriptor, 1, 1000);
        if (ready < 0 && errno != EINTR)
            break;
        if (ready <= 0) {
            request_number--;
            continue;
        }
        int client = accept(listener, NULL, NULL);
        if (client < 0)
            continue;
        struct timeval timeout = {5, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        char request[8192];
        size_t used = 0;
        while (used < sizeof(request) - 1) {
            ssize_t size = recv(client, request + used, sizeof(request) - 1 - used, 0);
            if (size <= 0)
                break;
            used += (size_t)size;
            request[used] = 0;
            if (strstr(request, "\r\n\r\n"))
                break;
        }
        request[used] = 0;
        char path[256] = "";
        if (sscanf(request, "GET %255s HTTP/", path) != 1) {
            close(client);
            continue;
        }
        if (!strcmp(path, "/") || !strcmp(path, "/acceptance.html")) {
            respond(client, "text/html; charset=utf-8", page, (size_t)length);
        } else if (!strcmp(path, "/echo")) {
            respond(client, "text/plain", "network-ok", 10);
        } else if (!strcmp(path, "/report?status=pass") || !strcmp(path, "/report?status=fail")) {
            result = !strcmp(path, "/report?status=pass") ? 0 : 4;
            respond(client, "text/plain", "recorded", 8);
            close(client);
            break;
        } else {
            respond(client, "text/plain", "not found", 9);
        }
        close(client);
    }
    close(listener);
    free(page);
    puts(result == 0 ? "FIREFOX_PAGE_PROBE_PASS" : "FIREFOX_PAGE_PROBE_FAIL_OR_TIMEOUT");
    return result;
}
