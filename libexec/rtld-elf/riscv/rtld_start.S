/*-
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Andrew Turner under
 * sponsorship from the FreeBSD Foundation.
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

/*
 * func_ptr_type
 * _rtld(Elf_Addr *sp, func_ptr_type *exit_proc, Obj_Entry **objp)
 */

ENTRY(.rtld_start)
	mv	x8, a0		/* Put ps_strings in a callee-saved register */
	mv	x9, sp		/* And the stack pointer */

	/* Handle the old style stack */
	/* TODO: Remove this when the kernel correctly aligns the stack */
	//bnez	a0, 1f
	//mv	a0, sp		/* sp points to the args */
	//andi	sp, a0, ~0xf	/* Align the stack as needed */

1:	addi	sp, sp, -16	/* Make room for obj_main & exit proc */

	mv	a1, sp		/* exit_proc */
	addi	a2, a1, 8	/* obj_main */
	jal	_rtld		/* Call the loader */
	mv	t0, a0		/* Backup the entry point */

	ld	a2, 0(sp)	/* Load cleanup */
	ld	a1, 8(sp)	/* Load obj_main */
	mv	a0, x8		/* Restore ps_strings */
	mv	sp, x9		/* Restore the stack pointer */
	jr	t0		/* Jump to the entry point */

#if 0
	mov	x19, x0		/* Put ps_strings in a callee-saved register */
	mov	x20, sp		/* And the stack pointer */

	/* Handle the old style stack */
	/* TODO: Remove this when the kernel correctly aligns the stack */
	cbnz	x0, 1f
	mov	x0, sp		/* sp points to the args */
	and	sp, x0, #~0xf	/* Align the stack as needed */

1:	sub	sp, sp, #16	/* Make room for obj_main & exit proc */

	mov	x1, sp		/* exit_proc */
	add	x2, x1, #8	/* obj_main */
	bl	_rtld		/* Call the loader */
	mov	x8, x0		/* Backup the entry point */

	ldr	x2, [sp]	/* Load cleanup */
	ldr	x1, [sp, #8]	/* Load obj_main */
	mov	x0, x19		/* Restore ps_strings */
	mov	sp, x20		/* Restore the stack pointer */
	br	x8		/* Jump to the entry point */
#endif
END(.rtld_start)

/*
 * sp + 0 = &GOT[x + 3]
 * sp + 8 = RA
 * x16 = &GOT[2]
 * x17 = &_rtld_bind_start
 */

/* netbsd hint
 * t0 = obj pointer
 * t1 = reloc offset
 */

ENTRY(_rtld_bind_start)
	//mv	t1, sp

	/* Save the arguments */
	addi	sp, sp, -72
	sd	a0, (8 * 0)(sp)
	sd	a1, (8 * 1)(sp)
	sd	a2, (8 * 2)(sp)
	sd	a3, (8 * 3)(sp)
	sd	a4, (8 * 4)(sp)
	sd	a5, (8 * 5)(sp)
	sd	a6, (8 * 6)(sp)
	sd	a7, (8 * 7)(sp)
	sd	ra, (8 * 8)(sp)

	/* Save any floating-point arguments */

	/* Each rela item has 3 entriesso we need reloff = 3 * index */
	slli	a1, t1, 1
	add	a1, a1, t1

	/* Load obj */
	mv	a0, t0

	/* Call into rtld */
	jal	_rtld_bind

	/* Restore the registers saved by the plt code */
	//ld	
	//ldp	xzr, x30, [sp, #(5 * 16 + 4 * 32)]

	/* Backup the address to branch to */
	mv	t0, a0

	/* restore the arguments */
	ld	a0, (8 * 0)(sp)
	ld	a1, (8 * 1)(sp)
	ld	a2, (8 * 2)(sp)
	ld	a3, (8 * 3)(sp)
	ld	a4, (8 * 4)(sp)
	ld	a5, (8 * 5)(sp)
	ld	a6, (8 * 6)(sp)
	ld	a7, (8 * 7)(sp)
	ld	ra, (8 * 8)(sp)
	addi	sp, sp, 72

	/* And the part of the stack the plt entry handled */
	//addi	sp, sp, 16

	/* Call into the correct function */
	jr	t0
#if 0
	mov	x17, sp

	/* Save the arguments */
	stp	x0, x1, [sp, #-16]!
	stp	x2, x3, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!
	stp	x8, xzr, [sp, #-16]!

	/* Save any floating-point arguments */
	stp	q0, q1, [sp, #-32]!
	stp	q2, q3, [sp, #-32]!
	stp	q4, q5, [sp, #-32]!
	stp	q6, q7, [sp, #-32]!

	/* Calculate reloff */
	ldr	x2, [x17, #0]	/* Get the address of the entry */
	sub	x1, x2, x16	/* Find its offset */
	sub	x1, x1, #8	/* Adjust for x16 not being at offset 0 */
	/* Each rela item has 3 entriesso we need reloff = 3 * index */
	lsl	x3, x1, #1	/* x3 = 2 * offset */
	add	x1, x1, x3	/* x1 = x3 + offset = 3 * offset */

	/* Load obj */
	ldr	x0, [x16, #-8]

	/* Call into rtld */
	bl	_rtld_bind

	/* Restore the registers saved by the plt code */
	ldp	xzr, x30, [sp, #(5 * 16 + 4 * 32)]

	/* Backup the address to branch to */
	mov	x16, x0

	/* restore the arguments */
	ldp	q6, q7, [sp], #32
	ldp	q4, q5, [sp], #32
	ldp	q2, q3, [sp], #32
	ldp	q0, q1, [sp], #32
	ldp	x8, xzr, [sp], #16
	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x2, x3, [sp], #16
	ldp	x0, x1, [sp], #16
	/* And the part of the stack the plt entry handled */
	add	sp, sp, #16

	/* Call into the correct function */
	br	x16
#endif
END(_rtld_bind_start)

/*
 * uint64_t _rtld_tlsdesc(struct tlsdesc *);
 *
 * struct tlsdesc {
 *  uint64_t ptr;
 *  uint64_t data;
 * };
 *
 * Returns the data.
 */
ENTRY(_rtld_tlsdesc)
	ld	a0, 8(a0)
#if 0
	ldr	x0, [x0, #8]
#endif
	ret
END(_rtld_tlsdesc)
