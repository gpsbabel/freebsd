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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/kdb.h>
#include <sys/kernel.h>
#include <sys/priv.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/cons.h>
#include <sys/consio.h>
#include <sys/tty.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/htif.h>

#include "htif.h"

#include <dev/ofw/openfirm.h>

#include <ddb/ddb.h>

//#include "riscvcall.h"

//#define MAMBOBURSTLEN	4096	/* max number of bytes to write in one chunk */
#define MAMBOBURSTLEN	1	/* max number of bytes to write in one chunk */

#define MAMBO_CONSOLE_WRITE	0
#define MAMBO_CONSOLE_READ	60

extern uint64_t htif_ring_last;
extern uint64_t htif_ring_cursor;
extern uint64_t console_intr;

static tsw_outwakeup_t riscvtty_outwakeup;

static struct ttydevsw riscv_ttydevsw = {
	.tsw_flags	= TF_NOPREFIX,
	.tsw_outwakeup	= riscvtty_outwakeup,
};

static int			polltime;
static struct callout		riscv_callout;
static struct tty 		*tp = NULL;

#if defined(KDB)
static int			alt_break_state;
#endif

static void	riscv_timeout(void *);

static cn_probe_t	riscv_cnprobe;
static cn_init_t	riscv_cninit;
static cn_term_t	riscv_cnterm;
static cn_getc_t	riscv_cngetc;
static cn_putc_t	riscv_cnputc;
static cn_grab_t	riscv_cngrab;
static cn_ungrab_t	riscv_cnungrab;

CONSOLE_DRIVER(riscv);

#define	QUEUE_SIZE	256
struct queue_entry {
	uint64_t data;
	uint64_t used;
	struct queue_entry *next;
};

struct queue_entry cnqueue[QUEUE_SIZE];
struct queue_entry *entry_last;
struct queue_entry *entry_served;

static void 
htif_early_putc(int c)
{
	uint64_t cmd;

	cmd = 0x101000000000000;
	cmd |= c;

	htif_command(cmd, ECALL_HTIF_CMD);
}

static uint8_t
htif_getc(void)
{
	uint64_t cmd;
	uint8_t res;

	cmd = 0x100000000000000;

	res = htif_command(cmd, ECALL_HTIF_CMD);

	return (res);
}

static void
riscv_putc(int c)
{
	uint64_t *cc = (uint64_t*)&console_intr;
	//*cc = 0;
	console_intr = 0;

	//uint64_t addr;
	uint64_t val;
	val = 0;

	__asm __volatile(
		"li	%0, 0\n"
		"sd	%0, 0(%1)" : "=&r"(val) : "r"(cc)
	);

	htif_early_putc(c);

	uint64_t tmp;

	tmp = 0;

	__asm __volatile(
		"li	%0, 1 \n"
		"slli	%0, %0, 12 \n"
		"1:\n"
		"addi	%0, %0, -1 \n"
		"beqz	%0, 2f \n"
		"ld	%1, 0(%2)\n"
		"beqz	%1, 1b\n"
		"2:\n"
		: "=&r"(tmp), "=&r"(val) : "r"(cc)
	);
}

static void
cn_drvinit(void *unused)
{

	if (riscv_consdev.cn_pri != CN_DEAD &&
	    riscv_consdev.cn_name[0] != '\0') {
		//if (OF_finddevice("/riscv") == -1)
		//	return;

		tp = tty_alloc(&riscv_ttydevsw, NULL);
		tty_init_console(tp, 0);
		tty_makedev(tp, NULL, "%s", "rcons");

		polltime = 1;

		callout_init(&riscv_callout, 1);
		callout_reset(&riscv_callout, polltime, riscv_timeout, NULL);
	}
}

SYSINIT(cndev, SI_SUB_CONFIGURE, SI_ORDER_MIDDLE, cn_drvinit, NULL);

static void
riscvtty_outwakeup(struct tty *tp)
{
	int len;
	int i;
	u_char buf[MAMBOBURSTLEN];

	for (;;) {
		len = ttydisc_getc(tp, buf, sizeof buf);
		if (len == 0)
			break;
		//riscvcall(MAMBO_CONSOLE_WRITE, buf, (register_t)len, 1UL);
		for (i = 0; i < len; i++)
			riscv_putc(buf[i]);
			//htif_early_putc(buf[i]);
	}
}

static void
riscv_timeout(void *v)
{
	int 	c;

	tty_lock(tp);
	while ((c = riscv_cngetc(NULL)) != -1)
		ttydisc_rint(tp, c, 0);
	ttydisc_rint_done(tp);
	tty_unlock(tp);

	callout_reset(&riscv_callout, polltime, riscv_timeout, NULL);
}

void
riscv_console_intr(uint8_t c)
{

	entry_last->data = c;
	entry_last->used = 1;
	entry_last = entry_last->next;
}

static void
riscv_cnprobe(struct consdev *cp)
{
	//if (OF_finddevice("/riscv") == -1) {
	//	cp->cn_pri = CN_DEAD;
	//	return;
	//}

	cp->cn_pri = CN_NORMAL;
}

static void
riscv_cninit(struct consdev *cp)
{

	/* XXX: This is the alias, but that should be good enough */
	strcpy(cp->cn_name, "rcons");

	int i;

	for (i = 0; i < QUEUE_SIZE; i++) {
		if (i == (QUEUE_SIZE - 1))
			cnqueue[i].next = &cnqueue[0];
		else
			cnqueue[i].next = &cnqueue[i+1];
		cnqueue[i].data = 0;
		cnqueue[i].used = 0;
	}
	entry_last = &cnqueue[0];
	entry_served = &cnqueue[0];
}

static void
riscv_cnterm(struct consdev *cp)
{
}

static void
riscv_cngrab(struct consdev *cp)
{
}

static void
riscv_cnungrab(struct consdev *cp)
{
}

static int
riscv_cngetc(struct consdev *cp)
{
	uint8_t data;
	int ch;

	ch = htif_getc();

	if (entry_served->used == 1) {
		data = entry_served->data;
		entry_served->used = 0;
		entry_served = entry_served->next;
		return (data & 0xff);
	}
	return (-1);

	//ch = c;
	if (ch > 0 && ch < 0xff) {
		//*cc = 0;
#if defined(KDB)
		kdb_alt_break(ch, &alt_break_state);
#endif
		//return (ch);
	}

	return (-1);
}

static void
riscv_cnputc(struct consdev *cp, int c)
{
	//char cbuf;
	//cbuf = c;
	//riscvcall(MAMBO_CONSOLE_WRITE, &cbuf, 1UL, 1UL);

	riscv_putc(c);
}
