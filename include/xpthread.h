/**
 * @file xpthread.h
 * @brief Cross-platform pthread emulation layer.
 *
 * This API aims to match POSIX pthread semantics where possible.
 * On Windows, functionality is implemented using Win32 primitives and
 * may differ in behavior, guarantees, or supported features.
 *
 * @note This is NOT a complete pthread implementation.
 *       Unsupported or emulated features are documented per-function.
 *
 * @author MrR736
 * @date 2026
 * @copyright GPL-3
 */

#ifndef XPRHREAD_H
#define XPRHREAD_H

#include <time.h>

#ifdef _WIN32
#include <windows.h>

/** Mutex type (maps to CRITICAL_SECTION) */
typedef CRITICAL_SECTION xpthread_mutex_t;

/**
 * One-time initialization control.
 *
 * @note Uses INIT_ONCE internally.
 *       Behavior matches pthread_once() for basic use cases.
 */
typedef INIT_ONCE xpthread_once_t;

/**
 * Thread handle.
 *
 * @note This is a Win32 HANDLE, not an opaque pthread_t.
 */
typedef HANDLE xpthread_t;

/**
 * Thread attributes (unused).
 *
 * @note Thread attributes are ignored on Windows.
 */
typedef DWORD xpthread_attr_t;

#define XPTHREADCALL __stdcall

#define XPTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT
#define XPTHREAD_MUTEX_INITIALIZER {0}

#define XPTHREAD_CANCEL_ENABLE  1
#define XPTHREAD_CANCEL_DISABLE 0

#else /* POSIX */

#include <pthread.h>

typedef pthread_mutex_t	xpthread_mutex_t;
typedef pthread_once_t	xpthread_once_t;
typedef pthread_t	xpthread_t;
typedef pthread_attr_t	xpthread_attr_t;

#define XPTHREADCALL

#define XPTHREAD_ONCE_INIT PTHREAD_ONCE_INIT
#define XPTHREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#ifdef __ANDROID__
#define XPTHREAD_CANCEL_ENABLE  1
#define XPTHREAD_CANCEL_DISABLE 0
#else
#define XPTHREAD_CANCEL_ENABLE  PTHREAD_CANCEL_ENABLE
#define XPTHREAD_CANCEL_DISABLE PTHREAD_CANCEL_DISABLE
#endif

#endif /* _WIN32 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute a function exactly once.
 *
 * POSIX: pthread_once().
 *
 * Windows: Implemented via INIT_ONCE.
 *
 * @note Recursive calls from init_routine() have undefined behavior
 *       (matches POSIX).
 */
int XPTHREADCALL xpthread_once(
	xpthread_once_t *once_control,
	void (*init_routine)(void)
);

/**
 * @brief Create a new thread.
 *
 * POSIX: pthread_create().
 *
 * Windows:
 * - Thread is created via _beginthreadex().
 * - attr is ignored.
 *
 * @note Detached state via attributes is NOT supported on Windows.
 */
int XPTHREADCALL xpthread_create(
	xpthread_t *thread,
	const xpthread_attr_t *attr,
	void *(*start_routine)(void *),
	void *arg
);

/**
 * @brief Wait for a thread to terminate.
 *
 * POSIX: pthread_join().
 *
 * Windows:
 * - Waits on the thread HANDLE.
 * - Thread return values are NOT propagated.
 *
 * @note If retval is non-NULL, it is set to NULL on Windows.
 */
int XPTHREADCALL xpthread_join(xpthread_t thread, void **retval);

/**
 * @brief Get the calling thread identifier.
 *
 * POSIX: pthread_self().
 *
 * Windows:
 * - Returns a pseudo-handle from GetCurrentThread().
 *
 * @note On Windows, the returned value is only reliably comparable
 *       within the same thread.
 */
xpthread_t XPTHREADCALL xpthread_self(void);

/**
 * @brief Terminate the calling thread.
 *
 * POSIX: pthread_exit().
 *
 * Windows:
 * - Calls _endthreadex().
 *
 * @note retval is ignored on Windows.
 */
void XPTHREADCALL xpthread_exit(void *retval);

/**
 * @brief Detach a thread.
 *
 * POSIX: pthread_detach().
 *
 * Windows:
 * - Closes the thread HANDLE.
 *
 * @note After detaching, xpthread_join() must not be called.
 */
