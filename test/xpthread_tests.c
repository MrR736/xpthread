#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "xpthread.h"

// Global shared data
static xpthread_mutex_t mutex;
static int counter = 0;

// Thread function
void *thread_func(void *arg) {
    int id = *(int *)arg;
    free(arg); // free allocated memory

    printf("Thread %d: started\n", id);

    // Lock mutex
    xpthread_mutex_lock(&mutex);
    printf("Thread %d: acquired mutex\n", id);

    // Increment counter
    int old = counter;
    counter = old + 1;
    printf("Thread %d: counter %d -> %d\n", id, old, counter);

    xpthread_mutex_unlock(&mutex);
    printf("Thread %d: released mutex\n", id);

    return (void *)(size_t)(id * 10); // return value
}

// Function for xpthread_once test
void once_func(void) {
    printf("xpthread_once: called exactly once\n");
}

int main(void) {
    printf("xpthread test start\n");

    // --- Test xpthread_once ---
    xpthread_once_t once_control = XPTHREAD_ONCE_INIT;
    xpthread_once(&once_control, once_func);
    xpthread_once(&once_control, once_func); // second call should not call function

    // --- Test thread creation and join ---
    const int N = 4;
    xpthread_t threads[N];

    xpthread_mutex_init(&mutex);

    for (int i = 0; i < N; i++) {
        int *id = malloc(sizeof(int)); // allocate separate memory for each thread
        if (!id) {
            fprintf(stderr, "Failed to allocate memory\n");
            return 1;
        }
        *id = i + 1;

        if (xpthread_create(&threads[i], NULL, thread_func, id) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i + 1);
            free(id);
            return 1;
        }
    }

    // Join threads
    for (int i = 0; i < N; i++) {
        void *retval = NULL;
        xpthread_join(threads[i], &retval);
        printf("Thread %d joined, return value = %zu\n", i + 1, (size_t)retval);
    }

    printf("All threads finished, counter = %d\n", counter);

    // --- Test timed lock ---
    struct timespec ts;
    xpthread_get_realtime(&ts); // get current time
    ts.tv_sec += 1; // 1 second timeout

    if (xpthread_mutex_timedlock(&mutex, &ts) == 0) {
        printf("Timed lock acquired\n");
        xpthread_mutex_unlock(&mutex);
    } else {
        printf("Timed lock timed out\n");
    }

    // --- Test mutex trylock ---
    if (xpthread_mutex_trylock(&mutex) == 0) {
        printf("Trylock succeeded\n");
        xpthread_mutex_unlock(&mutex);
    } else {
        printf("Trylock failed\n");
    }

    printf("xpthread test finished\n");
    return 0;
}
