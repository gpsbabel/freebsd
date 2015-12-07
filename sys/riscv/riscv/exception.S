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

#include <machine/asm.h>
__FBSDID("$FreeBSD$");

#include "assym.s"

#include <machine/trap.h>

.macro save_registers el
	addi	sp, sp, -288
	/* x0 is hard-wired zero */
	sd	x1, (TF_X + 1 * 8)(sp)
	/* sd	x2, (TF_X + 2 * 8)(sp) */
	sd	x3, (TF_X + 3 * 8)(sp)
	sd	x4, (TF_X + 4 * 8)(sp)
	sd	x5, (TF_X + 5 * 8)(sp)
	sd	x6, (TF_X + 6 * 8)(sp)
	sd	x7, (TF_X + 7 * 8)(sp)
	sd	x8, (TF_X + 8 * 8)(sp)
	sd	x9, (TF_X + 9 * 8)(sp)
	sd	x10, (TF_X + 10 * 8)(sp)
	sd	x11, (TF_X + 11 * 8)(sp)
	sd	x12, (TF_X + 12 * 8)(sp)
	sd	x13, (TF_X + 13 * 8)(sp)
	sd	x14, (TF_X + 14 * 8)(sp)
	sd	x15, (TF_X + 15 * 8)(sp)
	sd	x16, (TF_X + 16 * 8)(sp)
	sd	x17, (TF_X + 17 * 8)(sp)
	sd	x18, (TF_X + 18 * 8)(sp)
	sd	x19, (TF_X + 19 * 8)(sp)
	sd	x20, (TF_X + 20 * 8)(sp)
	sd	x21, (TF_X + 21 * 8)(sp)
	sd	x22, (TF_X + 22 * 8)(sp)
	sd	x23, (TF_X + 23 * 8)(sp)
	sd	x24, (TF_X + 24 * 8)(sp)
	sd	x25, (TF_X + 25 * 8)(sp)
	sd	x26, (TF_X + 26 * 8)(sp)
	sd	x27, (TF_X + 27 * 8)(sp)
	sd	x28, (TF_X + 28 * 8)(sp)
	sd	x29, (TF_X + 29 * 8)(sp)
	sd	x30, (TF_X + 30 * 8)(sp)
	sd	x31, (TF_X + 31 * 8)(sp)

	/* XXX: panic - spin if stack is not kernel one */
.if \el == 1	/* kernel */
	mv	t0, sp
	srli	t0, t0, 63
1:
	beqz	t0, 1b
.endif

.if \el == 1
	/* Store kernel sp */
	sd	x2, (TF_X + 2 * 8)(sp)
.else
	/* Store user sp */
	csrr	t0, sscratch
	sd	t0, (TF_X + 2 * 8)(sp)
.endif
	li	t0, 0
	csrw	sscratch, t0

	csrr	t0, sepc
	sd	t0, 256(sp)

	csrr	t0, sstatus
	sd	t0, 264(sp)

	csrr	t0, sbadaddr
	sd	t0, 272(sp)

	csrr	t0, scause
	sd	t0, 280(sp)
.endm

.macro load_registers el
	ld	t0, 264(sp)
	csrw	sstatus, t0

	ld	t0, 256(sp)
	csrw	sepc, t0

.if \el == 0
	/* Load user sp */
	ld	t0, (TF_X + 2 * 8)(sp)
	csrw	sscratch, t0
.endif

	/* x0 is hard-wired zero */
	ld	x1, (TF_X + 1 * 8)(sp)
	/* x2 is sp */
	ld	x3, (TF_X + 3 * 8)(sp)
	ld	x4, (TF_X + 4 * 8)(sp)
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

	addi	sp, sp, 288
.endm

ENTRY(cpu_exception_handler_sv)
	save_registers 1
	mv	a0, sp
	call	_C_LABEL(do_trap)
	load_registers 1
	eret
END(cpu_exception_handler_sv)

ENTRY(cpu_exception_handler_user)
	csrrw	sp, sscratch, sp
	save_registers 0
	mv	a0, sp
	call	_C_LABEL(do_trap_user)
	do_ast
	load_registers 0
	csrrw	sp, sscratch, sp
	eret
