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

#include "assym.s"

/*
 * One of the fu* or su* functions failed, return -1.
 */
ENTRY(fsu_fault)
	SET_FAULT_HANDLER(x0, a1)	/* Reset the handler function */
	li	a0, -1
	ret
END(fsu_fault)

/*
 * int casueword32(volatile uint32_t *, uint32_t, uint32_t *, uint32_t)
 */
ENTRY(casueword32)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x4)	/* And set it */
1:	ldxr	w4, [x0]		/* Load-exclusive the data */
	cmp	w4, w1			/* Compare */
	b.ne	2f			/* Not equal, exit */
	stxr	w5, w3, [x0]		/* Store the new data */
	cbnz	w5, 1b			/* Retry on failure */
	ldrb	w0, [x0]		/* Try loading the data */
2:	SET_FAULT_HANDLER(xzr, x5)	/* Reset the fault handler */
	str	w4, [x2]		/* Store the read data */
	ret				/* Return */
#endif
END(casueword32)

/*
 * int casueword(volatile u_long *, u_long, u_long *, u_long)
 */
ENTRY(casueword)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x4)	/* And set it */
1:	ldxr	x4, [x0]		/* Load-exclusive the data */
	cmp	x4, x1			/* Compare */
	b.ne	2f			/* Not equal, exit */
	stxr	w5, x3, [x0]		/* Store the new data */
	cbnz	w5, 1b			/* Retry on failure */
	ldrb	w0, [x0]		/* Try loading the data */
2:	SET_FAULT_HANDLER(xzr, x5)	/* Reset the fault handler */
	str	x4, [x2]		/* Store the read data */
	ret				/* Return */
#endif
END(casueword)

/*
 * int fubyte(volatile const void *)
 */
ENTRY(fubyte)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x1)	/* And set it */
	ldrb	w0, [x0]		/* Try loading the data */
	SET_FAULT_HANDLER(xzr, x1)	/* Reset the fault handler */
	ret				/* Return */
#endif
END(fubyte)

/*
 * int fuword(volatile const void *)
 */
ENTRY(fuword16)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x1)	/* And set it */
	ldrh	w0, [x0]		/* Try loading the data */
	SET_FAULT_HANDLER(xzr, x1)	/* Reset the fault handler */
	ret				/* Return */
#endif
END(fuword16)

/*
 * int32_t fueword32(volatile const void *, int32_t *)
 */
ENTRY(fueword32)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x2)	/* And set it */
	ldr	w0, [x0]		/* Try loading the data */
	SET_FAULT_HANDLER(xzr, x2)	/* Reset the fault handler */
	str	w0, [x1]		/* Save the data in kernel space */
	mov	w0, #0			/* Success */
	ret				/* Return */
#endif
END(fueword32)

/*
 * long fueword(volatile const void *, int64_t *)
 * int64_t fueword64(volatile const void *, int64_t *)
 */
ENTRY(fueword)
EENTRY(fueword64)
	la	a6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(a6, a2)	/* And set it */
	ld	a0, 0(a0)		/* Try loading the data */
	SET_FAULT_HANDLER(x0, a2)	/* Reset the fault handler */
	sd	a0, 0(a1)		/* Save the data in kernel space */
	li	a0, 0			/* Success */
	ret				/* Return */
EEND(fueword64)
END(fueword)

/*
 * int subyte(volatile void *, int)
 */
ENTRY(subyte)
	la	a6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(a6, a2)	/* And set it */
	sb	a1, 0(a0)		/* Try storing the data */
	SET_FAULT_HANDLER(x0, a2)	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(subyte)

/*
 * int suword16(volatile void *, int)
 */
ENTRY(suword16)
	call	panic
#if 0
	adr	x6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x2)	/* And set it */
	strh	w1, [x0]		/* Try storing the data */
	SET_FAULT_HANDLER(xzr, x2)	/* Reset the fault handler */
	mov	x0, #0			/* Success */
	ret				/* Return */
#endif
END(suword16)

/*
 * int suword32(volatile void *, int)
 */
ENTRY(suword32)
	la	a6, fsu_fault		/* Load the fault handler */
	SET_FAULT_HANDLER(a6, a2)	/* And set it */
	sw	a1, 0(a0)		/* Try storing the data */
	SET_FAULT_HANDLER(x0, a2)	/* Reset the fault handler */
	li	a0, 0			/* Success */
	ret				/* Return */
END(suword32)

/*
 * int suword(volatile void *, long)
 */
ENTRY(suword)
EENTRY(suword64)
	la	a6, fsu_fault
	SET_FAULT_HANDLER(a6, a2)
	sd	a1, 0(a0)
	SET_FAULT_HANDLER(x0, a2)
	li	a0, 0
	ret
EEND(suword64)
END(suword)

/*
 * fuswintr and suswintr are just like fusword and susword except that if
 * the page is not in memory or would cause a trap, then we return an error.
 * The important thing is to prevent sleep() and switch().
 */

/*
 * Special handler so the trap code knows not to sleep.
 */
ENTRY(fsu_intr_fault)
	call	panic
#if 0
	SET_FAULT_HANDLER(xzr, x1)	/* Reset the handler function */
	mov	x0, #-1
	ret
#endif
END(fsu_fault)

/*
 * int fuswintr(void *)
 */
ENTRY(fuswintr)
	call	panic
#if 0
	adr	x6, fsu_intr_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x1)	/* And set it */
	ldr	w0, [x0]		/* Try loading the data */
	SET_FAULT_HANDLER(xzr, x1)	/* Reset the fault handler */
	ret				/* Return */
#endif
END(fuswintr)

/*
 * int suswintr(void *base, int word)
 */
ENTRY(suswintr)
	call	panic
#if 0
	adr	x6, fsu_intr_fault	/* Load the fault handler */
	SET_FAULT_HANDLER(x6, x2)	/* And set it */
	str	w1, [x0]		/* Try storing the data */
	SET_FAULT_HANDLER(xzr, x2)	/* Reset the fault handler */
	mov	x0, #0			/* Success */
	ret				/* Return */
#endif
END(suswintr)

ENTRY(setjmp)
	call	panic
#if 0
	/* Store the stack pointer */
	mov	x8, sp
	str	x8, [x0]

	/* Store the general purpose registers and lr */
	stp	x19, x20, [x0], #16
	stp	x21, x22, [x0], #16
	stp	x23, x24, [x0], #16
	stp	x25, x26, [x0], #16
	stp	x27, x28, [x0], #16
	stp	x29, lr, [x0], #16

	/* Return value */
	mov	x0, #0
	ret
#endif
END(setjmp)

ENTRY(longjmp)
	call	panic
#if 0
	/* Restore the stack pointer */
	ldr	x8, [x0], #8
	mov	sp, x8

	/* Restore the general purpose registers and lr */
	ldp	x19, x20, [x0], #16
	ldp	x21, x22, [x0], #16
	ldp	x23, x24, [x0], #16
	ldp	x25, x26, [x0], #16
	ldp	x27, x28, [x0], #16
	ldp	x29, lr, [x0], #16

	/* Load the return value */
	mov	x0, x1
	ret
#endif
END(longjmp)
