/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 * Portions Copyright 2013 Justin Hibbits jhibbits@freebsd.org
 * Portions Copyright 2013 Howard Su howardsu@freebsd.org
 * Portions Copyright 2015-2016 Ruslan Bukin <br@bsdpad.com>
 *
 * $FreeBSD$
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>

#include <sys/dtrace.h>

#include "fbt.h"

#define	MIPS_BREAK		0x0000000d
#define	FBT_PATCHVAL		(MIPS_BREAK)
#define	FBT_ENTRY		"entry"
#define	FBT_RETURN		"return"

int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t rval)
{
	solaris_cpu_t *cpu;
	fbt_probe_t *fbt;

	cpu = &solaris_cpu[curcpu];
	fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			cpu->cpu_dtrace_caller = addr;

			dtrace_probe(fbt->fbtp_id, frame->a0,
			    frame->a1, frame->a2,
			    frame->a3, frame->a4);

			cpu->cpu_dtrace_caller = 0;
			return (fbt->fbtp_savedval);
		}
	}

	return (0);
}

void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{

	*fbt->fbtp_patchpoint = val;
	//cpu_icache_sync_range((vm_offset_t)fbt->fbtp_patchpoint, 4);
}

int
fbt_provide_module_function(linker_file_t lf, int symindx,
    linker_symval_t *symval, void *opaque)
{
	fbt_probe_t *fbt, *retfbt;
	uint32_t *target, *start;
	uint32_t *instr, *limit;
	const char *name;
	char *modname;
	int offs;

	modname = opaque;
	name = symval->name;

	/* Check if function is excluded from instrumentation */
	if (fbt_excluded(name))
		return (0);

	instr = (uint32_t *)(symval->value);
	limit = (uint32_t *)(symval->value + symval->size);

	/* Look for store double to ra register */
	for (; instr < limit; instr++) {
		if ((*instr & SD_MASK) == SD_RA)
			break;
	}

	if (instr >= limit)
		return (0);

	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 3, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_rval = DTRACE_INVOP_SD;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	lf->fbt_nentries++;

	retfbt = NULL;
again:
	//printf("start look for instr\n");
	for (; instr < limit; instr++) {
		//printf("instr %x\n", *instr);
#if 0
		if ((*instr >> DTRACE_OP_SHIFT) == DTRACE_OP_DADDIU) {
			if ((*instr >> DTRACE_RS_SHIFT) & DTRACE_RS_MASK) == DTRACE_REG_SP)
		}
		if ((*instr & DTRACE_DADDIU_MASK) == DTRACE_DADDIU_SP_SP) {
			break;
		}
#endif
		if ((*instr & LD_RA_SP_MASK) == LD_RA_SP)
			break;
#if 0
		if (*instr == RET_INSTR) {
			instr++; /* branch delay */
			break;
		}
		else if ((*instr & JAL_MASK) == JAL_INSTR) {
			offs = (*instr & JAL_DATA_MASK);
			offs *= 4;
			target = (uint32_t *)(0xffffffff80000000 + offs);
			start = (uint32_t *)symval->value;
			//printf("start 0x%016lx target 0x%016lx\n", (uint64_t)start, (uint64_t)target);
			if (target >= limit || target < start)
				break;
		}
#endif
	}

	if (instr >= limit) {
		printf("fail: %x (0x%016lx) start 0x%016lx limit 0x%016lx\n",
		   *instr, (uint64_t)instr, (uint64_t)symval->value, (uint64_t)limit);
		return (0);
	} else {
		printf("found: %x\n", *instr);
	}

	/*
	 * We have a winner!
	 */
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
	fbt->fbtp_name = name;
	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, 3, fbt);
	} else {
		retfbt->fbtp_next = fbt;
		fbt->fbtp_id = retfbt->fbtp_id;
	}
	retfbt = fbt;

	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
	fbt->fbtp_symindx = symindx;
	if ((*instr & JAL_MASK) == JAL_INSTR)
		fbt->fbtp_rval = DTRACE_INVOP_J;
	else
		fbt->fbtp_rval = DTRACE_INVOP_RET;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	lf->fbt_nentries++;

	instr++;
	goto again;
}
