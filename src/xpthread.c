#include <errno.h>
#include <stdint.h>
#include "xpthread.h"

#ifdef __ANDROID__
#include <signal.h>
#define SETUP_SIG_HANDLER(sig) \
	do { \
		struct sigaction sa; \
		sa.sa_handler = signal_handler; \
		sa.sa_flags = 0; \
		sigemptyset(&sa.sa_mask); \
		sigaction(sig, &sa, NULL); \
	} while (0)

// Macros for signal handling
#define ENABLE_SIGNAL(sig) \
	do { \
		SETUP_SIG_HANDLER(sig);  /* Ensure SIG is handled by empty_signal_handler */ \
		sigset_t set; \
		sigemptyset(&set); \
		sigaddset(&set, sig); \
		pthread_sigmask(SIG_UNBLOCK, &set, NULL); \
	} while (0)

#define DISABLE_SIGNAL(sig) \
	do { \
		sigset_t set; \
		sigemptyset(&set); \
		sigaddset(&set, sig); \
		pthread_sigmask(SIG_BLOCK, &set, NULL); \
	} while (0)
#endif

#ifdef _WIN32
#include <process.h>

#if defined(_MSC_VER)
# define XPTHREAD_TLS __declspec(thread)
#else
# define XPTHREAD_TLS _Thread_local
#endif

typedef struct { void (*fn)(void); } once_ctx;

typedef struct {
	void *(*start_routine)(void *);
	void *arg;
	volatile LONG cancel_requested;
	void *retval;
} xpthread_win_ctx;

static XPTHREAD_TLS xpthread_win_ctx *xpthread_self_ctx = NULL; //

XPTHREAD_TLS volatile int xpthread_cancel_flag = 0;

BOOL CALLBACK once_wrapper(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
	once_ctx* ctx = (once_ctx*)Parameter;
	if (!ctx || !ctx->fn) return FALSE;
	ctx->fn();
	return TRUE;
}

// Wrap the user thread function to set up cancellation
static unsigned __stdcall thread_wrapper(void *arg) {
	xpthread_win_ctx *ctx = (xpthread_win_ctx *)arg;
	xpthread_self_ctx = ctx;
	void *ret = ctx->start_routine(ctx->arg);
	ctx->retval = ret;
	_endthreadex(0);
	return 0;
}
#endif

int XPTHREADCALL xpthread_once(xpthread_once_t *once_control, void (*init_routine)(void)) {
#ifdef _WIN32
	once_ctx ctx = { init_routine };
	BOOL result = InitOnceExecuteOnce(once_control, once_wrapper, &ctx, NULL);
	return result ? 0 : -1;
#else
	return pthread_once(once_control, init_routine);
#endif
}

int XPTHREADCALL xpthread_create(xpthread_t *thread,
				 const xpthread_attr_t *attr,
				 void *(*start_routine)(void *),void *arg)
{
#ifdef _WIN32
	(void)attr;

	xpthread_win_ctx *ctx = calloc(1, sizeof(*ctx));
	if (!ctx) return ENOMEM;

	ctx->start_routine = start_routine;
	ctx->arg = arg;

	HANDLE h = (HANDLE)_beginthreadex(
		NULL, 0, thread_wrapper, ctx, 0, NULL
	);

	if (!h) {
		free(ctx);
		return EAGAIN;
	}

	*thread = h;
	return 0;
#else
	return pthread_create(thread, attr, start_routine, arg);
#endif
}

int XPTHREADCALL xpthread_join(xpthread_t thread, void **retval) {
#ifdef _WIN32
	WaitForSingleObject(thread, INFINITE);
	DWORD code;
	GetExitCodeThread(thread, &code);
	if (retval && xpthread_self_ctx) *retval = xpthread_self_ctx->retval;
	CloseHandle(thread);
	return 0;
#else
	return pthread_join(thread,retval);
#endif
}

xpthread_t XPTHREADCALL xpthread_self(void) {
#ifdef _WIN32
	return GetCurrentThread();
#else
	return pthread_self();
#endif
}

int XPTHREADCALL xpthread_detach(xpthread_t thread) {
#ifdef _WIN32
	return CloseHandle(thread) ? 0 : -1;
#else
	return pthread_detach(thread);
#endif
}

int XPTHREADCALL xpthread_setcancelstate(int state, int *oldstate) {
	static __thread int cancel_state = XPTHREAD_CANCEL_ENABLE; // per-thread
#ifdef __ANDROID__
	if (oldstate != NULL) {
		*(int*)oldstate = thread_cancel_state;
	}
	if (state == PTHREAD_CANCEL_ENABLE) {
		thread_cancel_state = PTHREAD_CANCEL_ENABLE;
		ENABLE_SIGNAL(SIGUSR1);
	} else if (state == PTHREAD_CANCEL_DISABLE) {
		thread_cancel_state = PTHREAD_CANCEL_DISABLE;
		DISABLE_SIGNAL(SIGUSR1);
	}
#else
	if (oldstate) *oldstate = cancel_state;
	cancel_state = state;
#endif
	return 0;
}

int XPTHREADCALL xpthread_setcanceltype(int type, int *oldtype) {
#ifdef _WIN32
	static XPTHREAD_TLS int cancel_state = XPTHREAD_CANCEL_ENABLE;
	if (oldtype)
		*oldtype = cancel_state;
	cancel_state = type;
	return 0;
#elif defined(__ANDROID__)
	static __thread int cancel_type = XPTHREAD_CANCEL_ENABLE;
	if (oldtype) *oldtype = cancel_type;
	cancel_type = type;
	if (type == PTHREAD_CANCEL_ENABLE) ENABLE_SIGNAL(SIGUSR1);
	else if (type == PTHREAD_CANCEL_DISABLE) DISABLE_SIGNAL(SIGUSR1);
	return 0;
#else
	return pthread_setcanceltype(type, oldtype);
#endif
}

