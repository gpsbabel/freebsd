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

#ifndef _MACHINE_PTE_H_
#define	_MACHINE_PTE_H_

#ifndef LOCORE
typedef	uint64_t	pd_entry_t;		/* page directory entry */
typedef	uint64_t	pt_entry_t;		/* page table entry */
#endif

#ifdef SMP
#define	ATTR_DEFAULT	(ATTR_AF | ATTR_SH(ATTR_SH_IS))
#else
#define	ATTR_DEFAULT	(ATTR_AF)
#endif

#define	ATTR_DESCR_MASK	3

/* Level 0 table, 512GiB per entry */
#define	L0_SHIFT	39
#define	L0_INVAL	0x0 /* An invalid address */
#define	L0_BLOCK	0x1 /* A block */
	/* 0x2 also marks an invalid address */
#define	L0_TABLE	0x3 /* A next-level table */

/* Level 1 table, 1GiB per entry */
#define	L1_SHIFT	30
#define	L1_SIZE 	(1 << L1_SHIFT)
#define	L1_OFFSET 	(L1_SIZE - 1)
#define	L1_INVAL	L0_INVAL
#define	L1_BLOCK	L0_BLOCK
#define	L1_TABLE	L0_TABLE

/* Level 2 table, 2MiB per entry */
#define	L2_SHIFT	21
#define	L2_SIZE 	(1 << L2_SHIFT)
#define	L2_OFFSET 	(L2_SIZE - 1)
#define	L2_INVAL	L0_INVAL
#define	L2_BLOCK	L0_BLOCK
#define	L2_TABLE	L0_TABLE

#define	L2_BLOCK_MASK	UINT64_C(0xffffffe00000)

/* Level 3 table, 4KiB per entry */
#define	L3_SHIFT	12
#define	L3_SIZE 	(1 << L3_SHIFT)
#define	L3_OFFSET 	(L3_SIZE - 1)
#define	L3_INVAL	0x0
	/* 0x1 is reserved */
	/* 0x2 also marks an invalid address */
#define	L3_PAGE		0x3

#define	Ln_ENTRIES	(1 << 9)
#define	Ln_ADDR_MASK	(Ln_ENTRIES - 1)
#define	Ln_TABLE_MASK	((1 << 12) - 1)

/* RISC-V */

#define	ATTR_MASK	0x3ff
/* Bits 9:7 are reserved for software */
#define	ATTR_SW_MANAGED	(1UL << 8)
#define	ATTR_SW_WIRED	(1UL << 7)
#define	ATTR_DIRTY	(1 << 6) /* Virtual page is written */
#define	ATTR_REF	(1 << 5) /* Virtual page is referenced */

#define	PTE_VALID	(1 << 0)	/* Valid */
#define	PTE_TYPE_S	1
#define	PTE_TYPE_M	(0xf << PTE_TYPE_S)
#define	PTE_TYPE_PTR	0
#define	PTE_TYPE_PTR_G	1
#define	PTE_TYPE_SROURX	2	/* Supervisor read-only, user read-execute page. */
#define	PTE_TYPE_SRWURWX 3	/* Supervisor read-write, user read-write-execute page. */
#define	PTE_TYPE_SURO	4	/* Supervisor and user read-only page. */
#define	PTE_TYPE_SURW	5	/* Supervisor and user read-write page. */
#define	PTE_TYPE_SURX	6	/* Supervisor and user read-execute page. */
#define	PTE_TYPE_SURWX	7	/* Supervisor and User Read Write Execute */
#define	PTE_TYPE_SRO	8	/* Supervisor read-only page. */
#define	PTE_TYPE_SRW	9	/* Supervisor read-write page. */
#define	PTE_TYPE_SRX	10	/* Supervisor read-execute page. */
#define	PTE_TYPE_SRWX	11	/* Supervisor read-write-execute page. */
#define	PTE_TYPE_SRO_G	12	/* Supervisor read-only page--global mapping. */
#define	PTE_TYPE_SRW_G	13	/* Supervisor read-write page--global mapping. */
#define	PTE_TYPE_SRX_G	14	/* Supervisor read-execute page--global mapping. */
#define	PTE_TYPE_SRWX_G	15	/* Supervisor Read Write Execute Global */

#define	PTE_PPN0_S	10
#define	PTE_PPN1_S	19
#define	PTE_PPN2_S	28
#define	PTE_PPN3_S	37
#define	PTE_SIZE	8

#endif /* !_MACHINE_PTE_H_ */

/* End of pte.h */
