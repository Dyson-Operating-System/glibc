/* Copyright (C) 1992,1993,1995-2000,2002-2006,2007-2012
	Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper, <drepper@gnu.org>, August 1995.
   OpenSolaris bits contributed by David Bartley
    <dtbartle@csclub.uwaterloo.ca>, 2008.
   OpenSolaris bits for amd64 contributed by Igor Pashev
    <pashev.igor@gmail.com>, 2012.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _OPENSOLARIS_X86_64_SYSDEP_H
#define _OPENSOLARIS_X86_64_SYSDEP_H 1

#define DECLARE_INLINE_SYSCALL(ret, name, args...) \
    extern ret __syscall_##name (args)

/* There is some commonality.  */
#include <sysdeps/unix/x86_64/sysdep.h>
#include <syscallP.h>

/* XXX: This needs to come before #include <tls.h>.  */

/* Pointer mangling support.  */
#if defined NOT_IN_libc && defined IS_IN_rtld
/* We cannot use the thread descriptor because in ld.so we use setjmp
   earlier than the descriptor is initialized.  Using a global variable
   is too complicated here since we have no PC-relative addressing mode.  */
#else
# ifdef __ASSEMBLER__
#  define PTR_MANGLE(reg)   xorq %fs:POINTER_GUARD, reg;              \
                rolq $17, reg
#  define PTR_DEMANGLE(reg) rorq $17, reg;                     \
                xorq %fs:POINTER_GUARD, reg
# else
#  define PTR_MANGLE(var)   asm ("xorq %%fs:%c2, %0\n"            \
                     "rolq $17, %0"                \
                     : "=r" (var)                 \
                     : "0" (var),                 \
                       "i" (offsetof (tcbhead_t,          \
                              pointer_guard)))
#  define PTR_DEMANGLE(var) asm ("rorq $17, %0\n"                  \
                     "xorq %%fs:%c2, %0"              \
                     : "=r" (var)                 \
                     : "0" (var),                 \
                       "i" (offsetof (tcbhead_t,          \
                              pointer_guard)))
# endif
#endif


/* Defines RTLD_PRIVATE_ERRNO and USE_DL_SYSINFO.  */
#include <dl-sysdep.h>
#include <tls.h>

/* It's too much trouble to pull in all of errno.h just for this */
#ifndef ERESTART
# define ERESTART   91  /* Restartable system call.  */
# define EINTR		4	/* Interrupted system call.  */
#endif

#ifdef __ASSEMBLER__

/* This is needed so that we know when to set %edx to -1 on error.  */
#ifdef SYSCALL_64BIT_RETURN
# define SYSCALL_64BIT_RETURN_ASM   orl $-1, %edx;
#else
# define SYSCALL_64BIT_RETURN_ASM
#endif

#ifdef SYSCALL_RESTARTABLE
# define DO_RESTART \
    cmpl $ERESTART, %eax; \
    je L(restart);
#else
# define DO_RESTART
#endif

/* We don't want the label for the error handle to be global when we define
   it here.  */
#ifdef PIC
# define SYSCALL_ERROR_LABEL 0f
#else
# define SYSCALL_ERROR_LABEL syscall_error
#endif

#undef  PSEUDO
#define PSEUDO(name, syscall_name, args)                      \
  .text;                                      \
  ENTRY (name)                                    \
  L(restart):                                       \
    DO_CALL (syscall_name, args);                         \
    jnb 2f;										\
    DO_RESTART                             \
    jmp SYSCALL_ERROR_LABEL;                           \
2:                                              \
  L(pseudo_end):

