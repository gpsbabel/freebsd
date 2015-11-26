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
 * RISC-V Timer
 */

#include "opt_platform.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/timeet.h>
#include <sys/timetc.h>
#include <sys/watchdog.h>

#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/asm.h>
#include <machine/trap.h>

#ifdef FDT
#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>
#endif

struct riscv_tmr_softc {
	struct resource		*res[4];
	void			*ihl[4];
	uint32_t		clkfreq;
	struct eventtimer	et;
	bool			physical;
};

static struct riscv_tmr_softc *riscv_tmr_sc = NULL;

static struct resource_spec timer_spec[] = {
	{ SYS_RES_IRQ,		0,	RF_ACTIVE },
	{ -1, 0 }
};

static timecounter_get_t riscv_tmr_get_timecount;

static struct timecounter riscv_tmr_timecount = {
	.tc_name           = "RISC-V Timecounter",
	.tc_get_timecount  = riscv_tmr_get_timecount,
	.tc_poll_pps       = NULL,
	.tc_counter_mask   = ~0u,
	.tc_frequency      = 0,
	.tc_quality        = 1000,
};

#ifdef __riscv__
#define	get_el0(x)	cp15_## x ##_get()
#define	get_el1(x)	cp15_## x ##_get()
#define	set_el0(x, val)	cp15_## x ##_set(val)
#define	set_el1(x, val)	cp15_## x ##_set(val)
#else /* __aarch64__ */
#define	get_el0(x)	(0)
#define	get_el1(x)	(0)
#define	set_el0(x, val)	(0)
#define	set_el1(x, val) (0)
#endif

static void
set_mtimecmp(int c)
{

	__asm __volatile(
		"mv	t5, %0\n"
		"mv	t6, %1\n"
		"ecall" :: "r"(ECALL_MTIMECMP), "r"(c)
	);
}

static void
clear_pending(void)
{

	__asm __volatile(
		"mv	t5, %0\n"
		"ecall" :: "r"(ECALL_CLEAR_PENDING)
	);
}

static int
get_freq(void)
{

	return (0);
}

static long
get_cntxct(bool physical)
{
	uint64_t val;

	__asm __volatile("csrr %0, stime" : "=r"(val));

	return (val);
}

static unsigned
riscv_tmr_get_timecount(struct timecounter *tc)
{

	return (get_cntxct(riscv_tmr_sc->physical));
}

static int
riscv_tmr_start(struct eventtimer *et, sbintime_t first, sbintime_t period)
{
	struct riscv_tmr_softc *sc;
	int counts;

	//struct thread *td;
	//td = curthread;
	//printf("riscv_tmr_start first %d period %d sstatus 0x%016lx sc %d d %d\n",
	//		first, period, csr_read(sstatus), td->td_md.md_spinlock_count, td->td_md.md_saved_daif);

	sc = (struct riscv_tmr_softc *)et->et_priv;

	if (first != 0) {
		counts = ((uint32_t)et->et_frequency * first) >> 32;
		set_mtimecmp(counts);

		return (0);
	}

	return (EINVAL);

}

static int
riscv_tmr_stop(struct eventtimer *et)
{
	struct riscv_tmr_softc *sc;
	//int ctrl;

	printf("riscv_tmr_stop\n");

	sc = (struct riscv_tmr_softc *)et->et_priv;

	//ctrl = get_ctrl(sc->physical);
	//ctrl &= GT_CTRL_ENABLE;
	//set_ctrl(ctrl, sc->physical);

	return (0);
}

static int
riscv_tmr_intr(void *arg)
{
	struct riscv_tmr_softc *sc;

	sc = (struct riscv_tmr_softc *)arg;

	/* Clear pending */
	/* not implemented in spike */
	//csr_clear(sip, (1 << 5));

	clear_pending();

	if (sc->et.et_active)
		sc->et.et_event_cb(&sc->et, sc->et.et_arg);

	return (FILTER_HANDLED);
}

#ifdef FDT
static int
riscv_tmr_fdt_probe(device_t dev)
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if (ofw_bus_is_compatible(dev, "riscv,timer")) {
		device_set_desc(dev, "RISC-V Timer");
		return (BUS_PROBE_DEFAULT);
	}

	return (ENXIO);
}
#endif

