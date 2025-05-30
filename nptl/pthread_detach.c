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

#include <errno.h>
#include <pthreadP.h>
#include <atomic.h>


int
pthread_detach (th)
     pthread_t th;
{
#ifndef PTHREAD_T_IS_TID
  struct pthread *pd = (struct pthread *) th;
#else
  struct pthread *pd = __find_in_stack_list (th);
#endif

  /* Make sure the descriptor is valid.  */
  if (INVALID_NOT_TERMINATED_TD_P (pd))
    /* Not a valid thread handle.  */
    return ESRCH;

  int result = 0;

  /* Mark the thread as detached.  */
  if (atomic_compare_and_exchange_bool_acq (&pd->joinid, pd, NULL))
    {
      /* There are two possibilities here.  First, the thread might
	 already be detached.  In this case we return EINVAL.
	 Otherwise there might already be a waiter.  The standard does
	 not mention what happens in this case.  */
      if (IS_DETACHED (pd))
	result = EINVAL;
    }
  else
    /* Check whether the thread terminated meanwhile.  In this case we
       will just free the TCB.  */
    if ((pd->cancelhandling & EXITING_BITMASK) != 0)
      /* Note that the code in __free_tcb makes sure each thread
	 control block is freed only once.  */
      __free_tcb (pd);
#ifdef NEED_TDETACH
    else
      {
        INTERNAL_SYSCALL_DECL (err);
        result = INTERNAL_SYSCALL (tdetach, err, 1, th);
      }
#endif

  return result;
}