/* To "subcall" we have to move arguments for freeing the first
 * position for a "subcall" number. E. g. for zone_list(a, b):
 *
 * zone_list(a, b):     rdi=a, rsi=b          _ zone_list
 * |                                         /
 * \_ zone(4, a, b):    rax=<syscall#>, rdi=4, rsi=a, rdx=b
 *
 * In general:
 * syscall -> eax
 * subcall -> rdi -> rsi -> rdx -> rcx -> r8 -> r9
 * But before syscall itself we have to move rcx -> r10,
 * so we copy rdx directly into r10 (rdx -> r10) and
 * use DO_SYSCALL instead of DO_CALL.
*/
#define	PSEUDO_SUBCALL(name, syscall_name, subcall_name, args)				      \
  .text;                                      \
  ENTRY (name)                                    \
    SUBCALL_MOVE_ARGS_##args                       \
    movq $SYS_ify (SUB_##subcall_name), %rdi;							\
  L(restart):                                       \
    DO_SYSCALL (syscall_name);                         \
    jnb 2f;										\
    DO_RESTART                             \
    jmp SYSCALL_ERROR_LABEL;                           \
2:											\
  L(pseudo_end):

#undef  PSEUDO_END
#define PSEUDO_END(name)                              \
  SYSCALL_ERROR_HANDLER                               \
  END (name)

#undef  PSEUDO_NOERRNO
#define PSEUDO_NOERRNO(name, syscall_name, args)                  \
  .text;                                      \
  ENTRY (name)                                    \
    DO_CALL (syscall_name, args)

#define	PSEUDO_SUBCALL_NOERRNO(name, syscall_name, subcall_name, args)	      \
  .text;                                      \
  ENTRY (name)                                    \
    SUBCALL_MOVE_ARGS_##args                       \
    movq $SYS_ify (SUB_##subcall_name), %rdi;							\
    DO_SYSCALL (syscall_name);                         \

#undef  PSEUDO_END_NOERRNO
#define PSEUDO_END_NOERRNO(name)                          \
  END (name)

#define ret_NOERRNO ret

/* The function has to return the error code.  */
#undef  PSEUDO_ERRVAL
#define PSEUDO_ERRVAL(name, syscall_name, args) \
  .text;                                      \
  ENTRY (name)                                    \
    DO_CALL (syscall_name, args);                         \
    jnb 1f;										\
    cmpl $ERESTART, %eax;                   \
    jne 2f;                        \
    movl $EINTR, %eax;                  \
    jmp 2f;                             \
1:  xorl %eax, %eax;                        \
2:;

#define PSEUDO_SUBCALL_ERRVAL(name, syscall_name, subcall_name, args)         \
  .text;                                      \
  ENTRY (name)                                    \
    SUBCALL_MOVE_ARGS_##args                       \
    movq $SYS_ify (SUB_##subcall_name), %rdi;							\
  L(restart):                                       \
    DO_SYSCALL (syscall_name);                         \
    jnb 1f;										\
    cmpl $ERESTART, %eax;                   \
    jne 2f;                                      \
    movl $EINTR, %eax;                  \
    jmp 2f;                             \
1:  xorl %eax, %eax;                        \
2:											\


#define SUBCALL_MOVE_ARGS_0 /* cannot happen */
#define SUBCALL_MOVE_ARGS_1 /* nothing */
#define SUBCALL_MOVE_ARGS_2 movq %rdi, %rsi;
#define SUBCALL_MOVE_ARGS_3 movq %rsi, %rdx; SUBCALL_MOVE_ARGS_2

/* Copy directly into r10 skipping rcx.
 * See comment before PSEUDO_SUBCALL: */
#define SUBCALL_MOVE_ARGS_4 movq %rdx, %r10; SUBCALL_MOVE_ARGS_3

#define SUBCALL_MOVE_ARGS_5 movq %rcx, %r8; SUBCALL_MOVE_ARGS_4
#define SUBCALL_MOVE_ARGS_6 movq %r8, %r9; SUBCALL_MOVE_ARGS_5



#undef  PSEUDO_END_ERRVAL
#define PSEUDO_END_ERRVAL(name) \
  END (name)

#define ret_ERRVAL ret

#undef  PSEUDO_FASTTRAP
#define PSEUDO_FASTTRAP(name, trap_name, args)                \
  .text;                                      \
  ENTRY (name)                                    \
    movl $T_##trap_name, %eax;                 \
    int $T_FASTTRAP;                    \
  L(pseudo_end):

# if defined PIC && defined RTLD_PRIVATE_ERRNO
#  define SYSCALL_SET_ERRNO			\
  lea rtld_errno(%rip), %RCX_LP;		\
  neg %eax;					\
  movl %eax, (%rcx)
# else
#  ifndef NOT_IN_libc
#   define SYSCALL_ERROR_ERRNO __libc_errno
#  else
#   define SYSCALL_ERROR_ERRNO errno
#  endif
#  define SYSCALL_SET_ERRNO			\
  movq SYSCALL_ERROR_ERRNO@GOTTPOFF(%rip), %rcx;\
  neg %eax;					\
  movl %eax, %fs:(%rcx);
# endif

# ifndef PIC
#  define SYSCALL_ERROR_HANDLER	/* Nothing here; code in sysdep.S is used.  */
# else
#  define SYSCALL_ERROR_HANDLER			\
0:						\
  SYSCALL_SET_ERRNO;				\
  or $-1, %RAX_LP;				\
  ret;
# endif	/* PIC */



/*
 * We define "pure" syscall for using in PSEUDO_SUBCALL* macros
 * to avoid overhead rdx -> rcx -> r10, and just use rdx -> r10
 */
#define DO_SYSCALL(syscall_name)                              \
    movl $SYS_ify (syscall_name), %eax;                       \
    syscall

#undef  DO_CALL
#define DO_CALL(syscall_name, args)                           \
    DOARGS_##args                                             \
    DO_SYSCALL(syscall_name)

/*
 * See nice comment in sysdeps/unix/sysv/linux/x86_64/sysdep.h
 * about registers usage in function calls and in syscalls.
 * kOpenSolaris/x86_64 uses the same syscall convention as linux.
 */
# define DOARGS_0 /* nothing */
# define DOARGS_1 /* nothing */
# define DOARGS_2 /* nothing */
# define DOARGS_3 /* nothing */
# define DOARGS_4 movq %rcx, %r10;
# define DOARGS_5 DOARGS_4
# define DOARGS_6 DOARGS_5
# define DOARGS_7 DOARGS_6
# define DOARGS_8 DOARGS_7

#else   /* !__ASSEMBLER__ */

/* TODO: This is a terrible implementation of INTERNAL_SYSCALL.  */

/* Some NPTL code calls these macros.  */
# include <errno.h>
# define INTERNAL_SYSCALL(name, err, nr, args...) __internal_##name##_##nr (&(err), args)
# define INTERNAL_SYSCALL_DECL(err) int err = 0;
# define INTERNAL_SYSCALL_ERROR_P(val, err) (err != 0)
# define INTERNAL_SYSCALL_ERRNO(val, err) (err)

#endif /* __ASSEMBLER__ */

#endif /* _OPENSOLARIS_X86_64_SYSDEP_H */
