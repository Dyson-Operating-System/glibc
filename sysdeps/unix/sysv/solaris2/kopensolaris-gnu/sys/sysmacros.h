/* Copyright (C) 2008 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by David Bartley <dtbartle@csclub.uwaterloo.ca>, 2008.

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

#ifndef _SYS_SYSMACROS_H
#define _SYS_SYSMACROS_H

#include <features.h>
#include <sys/isa_defs.h>

#ifdef __USE_MISC

# define dtob(DD)	((DD) << DEV_BSHIFT)
# define btod(BB)	(((BB) + DEV_BSIZE - 1) >> DEV_BSHIFT)
# define btodt(BB)	((BB) >> DEV_BSHIFT)
# define lbtod(BB)	(((offset_t)(BB) + DEV_BSIZE - 1) >> DEV_BSHIFT)

# define O_BITSMAJOR	7
# define O_BITSMINOR	8
# define O_MAXMAJ	0x7f
# define O_MAXMIN	0xff
# define L_BITSMAJOR32	14
# define L_BITSMINOR32	18
# define L_MAXMAJ32	0x3fff
# define L_MAXMIN32	0x3ffff

# ifdef _LP64
#  define L_BITSMAJOR	32
#  define L_BITSMINOR	32
#  define L_MAXMAJ	0xfffffffful
#  define L_MAXMIN	0xfffffffful
# else
#  define L_BITSMAJOR	L_BITSMAJOR32
#  define L_BITSMINOR	L_BITSMINOR32
#  define L_MAXMAJ	L_MAXMAJ32
#  define L_MAXMIN	L_MAXMIN32
# endif

# define major(x)	(major_t)(((dev_t)(x)) >> L_BITSMINOR)
# define getmajor(x)	major(x)
# define minor(x)	(minor_t)((x) & L_MAXMIN)
# define getminor(x)	minor(x)

# define makedev(x, y)	(dev_t)(((dev_t)(x) << L_BITSMINOR) | \
	((y) & L_MAXMIN))
# define makedevice(x, y)	makedev(x, y)

# define emajor(x)	(major_t)(((unsigned int)(x) >> O_BITSMINOR) > \
	O_MAXMAJ) ? NODEV : (((unsigned int)(x) >> O_BITSMINOR) & O_MAXMAJ)
# define eminor(x)	(minor_t)((x) & O_MAXMIN)

# define cmpdev(x)	(o_dev_t)((((x) >> L_BITSMINOR) > O_MAXMAJ || \
	((x) & L_MAXMIN) > O_MAXMIN) ? NODEV : \
	((((x) >> L_BITSMINOR) << O_BITSMINOR) | ((x) & O_MAXMIN)))
# define expdev(x)	(dev_t)(((dev_t)(((x) >> O_BITSMINOR) & O_MAXMAJ) << \
	L_BITSMINOR) | ((x) & O_MAXMIN))

# ifndef howmany
#  define howmany(x, y)	(((x)+((y)-1))/(y))
# endif

# ifndef roundup
#  define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
# endif

# define IS_P2ALIGNED(v, a)	((((uintptr_t)(v)) & ((uintptr_t)(a) - 1)) == 0)
# define ISP2(x)			(((x) & ((x) - 1)) == 0)

# define P2ALIGN(x, align)		((x) & -(align))
# define P2PHASE(x, align)		((x) & ((align) - 1))
# define P2NPHASE(x, align)		(-(x) & ((align) - 1))
# define P2ROUNDUP(x, align)		(-(-(x) & -(align)))
# define P2END(x, align)			(-(~(x) & -(align)))
# define P2PHASEUP(x, align, phase)	((phase) - (((phase) - (x)) & -(align)))
# define P2CROSS(x, y, align)		(((x) ^ (y)) > (align) - 1)
# define P2SAMEHIGHBIT(x, y)		(((x) ^ (y)) < ((x) & (y)))

# define P2ALIGN_TYPED(x, align, type)	((type)(x) & -(type)(align))
# define P2PHASE_TYPED(x, align, type)	((type)(x) & ((type)(align) - 1))
# define P2NPHASE_TYPED(x, align, type)	(-(type)(x) & ((type)(align) - 1))
# define P2ROUNDUP_TYPED(x, align, type)	(-(-(type)(x) & -(type)(align)))
# define P2END_TYPED(x, align, type)	(-(~(type)(x) & -(type)(align)))
# define P2PHASEUP_TYPED(x, align, phase, type)	((type)(phase) - \
	(((type)(phase) - (type)(x)) & -(type)(align)))
# define P2CROSS_TYPED(x, y, align, type)	(((type)(x) ^ (type)(y)) > \
	(type)(align) - 1)
# define P2SAMEHIGHBIT_TYPED(x, y, type)	(((type)(x) ^ (type)(y)) < \
	((type)(x) & (type)(y)))

/* For compatibility with GNU/Linux.  */
# define gnu_dev_major(dev) major (dev)
# define gnu_dev_minor(dev) minor (dev)
# define gnu_dev_makedev(maj, min) makedev (maj, min)

#endif /* __USE_MISC */

#endif	/* _SYS_SYSMACROS_H */
