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

#define	PTE_VALID	(1 << 0)	/* Valid */
#define	PTE_TYPE_S	1
#define	PTE_TYPE_M	0xf
#define	PTE_TYPE_PTR	0
#define	PTE_TYPE_PTR_G	1
#define	PTE_TYPE_SRWX	0xb		/* Supervisor Read Write Execute */
#define	PTE_TYPE_SRWX_G	0xf		/* Supervisor Read Write Execute Global */
#define	PTE_PPN0_S	10
#define	PTE_PPN1_S	19
#define	PTE_PPN2_S	28
#define	PTE_PPN3_S	37
#define	PTE_SIZE	8

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

	/* Reset vector */
	.text
	.align 8
	.globl _start
_start:
	li	s0, ((MSTATUS_VM_SV48 << MSTATUS_VM_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV0_SHIFT) | \
		     (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT) | \
		     (MSTATUS_PRV_U << MSTATUS_PRV2_SHIFT));
	csrw	mstatus, s0

	//la	s1, _C_LABEL(_end)	/* page aligned end of the kernel */
	//csrw	sptbr, s1	// set the page table base
	//la	s0, _C_LABEL(cpu_exception_handler)
	//csrw	mtvec, s0

	la	t0, 0
	csrr	t1, mstatus
	sw	t1, (t0)
	
	/* li	s11, VM_MAX_KERNEL_ADDRESS */
	li	s11, 0xffffffff80000000

	li	s10, PAGE_SIZE
	li	s9, (PAGE_SIZE * KSTACK_PAGES)

	/* Load the gp address so we can use it */
	la	gp, _gp

	/* Page tables */

	/* Level 0 */
	la	s1, pagetable
	la	s2, pagetable_l1	/* Link to next level PN */
	srli	s2, s2, PAGE_SHIFT

	li	a5, 0xffffffff80000000
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

	li	a5, 0xffffffff80000000
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

	/* Level 2 superpages (1 GB) */
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
	la	s1, pagetable
	csrw	sptbr, s1

	/* Page tables END */

	/* Enter supervisor mode */
	li	s0, ((MSTATUS_VM_SV48 << MSTATUS_VM_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV0_SHIFT) | \
		     (MSTATUS_PRV_S << MSTATUS_PRV1_SHIFT) | \
		     (MSTATUS_PRV_M << MSTATUS_PRV2_SHIFT));
	csrw	mstatus, s0

	/* Jump to virtual addressation */
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
	li	t1, 0xffffffff80000000
	add	t0, t0, t1
	sd	t0, 8(sp) /* kern_l1pt */

	li	t0, 0
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

ENTRY(cpu_exception_handler)
	addi	t0, t0, 2
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
pagetable:
	.space  PAGE_SIZE
pagetable_l1:
	.space	PAGE_SIZE
pagetable_l2:
	.space	PAGE_SIZE
pagetable_end:

ENTRY(bad_trap)
	j bad_trap
END(bad_trap)

sv_trap:
	/* Save mepc, point to the next instr */
	csrr	t1, mepc
	addi	t1, t1, 4

	li	t4, ECALL_LOW_PRINTC
	beq	t5, t4, low_printc	

	/* Exit trap */
	csrw	mepc, t1
	eret

low_printc:
	li	t5, 0
	li	t0, 0x101000000000000
	add	t0, t0, t6

1:
	csrrw	t0, mtohost, t0
	bnez	t0, 1b

	csrr	t0, mfromhost
	li	t0, 0
	csrw	mfromhost, t0

	/* Exit trap */
	csrw	mepc, t1
	eret