static int
riscv_tmr_attach(device_t dev)
{
	struct riscv_tmr_softc *sc;

#ifdef FDT
	phandle_t node;
	pcell_t clock;
#endif
	int error;

	sc = device_get_softc(dev);
	if (riscv_tmr_sc)
		return (ENXIO);

#ifdef FDT
	/* Get the base clock frequency */
	node = ofw_bus_get_node(dev);
	if (node > 0) {
		error = OF_getprop(node, "clock-frequency", &clock,
		    sizeof(clock));
		if (error > 0) {
			sc->clkfreq = fdt32_to_cpu(clock);
		}
	}
#endif

	sc->clkfreq = 1000000;

	if (sc->clkfreq == 0) {
		/* Try to get clock frequency from timer */
		sc->clkfreq = get_freq();
	}

	if (sc->clkfreq == 0) {
		device_printf(dev, "No clock frequency specified\n");
		return (ENXIO);
	}

	if (bus_alloc_resources(dev, timer_spec, sc->res)) {
		device_printf(dev, "could not allocate resources\n");
		return (ENXIO);
	}

#ifdef __riscv__
	sc->physical = true;
#else /* __aarch64__ */
	sc->physical = false;
#endif

	riscv_tmr_sc = sc;

	/* Setup IRQs handler */
	error = bus_setup_intr(dev, sc->res[0], INTR_TYPE_CLK,
	    riscv_tmr_intr, NULL, sc, &sc->ihl[0]);
	if (error) {
		device_printf(dev, "Unable to alloc int resource.\n");
		return (ENXIO);
	}

	riscv_tmr_timecount.tc_frequency = sc->clkfreq;
	tc_init(&riscv_tmr_timecount);

	sc->et.et_name = "RISC-V Eventtimer";
	sc->et.et_flags = ET_FLAGS_ONESHOT | ET_FLAGS_PERCPU;
	sc->et.et_quality = 1000;

	sc->et.et_frequency = sc->clkfreq;
	sc->et.et_min_period = (0x00000002LLU << 32) / sc->et.et_frequency;
	sc->et.et_max_period = (0xfffffffeLLU << 32) / sc->et.et_frequency;
	sc->et.et_start = riscv_tmr_start;
	sc->et.et_stop = riscv_tmr_stop;
	sc->et.et_priv = sc;
	et_register(&sc->et);

#if 0
	/* Atomic tests */
	uint64_t t1;
	uint64_t t;
	t = 0;
	atomic_add_long(&t, 1);
	printf("expect 1 == %d\n", t);
	t1 = atomic_swap_64(&t, 0);
	printf("expect 0 1 == %d %d\n", t, t1);
	atomic_subtract_long(&t1, 1);
	printf("expect 0 == %d\n", t1);

	atomic_add_long(&t, 1);
	printf("expect 1 == %d\n", t);
	atomic_clear_long(&t, 1);
	printf("clear expect 0 == %d\n", t);

	uint32_t t2;
	t2 = 0;
	atomic_add_int(&t2, 1);
	printf("expect 1 == %d\n", t2);
	atomic_subtract_int(&t2, 1);
	printf("expect 0 == %d\n", t2);
#endif

	return (0);
}

#ifdef FDT
static device_method_t riscv_tmr_fdt_methods[] = {
	DEVMETHOD(device_probe,		riscv_tmr_fdt_probe),
	DEVMETHOD(device_attach,	riscv_tmr_attach),
	{ 0, 0 }
};

static driver_t riscv_tmr_fdt_driver = {
	"timer",
	riscv_tmr_fdt_methods,
	sizeof(struct riscv_tmr_softc),
};

static devclass_t riscv_tmr_fdt_devclass;

EARLY_DRIVER_MODULE(timer, simplebus, riscv_tmr_fdt_driver, riscv_tmr_fdt_devclass,
    0, 0, BUS_PASS_TIMER + BUS_PASS_ORDER_MIDDLE);
EARLY_DRIVER_MODULE(timer, ofwbus, riscv_tmr_fdt_driver, riscv_tmr_fdt_devclass,
    0, 0, BUS_PASS_TIMER + BUS_PASS_ORDER_MIDDLE);
#endif

void
DELAY(int usec)
{
	int32_t counts, counts_per_usec;
	uint32_t first, last;

	printf("DELAY %d\n", usec);

	/*
	 * Check the timers are setup, if not just
	 * use a for loop for the meantime
	 */
	if (riscv_tmr_sc == NULL) {
		for (; usec > 0; usec--)
			for (counts = 200; counts > 0; counts--)
				/*
				 * Prevent the compiler from optimizing
				 * out the loop
				 */
				cpufunc_nullop();
		return;
	}

	/* Get the number of times to count */
	counts_per_usec = ((riscv_tmr_timecount.tc_frequency / 1000000) + 1);

	/*
	 * Clamp the timeout at a maximum value (about 32 seconds with
	 * a 66MHz clock). *Nobody* should be delay()ing for anywhere
	 * near that length of time and if they are, they should be hung
	 * out to dry.
	 */
	if (usec >= (0x80000000U / counts_per_usec))
		counts = (0x80000000U / counts_per_usec) - 1;
	else
		counts = usec * counts_per_usec;

	first = get_cntxct(riscv_tmr_sc->physical);

	while (counts > 0) {
		last = get_cntxct(riscv_tmr_sc->physical);
		counts -= (int32_t)(last - first);
		first = last;
	}
}
