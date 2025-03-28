/* Copyright (C) 1991, 1995, 1996, 1997, 2002, 2008
    Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#include <inline-syscall.h>
#include <errno.h>
#include <sys/stat.h>

DECLARE_INLINE_SYSCALL (int, lstat64, const char *path, struct stat64 *buf);

int
__lxstat64 (int vers, const char *file, struct stat64 *buf)
{
  if(vers != _STAT_VER)
    {
      __set_errno (EINVAL);
      return -1;
    }

  return INLINE_SYSCALL (lstat64, 2, file, buf);
}

hidden_def (__lxstat64)
