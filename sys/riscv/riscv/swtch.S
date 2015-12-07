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
 */

#include "assym.s"
#include "opt_sched.h"

#include <machine/asm.h>

__FBSDID("$FreeBSD$");

/*
 * void cpu_throw(struct thread *old, struct thread *new)
 */
ENTRY(cpu_throw)
	la	x14, pcpup
	ld	x14, 0(x14)
	/* Store the new curthread */
	sd	a1, PC_CURTHREAD(x14)
	/* And the new pcb */
	ld	x13, TD_PCB(a1)
	sd	x13, PC_CURPCB(x14)

	sfence.vm

	/* Switch to the new pmap */
	ld	t0, PCB_L1ADDR(x13)
	csrw	sptbr0, t0

	/* Invalidate the TLB */
	sfence.vm

	/* ra, sp, gp, tp */
	ld	x1, (PCB_REGS + 1 * 8)(x13)
	ld	x2, (PCB_REGS + 2 * 8)(x13)
	ld	x3, (PCB_REGS + 3 * 8)(x13)
	ld	x4, (PCB_REGS + 4 * 8)(x13)

	/* we use these in fork_trampoline */
	//ld	x5, (PCB_REGS + 5 * 8)(x13)
	//ld	x6, (PCB_REGS + 6 * 8)(x13)

	/* s0 - s11 */
	ld	x8, (PCB_REGS + 8 * 8)(x13)
	ld	x9, (PCB_REGS + 9 * 8)(x13)
	ld	x18, (PCB_REGS + 18 * 8)(x13)
	ld	x19, (PCB_REGS + 19 * 8)(x13)
	ld	x20, (PCB_REGS + 20 * 8)(x13)
	ld	x21, (PCB_REGS + 21 * 8)(x13)
	ld	x22, (PCB_REGS + 22 * 8)(x13)
	ld	x23, (PCB_REGS + 23 * 8)(x13)
	ld	x24, (PCB_REGS + 24 * 8)(x13)
	ld	x25, (PCB_REGS + 25 * 8)(x13)
	ld	x26, (PCB_REGS + 26 * 8)(x13)
	ld	x27, (PCB_REGS + 27 * 8)(x13)

	ret

	//la	a0, .Lcpu_throw_panic_str
	//call	panic
#if 0
#ifdef VFP
	/* Backup the new thread pointer around a call to C code */
	mov	x19, x1
	bl	vfp_discard
	mov	x1, x19
