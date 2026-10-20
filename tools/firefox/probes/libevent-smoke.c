/* Check descriptor dispatch, timers, and wakeup from another thread. */
#include <event2/event.h>
#include <event2/thread.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

struct state {
    struct event_base* base;
    int output;
    int descriptor_seen;
    int timer_seen;
    int failed;
};

static void finish(struct state* state)
{
    if (state->descriptor_seen && state->timer_seen)
        event_base_loopbreak(state->base);
}

static void descriptor_ready(evutil_socket_t descriptor, short events, void* data)
{
    struct state* state = data;
    char byte = 0;
    (void)events;
    if (read(descriptor, &byte, 1) != 1 || byte != 'x')
        state->failed = 1;
    state->descriptor_seen = 1;
    finish(state);
}

static void timer_ready(evutil_socket_t descriptor, short events, void* data)
{
    struct state* state = data;
    (void)descriptor;
    (void)events;
    state->timer_seen = 1;
    finish(state);
}

static void watchdog(evutil_socket_t descriptor, short events, void* data)
{
    struct state* state = data;
    (void)descriptor;
    (void)events;
    state->failed = 1;
    event_base_loopbreak(state->base);
}

static void* worker(void* data)
{
    struct state* state = data;
    usleep(20000);
    if (write(state->output, "x", 1) != 1)
        return (void*)1;
    usleep(20000);
    struct timeval immediately = {0, 0};
    return event_base_once(state->base, -1, EV_TIMEOUT, timer_ready,
        state, &immediately) == 0 ? NULL : (void*)1;
}

int main(void)
{
    if (evthread_use_pthreads() != 0)
        return 1;
    struct state state = {0};
    state.base = event_base_new();
    int descriptors[2];
    if (!state.base || pipe(descriptors) != 0)
        return 2;
    printf("libevent backend: %s\n", event_base_get_method(state.base));
    state.output = descriptors[1];
    struct event* reader = event_new(state.base, descriptors[0], EV_READ,
        descriptor_ready, &state);
    struct event* limit = evtimer_new(state.base, watchdog, &state);
    struct timeval deadline = {3, 0};
    if (!reader || !limit || event_add(reader, NULL) != 0
        || evtimer_add(limit, &deadline) != 0)
        return 3;
    pthread_t thread;
    if (pthread_create(&thread, NULL, worker, &state) != 0)
        return 3;
    int dispatched = event_base_dispatch(state.base);
    void* result = NULL;
    pthread_join(thread, &result);
    event_free(reader);
    event_free(limit);
    event_base_free(state.base);
    close(descriptors[0]);
    close(descriptors[1]);
    if (dispatched < 0 || result || state.failed
        || !state.descriptor_seen || !state.timer_seen)
        return 4;
    puts("LIBEVENT_DESCRIPTOR_TIMER_THREAD_WAKEUP_PASS");
    return 0;
}