END(cpu_exception_handler_user)

/*
 * Trap handlers
 */

#define	HANDLE_SUPERVISOR_TRAP_IN_MACHINE_MODE 0 \
    | (0 << (31 - 0)) /* IF misaligned */           \
    | (0 << (31 - 1)) /* IF fault */                \
    | (1 << (31 - 2)) /* illegal instruction */     \
    | (0 << (31 - 3)) /* breakpoint */              \
    | (1 << (31 - 4)) /* load misaligned */         \
    | (0 << (31 - 5)) /* load fault */              \
    | (1 << (31 - 6)) /* store misaligned */        \
    | (0 << (31 - 7)) /* store fault */             \
    | (0 << (31 - 8)) /* user environment call */   \
    | (1 << (31 - 9)) /* super environment call */

	.text
bad_trap:
	j bad_trap

user_trap:
	csrrw	sp, mscratch, sp
	addi	sp, sp, -64
	sd	t0, (8 * 0)(sp)
	sd	t1, (8 * 1)(sp)
	sd	t2, (8 * 2)(sp)
	sd	t3, (8 * 3)(sp)
	sd	t4, (8 * 4)(sp)
	sd	t5, (8 * 5)(sp)
	sd	a0, (8 * 7)(sp)

	la	t2, _C_LABEL(cpu_exception_handler_user)

	csrr    t0, mcause
	bltz    t0, .Linterrupt
	j	.Lexit_mrts

supervisor_trap:
	/* Save state */
	csrrw	sp, mscratch, sp
	addi	sp, sp, -64
	sd	t0, (8 * 0)(sp)
	sd	t1, (8 * 1)(sp)
	sd	t2, (8 * 2)(sp)
	sd	t3, (8 * 3)(sp)
	sd	t4, (8 * 4)(sp)
	sd	t5, (8 * 5)(sp)
	sd	a0, (8 * 7)(sp)

	la	t2, _C_LABEL(cpu_exception_handler_sv)

	csrr	t0, mcause
	bltz	t0, .Linterrupt

	li	t1, HANDLE_SUPERVISOR_TRAP_IN_MACHINE_MODE
	sllw	t1, t1, t0
	bltz	t1, .Lhandle_trap_in_machine_mode
	j	.Lexit_mrts

.Linterrupt:
	/* Type of interrupt ? */
	csrr	t0, mcause
	andi	t0, t0, 3
	li	t1, 0
	beq	t1, t0, software_interrupt
	li	t1, 1
	beq	t1, t0, timer_interrupt
	li	t1, 2
	beq	t1, t0, htif_interrupt

	/* not reached */
1:
	j	1b

software_interrupt:
	/* Clear supervisor software interrupt */
	//li	t0, MIP_SSIP
	//csrc	mip, t0
	j	.Lexit_mrts

timer_interrupt:
	/* Disable machine timer interrupts */
	li	t0, MIE_MTIE
	csrc	mie, t0

	/* Clear machine pending */
	li	t0, MIP_MTIP
	csrc	mip, t0

	/* Post supervisor interrupt */
	li	t0, MIP_STIP
	csrs	mip, t0

	/* If PRV1 is PRV_U (user) then serve a trap */
	csrr	t0, mstatus
	li	t1, (MSTATUS_PRV_M << MSTATUS_PRV1_SHIFT)
	and	t0, t0, t1
	beqz	t0, 1f

	/* If PRV1 is supervisor and interrupts was enabled, then serve a trap */
	csrr	t0, mstatus
	li	t1, (SR_IE1 | (MSTATUS_PRV_M << MSTATUS_PRV1_SHIFT))
	and	t0, t0, t1
	li	t1, (SR_IE1 | (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT))
	beq	t0, t1, 1f

	j	.Lexit_no_mepc

1:
	/* Serve a trap in supervisor mode */
	j	.Lexit_mrts

htif_interrupt:
1:
	li	t5, 0
	csrrw	t5, mfromhost, t5
	beqz	t5, 3f

	/* Console PUT intr ? */
	mv	t1, t5
	li	t0, 0x101
	srli	t1, t1, 48
	bne	t1, t0, 2f
	/* Yes */
	la	t0, console_intr
	li	t1, 1
	sd	t1, 0(t0)
	j	3f

