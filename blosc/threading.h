/*
 * Code for simulating pthreads API on Windows.  This is Git-specific,
 * but it is enough for Numexpr needs too.
 *
 * Copyright (C) 2009 Andrzej K. Haczewski <ahaczewski@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * DISCLAIMER: The implementation is Git-specific, it is subset of original
 * Pthreads API, without lots of other features that Git doesn't use.
 * Git also makes sure that the passed arguments are valid, so there's
 * no need for double-checking.
 */


#ifndef BLOSC_THREADING_H
#define BLOSC_THREADING_H

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

/*
 * Defines that adapt Windows API threads to pthreads API
 */
#define blosc_pthread_mutex_t CRITICAL_SECTION

#define blosc_pthread_mutex_init(a,b) InitializeCriticalSection((a))
#define blosc_pthread_mutex_destroy(a) DeleteCriticalSection((a))
#define blosc_pthread_mutex_lock EnterCriticalSection
#define blosc_pthread_mutex_unlock LeaveCriticalSection

/*
 * Implement simple condition variable for Windows threads, based on ACE
 * implementation.
 *
 * See original implementation: http://bit.ly/1vkDjo
 * ACE homepage: http://www.cse.wustl.edu/~schmidt/ACE.html
 * See also: http://www.cse.wustl.edu/~schmidt/win32-cv-1.html
 */
typedef struct {
	LONG waiters;
	int was_broadcast;
	CRITICAL_SECTION waiters_lock;
	HANDLE sema;
	HANDLE continue_broadcast;
} blosc_pthread_cond_t;

int blosc_pthread_cond_init(blosc_pthread_cond_t *cond, const void *unused);
int blosc_pthread_cond_destroy(blosc_pthread_cond_t *cond);
int blosc_pthread_cond_wait(blosc_pthread_cond_t *cond, CRITICAL_SECTION *mutex);
int blosc_pthread_cond_signal(blosc_pthread_cond_t *cond);
int blosc_pthread_cond_broadcast(blosc_pthread_cond_t *cond);

/*
 * Simple thread creation implementation using pthread API
 */
typedef struct {
	HANDLE handle;
	void *(*start_routine)(void*);
	void *arg;
} blosc_pthread_t;

int blosc_pthread_create(blosc_pthread_t *thread, const void *unused,
			  void *(*start_routine)(void*), void *arg);

/*
 * To avoid the need of copying a struct, we use small macro wrapper to pass
 * pointer to win32_pthread_join instead.
 */
#define blosc_pthread_join(a, b) blosc_pthread_join_impl(&(a), (b))

int blosc_pthread_join_impl(blosc_pthread_t *thread, void **value_ptr);

/**
 * pthread_once implementation based on the MS Windows One-Time Initialization
 * (https://docs.microsoft.com/en-us/windows/desktop/Sync/one-time-initialization)
 * APIs.
 */
typedef INIT_ONCE blosc_pthread_once_t;
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT
int blosc_pthread_once(blosc_pthread_once_t* once_control, void (*init_routine)(void));

#else /* not _WIN32 */

#include <pthread.h>

#define blosc_pthread_mutex_t pthread_mutex_t
#define blosc_pthread_mutex_init(a, b) pthread_mutex_init((a), (b))
#define blosc_pthread_mutex_destroy(a) pthread_mutex_destroy((a))
#define blosc_pthread_mutex_lock(a) pthread_mutex_lock((a))
#define blosc_pthread_mutex_unlock(a) pthread_mutex_unlock((a))

#define blosc_pthread_cond_t pthread_cond_t
#define blosc_pthread_cond_init(a, b) pthread_cond_init((a), (b))
#define blosc_pthread_cond_destroy(a) pthread_cond_destroy((a))
#define blosc_pthread_cond_wait(a, b) pthread_cond_wait((a), (b))
#define blosc_pthread_cond_signal(a) pthread_cond_signal((a))
#define blosc_pthread_cond_broadcast(a) pthread_cond_broadcast((a))

#define blosc_pthread_t pthread_t
#define blosc_pthread_create(a, b, c, d) pthread_create((a), (b), (c), (d))
#define blosc_pthread_join(a, b) pthread_join((a), (b))

#define blosc_pthread_once_t pthread_once_t
#define blosc_pthread_once(a, b) pthread_once((a), (b))

#endif

#endif /* BLOSC_THREADING_H */
