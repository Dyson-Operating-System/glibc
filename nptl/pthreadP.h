/* Copyright (C) 2002-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _PTHREADP_H
#define _PTHREADP_H	1

#include <pthread.h>
#include <setjmp.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include "descr.h"
#include <tls.h>
#include <lowlevellock.h>
#include <stackinfo.h>
#include <internaltypes.h>
#include <pthread-functions.h>
#include <atomic.h>
#include <kernel-features.h>
#include <errno.h>

/* Atomic operations on TLS memory.  */
#ifndef THREAD_ATOMIC_CMPXCHG_VAL
# define THREAD_ATOMIC_CMPXCHG_VAL(descr, member, new, old) \
  atomic_compare_and_exchange_val_acq (&(descr)->member, new, old)
#endif

#ifndef THREAD_ATOMIC_BIT_SET
# define THREAD_ATOMIC_BIT_SET(descr, member, bit) \
  atomic_bit_set (&(descr)->member, bit)
#endif


/* Adaptive mutex definitions.  */
#ifndef MAX_ADAPTIVE_COUNT
# define MAX_ADAPTIVE_COUNT 100
#endif


/* Magic cookie representing robust mutex with dead owner.  */
#define PTHREAD_MUTEX_INCONSISTENT	INT_MAX
/* Magic cookie representing not recoverable robust mutex.  */
#define PTHREAD_MUTEX_NOTRECOVERABLE	(INT_MAX - 1)


/* Internal mutex type value.  */
enum
{
  PTHREAD_MUTEX_KIND_MASK_NP = 3,

  PTHREAD_MUTEX_ELISION_NP    = 256,
  PTHREAD_MUTEX_NO_ELISION_NP = 512,

  PTHREAD_MUTEX_ROBUST_NORMAL_NP = 16,
  PTHREAD_MUTEX_ROBUST_RECURSIVE_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_ROBUST_ADAPTIVE_NP
  = PTHREAD_MUTEX_ROBUST_NORMAL_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_PRIO_INHERIT_NP = 32,
  PTHREAD_MUTEX_PI_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_NORMAL,
  PTHREAD_MUTEX_PI_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_PI_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_PI_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_PI_ROBUST_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_NORMAL_NP,
  PTHREAD_MUTEX_PI_ROBUST_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_RECURSIVE_NP,
  PTHREAD_MUTEX_PI_ROBUST_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_ERRORCHECK_NP,
  PTHREAD_MUTEX_PI_ROBUST_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_ADAPTIVE_NP,
  PTHREAD_MUTEX_PRIO_PROTECT_NP = 64,
  PTHREAD_MUTEX_PP_NORMAL_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_NORMAL,
  PTHREAD_MUTEX_PP_RECURSIVE_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_PP_ERRORCHECK_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_PP_ADAPTIVE_NP
  = PTHREAD_MUTEX_PRIO_PROTECT_NP | PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_ELISION_FLAGS_NP
  = PTHREAD_MUTEX_ELISION_NP | PTHREAD_MUTEX_NO_ELISION_NP,

  PTHREAD_MUTEX_TIMED_ELISION_NP =
	  PTHREAD_MUTEX_TIMED_NP | PTHREAD_MUTEX_ELISION_NP,
  PTHREAD_MUTEX_TIMED_NO_ELISION_NP =
	  PTHREAD_MUTEX_TIMED_NP | PTHREAD_MUTEX_NO_ELISION_NP,
};
#define PTHREAD_MUTEX_PSHARED_BIT 128

#define PTHREAD_MUTEX_TYPE(m) \
  ((m)->__data.__kind & 127)
/* Don't include NO_ELISION, as that type is always the same
   as the underlying lock type.  */
#define PTHREAD_MUTEX_TYPE_ELISION(m) \
  ((m)->__data.__kind & (127|PTHREAD_MUTEX_ELISION_NP))

#if LLL_PRIVATE == 0 && LLL_SHARED == 128
# define PTHREAD_MUTEX_PSHARED(m) \
  ((m)->__data.__kind & 128)
#else
# define PTHREAD_MUTEX_PSHARED(m) \
  (((m)->__data.__kind & 128) ? LLL_SHARED : LLL_PRIVATE)
#endif