2:
	/* put entry */
	la	t0, htif_ring_cursor
	beqz	t0, 3f		/* not initialized */
	ld	t0, 0(t0)	/* load struct */
	sd	t5, 0(t0)	/* put entry */
	li	t4, 1
	sd	t4, 8(t0)	/* mark used */
	ld	t4, 16(t0)	/* take next */
	/* update cursor */
	la	t0, htif_ring_cursor
	sd	t4, 0(t0)

	/* Post supervisor software interrupt */
	li	t0, MIP_SSIP
	csrs	mip, t0

3:
	j	.Lexit_no_mepc

.Lhandle_trap_in_machine_mode:
	/* Save mepc, point to the next instr */
	csrr	t1, mepc
	addi	t1, t1, 4		/* Next instruction */
	li	t4, ECALL_HTIF_CMD
	beq	t5, t4, htif_cmd
	li	t4, ECALL_HTIF_GET_ENTRY
	beq	t5, t4, htif_get_entry
	li	t4, ECALL_MTIMECMP
	beq	t5, t4, set_mtimecmp
	li	t4, ECALL_CLEAR_PENDING
	beq	t5, t4, clear_pending
	li	t4, ECALL_MCPUID_GET
	beq	t5, t4, mcpuid_get
	li	t4, ECALL_MIMPID_GET
	beq	t5, t4, mimpid_get
	j	.Lexit

mcpuid_get:
	csrr	t6, mcpuid
	j	.Lexit

mimpid_get:
	csrr	t6, mimpid
	j	.Lexit

htif_get_entry:
	li	t6, 0		/* preset return value */
	la	t0, htif_ring_last
	ld	t0, 0(t0)	/* load struct */
	ld	t4, 8(t0)	/* get used */
	beqz	t4, 1f
	ld	t6, 0(t0)	/* get entry */
	li	t4, 0
	sd	t4, 8(t0)	/* mark free */
	sd	t4, 0(t0)	/* free entry, just in case */
	ld	t4, 16(t0)	/* take next */
	/* update ring_last */
	la	t0, htif_ring_last
	sd	t4, 0(t0)
1:
	/* Exit. Result is stored in t6 */
	j	.Lexit

htif_cmd:
	mv	t0, t6
1:
	csrrw	t0, mtohost, t0
	bnez	t0, 1b
	j	.Lexit

set_mtimecmp:
	csrr	t2, stime
	add	t6, t6, t2
	csrw	mtimecmp, t6

	/* Enable interrupts */
	li	t0, (MIE_MTIE | MIE_STIE)
	csrs	mie, t0
	j	.Lexit

clear_pending:
	li      t0, MIP_STIP
	csrc    mip, t0
	j	.Lexit

/*
 * Trap exit functions
 */
.Lexit:
	/* Next instruction is in t1 */
	csrw    mepc, t1
.Lexit_no_mepc:
	/* Restore state */
	ld	t0, (8 * 0)(sp)
	ld	t1, (8 * 1)(sp)
	ld	t2, (8 * 2)(sp)
	ld	t3, (8 * 3)(sp)
	ld	t4, (8 * 4)(sp)
	ld	t5, (8 * 5)(sp)
	ld	a0, (8 * 7)(sp)
	addi	sp, sp, 64
	csrrw	sp, mscratch, sp
	eret

/*
 * Redirect to supervisor
 */
.Lexit_mrts:
	/* Setup exception handler */
	li	t1, KERNBASE
	add	t2, t2, t1
	csrw	stvec, t2

	/* Restore state */
	ld	t0, (8 * 0)(sp)
	ld	t1, (8 * 1)(sp)
	ld	t2, (8 * 2)(sp)
	ld	t3, (8 * 3)(sp)
	ld	t4, (8 * 4)(sp)
	ld	t5, (8 * 5)(sp)
	ld	a0, (8 * 7)(sp)
	addi	sp, sp, 64
	csrrw	sp, mscratch, sp

	/* Redirect to supervisor */
	mrts
