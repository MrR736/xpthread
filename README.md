# xpthread

A small cross-platform **pthread compatibility layer** that allows pthread-style
code to build and run on both POSIX systems and Windows.

On POSIX platforms, `xpthread` is a thin wrapper over `pthread`.
On Windows, pthread concepts are **emulated using Win32 primitives**.

> ⚠️ This is **not** a full pthreads implementation.
> Unsupported or partially supported features are explicitly documented.

---

## Goals

- Allow portable C/C++ codebases written against pthreads to compile on Windows
- Keep the API surface small and predictable
- Prefer **clarity over false compatibility**
- Avoid pulling in large dependencies (e.g. pthreads-win32)

---

## Non-Goals

- Full POSIX conformance
- Binary compatibility with `pthread`
- Supporting advanced pthread features (robust mutexes, priority inheritance, etc.)

---

## Supported Platforms

| Platform | Status |
|--------|--------|
| Linux | Native (pthread passthrough) |
| macOS | Native (pthread passthrough) |
| Android | Native (pthread passthrough, limited cancellation) |
| Windows | Emulated using Win32 APIs |

---

## Design Overview

| Concept | POSIX | Windows |
|------|------|--------|
| Thread | `pthread_t` | `HANDLE` |
| Mutex | `pthread_mutex_t` | `CRITICAL_SECTION` |
| Once | `pthread_once_t` | `INIT_ONCE` |
| Cancellation | Full | **Not supported** |

All Windows deviations are documented in `xpthread.h`.

---

## API Coverage Summary

### Threads

| Function | POSIX | Windows |
|--------|-------|--------|
| `xpthread_create` | ✅ | ⚠️ attrs ignored |
| `xpthread_join` | ✅ | ⚠️ no return value |
| `xpthread_detach` | ✅ | ⚠️ closes HANDLE |
| `xpthread_self` | ✅ | ⚠️ pseudo-handle |
| `xpthread_exit` | ✅ | ⚠️ return ignored |

---

### Cancellation (Limited)

| Function | POSIX | Windows |
|--------|-------|--------|
| `xpthread_cancel` | ✅ | ❌ no-op |
| `xpthread_testcancel` | ✅ | ❌ no-op |
| `xpthread_setcancelstate` | ✅ | ❌ no-op |
| `xpthread_setcanceltype` | ✅ | ❌ no-op |

> **Important:** Thread cancellation is **not implemented on Windows**.
> Code relying on cancellation for correctness will NOT behave portably.

---

### Mutexes

| Feature | POSIX | Windows |
|------|------|--------|
| Normal lock/unlock | ✅ | ✅ |
| Trylock | ✅ | ✅ |
| Timed lock | ✅ | ⚠️ emulated |
| Recursive mutex | ✅ | ⚠️ implicit |
| Robust mutex | ✅ | ❌ unsupported |
| Priority ceiling | ✅ | ❌ unsupported |

Windows mutexes are implemented using `CRITICAL_SECTION`:
- Non-cancelable
- Non-robust
- No priority inheritance or ceiling

---

## Timed Locks

`xpthread_mutex_timedlock()` is:

- Native on POSIX
- **Emulated on Windows** using polling and sleep

Timeout precision on Windows is **coarse** and should not be relied upon for
high-resolution timing.

---

## Thread Attributes

Thread attributes (`xpthread_attr_t`) are:

- Fully supported on POSIX
- **Ignored on Windows**

Detached state via attributes is not supported on Windows.
Use `xpthread_detach()` instead.

---

## Error Handling

- POSIX platforms return pthread-style error codes
- Windows behavior is best-effort and may return generic failures

Do **not** rely on specific `errno` values for portability.

---

## Usage Example

```c
#include "xpthread.h"

static xpthread_mutex_t lock; // XPTHREAD_MUTEX_INITIALIZER don't work in windows

void *worker(void *arg) {
    xpthread_mutex_init(&lock);
    xpthread_mutex_lock(&lock);
    /* critical section */
    xpthread_mutex_unlock(&lock);
    return NULL;
}

int main(void) {
    xpthread_t th;
    xpthread_create(&th, NULL, worker, NULL);
    xpthread_join(th, NULL);
    return 0;
}
