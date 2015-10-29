/*-
 * Copyright (c) 2015 The FreeBSD Foundation
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
 *
 */

#include <machine/asm.h>
__FBSDID("$FreeBSD: head/sys/arm64/arm64/copyinout.S 281494 2015-04-13 14:43:10Z andrew $");

#include <sys/errno.h>

#include "assym.s"

/*
 * Fault handler for the copy{in,out} functions below.
 */
ENTRY(copyio_fault)
	SET_FAULT_HANDLER(x0, a1) /* Clear the handler */
	li	a0, EFAULT
	ret
#if 0
	SET_FAULT_HANDLER(xzr, x1) /* Clear the handler */
	mov	x0, #EFAULT
	ret
#endif
END(copyio_fault)

/*
 * Copies from a kernel to user address
 *
 * int copyout(const void *kaddr, void *udaddr, size_t len)
 */
ENTRY(copyout)
	beqz	a2, 2f		/* If len == 0 then skip loop */

	la	a6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(a6, a7) /* Set the handler */

1:	lb	a4, 0(a0)	/* Load from kaddr */
	addi	a0, a0, 1
	sb	a4, 0(a1)	/* Store in uaddr */
	addi	a1, a1, 1
	addi	a2, a2, -1	/* len-- */
	bnez	a2, 1b

	SET_FAULT_HANDLER(x0, a7) /* Clear the handler */

2:	li	a0, 0		/* return 0 */
	ret
#if 0
	cbz	x2, 2f		/* If len == 0 then skip loop */

	adr	x6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(x6, x7) /* Set the handler */

1:	ldrb	w4, [x0], #1	/* Load from kaddr */
	strb	w4, [x1], #1	/* Store in uaddr */
	sub	x2, x2, #1	/* len-- */
	cbnz	x2, 1b

	SET_FAULT_HANDLER(xzr, x7) /* Clear the handler */

2:	mov	x0, xzr		/* return 0 */
	ret
#endif
END(copyout)

/*
 * Copies from a user to kernel address
 *
 * int copyin(const void *uaddr, void *kdaddr, size_t len)
 */
ENTRY(copyin)
	beqz	a2, 2f		/* If len == 0 then skip loop */

	la	a6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(a6, a7) /* Set the handler */

1:	lb	a4, 0(a0)	/* Load from uaddr */
	addi	a0, a0, 1
	sb	a4, 0(a1)	/* Store in kaddr */
	addi	a1, a1, 1
	addi	a2, a2, -1	/* len-- */
	bnez	a2, 1b

	SET_FAULT_HANDLER(x0, a7) /* Clear the handler */

2:	li	a0, 0		/* return 0 */
	ret
#if 0
	cbz	x2, 2f		/* If len == 0 then skip loop */

	adr	x6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(x6, x7) /* Set the handler */

1:	ldrb	w4, [x0], #1	/* Load from uaddr */
	strb	w4, [x1], #1	/* Store in kaddr */
	sub	x2, x2, #1	/* len-- */
	cbnz	x2, 1b

	SET_FAULT_HANDLER(xzr, x7) /* Clear the handler */

2:	mov	x0, xzr		/* return 0 */
	ret
#endif
END(copyin)

/*
 * Copies a string from a user to kernel address
 *
 * int copyinstr(const void *udaddr, void *kaddr, size_t len, size_t *done)
 */
ENTRY(copyinstr)
	mv	a5, x0		/* count = 0 */
	beqz	a2, 3f		/* If len == 0 then skip loop */

	la	a6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(a6, a7) /* Set the handler */

1:	lb	a4, 0(a0)
	addi	a0, a0, 1
	sb	a4, 0(a1)
	addi	a1, a1, 1
	beqz	a4, 2f
	addi	a2, a2, -1	/* len-- */
	addi	a5, a5, 1	/* count++ */
	bnez	a2, 1b
	
2:	SET_FAULT_HANDLER(x0, a7) /* Clear the handler */

3:	beqz	a3, 4f		/* Check if done != NULL */
	addi	a5, a5, 1	/* count++ */
	sd	a5, 0(a3)	/* done = count */

4:	mv	a0, x0		/* return 0 */
	ret
#if 0
	mov	x5, xzr		/* count = 0 */
	cbz	x2, 3f		/* If len == 0 then skip loop */

	adr	x6, copyio_fault /* Get the handler address */
	SET_FAULT_HANDLER(x6, x7) /* Set the handler */

1:	ldrb	w4, [x0], #1	/* Load from uaddr */
	strb	w4, [x1], #1	/* Store in kaddr */
	cbz	w4, 2f		/* If == 0 then break */
	sub	x2, x2, #1	/* len-- */
	add	x5, x5, #1	/* count++ */
	cbnz	x2, 1b

2:	SET_FAULT_HANDLER(xzr, x7) /* Clear the handler */

3:	cbz	x3, 4f		/* Check if done != NULL */
	add	x5, x5, #1	/* count++ */
	str	x5, [x3]	/* done = count */

4:	mov	x0, xzr		/* return 0 */
	ret
#endif
END(copyinstr)