/* The kernel when waking robust mutexes on exit never uses
   FUTEX_PRIVATE_FLAG FUTEX_WAKE.  */
#define PTHREAD_ROBUST_MUTEX_PSHARED(m) LLL_SHARED

/* Ceiling in __data.__lock.  __data.__lock is signed, so don't
   use the MSB bit in there, but in the mask also include that bit,
   so that the compiler can optimize & PTHREAD_MUTEX_PRIO_CEILING_MASK
   masking if the value is then shifted down by
   PTHREAD_MUTEX_PRIO_CEILING_SHIFT.  */
#define PTHREAD_MUTEX_PRIO_CEILING_SHIFT	19
#define PTHREAD_MUTEX_PRIO_CEILING_MASK		0xfff80000


/* Flags in mutex attr.  */
#define PTHREAD_MUTEXATTR_PROTOCOL_SHIFT	28
#define PTHREAD_MUTEXATTR_PROTOCOL_MASK		0x30000000
#define PTHREAD_MUTEXATTR_PRIO_CEILING_SHIFT	12
#define PTHREAD_MUTEXATTR_PRIO_CEILING_MASK	0x00fff000
#define PTHREAD_MUTEXATTR_FLAG_ROBUST		0x40000000
#define PTHREAD_MUTEXATTR_FLAG_PSHARED		0x80000000
#define PTHREAD_MUTEXATTR_FLAG_BITS \
  (PTHREAD_MUTEXATTR_FLAG_ROBUST | PTHREAD_MUTEXATTR_FLAG_PSHARED \
   | PTHREAD_MUTEXATTR_PROTOCOL_MASK | PTHREAD_MUTEXATTR_PRIO_CEILING_MASK)


/* Check whether rwlock prefers readers.   */
#define PTHREAD_RWLOCK_PREFER_READER_P(rwlock) \
  ((rwlock)->__data.__flags == 0)


/* Bits used in robust mutex implementation.  */
#define FUTEX_WAITERS		0x80000000
#define FUTEX_OWNER_DIED	0x40000000
#define FUTEX_TID_MASK		0x3fffffff


/* Internal variables.  */


/* Default pthread attributes.  */
extern struct pthread_attr __default_pthread_attr attribute_hidden;
extern int __default_pthread_attr_lock attribute_hidden;

/* Size and alignment of static TLS block.  */
extern size_t __static_tls_size attribute_hidden;
extern size_t __static_tls_align_m1 attribute_hidden;

/* Flag whether the machine is SMP or not.  */
extern int __is_smp attribute_hidden;

/* Thread descriptor handling.  */
extern list_t __stack_user;
hidden_proto (__stack_user)

/* Attribute handling.  */
extern struct pthread_attr *__attr_list attribute_hidden;
#ifndef lll_define
extern int __attr_list_lock attribute_hidden;
#else
lll_define (extern, __attr_list_lock attribute_hidden);
#endif

/* First available RT signal.  */
extern int __current_sigrtmin attribute_hidden;
/* Last available RT signal.  */
extern int __current_sigrtmax attribute_hidden;

/* Concurrency handling.  */
extern int __concurrency_level attribute_hidden;

/* Thread-local data key handling.  */
extern struct pthread_key_struct __pthread_keys[PTHREAD_KEYS_MAX];
hidden_proto (__pthread_keys)

/* Number of threads running.  */
extern unsigned int __nptl_nthreads attribute_hidden;

#ifndef __ASSUME_SET_ROBUST_LIST
/* Negative if we do not have the system call and we can use it.  */
extern int __set_robust_list_avail attribute_hidden;
#endif

/* Thread Priority Protection.  */
extern int __sched_fifo_min_prio attribute_hidden;
extern int __sched_fifo_max_prio attribute_hidden;
extern void __init_sched_fifo_prio (void) attribute_hidden;
extern int __pthread_tpp_change_priority (int prev_prio, int new_prio)
     attribute_hidden;
extern int __pthread_current_priority (void) attribute_hidden;

/* The library can run in debugging mode where it performs a lot more
   tests.  */
