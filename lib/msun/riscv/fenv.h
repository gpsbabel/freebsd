/*-
 * Copyright (c) 2015 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract FA8750-10-C-0237
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * This software was developed by the University of Cambridge Computer
 * Laboratory as part of the CTSRD Project, with support from the UK Higher
 * Education Innovation Fund (HEIF).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * RISC-V FPU, Chapter 6, The RISC-V Instruction Set Manual,
 * Volume I: User-Level ISA, Version 2.0
 */

#ifndef	_FENV_H_
#define	_FENV_H_

#include <sys/_types.h>

#ifndef	__fenv_static
#define	__fenv_static	static
#endif

typedef	__uint64_t	fenv_t;
typedef	__uint64_t	fexcept_t;

/* Exception flags */
#define	FE_INVALID	0x00000001
#define	FE_DIVBYZERO	0x00000002
#define	FE_OVERFLOW	0x00000004
#define	FE_UNDERFLOW	0x00000008
#define	FE_INEXACT	0x00000010
#define	FE_ALL_EXCEPT	(FE_DIVBYZERO | FE_INEXACT | \
			 FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW)

/*
 * RISC-V Rounding modes
 */
#define	FRM_SHIFT	5
#define	FRM_MASK	(0x7 << FRM_SHIFT)
#define	FRM_RNE		0 /* Round to Nearest, ties to Even */
#define	FRM_RTZ		1 /* Round towards Zero */
#define	FRM_RDN		2 /* Round Down (towards - infinum) */
#define	FRM_RUP		3 /* Round Up (towards + infinum) */
#define	FRM_RMM		4 /* Round to Nearest, ties to Max Magnitude */
#define	_ROUND_SHIFT	5
#define	_ROUND_MASK	(0x7 << _ROUND_SHIFT)

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define	FE_DFL_ENV	(&__fe_dfl_env)

/* We need to be able to map status flag positions to mask flag positions */
#define _FPUSW_SHIFT	0
#define	_ENABLE_MASK	(FE_ALL_EXCEPT << _FPUSW_SHIFT)

#define	__wr_fcsr(__r)	__asm __volatile("csrw fcsr, %0" :: "r" (__r))
#define	__rd_fcsr(__r)	__asm __volatile("csrr %0, fcsr" : "=r" (*(__r)))
#define	__set_fcsr(__r)	__asm __volatile("csrs fcsr, %0" :: "r" (__r))
#define	__clr_fcsr(__r)	__asm __volatile("csrc fcsr, %0" :: "r" (__r))

__fenv_static __inline int
feclearexcept(int __excepts)
{

	__clr_fcsr(__excepts);

	return (0);
}

__fenv_static inline int
fegetexceptflag(fexcept_t *__flagp, int __excepts)
{
	fexcept_t __r;

	__rd_fcsr(&__r);
	*__flagp = (__r & __excepts);
	return (0);
}

__fenv_static inline int
fesetexceptflag(const fexcept_t *__flagp, int __excepts)
{
	fexcept_t __r;

	__r = (*__flagp & __excepts);
	__set_fcsr(__r);

	return (0);
}

__fenv_static inline int
feraiseexcept(int __excepts)
{
	fexcept_t __r;

	__r = __excepts;
	__set_fcsr(__r);

	return (0);
}

__fenv_static inline int
fetestexcept(int __excepts)
{
	fexcept_t __r;

	__rd_fcsr(&__r);
	return (__r & __excepts);
}

__fenv_static inline int
fegetround(void)
{
	fenv_t __r;

	__rd_fcsr(&__r);
	return ((__r >> _ROUND_SHIFT) & _ROUND_MASK);
}

__fenv_static inline int
fesetround(int __round)
{
	fenv_t __r;

	if (__round & ~_ROUND_MASK)
		return (-1);

	__rd_fcsr(&__r);
	__r &= ~(_ROUND_MASK << _ROUND_SHIFT);
	__r |= __round << _ROUND_SHIFT;
	__wr_fcsr(__r);

	return (0);
}

__fenv_static inline int
fegetenv(fenv_t *__envp)
{
	fenv_t __r;

	__rd_fcsr(&__r);
	*__envp = __r & (FE_ALL_EXCEPT | (_ROUND_MASK << _ROUND_SHIFT));

	return (0);
}

__fenv_static inline int
feholdexcept(fenv_t *__envp)
{
	fenv_t __r;

	__rd_fcsr(&__r);
	*__envp |= __r & (FE_ALL_EXCEPT | (_ROUND_MASK << _ROUND_SHIFT));

	__r &= ~(_ENABLE_MASK);
	__wr_fcsr(__r);

	return (0);
}

__fenv_static inline int
fesetenv(const fenv_t *__envp)
{

	__wr_fcsr((*__envp) & (FE_ALL_EXCEPT | (_ROUND_MASK << _ROUND_SHIFT)));
	return (0);
}

__fenv_static inline int
feupdateenv(const fenv_t *__envp)
{
	fexcept_t __r;

	__rd_fcsr(&__r);
	fesetenv(__envp);
	feraiseexcept(__r & FE_ALL_EXCEPT);
	return (0);
}

#if __BSD_VISIBLE

/* We currently provide no external definitions of the functions below. */

static inline int
feenableexcept(int __mask)
{
	fenv_t __old_r, __new_r;

	__rd_fcsr(&__old_r);
	__new_r = __old_r | ((__mask & FE_ALL_EXCEPT) << _FPUSW_SHIFT);
	__wr_fcsr(__new_r);
	return ((__old_r >> _FPUSW_SHIFT) & FE_ALL_EXCEPT);
}

static inline int
fedisableexcept(int __mask)
{
	fenv_t __old_r, __new_r;

	__rd_fcsr(&__old_r);
	__new_r = __old_r & ~((__mask & FE_ALL_EXCEPT) << _FPUSW_SHIFT);
	__wr_fcsr(__new_r);
	return ((__old_r >> _FPUSW_SHIFT) & FE_ALL_EXCEPT);
}

static inline int
fegetexcept(void)
{
	fenv_t __r;

	__rd_fcsr(&__r);
	return ((__r & _ENABLE_MASK) >> _FPUSW_SHIFT);
}

#endif /* __BSD_VISIBLE */

__END_DECLS

#endif	/* !_FENV_H_ */
