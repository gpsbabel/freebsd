/*-
 * Copyright (c) 2006 Peter Wemm
 * Copyright (c) 2015 The FreeBSD Foundation
 * All rights reserved.
 *
 * This software was developed by Andrew Turner under
 * sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_watchdog.h"

#include "opt_watchdog.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cons.h>
#include <sys/kernel.h>
#include <sys/kerneldump.h>
#include <sys/msgbuf.h>
#include <sys/watchdog.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/vm_page.h>
#include <vm/vm_phys.h>
#include <vm/pmap.h>

#include <machine/md_var.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <machine/minidump.h>

CTASSERT(sizeof(struct kerneldumpheader) == 512);

/*
 * Don't touch the first SIZEOF_METADATA bytes on the dump device. This
 * is to protect us from metadata and to protect metadata from us.
 */
#define	SIZEOF_METADATA		(64*1024)

uint64_t *vm_page_dump;
int vm_page_dump_size;

CTASSERT(sizeof(*vm_page_dump) == 8);

int
minidumpsys(struct dumperinfo *di)
{

	panic("minidumpsys");

	return (0);
}

void
dump_add_page(vm_paddr_t pa)
{
	int idx, bit;

	pa >>= PAGE_SHIFT;
	idx = pa >> 6;		/* 2^6 = 64 */
	bit = pa & 63;
	atomic_set_long(&vm_page_dump[idx], 1ul << bit);
}

void
dump_drop_page(vm_paddr_t pa)
{
	int idx, bit;

	pa >>= PAGE_SHIFT;
	idx = pa >> 6;		/* 2^6 = 64 */
	bit = pa & 63;
	atomic_clear_long(&vm_page_dump[idx], 1ul << bit);
}
