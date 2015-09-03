/*-
 * Copyright (c) 2015 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
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

#define	MSTATUS_MPRV		(1 << 16)
#define	MSTATUS_PRV0_SHIFT	1
#define	MSTATUS_PRV1_SHIFT	4
#define	MSTATUS_PRV2_SHIFT	7
#define	MSTATUS_PRV_MASK	0x3
#define	MSTATUS_PRV_U		0
#define	MSTATUS_PRV_S		1
#define	MSTATUS_PRV_H		2
#define	MSTATUS_PRV_M		3

#define	MSTATUS_VM_SHIFT	17
#define	MSTATUS_VM_MASK		0x1f
#define	MSTATUS_VM_MBARE	0
#define	MSTATUS_VM_MBB		1
#define	MSTATUS_VM_MBBID	2
#define	MSTATUS_VM_SV32		8
#define	MSTATUS_VM_SV39		9
#define	MSTATUS_VM_SV48		10

#define	INIT_STACK_SIZE	(PAGE_SIZE * 4)

	.globl	kernbase
	.set	kernbase, KERNBASE

	/* Trap entries */
	.text

mentry:
	/* User mode entry point (mtvec + 0x000) */
	.align 6
	j bad_trap

	/* Supervisor mode entry point (mtvec + 0x040) */
	.align 6
	j sv_trap

	.align 3
hardstack:
	.space	32
hardstack_end:

	/* Reset vector */
	.text
	.align 8
	.globl _start
_start:
	//li	s11, VM_MIN_KERNEL_ADDRESS
	li	s11, 0xffffffffc0000000

	/*
	li	s0, ((MSTATUS_VM_SV48 << MSTATUS_VM_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV0_SHIFT) | \
		     (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT) | \
		     (MSTATUS_PRV_U << MSTATUS_PRV2_SHIFT));
	*/
	//csrw	mstatus, s0

	//la	s1, _C_LABEL(_end)	/* page aligned end of the kernel */
	//csrw	sptbr, s1	// set the page table base
	//csrw	sscratch, 0

	/* Setup supervisor exception handler */
	la	s0, _C_LABEL(cpu_exception_handler)
	add	s0, s0, s11
	csrw	stvec, s0

	li	s10, PAGE_SIZE
	li	s9, (PAGE_SIZE * KSTACK_PAGES)

	/* Load the gp address so we can use it */
	//la	t0, _gp
	//add	t0, t0, s11
	//mv	gp, t0

	/* Page tables */

	/* Level 0 */
	la	s1, pagetable_l0
	la	s2, pagetable_l1	/* Link to next level PN */
	srli	s2, s2, PAGE_SHIFT

	li	a5, 0xffffffffc0000000
	srli	a5, a5, 39
	andi	a5, a5, 0x1ff
	li	t4, (PTE_VALID | (PTE_TYPE_PTR_G << PTE_TYPE_S))
	slli	t5, s2, PTE_PPN0_S	/* (s2 << PTE_PPN0_S) */
	or	t6, t4, t5

	/* Store single level0 PTE entry to position */
	li	a6, PTE_SIZE
	mulw	a5, a5, a6
	add	t0, s1, a5
	sd	t6, (t0)

	/* Level 1 */
	la	s1, pagetable_l1
	la	s2, pagetable_l2	/* Link to next level PN */
	srli	s2, s2, PAGE_SHIFT

	li	a5, 0xffffffffc0000000
	srli	a5, a5, 0x1e		/* >> 30 */
	andi	a5, a5, 0x1ff		/* & 0x1ff */
	li	t4, (PTE_VALID | (PTE_TYPE_PTR_G << PTE_TYPE_S))
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

	/* Page tables base */
	la	s1, pagetable_l0
	csrw	sptbr, s1

	/* Page tables END */

	/* Enter supervisor mode */
	li	s0, ((MSTATUS_VM_SV48 << MSTATUS_VM_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV0_SHIFT) | \
		     (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT) | \
		     (MSTATUS_PRV_U << MSTATUS_PRV2_SHIFT));
	csrw	mstatus, s0

	/* Jump to virtual address */
	la	t0, .Lmmu_on
	add	t0, t0, s11
	csrw	mepc, t0
	eret

.Lmmu_on:
	/* Initialize stack pointer */
	la	s3, initstack_end
	mv	sp, s3
	//sub	sp, sp, #PCB_SIZE

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