int XPTHREADCALL xpthread_equal(xpthread_t t1, xpthread_t t2) {
#ifdef _WIN32
	return t1 == t2;
#else
	return pthread_equal(t1,t2);
#endif
}

int XPTHREADCALL xpthread_cancel(xpthread_t th) {
#ifdef _WIN32
	if (!th) return EINVAL;
	return 0;
#elif defined(__ANDROID__)
	return pthread_kill(th,SIGUSR1);
#else
	return pthread_cancel(th);
#endif
}

void XPTHREADCALL xpthread_testcancel(void) {
#ifdef _WIN32
	if (xpthread_self_ctx && InterlockedCompareExchange(&xpthread_self_ctx->cancel_requested,0,0))
		_endthreadex(0);
#else
	pthread_testcancel();
#endif
}

int XPTHREADCALL xpthread_mutex_init(xpthread_mutex_t *mutex) {
#ifdef _WIN32
#if _WIN32_WINNT >= 0x0403
	if (!InitializeCriticalSectionAndSpinCount(mutex, 4000)) return ENOMEM;
#else
	InitializeCriticalSection(mutex); // may raise exception
#endif
	return 0;
#else
	return pthread_mutex_init(mutex, NULL);
#endif
}

int XPTHREADCALL xpthread_mutex_destroy(xpthread_mutex_t *mutex) {
#ifdef _WIN32
	DeleteCriticalSection(mutex);
	return 0;
#else
	return pthread_mutex_destroy(mutex);
#endif
}

int XPTHREADCALL xpthread_mutex_lock(xpthread_mutex_t *mutex) {
#ifdef _WIN32
	EnterCriticalSection(mutex);
	return 0;
#else
	return pthread_mutex_lock(mutex);
#endif
}

int XPTHREADCALL xpthread_mutex_unlock(xpthread_mutex_t *mutex) {
#ifdef _WIN32
	LeaveCriticalSection(mutex);
	return 0;
#else
	return pthread_mutex_unlock(mutex);
#endif
}

int XPTHREADCALL xpthread_mutex_trylock(xpthread_mutex_t *mutex) {
#ifdef _WIN32
	return TryEnterCriticalSection(mutex) ? 0 : EBUSY;
#else
	return pthread_mutex_trylock(mutex);
#endif
}

void XPTHREADCALL xpthread_get_realtime(struct timespec *ts) {
#ifdef _WIN32
	FILETIME ft;
	ULARGE_INTEGER uli;
	GetSystemTimeAsFileTime(&ft);
	uli.LowPart  = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	uint64_t t = uli.QuadPart - 116444736000000000ULL;
	ts->tv_sec  = t / 10000000ULL;
	ts->tv_nsec = (t % 10000000ULL) * 100;
#else
	clock_gettime(CLOCK_REALTIME, ts);
#endif
}


int XPTHREADCALL xpthread_mutex_timedlock(xpthread_mutex_t *mutex, const struct timespec *abstime)
{
#ifdef _WIN32
	DWORD timeout_ms;
	if (abstime) {
		struct timespec now;
		xpthread_get_realtime(&now);

		long sec_diff = abstime->tv_sec - now.tv_sec;
		long nsec_diff = abstime->tv_nsec - now.tv_nsec;
		if (nsec_diff < 0) {
			nsec_diff += 1000000000;
			sec_diff -= 1;
		}

		int64_t total_ms64 = (int64_t)sec_diff * 1000 + nsec_diff / 1000000;
		if (total_ms64 <= 0) return ETIMEDOUT;
	} else {
		timeout_ms = INFINITE;
	}

	DWORD elapsed = 0;
	const DWORD interval = 1; // ms
	while (elapsed < timeout_ms) {
		if (TryEnterCriticalSection(mutex)) return 0;
		Sleep(interval);
		elapsed += interval;
	}

	// final check after loop
	if (TryEnterCriticalSection(mutex)) return 0;

	return ETIMEDOUT;

#else
	return pthread_mutex_timedlock(mutex, abstime);
#endif
}

int XPTHREADCALL xpthread_mutex_getprioceiling(const xpthread_mutex_t *mutex, int *prioceiling) {
#ifdef _WIN32
	if (!prioceiling) return EINVAL;
	*prioceiling = 0;
	return 0;
#else
	return pthread_mutex_getprioceiling((pthread_mutex_t *)mutex, prioceiling);
#endif
}

int XPTHREADCALL xpthread_mutex_setprioceiling(xpthread_mutex_t *mutex, int prioceiling, int *old_ceiling) {
#ifdef _WIN32
	if (old_ceiling) *old_ceiling = 0;
	return 0;
#else
	return pthread_mutex_setprioceiling(mutex, prioceiling, old_ceiling);
#endif
}

int XPTHREADCALL xpthread_mutex_consistent(xpthread_mutex_t *mutex) {
#ifdef _WIN32
	(void)mutex;
	return 0;
#else
	return pthread_mutex_consistent((pthread_mutex_t *)mutex);
#endif
}

void XPTHREADCALL xpthread_exit(void *retval) {
#ifdef _WIN32
	unsigned code = 0;
	if (retval) code = (unsigned)(size_t)retval;
	_endthreadex(code);
#else
	pthread_exit(retval);
#endif
}
