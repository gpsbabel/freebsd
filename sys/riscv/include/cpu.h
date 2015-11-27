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

#ifndef _MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

#include <machine/atomic.h>
#include <machine/frame.h>

//#define	TRAPF_PC(tfp)		((tfp)->tf_lr)
//#define	TRAPF_USERMODE(tfp)	(((tfp)->tf_elr & (1ul << 63)) == 0)
//#define	TRAPF_PC(tfp)		((tfp)->tf_sepc)

#define	TRAPF_PC(tfp)		((tfp)->tf_x[1])
#define	TRAPF_USERMODE(tfp)	(((tfp)->tf_sepc & (1ul << 63)) == 0)

//#define	cpu_getstack(td)	((td)->td_frame->tf_sp)
//#define	cpu_setstack(td, sp)	((td)->td_frame->tf_sp = (sp))
#define	cpu_getstack(td)	((td)->td_frame->tf_x[2])
#define	cpu_setstack(td, sp)	((td)->td_frame->tf_x[2] = (sp))
#define	cpu_spinwait()		/* nothing */

/* Extract CPU affinity levels 0-3 */
#define	CPU_AFF0(mpidr)	(u_int)(((mpidr) >> 0) & 0xff)
#define	CPU_AFF1(mpidr)	(u_int)(((mpidr) >> 8) & 0xff)
#define	CPU_AFF2(mpidr)	(u_int)(((mpidr) >> 16) & 0xff)
#define	CPU_AFF3(mpidr)	(u_int)(((mpidr) >> 32) & 0xff)
#define	CPU_AFF_MASK	0xff00ffffffUL	/* Mask affinity fields in MPIDR_EL1 */

#ifdef _KERNEL

/*
 * 0x0000         CPU ID unimplemented
 * 0x0001         UC Berkeley Rocket repo
 * 0x0002­0x7FFE  Reserved for open-source repos
 * 0x7FFF         Reserved for extension
 * 0x8000         Reserved for anonymous source
 * 0x8001­0xFFFE  Reserved for proprietary implementations
 * 0xFFFF         Reserved for extension
 */

#define	CPU_IMPL_UNIMPLEMEN	0x0
#define	CPU_IMPL_UCB_ROCKET	0x1

#define	CPU_PART_RV32I	0x0
#define	CPU_PART_RV32E	0x1
#define	CPU_PART_RV64I	0x2
#define	CPU_PART_RV128I	0x3

#define	CPU_IMPL(midr)	(((midr) >> 24) & 0xff)
#define	CPU_PART(midr)	(((midr) >> 4) & 0xfff)
#define	CPU_VAR(midr)	(((midr) >> 20) & 0xf)
#define	CPU_REV(midr)	(((midr) >> 0) & 0xf)

#define	CPU_IMPL_TO_MIDR(val)	(((val) & 0xff) << 24)
#define	CPU_PART_TO_MIDR(val)	(((val) & 0xfff) << 4)
#define	CPU_VAR_TO_MIDR(val)	(((val) & 0xf) << 20)
#define	CPU_REV_TO_MIDR(val)	(((val) & 0xf) << 0)

#define	CPU_IMPL_MASK	(0xff << 24)
#define	CPU_PART_MASK	(0xfff << 4)
#define	CPU_VAR_MASK	(0xf << 20)
#define	CPU_REV_MASK	(0xf << 0)

#define CPU_MATCH(mask, impl, part, var, rev)						\
    (((mask) & PCPU_GET(midr)) == (CPU_IMPL_TO_MIDR((impl)) |		\
    CPU_PART_TO_MIDR((part)) | CPU_VAR_TO_MIDR((var)) |				\
    CPU_REV_TO_MIDR((rev))))

extern char btext[];
extern char etext[];

extern uint64_t __cpu_affinity[];

void	cpu_halt(void) __dead2;
void	cpu_reset(void) __dead2;
void	fork_trampoline(void);
void	identify_cpu(void);
void	swi_vm(void *v);

#define	CPU_AFFINITY(cpu)	__cpu_affinity[(cpu)]

static __inline uint64_t
get_cyclecount(void)
{

	/* TODO: This is bogus */
	return (1);
}

#define	ADDRESS_TRANSLATE_FUNC(stage)				\
static inline uint64_t						\
arm64_address_translate_ ##stage (uint64_t addr)		\
{								\
	uint64_t ret;						\
								\
	__asm __volatile(					\
	    "at " __STRING(stage) ", %1 \n"					\
	    "mrs %0, par_el1" : "=r"(ret) : "r"(addr));		\
								\
	return (ret);						\
}

ADDRESS_TRANSLATE_FUNC(s1e0r)
ADDRESS_TRANSLATE_FUNC(s1e0w)
ADDRESS_TRANSLATE_FUNC(s1e1r)
ADDRESS_TRANSLATE_FUNC(s1e1w)

#endif

#endif /* !_MACHINE_CPU_H_ */