.macro save_registers
	addi	sp, sp, -280
	/*	x0 is hard-wired zero */
	sd	x1, 8(sp)
	sd	x2, 16(sp)
	sd	x3, 24(sp)
	sd	x4, 32(sp)
	sd	x5, 40(sp)
	sd	x6, 48(sp)
	sd	x7, 56(sp)
	sd	x8, 64(sp)
	sd	x9, 72(sp)
	sd	x10, 80(sp)
	sd	x11, 88(sp)
	sd	x12, 96(sp)
	sd	x13, 104(sp)
	sd	x14, 112(sp)
	sd	x15, 120(sp)
	sd	x16, 128(sp)
	sd	x17, 136(sp)
	sd	x18, 144(sp)
	sd	x19, 152(sp)
	sd	x20, 160(sp)
	sd	x21, 168(sp)
	sd	x22, 176(sp)
	sd	x23, 184(sp)
	sd	x24, 192(sp)
	sd	x25, 200(sp)
	sd	x26, 208(sp)
	sd	x27, 216(sp)
	sd	x28, 224(sp)
	sd	x29, 232(sp)
	sd	x30, 240(sp)
	sd	x31, 248(sp)

	csrr	t1, sepc
	sd	t1, 0(sp)

	csrr	t1, sstatus
	sd	t1, 256(sp)

	csrr	t1, sbadaddr
	sd	t1, 264(sp)

	csrr	t1, scause
	sd	t1, 272(sp)
.endm

.macro load_registers
	ld	x1, 8(sp)
	ld	x2, 16(sp)
	ld	x3, 24(sp)
	ld	x4, 32(sp)
	ld	x5, 40(sp)
	ld	x6, 48(sp)
	ld	x7, 56(sp)
	ld	x8, 64(sp)
	ld	x9, 72(sp)
	ld	x10, 80(sp)
	ld	x11, 88(sp)
	ld	x12, 96(sp)
	ld	x13, 104(sp)
	ld	x14, 112(sp)
	ld	x15, 120(sp)
	ld	x16, 128(sp)
	ld	x17, 136(sp)
	ld	x18, 144(sp)
	ld	x19, 152(sp)
	ld	x20, 160(sp)
	ld	x21, 168(sp)
	ld	x22, 176(sp)
	ld	x23, 184(sp)
	ld	x24, 192(sp)
	ld	x25, 200(sp)
	ld	x26, 208(sp)
	ld	x27, 216(sp)
	ld	x28, 224(sp)
	ld	x29, 232(sp)
	ld	x30, 240(sp)
	ld	x31, 248(sp)
	addi	sp, sp, 280
.endm

ENTRY(cpu_exception_handler)
	save_registers
	mv	a0, sp
	call	_C_LABEL(do_trap)	/* Off we go */
	load_registers
	eret
END(cpu_exception_handler)

ENTRY(sigcode)
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
pagetable_l0:
	.space  PAGE_SIZE
pagetable_l1:
	.space	PAGE_SIZE
pagetable_l2:
	.space	PAGE_SIZE
pagetable_end:

	.globl init_pt_va
init_pt_va:
	.quad pagetable_l2	/* XXX: Keep page tables VA */

ENTRY(bad_trap)
	j bad_trap
END(bad_trap)

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

sv_trap:
	la	x16, hardstack_end
	addi	x16, x16, -16
	sd	t0, 0(x16)
	sd	t1, 8(x16)

	csrr	t0, mcause
	li	t1, HANDLE_SUPERVISOR_TRAP_IN_MACHINE_MODE
	sllw	t1, t1, t0
	bltz	t1, .Lhandle_trap_in_machine_mode

	# Handle trap in supervisor mode
	mrts

.Lhandle_trap_in_machine_mode:

	/* Save mepc, point to the next instr */
	csrr	t1, mepc
	addi	t1, t1, 4

	li	t4, ECALL_LOW_PRINTC
	beq	t5, t4, low_printc

	li	t4, ECALL_MTIMECMP
	beq	t5, t4, set_mtimecmp

	/* Exit trap */
	csrw	mepc, t1
	eret

low_printc:
	//li	t5, 0
	li	t0, 0x101000000000000
	//add	t0, t0, t6
	add	t0, t0, a0

2:
	csrrw	t0, mtohost, t0
	bnez	t0, 2b

	csrr	t0, mfromhost
	li	t0, 0
	csrw	mfromhost, t0

	/* Exit trap */
	csrw	mepc, t1

	ld	t0, 0(x16)
	ld	t1, 8(x16)
	eret

set_mtimecmp:
	csrw	mtimecmp, a0
	/* Exit trap */
	csrw    mepc, t1
	eret
