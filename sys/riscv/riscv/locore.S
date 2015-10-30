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

#include "assym.s"

#include <sys/syscall.h>
#include <machine/asm.h>
#include <machine/param.h>
#include <machine/trap.h>
#include <machine/pte.h>
//#include <machine/vmparam.h>

#define	INIT_STACK_SIZE	(PAGE_SIZE * 4)

	.globl	kernbase
	.set	kernbase, KERNBASE

	/* Trap entries */
	.text

mentry:
	/* User mode entry point (mtvec + 0x000) */
	.align 6
	j	user_trap

	/* Supervisor mode entry point (mtvec + 0x040) */
	.align 6
	j	supervisor_trap

	/* Hypervisor mode entry point (mtvec + 0x080) */
	.align 6
	j	bad_trap

	/* Machine mode entry point (mtvec + 0x0C0) */
	.align 6
	j	bad_trap

	/* Reset vector */
	.text
	.align 8
	.globl _start
_start:
	li	s11, KERNBASE

	la	t0, hardstack_end
	sub	t0, t0, s11
	csrw	mscratch, t0

	la	t0, mentry
	csrw	mtvec, t0

	li	t0, 0
	csrw	sscratch, t0

	li	s10, PAGE_SIZE
	li	s9, (PAGE_SIZE * KSTACK_PAGES)

	/* Page tables */

	/* Level 1 */
	la	s1, pagetable_l1
	la	s2, pagetable_l2	/* Link to next level PN */
	srli	s2, s2, PAGE_SHIFT

	li	a5, KERNBASE
	srli	a5, a5, 0x1e		/* >> 30 */
	andi	a5, a5, 0x1ff		/* & 0x1ff */
	li	t4, (PTE_VALID | (PTE_TYPE_PTR << PTE_TYPE_S))
	slli	t5, s2, PTE_PPN0_S	/* (s2 << PTE_PPN0_S) */
	or	t6, t4, t5

	/* Store single level1 PTE entry to position */
	li	a6, PTE_SIZE
	mulw	a5, a5, a6
	add	t0, s1, a5
	sd	t6, (t0)

	/* Level 2 superpages (512 x 2MiB) */
	la	s1, pagetable_l2
	li	t3, 512			/* Build 512 entries */
	li	t4, 0			/* Counter */
	li	t5, 0
2:
	li	t0, (PTE_VALID | (PTE_TYPE_SRWX_G << PTE_TYPE_S))
	slli	t2, t4, PTE_PPN1_S	/* << PTE_PPN1_S */
	or	t5, t0, t2
	sd	t5, (s1)		/* Store PTE entry to position */
	addi	s1, s1, PTE_SIZE

	addi	t4, t4, 1
	bltu	t4, t3, 2b

	/* Page tables base for kernel */
	la	s1, pagetable_l1
	csrw	sptbr1, s1

	/* Page tables END */

	/* Enter supervisor mode */
	li	s0, ((MSTATUS_VM_SV39 << MSTATUS_VM_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV_SHIFT) | \
		     (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT) | \
		     (MSTATUS_PRV_U << MSTATUS_PRV2_SHIFT));
	csrw	mstatus, s0

	/* Exit from machine mode */
	la	t0, .Lmmu_on
	add	t0, t0, s11
	csrw	mepc, t0
	eret

.Lmmu_on:
	/* Initialize stack pointer */
	la	s3, initstack_end
	mv	sp, s3
	addi	sp, sp, -PCB_SIZE

	/* Clear BSS  */
	la	a0, _C_LABEL(__bss_start)
	la	s1, _C_LABEL(_end)
	//li	s9, (PAGE_SIZE * KSTACK_PAGES)
	//add	s1, s1, s9
1:
	sb	zero, 0(a0)
	addi	a0, a0, 1
	bltu	a0, s1, 1b

	/* Fill riscv_bootparams */
	addi	sp, sp, -32

	li	t0, 0
	sd	t0, 0(sp) /* modulep */

	la	t0, pagetable_l1
	sd	t0, 8(sp) /* kern_l1pt */

	li	t0, 0xffffffffbffffe00
	sd	t0, 16(sp) /* kern_delta */

	la	t0, initstack_end
	sd	t0, 24(sp) /* kern_stack */

	mv	a0, sp
	call	_C_LABEL(initriscv)	/* Off we go */
	call	_C_LABEL(mi_startup)

	.align  4
initstack:
	.space  (PAGE_SIZE * KSTACK_PAGES)
initstack_end:
hardstack:
	.space  (PAGE_SIZE)
hardstack_end:

	.globl console_data
console_data:
	.space  (16)
//console_data:
//	.quad htif_data	/* XXX: Keep page tables VA */

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

.if \el == 1
	/* Store kernel sp */
	sd	x2, (TF_X + 2 * 8)(sp)
.else
	/* Store user sp */
	csrr	t0, sscratch
	sd	t0, (TF_X + 2 * 8)(sp)
.endif

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

//.if \el == 1
//	ld	x2, (TF_X + 2 * 8)(sp)
//.endif

	addi	sp, sp, 288
.endm

.macro	do_ast
1:
	la	a1, pcpup
	ld	a1, 0(a1)
	ld	a1, PC_CURTHREAD(a1)
	lw	a2, TD_FLAGS(a1)

	li	a3, (TDF_ASTPENDING|TDF_NEEDRESCHED)
	and	a2, a2, a3
	beqz	a2, 2f

	/* handle the ast */
	mv	a0, sp
	call	_C_LABEL(ast)

	/* Re-check for new ast scheduled */
	j	1b
2:
#if 0
	/* Disable interrupts */
	mrs	x19, daif
