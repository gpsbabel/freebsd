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

#ifndef _MACHINE_ASM_H_
#define	_MACHINE_ASM_H_

#undef __FBSDID
#if !defined(lint) && !defined(STRIP_FBSDID)
#define	__FBSDID(s)     .ident s
#else
#define	__FBSDID(s)     /* nothing */
#endif

#define	_C_LABEL(x)	x

#define	ENTRY(sym)						\
	.text; .globl sym; .type sym,@function; .align 2; sym:
#define	EENTRY(sym)						\
	.globl	sym; sym:
#define	END(sym) .size sym, . - sym
#define	EEND(sym)

#define	WEAK_REFERENCE(sym, alias)				\
	.weak alias;						\
	.set alias,sym

#if defined(PIC)
#define	PIC_SYM(x,y)	x ## @ ## y
#else
#define	PIC_SYM(x,y)	x
#endif

/*
 * Sets the trap fault handler. The exception handler will return to the
 * address in the handler register_t on a data abort or the x0 register_t to
 * clear the handler. The tmp parameter should be a register_t able to hold
 * the temporary data.
 */
#define	SET_FAULT_HANDLER(handler, tmp)					\
	la	tmp, pcpup;						\
	ld	tmp, 0(tmp);						\
	ld	tmp, PC_CURTHREAD(tmp);					\
	ld	tmp, TD_PCB(tmp);		/* Load the pcb */	\
	sd	handler, PCB_ONFAULT(tmp)	/* Set the handler */

#define	CSR_ZIMM(val)	\
	(__builtin_constant_p(val) && ((u_long)(val) < 0x20))

#define	csr_swap(csr, val)					\
({								\
								\
	if (CSR_ZIMM(val))  					\
		__asm __volatile(				\
			"csrrw %0, " #csr ", %1"		\
			: "=r" (val) : "i" (val));		\
	else 							\
		__asm __volatile(				\
			"csrrw %0, " #csr ", %1"		\
			: "=r" (val) : "r" (val));		\
								\
	val;							\
})

#define	csr_write(csr, val)					\
({								\
								\
	if (CSR_ZIMM(val)) 					\
		__asm __volatile(				\
			"csrw " #csr ", %0" : : "i" (val));	\
	else 							\
		__asm __volatile(				\
			"csrw " #csr ", %0" : : "r" (val));	\
								\
})

#define	csr_set(csr, val)					\
({								\
								\
	if (CSR_ZIMM(val)) 					\
		__asm __volatile(				\
			"csrs " #csr ", %0" : : "i" (val));	\
	else							\
		__asm __volatile(				\
			"csrs " #csr ", %0" : : "r" (val));	\
								\
})

#define	csr_clear(csr, val)					\
({								\
								\
	if (CSR_ZIMM(val))					\
		__asm __volatile(				\
			"csrc " #csr ", %0" : : "i" (val));	\
	else							\
		__asm __volatile(				\
			"csrc " #csr ", %0" : : "r" (val));	\
})

#define	csr_read(csr)						\
({								\
	u_long val;						\
								\
	__asm __volatile(					\
		"csrr %0, " #csr : "=r" (val));			\
								\
	val;							\
})

#endif /* _MACHINE_ASM_H_ */
