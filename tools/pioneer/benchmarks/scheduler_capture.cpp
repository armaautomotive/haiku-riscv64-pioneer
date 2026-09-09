#include <OS.h>
#include <system_profiler_defs.h>
#include <stdio.h>
#include <string.h>

extern "C" status_t _kern_system_profiler_start(system_profiler_parameters*);
extern "C" status_t _kern_system_profiler_stop(void);

int main()
{
    const size_t capacity = 16 * 1024 * 1024;
    system_profiler_buffer_header* buffer;
    area_id area = create_area("bounded scheduler capture", (void**)&buffer,
        B_ANY_ADDRESS, capacity, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
    if (area < B_OK)
        return 1;
    system_profiler_parameters parameters = {};
    parameters.buffer_area = area;
    parameters.flags = B_SYSTEM_PROFILER_SCHEDULING_EVENTS;
    parameters.locking_lookup_size = 64 * 1024;
    status_t status = _kern_system_profiler_start(&parameters);
    if (status != B_OK) {
        fprintf(stderr, "profiler start: %s\n", strerror(status));
        delete_area(area);
        return 1;
    }
    snooze(2000000);
    status = _kern_system_profiler_stop();
    if (status != B_OK) {
        fprintf(stderr, "profiler stop: %s\n", strerror(status));
        return 1;
    }
    // No buffers were consumed. Reject a near-full capture rather than hide loss.
    size_t length = buffer->size;
    if (buffer->start != 0 || length > (capacity - sizeof(*buffer)) / 2) {
        fprintf(stderr, "capture too full: %zu bytes\n", length);
        delete_area(area);
        return 1;
    }
    const unsigned char* data = (const unsigned char*)(buffer + 1);
    puts("time_ns,cpu,next_thread,previous_thread,previous_state");
    size_t offset = 0;
    while (offset < length) {
        system_profiler_event_header header;
        if (length - offset < sizeof(header))
            break;
        memcpy(&header, data + offset, sizeof(header));
        offset += sizeof(header);
        if (header.size > length - offset)
            break;
        if (header.event == B_SYSTEM_PROFILER_THREAD_SCHEDULED
            && header.size >= sizeof(system_profiler_thread_scheduled)) {
            system_profiler_thread_scheduled event;
            memcpy(&event, data + offset, sizeof(event));
            // Initial synthetic events use the collector CPU, not the target CPU.
            if (event.thread != event.previous_thread)
                printf("%lld,%u,%ld,%ld,%u\n", (long long)event.time,
                    header.cpu, (long)event.thread, (long)event.previous_thread,
                    event.previous_thread_state);
        }
        offset += header.size;
    }
    delete_area(area);
    return offset == length ? 0 : 1;
}
