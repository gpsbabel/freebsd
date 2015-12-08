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

/*
 * RISC-V Interrupt Controller
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/module.h>
#include <sys/rman.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/cpuset.h>
#include <sys/lock.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/smp.h>
#include <machine/asm.h>
#include <machine/trap.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

struct riscv_ric_softc {
	device_t		dev;
	uint32_t		nirqs;
};

static struct ofw_compat_data compat_data[] = {
	{"riscv,pic",		true},	/* Non-standard, used in FreeBSD dts. */
	{NULL,			false}
};

#include "pic_if.h"

//#define	NR_IRQS		2
//#define	IRQ_SOFTWARE	0
//#define	IRQ_TIMER	1

enum {
	IRQ_SOFTWARE,
	IRQ_TIMER,
	NIRQS
};
#define	SR_IE		(1 << 0)

static struct riscv_ric_softc *riscv_ric_sc = NULL;

static pic_dispatch_t ric_dispatch;
static pic_eoi_t ric_eoi;
static pic_mask_t ric_mask_irq;
static pic_unmask_t ric_unmask_irq;

static int
riscv_ric_probe(device_t dev)
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if (!ofw_bus_search_compatible(dev, compat_data)->ocd_data)
		return (ENXIO);

	device_set_desc(dev, "RISC-V Interrupt Controller");
	return (BUS_PROBE_DEFAULT);
}

static int
riscv_ric_attach(device_t dev)
{
	struct riscv_ric_softc *sc;

	if (riscv_ric_sc)
		return (ENXIO);

	sc = device_get_softc(dev);
	sc->dev = dev;

	riscv_ric_sc = sc;

	sc->nirqs = 2;
	riscv_register_root_pic(dev, sc->nirqs);

	//csr_set(sstatus, SR_IE);
	//csr_clear(sie, (1 << 5));

	return (0);
}

static void ric_dispatch(device_t dev, struct trapframe *frame)
{
	struct riscv_ric_softc *sc;
	uint32_t active_irq;

	sc = device_get_softc(dev);

	active_irq = (frame->tf_scause & 0xf);
	if (frame->tf_scause & (1 << 31)) {
		//printf("ric_dispatch %d\n", active_irq);
		riscv_dispatch_intr(active_irq, frame);
		//return;
	}
}

static void
ric_eoi(device_t dev, u_int irq)
{
	//struct riscv_ric_softc *sc = device_get_softc(dev);
	//printf("%s\n", __func__);
	//ric_c_write_4(sc, GICC_EOIR, irq);
}

void
ric_mask_irq(device_t dev, u_int irq)
{
	//struct riscv_ric_softc *sc = device_get_softc(dev);
	printf("ric_mask_irq\n");
	//ric_d_write_4(sc, GICD_ICENABLER(irq >> 5), (1UL << (irq & 0x1F)));
	//ric_c_write_4(sc, GICC_EOIR, irq);
}

void
ric_unmask_irq(device_t dev, u_int irq)
{
	struct riscv_ric_softc *sc;

	sc = device_get_softc(dev);

	//printf("ric_unmask_irq %d\n", irq);

	switch (irq) {
	case IRQ_TIMER:
		csr_set(sie, SIE_STIE);
		break;
	case IRQ_SOFTWARE:
		//csr_set(sie, SIE_SSIE);
		break;
	default:
		panic("Unknown irq %d\n", irq);
	}

	//panic("%s: %d\n", __func__, irq);
	//printf("%s: %d\n", __func__, irq);
	//ric_d_write_4(sc, GICD_ISENABLER(irq >> 5), (1UL << (irq & 0x1F)));
}

static device_method_t riscv_ric_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		riscv_ric_probe),
	DEVMETHOD(device_attach,	riscv_ric_attach),

	/* pic_if */
	DEVMETHOD(pic_dispatch,		ric_dispatch),
	DEVMETHOD(pic_eoi,		ric_eoi),
	DEVMETHOD(pic_mask,		ric_mask_irq),
	DEVMETHOD(pic_unmask,		ric_unmask_irq),

	DEVMETHOD_END
};

DECLARE_CLASS(riscv_ric_driver);
DEFINE_CLASS_0(ric, riscv_ric_driver, riscv_ric_methods,
    sizeof(struct riscv_ric_softc));

static devclass_t riscv_ric_devclass;

EARLY_DRIVER_MODULE(ric, simplebus, riscv_ric_driver,
    riscv_ric_devclass, 0, 0, BUS_PASS_INTERRUPT + BUS_PASS_ORDER_MIDDLE);
EARLY_DRIVER_MODULE(ric, ofwbus, riscv_ric_driver, riscv_ric_devclass,
    0, 0, BUS_PASS_INTERRUPT + BUS_PASS_ORDER_MIDDLE);
