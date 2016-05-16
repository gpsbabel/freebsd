/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * $FreeBSD$
 */
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#define _ASM
#define _LOCORE

#include <sys/cpuvar_defs.h>
#include <sys/dtrace.h>

#include <machine/riscvreg.h>
#include <machine/asm.h>

#include "assym.s"

/*
void dtrace_membar_producer(void)
*/
ENTRY(dtrace_membar_producer)
	RET
END(dtrace_membar_producer)

/*
void dtrace_membar_consumer(void)
*/
ENTRY(dtrace_membar_consumer)
	RET
END(dtrace_membar_consumer)

/*
dtrace_icookie_t dtrace_interrupt_disable(void)
*/
ENTRY(dtrace_interrupt_disable)
#if 0
	msr	daifset, #2
#endif
	RET
END(dtrace_interrupt_disable)

/*
void dtrace_interrupt_enable(dtrace_icookie_t cookie)
*/
ENTRY(dtrace_interrupt_enable)
#if 0
	msr	daifclr, #2
#endif
	RET
END(dtrace_interrupt_enable)
/*
uint8_t
dtrace_fuword8_nocheck(void *addr)
*/
ENTRY(dtrace_fuword8_nocheck)
	lb	a0, 0(a0)
	RET
END(dtrace_fuword8_nocheck)

/*
uint16_t
dtrace_fuword16_nocheck(void *addr)
*/
ENTRY(dtrace_fuword16_nocheck)
	lh	a0, 0(a0)
	RET
END(dtrace_fuword16_nocheck)

/*
uint32_t
dtrace_fuword32_nocheck(void *addr)
*/
ENTRY(dtrace_fuword32_nocheck)
	lw	a0, 0(a0)
	RET
END(dtrace_fuword32_nocheck)

/*
uint64_t
dtrace_fuword64_nocheck(void *addr)
*/
ENTRY(dtrace_fuword64_nocheck)
	ld	a0, 0(a0)
	RET
END(dtrace_fuword64_nocheck)

/*
void
dtrace_copy(uintptr_t uaddr, uintptr_t kaddr, size_t size)
*/
ENTRY(dtrace_copy)
#if 0
	cbz	x2, 2f		/* If len == 0 then skip loop */
1:
	ldrb	w4, [x0], #1	/* Load from uaddr */
	strb	w4, [x1], #1	/* Store in kaddr */
	sub	x2, x2, #1	/* len-- */
	cbnz	x2, 1b
2:
#endif
	RET
END(dtrace_copy)

/*
void
dtrace_copystr(uintptr_t uaddr, uintptr_t kaddr, size_t size,
    volatile uint16_t *flags)
XXX: Check for flags?
*/
ENTRY(dtrace_copystr)
#if 0
	cbz     x2, 2f          /* If len == 0 then skip loop */
 
1:	ldrb    w4, [x0], #1    /* Load from uaddr */
	strb    w4, [x1], #1    /* Store in kaddr */
	cbz     w4, 2f          /* If == 0 then break */
	sub     x2, x2, #1      /* len-- */
	cbnz    x2, 1b
2:
	RET
#endif
END(dtrace_copystr)

/*
uintptr_t
dtrace_caller(int aframes)
*/
ENTRY(dtrace_caller)
	li	a0, -1
	RET
END(dtrace_caller)

/*
uint32_t
dtrace_cas32(uint32_t *target, uint32_t cmp, uint32_t new)
*/
ENTRY(dtrace_cas32)
1:	lr.w	a3, 0(a0)	/* Load target */
	bne	a3, a1, 2f	/* *target != cmp ? return */
	sc.w	a4, a2, 0(a0)	/* Store new to target */
	bnez	a4, 1b		/* Try again if store not succeed */
2:	mv	a0, a3		/* Return the value loaded from target */
#if 0
1:	ldxr	w3, [x0]	/* Load target */
	cmp	w3, w1		/* Check if *target == cmp */
	bne	2f		/* No, return */
	stxr	w12, w2, [x0]	/* Store new to target */
	cbnz	w12, 1b		/* Try again if store not succeed */
2:	mov	w0, w3		/* Return the value loaded from target */
#endif
	RET
END(dtrace_cas32)

/*
void *
dtrace_casptr(volatile void *target, volatile void *cmp, volatile void *new)
*/
ENTRY(dtrace_casptr)
1:	lr.d	a3, 0(a0)	/* Load target */
	bne	a3, a1, 2f	/* *target != cmp ? return */
	sc.d	a4, a2, 0(a0)	/* Store new to target */
	bnez	a4, 1b		/* Try again if store not succeed */
2:	mv	a0, a3		/* Return the value loaded from target */
#if 0
1:	ldxr	x3, [x0]	/* Load target */
	cmp	x3, x1		/* Check if *target == cmp */
	bne	2f		/* No, return */
	stxr	w12, x2, [x0]	/* Store new to target */
	cbnz	w12, 1b		/* Try again if store not succeed */
2:	mov	x0, x3		/* Return the value loaded from target */
#endif
	RET
END(dtrace_casptr)