1:
	msr	daifset, #2

	/* Read the current thread flags */
	ldr	x1, [x18, #PC_CURTHREAD]	/* Load curthread */
	ldr	x2, [x1, #TD_FLAGS]

	/* Check if we have either bits set */
	mov	x3, #((TDF_ASTPENDING|TDF_NEEDRESCHED) >> 8)
	lsl	x3, x3, #8
	and	x2, x2, x3
	cmp	x2, #0
	b.eq	2f

	/* Restore interrupts */
	msr	daif, x19

	/* handle the ast */
	mov	x0, sp
	bl	_C_LABEL(ast)

	/* Re-check for new ast scheduled */
	b	1b
2:
#endif
.endm

ENTRY(cpu_exception_handler_sv)
	save_registers 1
	mv	a0, sp
	call	_C_LABEL(do_trap)
	load_registers 1
	eret
END(cpu_exception_handler_sv)

ENTRY(cpu_exception_handler_user)
	csrrw   sp, sscratch, sp
	save_registers 0
	mv	a0, sp
	call	_C_LABEL(do_trap_user)
	do_ast
	load_registers 0
	csrrw   sp, sscratch, sp
	eret
END(cpu_exception_handler_user)

ENTRY(sigcode)
	mv	a0, sp
	addi	a0, a0, SF_UC

1:
	li	t0, SYS_sigreturn
	ecall

	/* sigreturn failed, exit */
	li	t0, SYS_exit
	ecall

	j	1b

#if 0
	mov	x0, sp
	add	x0, x0, #SF_UC

1:
	mov	x8, #SYS_sigreturn
	svc	0

	/* sigreturn failed, exit */
	mov	x8, #SYS_exit
	svc	0

	b	1b
#endif
END(sigcode)
	/* This may be copied to the stack, keep it 16-byte aligned */
	.align	3
esigcode:

	.data
	.align	3
	.global	szsigcode
szsigcode:
	.quad	esigcode - sigcode

	.align	12
pagetable_l1:
	.space	PAGE_SIZE
pagetable_l2:
	.space	PAGE_SIZE
pagetable_end:

	.globl init_pt_va
init_pt_va:
	.quad pagetable_l2	/* XXX: Keep page tables VA */

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
  | (1 << (31 - 9)) /* super environment call */  \

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
	sd	t6, (8 * 6)(sp)

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
	sd	t6, (8 * 6)(sp)

	la	t2, _C_LABEL(cpu_exception_handler_sv)

	csrr	t0, mcause
	bltz	t0, .Linterrupt

	li	t1, HANDLE_SUPERVISOR_TRAP_IN_MACHINE_MODE
	sllw	t1, t1, t0
	bltz	t1, .Lhandle_trap_in_machine_mode
	j	.Lexit_mrts

.Linterrupt:
	/* HTIF interrupt ? */
	csrr	t0, mcause
	andi	t0, t0, 2
	bnez	t0, .Lhtif_interrupt

.Ltimer_interrupt:
	/* Disable interrupts */
	li	t0, MIE_MTIE
	csrc	mie, t0

	/* Clear pending */
	li	t0, MIP_MTIP
	csrc	mip, t0

	/* Post supervisor interrupt */
	li	t0, MIP_STIP
	csrs	mip, t0

	/* If PRV1 is user then serve a trap */
	csrr	t0, mstatus
	li	t1, (MSTATUS_PRV_M << MSTATUS_PRV1_SHIFT)
	and	t0, t0, t1
	beqz	t0, 1f

	/* If PRV1 is supervisor and interrupts was enabled, then serve a trap */
	csrr	t0, mstatus
	li	t1, (SR_IE1 | (MSTATUS_PRV_M << MSTATUS_PRV1_SHIFT))
	and	t0, t0, t1
	li	t1, (SR_IE1 | (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT))
	bne	t0, t1, .Lexit_no_mepc

1:
	/* Clear pending */
	li      t0, MIP_STIP
	csrc    mip, t0
	j	.Lexit_mrts

.Lhtif_interrupt:
	la	t0, console_data
	li	t1, 0
	csrrw	t1, mfromhost, t1
	sd	t1, 0(t0)

	/* Was supervisor interrupts enabled before trap ? */
	csrr	t0, mstatus
	li	t1, (SR_IE1)
	and	t0, t0, t1
	beqz	t0, .Lexit_no_mepc
	j	.Lexit_mrts

.Lhandle_trap_in_machine_mode:
	/* Save mepc, point to the next instr */
	csrr	t1, mepc
	addi	t1, t1, 4		/* Next instruction */
	li	t4, ECALL_LOW_PRINTC
	beq	t5, t4, low_printc
	li	t4, ECALL_MTIMECMP
	beq	t5, t4, set_mtimecmp
	li	t4, ECALL_CLEAR_PENDING
	beq	t5, t4, clear_pending
	j	.Lexit

low_printc:
	mv	t0, t6
2:
	csrrw	t0, mtohost, t0
	bnez	t0, 2b

	csrr	t6, mfromhost
	li	t0, 0
	csrw	mfromhost, t0
	j	.Lexit

set_mtimecmp:
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
	/* Exit trap */
	csrw    mepc, t1
.Lexit_no_mepc:
	/* Restore state */
	ld	t0, (8 * 0)(sp)
	ld	t1, (8 * 1)(sp)
	ld	t2, (8 * 2)(sp)
	ld	t3, (8 * 3)(sp)
	ld	t4, (8 * 4)(sp)
	ld	t5, (8 * 5)(sp)
	ld	t6, (8 * 6)(sp)
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
	ld	t6, (8 * 6)(sp)
	addi	sp, sp, 64
	csrrw	sp, mscratch, sp

	/* Redirect to supervisor */
	mrts