int XPTHREADCALL xpthread_detach(xpthread_t thread);

/**
 * @brief Set thread cancellation state.
 *
 * POSIX: pthread_setcancelstate().
 *
 * Windows:
 * - Thread cancellation is not supported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_setcancelstate(int state, int *oldstate);

/**
 * @brief Set thread cancellation type.
 *
 * POSIX: pthread_setcanceltype().
 *
 * Windows:
 * - Cancellation types are not supported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_setcanceltype(int type, int *oldtype);

/**
 * @brief Compare two thread identifiers.
 *
 * POSIX: pthread_equal().
 *
 * Windows:
 * - Compares HANDLE values.
 */
int XPTHREADCALL xpthread_equal(xpthread_t t1, xpthread_t t2);

/**
 * @brief Request thread cancellation.
 *
 * POSIX: pthread_cancel().
 *
 * Windows:
 * - Cancellation is not supported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_cancel(xpthread_t th);

/**
 * @brief Test for pending cancellation.
 *
 * POSIX: pthread_testcancel().
 *
 * Windows:
 * - Cancellation is not supported.
 *
 * @note NO-OP on Windows.
 */
void XPTHREADCALL xpthread_testcancel(void);

/**
 * @brief Initialize a mutex.
 *
 * POSIX: pthread_mutex_init().
 *
 * Windows:
 * - Initializes a CRITICAL_SECTION.
 *
 * @note Mutex attributes (type, robustness, priority ceiling)
 *       are not supported on Windows.
 */
int XPTHREADCALL xpthread_mutex_init(xpthread_mutex_t *mutex);

/**
 * @brief Destroy a mutex.
 */
int XPTHREADCALL xpthread_mutex_destroy(xpthread_mutex_t *mutex);

/**
 * @brief Lock a mutex.
 *
 * POSIX: pthread_mutex_lock().
 *
 * Windows:
 * - Enters CRITICAL_SECTION (non-cancelable).
 */
int XPTHREADCALL xpthread_mutex_lock(xpthread_mutex_t *mutex);

/**
 * @brief Unlock a mutex.
 */
int XPTHREADCALL xpthread_mutex_unlock(xpthread_mutex_t *mutex);

/**
 * @brief Try to lock a mutex.
 *
 * POSIX: pthread_mutex_trylock().
 *
 * Windows:
 * - Uses TryEnterCriticalSection().
 */
int XPTHREADCALL xpthread_mutex_trylock(xpthread_mutex_t *mutex);

/**
 * @brief Get the current real-time clock.
 *
 * POSIX: clock_gettime(CLOCK_REALTIME).
 *
 * Windows:
 * - Uses system time APIs.
 *
 * @note Resolution may differ from POSIX.
 */
void XPTHREADCALL xpthread_get_realtime(struct timespec *ts);

/**
 * @brief Lock a mutex with an absolute timeout.
 *
 * POSIX: pthread_mutex_timedlock().
 *
 * Windows:
 * - Emulated using polling and Sleep().
 *
 * @note Timeout precision is coarse and CPU-inefficient on Windows.
 */
int XPTHREADCALL xpthread_mutex_timedlock(
	xpthread_mutex_t *mutex,
	const struct timespec *abstime
);

/**
 * @brief Get mutex priority ceiling.
 *
 * POSIX: pthread_mutex_getprioceiling().
 *
 * Windows:
 * - Priority ceiling is not supported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_mutex_getprioceiling(
	const xpthread_mutex_t *mutex,
	int *prioceiling
);

/**
 * @brief Set mutex priority ceiling.
 *
 * POSIX: pthread_mutex_setprioceiling().
 *
 * Windows:
 * - Unsupported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_mutex_setprioceiling(
	xpthread_mutex_t *mutex,
	int prioceiling,
	int *old_ceiling
);

/**
 * @brief Mark mutex consistent after owner death.
 *
 * POSIX: pthread_mutex_consistent().
 *
 * Windows:
 * - Robust mutexes are not supported.
 *
 * @note NO-OP on Windows. Always returns success.
 */
int XPTHREADCALL xpthread_mutex_consistent(xpthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* XPRHREAD_H */