#endif

	/* Store the new curthread */
	str	x1, [x18, #PC_CURTHREAD]
	/* And the new pcb */
	ldr	x4, [x1, #TD_PCB]
	str	x4, [x18, #PC_CURPCB]

	/*
	 * TODO: We may need to flush the cache here.
	 */

	/* Switch to the new pmap */
	ldr	x5, [x4, #PCB_L1ADDR]
	msr	ttbr0_el1, x5
	isb

	/* Invalidate the TLB */
	dsb	sy
	tlbi	vmalle1is
	dsb	sy
	isb

	/* Restore the registers */
	ldp	x5, x6, [x4, #PCB_SP]
	mov	sp, x5
	msr	tpidr_el0, x6
	ldp	x8, x9, [x4, #PCB_REGS + 8 * 8]
	ldp	x10, x11, [x4, #PCB_REGS + 10 * 8]
	ldp	x12, x13, [x4, #PCB_REGS + 12 * 8]
	ldp	x14, x15, [x4, #PCB_REGS + 14 * 8]
	ldp	x16, x17, [x4, #PCB_REGS + 16 * 8]
	ldr	     x19, [x4, #PCB_REGS + 19 * 8]
	ldp	x20, x21, [x4, #PCB_REGS + 20 * 8]
	ldp	x22, x23, [x4, #PCB_REGS + 22 * 8]
	ldp	x24, x25, [x4, #PCB_REGS + 24 * 8]
	ldp	x26, x27, [x4, #PCB_REGS + 26 * 8]
	ldp	x28, x29, [x4, #PCB_REGS + 28 * 8]
	ldr	x30, [x4, #PCB_REGS + 30 * 8]

	ret
#endif
.Lcpu_throw_panic_str:
	.asciz "cpu_throw: %p\0"
END(cpu_throw)

/*
 * void cpu_switch(struct thread *old, struct thread *new, struct mtx *mtx)
 *
 * a0 = old
 * a1 = new
 * a2 = mtx
 * x3 to x7, x16 and x17 are caller saved
 */
ENTRY(cpu_switch)
	//la	a0, .Lcpu_switch_panic_str
	//li	a1, 0x61
	//call	panic

	la	x14, pcpup
	ld	x14, 0(x14)
	/* Store the new curthread */
	sd	a1, PC_CURTHREAD(x14)
	/* And the new pcb */
	ld	x13, TD_PCB(a1)
	sd	x13, PC_CURPCB(x14)

	/* Save the old context. */
	ld	x13, TD_PCB(a0)

	/* Store the callee-saved registers */

	/* ra, sp, gp, tp */
	sd	x1, (PCB_REGS + 1 * 8)(x13)
	sd	x2, (PCB_REGS + 2 * 8)(x13)
	sd	x3, (PCB_REGS + 3 * 8)(x13)
	sd	x4, (PCB_REGS + 4 * 8)(x13)

	/* we use these in fork_trampoline */
	sd	x5, (PCB_REGS + 5 * 8)(x13)
	sd	x6, (PCB_REGS + 6 * 8)(x13)

	/* s0 - s11 */
	sd	x8, (PCB_REGS + 8 * 8)(x13)
	sd	x9, (PCB_REGS + 9 * 8)(x13)
	sd	x18, (PCB_REGS + 18 * 8)(x13)
	sd	x19, (PCB_REGS + 19 * 8)(x13)
	sd	x20, (PCB_REGS + 20 * 8)(x13)
	sd	x21, (PCB_REGS + 21 * 8)(x13)
	sd	x22, (PCB_REGS + 22 * 8)(x13)
	sd	x23, (PCB_REGS + 23 * 8)(x13)
	sd	x24, (PCB_REGS + 24 * 8)(x13)
	sd	x25, (PCB_REGS + 25 * 8)(x13)
	sd	x26, (PCB_REGS + 26 * 8)(x13)
	sd	x27, (PCB_REGS + 27 * 8)(x13)

	/* And the old stack pointer */
	//sd	sp, PCB_SP(t0)

	/*
	 * Restore the saved context.
	 */
	ld	x13, TD_PCB(a1)

	/*
	 * TODO: We may need to flush the cache here if switching
	 * to a user process.
	 */

	sfence.vm

	/* Switch to the new pmap */
	ld	t0, PCB_L1ADDR(x13)
#if 0
	beqz	t0, 1f
	bgtz	t0, 2f
1:
	la	a0, .Lcpu_switch_panic_str
	//li	a1, 0x65
	call	panic
2:
#endif
	csrw	sptbr0, t0

	/* Invalidate the TLB */
	sfence.vm

	/* Release the old thread */
	sd	a2, TD_LOCK(a0)

	/* Restore the registers */
	//ld	x2, (PCB_SP)(t0)
	//mv	sp, t1
	//msr	tpidr_el0, t2

	/* ra, sp, gp, tp */
	ld	x1, (PCB_REGS + 1 * 8)(x13)
	ld	x2, (PCB_REGS + 2 * 8)(x13)
	ld	x3, (PCB_REGS + 3 * 8)(x13)
	ld	x4, (PCB_REGS + 4 * 8)(x13)

	/* We use these in fork_trampoline */
	ld	x5, (PCB_REGS + 5 * 8)(x13)
	ld	x6, (PCB_REGS + 6 * 8)(x13)

	/* s0 - s11 */
	ld	x8, (PCB_REGS + 8 * 8)(x13)
	ld	x9, (PCB_REGS + 9 * 8)(x13)
	ld	x18, (PCB_REGS + 18 * 8)(x13)
	ld	x19, (PCB_REGS + 19 * 8)(x13)
	ld	x20, (PCB_REGS + 20 * 8)(x13)
	ld	x21, (PCB_REGS + 21 * 8)(x13)
	ld	x22, (PCB_REGS + 22 * 8)(x13)
	ld	x23, (PCB_REGS + 23 * 8)(x13)
	ld	x24, (PCB_REGS + 24 * 8)(x13)
	ld	x25, (PCB_REGS + 25 * 8)(x13)
	ld	x26, (PCB_REGS + 26 * 8)(x13)
	ld	x27, (PCB_REGS + 27 * 8)(x13)

	ret
#if 0
	/* Store the new curthread */
	str	x1, [x18, #PC_CURTHREAD]
	/* And the new pcb */
	ldr	x4, [x1, #TD_PCB]
	str	x4, [x18, #PC_CURPCB]

	/*
	 * Save the old context.
	 */
	ldr	x4, [x0, #TD_PCB]

	/* Store the callee-saved registers */
	stp	x8, x9, [x4, #PCB_REGS + 8 * 8]
	stp	x10, x11, [x4, #PCB_REGS + 10 * 8]
	stp	x12, x13, [x4, #PCB_REGS + 12 * 8]
	stp	x14, x15, [x4, #PCB_REGS + 14 * 8]
	stp	x16, x17, [x4, #PCB_REGS + 16 * 8]
	stp	x18, x19, [x4, #PCB_REGS + 18 * 8]
	stp	x20, x21, [x4, #PCB_REGS + 20 * 8]
	stp	x22, x23, [x4, #PCB_REGS + 22 * 8]
	stp	x24, x25, [x4, #PCB_REGS + 24 * 8]
	stp	x26, x27, [x4, #PCB_REGS + 26 * 8]
	stp	x28, x29, [x4, #PCB_REGS + 28 * 8]
	str	x30, [x4, #PCB_REGS + 30 * 8]
	/* And the old stack pointer */
	mov	x5, sp
	mrs	x6, tpidr_el0
	stp	x5, x6, [x4, #PCB_SP]

#ifdef VFP
	mov	x19, x0
	mov	x20, x1
	mov	x21, x2
	bl	vfp_save_state
	mov	x2, x21
	mov	x1, x20
	mov	x0, x19
#endif

	/*
	 * Restore the saved context.
	 */
	ldr	x4, [x1, #TD_PCB]

	/*
	 * TODO: We may need to flush the cache here if switching
	 * to a user process.
	 */

	/* Switch to the new pmap */
	ldr	x5, [x4, #PCB_L1ADDR]
	msr	ttbr0_el1, x5
	isb

	/* Invalidate the TLB */
	dsb	sy
	tlbi	vmalle1is
	dsb	sy
	isb

	/* Release the old thread */
	str	x2, [x0, #TD_LOCK]
#if defined(SCHED_ULE) && defined(SMP)
	/* Read the value in blocked_lock */
	ldr	x0, =_C_LABEL(blocked_lock)
	ldr	x1, [x0]
	/* Load curthread */
	ldr	x2, [x18, #PC_CURTHREAD]
1:
	ldr	x3, [x2, #TD_LOCK]
	cmp	x3, x1
	b.eq	1b
#endif

	/* Restore the registers */
	ldp	x5, x6, [x4, #PCB_SP]
	mov	sp, x5
	msr	tpidr_el0, x6
	ldp	x8, x9, [x4, #PCB_REGS + 8 * 8]
	ldp	x10, x11, [x4, #PCB_REGS + 10 * 8]
	ldp	x12, x13, [x4, #PCB_REGS + 12 * 8]
	ldp	x14, x15, [x4, #PCB_REGS + 14 * 8]
	ldp	x16, x17, [x4, #PCB_REGS + 16 * 8]
	ldr	     x19, [x4, #PCB_REGS + 19 * 8]
	ldp	x20, x21, [x4, #PCB_REGS + 20 * 8]
	ldp	x22, x23, [x4, #PCB_REGS + 22 * 8]
	ldp	x24, x25, [x4, #PCB_REGS + 24 * 8]
	ldp	x26, x27, [x4, #PCB_REGS + 26 * 8]
	ldp	x28, x29, [x4, #PCB_REGS + 28 * 8]
	ldr	x30, [x4, #PCB_REGS + 30 * 8]

	str	xzr, [x4, #PCB_REGS + 18 * 8]
	ret
#endif
.Lcpu_switch_panic_str:
	.asciz "cpu_switch: %p\0"
END(cpu_switch)

/*
 * fork_exit(void (*callout)(void *, struct trapframe *), void *arg,
 *  struct trapframe *frame)
 *
 */

ENTRY(fork_trampoline)
	//la	a0, .Lcpu_switch_panic_str
	//li	a1, 0x62
	//call	panic

	mv	a0, x5
	mv	a1, x6
	mv	a2, sp
	call	_C_LABEL(fork_exit)

	//la	a0, .Lcpu_switch_panic_str
	//li	a1, 0x63
	//call	panic

	/* Restore sp and ra */
	//ld	ra, (8)(sp)
	//ld	sp, (0)(sp)

	/* Restore epc */
	ld	t0, 256(sp)
	csrw	sepc, t0

	/* Restore sstatus */
	// sstatus is zero at this point
	//ld	t0, 264(sp)
	//csrw	sstatus, t0

	li	t0, (1 << 3)
	csrw	sstatus, t0

	/* Restore the registers other than sp and ra */
	//ld	x3, (TF_X + 3 * 8)(sp)
	//ld	x4, (TF_X + 4 * 8)(sp)

	ld	x5, (TF_X + 5 * 8)(sp)
	ld	x6, (TF_X + 6 * 8)(sp)
	ld	x7, (TF_X + 7 * 8)(sp)
	ld	x8, (TF_X + 8 * 8)(sp)
	ld	x9, (TF_X + 9 * 8)(sp)
	ld	x10, (TF_X + 10 * 8)(sp)
	ld	x11, (TF_X + 11 * 8)(sp)
	ld	x12, (TF_X + 12 * 8)(sp)
	ld	x13, (TF_X + 13 * 8)(sp)
	ld	x14, (TF_X + 14 * 8)(sp)
	ld	x15, (TF_X + 15 * 8)(sp)
	ld	x16, (TF_X + 16 * 8)(sp)
	ld	x17, (TF_X + 17 * 8)(sp)
	ld	x18, (TF_X + 18 * 8)(sp)
	ld	x19, (TF_X + 19 * 8)(sp)
	ld	x20, (TF_X + 20 * 8)(sp)
	ld	x21, (TF_X + 21 * 8)(sp)
	ld	x22, (TF_X + 22 * 8)(sp)
	ld	x23, (TF_X + 23 * 8)(sp)
	ld	x24, (TF_X + 24 * 8)(sp)
	ld	x25, (TF_X + 25 * 8)(sp)
	ld	x26, (TF_X + 26 * 8)(sp)
	ld	x27, (TF_X + 27 * 8)(sp)
	ld	x28, (TF_X + 28 * 8)(sp)
	ld	x29, (TF_X + 29 * 8)(sp)
	ld	x30, (TF_X + 30 * 8)(sp)
	ld	x31, (TF_X + 31 * 8)(sp)

	ld	ra, (TF_X + 1 * 8)(sp)

	/* Save kernel stack so we can use it doing a user trap */
	csrw	sscratch, sp

	/* Load user sp */
	ld	sp, (TF_X + 2 * 8)(sp)
	/*
	 * TODO: interrupt can happen here, between ld and eret.
	 * It will came to supervisor handler
	 * with user stack causing some panic.
	 * Need to disable interrupts before changing sp.
	 */
	eret
END(fork_trampoline)

ENTRY(savectx)
	la	a0, .Lsavectx_panic_str
	call	panic
.Lsavectx_panic_str:
	.asciz "savectx_panic: %p\0"
END(savectx)

