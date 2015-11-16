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
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/pcpu.h>
#include <sys/proc.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/asm.h>
#include <machine/htif.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

#include "htif.h"
#include "htif_block.h"

extern uint64_t console_data;
extern uint64_t htif_ring_last;
extern uint64_t htif_ring_cursor;

struct ring_entry {
	uint64_t data;
	uint64_t used;
	uint64_t pnext;
	uint64_t *next;
};

#define	HTIF_RING_SIZE	1024

struct ring_entry htif_ring[HTIF_RING_SIZE];

uint64_t
htif_command(uint64_t cmd, uint64_t m)
{
	uint64_t res;

	__asm __volatile(
		"mv	t5, %2\n"
		"mv	t6, %1\n"
		"ecall\n"
		"mv	%0, t6" : "=&r"(res) : "r"(cmd), "r"(m)
	);

	return (res);
}

void
htif_intr(void)
{
	uint64_t entry;
	uint64_t cmd;

	cmd = 0;
	entry = htif_command(cmd, ECALL_HTIF_GET_ENTRY);
	while (entry) {
		//printf("entry 0x%016lx\n", entry);
		if ((entry >> 48) == 0x100)
			riscv_console_intr(entry & 0xff);
		//if ((entry >> 56) == 1)
		//if (((entry >> 48) && 0xff) == 1)

		if ((entry >> 56) == 0x2) {
			//printf("entry 0x%016lx\n", entry);
			htif_blk_intr(entry);
		}

		entry = htif_command(cmd, ECALL_HTIF_GET_ENTRY);
	}

	csr_clear(sip, SIE_SSIE);
}

int
htif_test(struct htif_softc *sc);

int
htif_test(struct htif_softc *sc)
{
	//char id[HTIF_MAX_ID] __aligned(HTIF_ALIGN);
	uint64_t data;
	uint64_t cmd;
	uint64_t paddr;
	int i;
	int len;
	char id[HTIF_MAX_ID] __aligned(HTIF_ALIGN);
	struct htif_dev_softc *dev_sc;

	//for (i = 0; i < HTIF_MAX_DEV; i++) {
	for (i = 0; i < 3; i++) {

		//cmd = 0x101000000000000;

		paddr = pmap_kextract((vm_offset_t)&id);
		//printf("paddr 0x%016lx\n", paddr);
		data = (paddr << 8) | 0xff;

		cmd = i;
		cmd <<= 56;
		cmd |= (HTIF_CMD_IDENTIFY << 48);
		cmd |= data;
		htif_command(cmd, ECALL_HTIF_CMD);

		DELAY(10000);

		len = strnlen(id, sizeof(id));
		if (len <= 0) {
			continue;
		}
		//printf("HTIF enumerate %d\n", i);
		//printf("len is %d id is %d\n", len, id[0]);

		//htif_tohost(i, HTIF_CMD_IDENTIFY, (__pa(id) << 8) | 0xFF);
		//htif_fromhost();

		if (i != 2)
			continue;

		/* block */
		//printf("adding block child\n");

		dev_sc = malloc(sizeof(struct htif_dev_softc), M_DEVBUF, M_NOWAIT | M_ZERO);
		dev_sc->sc = sc;
		dev_sc->index = i;
		dev_sc->id = malloc(HTIF_MAX_ID, M_DEVBUF, M_NOWAIT | M_ZERO);
		memcpy(dev_sc->id, id, HTIF_MAX_ID);

		dev_sc->dev = device_add_child(sc->dev, "htif_blk", -1);
		device_set_ivars(dev_sc->dev, dev_sc);
	}

	return (bus_generic_attach(sc->dev));
}

#if 0
void
htif_init(void)
{

	htif_test();
}

SYSINIT(htif, SI_SUB_CPU, SI_ORDER_ANY, htif_init, NULL);
#endif

static int
htif_probe(device_t dev) 
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if (!ofw_bus_is_compatible(dev, "riscv,htif"))
		return (ENXIO);

	printf("htif_probe\n");

	device_set_desc(dev, "HTIF bus");
	return (BUS_PROBE_DEFAULT);
}

static int
htif_attach(device_t dev)
{
	struct htif_softc *sc;

	sc = device_get_softc(dev);
	sc->dev = dev;
	mtx_init(&sc->sc_mtx, device_get_nameunit(dev), "htif_command", MTX_DEF);

#if 0
	int i;

	/* Initialize ring */
	memset(&htif_ring, 0, sizeof(struct ring_entry) * HTIF_RING_SIZE);
	for (i = 0; i < HTIF_RING_SIZE; i++) {
		if (i == (HTIF_RING_SIZE - 1))
			htif_ring[i].next = (uint64_t *)&htif_ring[0];
		else
			htif_ring[i].next = (uint64_t *)&htif_ring[i+1];
		htif_ring[i].pnext = vtophys(htif_ring[i].next);
		htif_ring[i].used = 0;
		htif_ring[i].data = 0;
	}
	uint64_t *cc = &htif_ring_cursor;
	*cc = vtophys((uint64_t)&htif_ring);

	uint64_t *cc1 = &htif_ring_last;
	*cc1 = vtophys((uint64_t)&htif_ring);
#endif

	csr_set(sie, SIE_SSIE);

	return (htif_test(sc));
}

static device_method_t htif_methods[] = {
	/* Bus interface */
	DEVMETHOD(device_probe,		htif_probe),
	DEVMETHOD(device_attach,	htif_attach),
	DEVMETHOD_END
};

static driver_t htif_driver = {
	"htif",
	htif_methods,
	sizeof(struct htif_softc)
};

static devclass_t htif_devclass;

DRIVER_MODULE(htif, simplebus, htif_driver,
    htif_devclass, 0, 0);