extern int __pthread_debug attribute_hidden;
/** For now disable debugging support.  */
#if 0
# define DEBUGGING_P __builtin_expect (__pthread_debug, 0)
# define INVALID_TD_P(pd) (DEBUGGING_P && __find_in_stack_list (pd) == NULL)
# define INVALID_NOT_TERMINATED_TD_P(pd) INVALID_TD_P (pd)
#else
# define DEBUGGING_P 0
/* Simplified test.  This will not catch all invalid descriptors but
   is better than nothing.  And if the test triggers the thread
   descriptor is guaranteed to be invalid.  */
# ifndef PTHREAD_T_IS_TID
# define INVALID_TD_P(pd) __builtin_expect ((pd)->tid <= 0, 0)
# define INVALID_NOT_TERMINATED_TD_P(pd) __builtin_expect ((pd)->tid < 0, 0)
# else
/* XXX: these macros evaluate pd twice */
#  define INVALID_TD_P(pd) __builtin_expect (!(pd) || (pd)->tid <= 0, 0)
#  define INVALID_NOT_TERMINATED_TD_P(pd) __builtin_expect (!(pd) || (pd)->tid < 0, 0)
# endif
#endif


/* Cancellation test.  */
#define CANCELLATION_P(self) \
  do {									      \
    int cancelhandling = THREAD_GETMEM (self, cancelhandling);		      \
    if (CANCEL_ENABLED_AND_CANCELED (cancelhandling))			      \
      {									      \
	THREAD_SETMEM (self, result, PTHREAD_CANCELED);			      \
	__do_cancel ();							      \
      }									      \
  } while (0)


