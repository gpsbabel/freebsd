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

#include <machine/setjmp.h>

ENTRY(setjmp)
	addi	sp, sp, -16
	sd	a0, 0(sp)
	sd	ra, 8(sp)

	//sub	sp, sp, #16
	//stp	x0, lr, [sp]

	/* Store the signal mask */
	addi	a2, a0, (_JB_SIGMASK * 8)	/* oset */
	li	a1, 0				/* set */
	li	a0, 1				/* SIG_BLOCK */
	jal	sigprocmask

	//add	x2, x0, #(_JB_SIGMASK * 8)	/* oset */
	//mov	x1, #0				/* set */
	//mov	x0, #1				/* SIG_BLOCK */
	//bl	sigprocmask

	ld	a0, 0(sp)
	ld	ra, 8(sp)
	addi	sp, sp, 16

	//ldp	x0, lr, [sp]
	//add	sp, sp, #16

	/* Store the magic value and stack pointer */
	la	t0, .Lmagic
	ld	t0, 0(t0)
	sd	t0, (0 * 8)(a0)
	sd	sp, (1 * 8)(a0)
	addi	a0, a0, 16

	/* Store the general purpose registers and lr */
	sd	s0, (0 * 8)(a0)
	sd	s1, (1 * 8)(a0)
	sd	s2, (2 * 8)(a0)
	sd	s3, (3 * 8)(a0)
	sd	s4, (4 * 8)(a0)
	sd	s5, (5 * 8)(a0)
	sd	s6, (6 * 8)(a0)
	sd	s7, (7 * 8)(a0)
	sd	s8, (8 * 8)(a0)
	sd	s9, (9 * 8)(a0)
	sd	s10, (10 * 8)(a0)
	sd	s11, (11 * 8)(a0)
	sd	x1, (12 * 8)(a0)	/* ra */
	addi	a0, a0, 104

	/* Store the vfp registers */
#if 0
	fsd	fs0, (0 * 8)(a0)
	fsd	fs1, (1 * 8)(a0)
	fsd	fs2, (2 * 8)(a0)
	fsd	fs3, (3 * 8)(a0)
	fsd	fs4, (4 * 8)(a0)
	fsd	fs5, (5 * 8)(a0)
	fsd	fs6, (6 * 8)(a0)
	fsd	fs7, (7 * 8)(a0)
	fsd	fs8, (8 * 8)(a0)
	fsd	fs9, (9 * 8)(a0)
	fsd	fs10, (10 * 8)(a0)
	fsd	fs11, (11 * 8)(a0)
#endif

	/* Store the magic value and stack pointer */
	//ldr	x8, .Lmagic
	//mov	x9, sp
	//stp	x8, x9, [x0], #16

	/* Store the general purpose registers and lr */
	//stp	x19, x20, [x0], #16
	//stp	x21, x22, [x0], #16
	//stp	x23, x24, [x0], #16
	//stp	x25, x26, [x0], #16
	//stp	x27, x28, [x0], #16
	//stp	x29, lr, [x0], #16

	/* Store the vfp registers */
	//stp	d8, d9, [x0], #16
	//stp	d10, d11, [x0], #16
	//stp	d12, d13, [x0], #16
	//stp	d14, d15, [x0]

	/* Return value */
	li	a0, 0
	ret
	.align	3
.Lmagic:
	.quad	_JB_MAGIC_SETJMP
END(setjmp)

ENTRY(longjmp)
	addi	sp, sp, -32
	sd	a0, (0 * 8)(sp)
	sd	ra, (1 * 8)(sp)
	sd	a1, (2 * 8)(sp)

	//sub	sp, sp, #32
	//stp	x0, lr, [sp]
	//str	x1, [sp, #16]

	/* Restore the signal mask */
	li	a2, 0				/* oset */
	addi	a1, a0, (_JB_SIGMASK * 8)	/* set */
	li	a0, 3				/* SIG_BLOCK */
	jal	sigprocmask

	//mov	x2, #0				/* oset */
	//add	x1, x0, #(_JB_SIGMASK * 8)	/* set */
	//mov	x0, #3				/* SIG_BLOCK */
	//bl	sigprocmask

	ld	a1, (2 * 8)(sp)
	ld	ra, (1 * 8)(sp)
	ld	a0, (0 * 8)(sp)
	addi	sp, sp, 32

	//ldr	x1, [sp, #16]
	//ldp	x0, lr, [sp]
	//add	sp, sp, #32

	/* Check the magic value */
	ld	t0, 0(a0)
	la	t1, .Lmagic
	ld	t1, 0(t1)
	bne	t0, t1, botch

	/* Restore the stack pointer */
	ld	t0, 8(a0)
	mv	sp, t0
	addi	a0, a0, 16

	/* Restore the general purpose registers and lr */
	ld	s0, (0 * 8)(a0)
	ld	s1, (1 * 8)(a0)
	ld	s2, (2 * 8)(a0)
	ld	s3, (3 * 8)(a0)
	ld	s4, (4 * 8)(a0)
	ld	s5, (5 * 8)(a0)
	ld	s6, (6 * 8)(a0)
	ld	s7, (7 * 8)(a0)
	ld	s8, (8 * 8)(a0)
	ld	s9, (9 * 8)(a0)
	ld	s10, (10 * 8)(a0)
	ld	s11, (11 * 8)(a0)
	ld	x1, (12 * 8)(a0)	/* ra */
	addi	a0, a0, 104

	/* Restore the vfp registers */
#if 0
	fld	fs0, (0 * 8)(a0)
	fld	fs1, (1 * 8)(a0)
	fld	fs2, (2 * 8)(a0)
	fld	fs3, (3 * 8)(a0)
	fld	fs4, (4 * 8)(a0)
	fld	fs5, (5 * 8)(a0)
	fld	fs6, (6 * 8)(a0)
	fld	fs7, (7 * 8)(a0)
	fld	fs8, (8 * 8)(a0)
	fld	fs9, (9 * 8)(a0)
	fld	fs10, (10 * 8)(a0)
	fld	fs11, (11 * 8)(a0)
#endif

	/* Check the magic value */
	//ldr	x8, [x0], #8
	//ldr	x9, .Lmagic
	//cmp	x8, x9
	//b.ne	botch

	/* Restore the stack pointer */
	//ldr	x8, [x0], #8
	//mov	sp, x8

	/* Restore the general purpose registers and lr */
	//ldp	x19, x20, [x0], #16
	//ldp	x21, x22, [x0], #16
	//ldp	x23, x24, [x0], #16
	//ldp	x25, x26, [x0], #16
	//ldp	x27, x28, [x0], #16
	//ldp	x29, lr, [x0], #16

	/* Restore the vfp registers */
	//ldp	d8, d9, [x0], #16
	//ldp	d10, d11, [x0], #16
	//ldp	d12, d13, [x0], #16
	//ldp	d14, d15, [x0]

	/* Load the return value */
	mv	a0, a1
	ret

botch:
	call	_C_LABEL(longjmperror)
	call	_C_LABEL(abort)
END(longjmp)