extern void __pthread_unwind (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute __attribute ((__noreturn__))
#if !defined SHARED && !defined IS_IN_libpthread
     weak_function
#endif
     ;
extern void __pthread_unwind_next (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute __attribute ((__noreturn__))
#ifndef SHARED
     weak_function
#endif
     ;
extern void __pthread_register_cancel (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute;
extern void __pthread_unregister_cancel (__pthread_unwind_buf_t *__buf)
     __cleanup_fct_attribute;
#if defined NOT_IN_libc && defined IS_IN_libpthread
hidden_proto (__pthread_unwind)
hidden_proto (__pthread_unwind_next)
hidden_proto (__pthread_register_cancel)
hidden_proto (__pthread_unregister_cancel)
# ifdef SHARED
extern void attribute_hidden pthread_cancel_init (void);
extern void __unwind_freeres (void);
# endif
#endif


/* Called when a thread reacts on a cancellation request.  */
static inline void
__attribute ((noreturn, always_inline))
__do_cancel (void)
{
  struct pthread *self = THREAD_SELF;

  /* Make sure we get no more cancellations.  */
  THREAD_ATOMIC_BIT_SET (self, cancelhandling, EXITING_BIT);

  __pthread_unwind ((__pthread_unwind_buf_t *)
		    THREAD_GETMEM (self, cleanup_jmp_buf));
}


/* Set cancellation mode to asynchronous.  */
#define CANCEL_ASYNC() \
  __pthread_enable_asynccancel ()
/* Reset to previous cancellation mode.  */
#define CANCEL_RESET(oldtype) \
  __pthread_disable_asynccancel (oldtype)

#if !defined NOT_IN_libc
/* Same as CANCEL_ASYNC, but for use in libc.so.  */
# define LIBC_CANCEL_ASYNC() \
  __libc_enable_asynccancel ()
/* Same as CANCEL_RESET, but for use in libc.so.  */
# define LIBC_CANCEL_RESET(oldtype) \
  __libc_disable_asynccancel (oldtype)
# define LIBC_CANCEL_HANDLED() \
  __asm (".globl " __SYMBOL_PREFIX "__libc_enable_asynccancel"); \
  __asm (".globl " __SYMBOL_PREFIX "__libc_disable_asynccancel")
#elif defined NOT_IN_libc && defined IS_IN_libpthread
# define LIBC_CANCEL_ASYNC() CANCEL_ASYNC ()
# define LIBC_CANCEL_RESET(val) CANCEL_RESET (val)
# define LIBC_CANCEL_HANDLED() \
  __asm (".globl " __SYMBOL_PREFIX "__pthread_enable_asynccancel"); \
  __asm (".globl " __SYMBOL_PREFIX "__pthread_disable_asynccancel")
#elif defined NOT_IN_libc && defined IS_IN_librt
# define LIBC_CANCEL_ASYNC() \
  __librt_enable_asynccancel ()
# define LIBC_CANCEL_RESET(val) \
  __librt_disable_asynccancel (val)
# define LIBC_CANCEL_HANDLED() \
  __asm (".globl " __SYMBOL_PREFIX "__librt_enable_asynccancel"); \
  __asm (".globl " __SYMBOL_PREFIX "__librt_disable_asynccancel")
#else
# define LIBC_CANCEL_ASYNC()	0 /* Just a dummy value.  */
# define LIBC_CANCEL_RESET(val)	((void)(val)) /* Nothing, but evaluate it.  */
# define LIBC_CANCEL_HANDLED()	/* Nothing.  */
#endif

/* The signal used for asynchronous cancelation.  */
#ifndef SIGCANCEL
# define SIGCANCEL	__SIGRTMIN
#endif


/* Signal needed for the kernel-supported POSIX timer implementation.
   We can reuse the cancellation signal since we can distinguish
   cancellation from timer expirations.  */
#define SIGTIMER	SIGCANCEL


/* Signal used to implement the setuid et.al. functions.  */
#ifndef NO_SETXID_SUPPORT
#define SIGSETXID	(__SIGRTMIN + 1)
#endif

/* Used to communicate with signal handler.  */
extern struct xid_command *__xidcmd attribute_hidden;


/* Internal prototypes.  */

/* Thread list handling.  */
#ifndef PTHREAD_T_IS_TID
extern struct pthread *__find_in_stack_list (struct pthread *pd)
     attribute_hidden internal_function;
#else
extern struct pthread *__find_in_stack_list (pthread_t tid)
     attribute_hidden internal_function;
#endif

/* Deallocate a thread's stack after optionally making sure the thread
   descriptor is still valid.  */
extern void __free_tcb (struct pthread *pd) attribute_hidden internal_function;

/* Free allocated stack.  */
extern void __deallocate_stack (struct pthread *pd)
     attribute_hidden internal_function;

/* Mark all the stacks except for the current one as available.  This
   function also re-initializes the lock for the stack cache.  */
extern void __reclaim_stacks (void) attribute_hidden;

/* Make all threads's stacks executable.  */
extern int __make_stacks_executable (void **stack_endp)
     internal_function attribute_hidden;

/* longjmp handling.  */
extern void __pthread_cleanup_upto (__jmp_buf target, char *targetframe);
#if defined NOT_IN_libc && defined IS_IN_libpthread
hidden_proto (__pthread_cleanup_upto)
#endif


/* Functions with versioned interfaces.  */
extern int __pthread_create_2_1 (pthread_t *newthread,
				 const pthread_attr_t *attr,
				 void *(*start_routine) (void *), void *arg);
extern int __pthread_create_2_0 (pthread_t *newthread,
				 const pthread_attr_t *attr,
				 void *(*start_routine) (void *), void *arg);
extern int __pthread_attr_init_2_1 (pthread_attr_t *attr);
extern int __pthread_attr_init_2_0 (pthread_attr_t *attr);


/* Event handlers for libthread_db interface.  */
extern void __nptl_create_event (void);
extern void __nptl_death_event (void);
hidden_proto (__nptl_create_event)
hidden_proto (__nptl_death_event)

/* Register the generation counter in the libpthread with the libc.  */
#ifdef TLS_MULTIPLE_THREADS_IN_TCB
extern void __libc_pthread_init (unsigned long int *ptr,
				 void (*reclaim) (void),
				 const struct pthread_functions *functions)
     internal_function;
#else
extern int *__libc_pthread_init (unsigned long int *ptr,
				 void (*reclaim) (void),
				 const struct pthread_functions *functions)
     internal_function;

/* Variable set to a nonzero value either if more than one thread runs or ran,
   or if a single-threaded process is trying to cancel itself.  See
   nptl/descr.h for more context on the single-threaded process case.  */
extern int __pthread_multiple_threads attribute_hidden;
/* Pointer to the corresponding variable in libc.  */
extern int *__libc_multiple_threads_ptr attribute_hidden;
#endif

/* Find a thread given its TID.  */
extern struct pthread *__find_thread_by_id (pid_t tid) attribute_hidden
#ifdef SHARED
;
#else
weak_function;
#define __find_thread_by_id(tid) \
  (__find_thread_by_id ? (__find_thread_by_id) (tid) : (struct pthread *) NULL)
#endif

extern void __pthread_init_static_tls (struct link_map *) attribute_hidden;

extern size_t __pthread_get_minstack (const pthread_attr_t *attr);

/* Namespace save aliases.  */
extern int __pthread_getschedparam (pthread_t thread_id, int *policy,
				    struct sched_param *param);
extern int __pthread_setschedparam (pthread_t thread_id, int policy,
				    const struct sched_param *param);
extern int __pthread_setcancelstate (int state, int *oldstate);
extern int __pthread_mutex_init (pthread_mutex_t *__mutex,
				 const pthread_mutexattr_t *__mutexattr);
extern int __pthread_mutex_destroy (pthread_mutex_t *__mutex);
extern int __pthread_mutex_trylock (pthread_mutex_t *_mutex);
extern int __pthread_mutex_lock (pthread_mutex_t *__mutex);
extern int __pthread_mutex_cond_lock (pthread_mutex_t *__mutex)
     attribute_hidden internal_function;
extern void __pthread_mutex_cond_lock_adjust (pthread_mutex_t *__mutex)
     attribute_hidden internal_function;
extern int __pthread_mutex_unlock (pthread_mutex_t *__mutex);
extern int __pthread_mutex_unlock_usercnt (pthread_mutex_t *__mutex,
					   int __decr)
     attribute_hidden internal_function;
extern int __pthread_mutexattr_init (pthread_mutexattr_t *attr);
extern int __pthread_mutexattr_destroy (pthread_mutexattr_t *attr);
extern int __pthread_mutexattr_settype (pthread_mutexattr_t *attr, int kind);
extern int __pthread_attr_destroy (pthread_attr_t *attr);
extern int __pthread_attr_getdetachstate (const pthread_attr_t *attr,
					  int *detachstate);
extern int __pthread_attr_setdetachstate (pthread_attr_t *attr,
					  int detachstate);
extern int __pthread_attr_getinheritsched (const pthread_attr_t *attr,
					   int *inherit);
extern int __pthread_attr_setinheritsched (pthread_attr_t *attr, int inherit);
extern int __pthread_attr_getschedparam (const pthread_attr_t *attr,
					 struct sched_param *param);
extern int __pthread_attr_setschedparam (pthread_attr_t *attr,
					 const struct sched_param *param);
extern int __pthread_attr_getschedpolicy (const pthread_attr_t *attr,
					  int *policy);
extern int __pthread_attr_setschedpolicy (pthread_attr_t *attr, int policy);
extern int __pthread_attr_getscope (const pthread_attr_t *attr, int *scope);
extern int __pthread_attr_setscope (pthread_attr_t *attr, int scope);
extern int __pthread_attr_getstackaddr (const pthread_attr_t *__restrict
					__attr, void **__restrict __stackaddr);
extern int __pthread_attr_setstackaddr (pthread_attr_t *__attr,
					void *__stackaddr);
extern int __pthread_attr_getstacksize (const pthread_attr_t *__restrict
					__attr,
					size_t *__restrict __stacksize);
extern int __pthread_attr_setstacksize (pthread_attr_t *__attr,
					size_t __stacksize);
extern int __pthread_attr_getstack (const pthread_attr_t *__restrict __attr,
				    void **__restrict __stackaddr,
				    size_t *__restrict __stacksize);
extern int __pthread_attr_setstack (pthread_attr_t *__attr, void *__stackaddr,
				    size_t __stacksize);
extern int __pthread_rwlock_init (pthread_rwlock_t *__restrict __rwlock,
				  const pthread_rwlockattr_t *__restrict
				  __attr);
extern int __pthread_rwlock_destroy (pthread_rwlock_t *__rwlock);
extern int __pthread_rwlock_rdlock (pthread_rwlock_t *__rwlock);
extern int __pthread_rwlock_tryrdlock (pthread_rwlock_t *__rwlock);
extern int __pthread_rwlock_wrlock (pthread_rwlock_t *__rwlock);
extern int __pthread_rwlock_trywrlock (pthread_rwlock_t *__rwlock);
extern int __pthread_rwlock_unlock (pthread_rwlock_t *__rwlock);
extern int __pthread_cond_broadcast (pthread_cond_t *cond);
extern int __pthread_cond_destroy (pthread_cond_t *cond);
extern int __pthread_cond_init (pthread_cond_t *cond,
				const pthread_condattr_t *cond_attr);
extern int __pthread_cond_signal (pthread_cond_t *cond);
extern int __pthread_cond_wait (pthread_cond_t *cond, pthread_mutex_t *mutex);
extern int __pthread_cond_timedwait (pthread_cond_t *cond,
				     pthread_mutex_t *mutex,
				     const struct timespec *abstime);
extern int __pthread_condattr_destroy (pthread_condattr_t *attr);
extern int __pthread_condattr_init (pthread_condattr_t *attr);
extern int __pthread_key_create (pthread_key_t *key, void (*destr) (void *));
extern void *__pthread_getspecific (pthread_key_t key);
extern int __pthread_setspecific (pthread_key_t key, const void *value);
extern int __pthread_once (pthread_once_t *once_control,
			   void (*init_routine) (void));
extern int __pthread_atfork (void (*prepare) (void), void (*parent) (void),
			     void (*child) (void));
extern pthread_t __pthread_self (void);
extern int __pthread_equal (pthread_t thread1, pthread_t thread2);
extern int __pthread_kill (pthread_t threadid, int signo);
extern void __pthread_exit (void *value) __attribute__ ((__noreturn__));
extern int __pthread_setcanceltype (int type, int *oldtype);
extern int __pthread_enable_asynccancel (void) attribute_hidden;
extern void __pthread_disable_asynccancel (int oldtype)
     internal_function attribute_hidden;

#if defined NOT_IN_libc && defined IS_IN_libpthread
hidden_proto (__pthread_mutex_init)
hidden_proto (__pthread_mutex_destroy)
hidden_proto (__pthread_mutex_lock)
hidden_proto (__pthread_mutex_unlock)
hidden_proto (__pthread_rwlock_rdlock)
hidden_proto (__pthread_rwlock_wrlock)
hidden_proto (__pthread_rwlock_unlock)
hidden_proto (__pthread_key_create)
hidden_proto (__pthread_getspecific)
hidden_proto (__pthread_setspecific)
hidden_proto (__pthread_once)
#endif

extern int __pthread_cond_broadcast_2_0 (pthread_cond_2_0_t *cond);
extern int __pthread_cond_destroy_2_0 (pthread_cond_2_0_t *cond);
extern int __pthread_cond_init_2_0 (pthread_cond_2_0_t *cond,
				    const pthread_condattr_t *cond_attr);
extern int __pthread_cond_signal_2_0 (pthread_cond_2_0_t *cond);
extern int __pthread_cond_timedwait_2_0 (pthread_cond_2_0_t *cond,
					 pthread_mutex_t *mutex,
					 const struct timespec *abstime);
extern int __pthread_cond_wait_2_0 (pthread_cond_2_0_t *cond,
				    pthread_mutex_t *mutex);

extern int __pthread_getaffinity_np (pthread_t th, size_t cpusetsize,
				     cpu_set_t *cpuset);

/* The two functions are in libc.so and not exported.  */
extern int __libc_enable_asynccancel (void) attribute_hidden;
extern void __libc_disable_asynccancel (int oldtype)
     internal_function attribute_hidden;


/* The two functions are in librt.so and not exported.  */
extern int __librt_enable_asynccancel (void) attribute_hidden;
extern void __librt_disable_asynccancel (int oldtype)
     internal_function attribute_hidden;

#ifdef IS_IN_libpthread
/* Special versions which use non-exported functions.  */
extern void __pthread_cleanup_push (struct _pthread_cleanup_buffer *buffer,
				    void (*routine) (void *), void *arg)
     attribute_hidden;
# undef pthread_cleanup_push
# define pthread_cleanup_push(routine,arg) \
  { struct _pthread_cleanup_buffer _buffer;				      \
    __pthread_cleanup_push (&_buffer, (routine), (arg));

extern void __pthread_cleanup_pop (struct _pthread_cleanup_buffer *buffer,
				   int execute) attribute_hidden;
# undef pthread_cleanup_pop
# define pthread_cleanup_pop(execute) \
    __pthread_cleanup_pop (&_buffer, (execute)); }
#endif

extern void __pthread_cleanup_push_defer (struct _pthread_cleanup_buffer *buffer,
					  void (*routine) (void *), void *arg);
extern void __pthread_cleanup_pop_restore (struct _pthread_cleanup_buffer *buffer,
					   int execute);

/* Old cleanup interfaces, still used in libc.so.  */
extern void _pthread_cleanup_push (struct _pthread_cleanup_buffer *buffer,
				   void (*routine) (void *), void *arg);
extern void _pthread_cleanup_pop (struct _pthread_cleanup_buffer *buffer,
				  int execute);
extern void _pthread_cleanup_push_defer (struct _pthread_cleanup_buffer *buffer,
					 void (*routine) (void *), void *arg);
extern void _pthread_cleanup_pop_restore (struct _pthread_cleanup_buffer *buffer,
					  int execute);

extern void __nptl_deallocate_tsd (void) attribute_hidden;

extern int __nptl_setxid (struct xid_command *cmdp) attribute_hidden;
#ifndef SHARED
extern void __nptl_set_robust (struct pthread *self);
#endif

extern void __free_stacks (size_t limit) attribute_hidden;

extern void __wait_lookup_done (void) attribute_hidden;

#ifdef SHARED
# define PTHREAD_STATIC_FN_REQUIRE(name)
#else
# define PTHREAD_STATIC_FN_REQUIRE(name) __asm (".globl " #name);
#endif

/* Test if the mutex is suitable for the FUTEX_WAIT_REQUEUE_PI operation.  */
#if (defined lll_futex_wait_requeue_pi \
     && defined __ASSUME_REQUEUE_PI)
# define USE_REQUEUE_PI(mut) \
   ((mut) && (mut) != (void *) ~0l \
    && (((mut)->__data.__kind \
	 & (PTHREAD_MUTEX_PRIO_INHERIT_NP | PTHREAD_MUTEX_ROBUST_NORMAL_NP)) \
	== PTHREAD_MUTEX_PRIO_INHERIT_NP))
#else
# define USE_REQUEUE_PI(mut) 0
#endif

/* Returns 0 if POL is a valid scheduling policy.  */
static inline int
check_sched_policy_attr (int pol)
{
  if (pol == SCHED_OTHER || pol == SCHED_FIFO || pol == SCHED_RR)
    return 0;

  return EINVAL;
}

/* Returns 0 if PR is within the accepted range of priority values for
   the scheduling policy POL or EINVAL otherwise.  */
static inline int
check_sched_priority_attr (int pr, int pol)
{
  int min = __sched_get_priority_min (pol);
  int max = __sched_get_priority_max (pol);

  if (min >= 0 && max >= 0 && pr >= min && pr <= max)
    return 0;

  return EINVAL;
}

/* Returns 0 if ST is a valid stack size for a thread stack and EINVAL
   otherwise.  */
static inline int
check_stacksize_attr (size_t st)
{
  if (st >= PTHREAD_STACK_MIN)
    return 0;

  return EINVAL;
}

/* Defined in pthread_setaffinity.c.  */
extern size_t __kernel_cpumask_size attribute_hidden;
extern int __determine_cpumask_size (pid_t tid);

/* Returns 0 if CS and SZ are valid values for the cpuset and cpuset size
   respectively.  Otherwise it returns an error number.  */
static inline int
check_cpuset_attr (const cpu_set_t *cs, const size_t sz)
{
  if (__kernel_cpumask_size == 0)
    {
      int res = __determine_cpumask_size (THREAD_SELF->tid);
      if (res)
	return res;
    }

  /* Check whether the new bitmask has any bit set beyond the
     last one the kernel accepts.  */
  for (size_t cnt = __kernel_cpumask_size; cnt < sz; ++cnt)
    if (((char *) cs)[cnt] != '\0')
      /* Found a nonzero byte.  This means the user request cannot be
	 fulfilled.  */
      return EINVAL;

  return 0;
}

#endif	/* pthreadP.h */
