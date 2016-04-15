/*-
 * Copyright (c) 2016 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/timeet.h>
#include <sys/timetc.h>

#include <dev/mmc/bridge.h>
#include <dev/mmc/mmcreg.h>
#include <dev/mmc/mmcbrvar.h>

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/mmc/host/dwmmc_reg.h>
#include <dev/mmc/host/dwmmc_var.h>

#include <dev/spibus/spi.h>
#include <dev/spibus/spibusvar.h>

#include "spibus_if.h"
#include "mmcbr_if.h"

const uint8_t crc7_be_syndrome[256] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e,
	0x90, 0x82, 0xb4, 0xa6, 0xd8, 0xca, 0xfc, 0xee,
	0x32, 0x20, 0x16, 0x04, 0x7a, 0x68, 0x5e, 0x4c,
	0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc,
	0x64, 0x76, 0x40, 0x52, 0x2c, 0x3e, 0x08, 0x1a,
	0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a,
	0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28,
	0xc6, 0xd4, 0xe2, 0xf0, 0x8e, 0x9c, 0xaa, 0xb8,
	0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6,
	0x58, 0x4a, 0x7c, 0x6e, 0x10, 0x02, 0x34, 0x26,
	0xfa, 0xe8, 0xde, 0xcc, 0xb2, 0xa0, 0x96, 0x84,
	0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06, 0x14,
	0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2,
	0x3c, 0x2e, 0x18, 0x0a, 0x74, 0x66, 0x50, 0x42,
	0x9e, 0x8c, 0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0,
	0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70,
	0x82, 0x90, 0xa6, 0xb4, 0xca, 0xd8, 0xee, 0xfc,
	0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c,
	0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce,
	0x20, 0x32, 0x04, 0x16, 0x68, 0x7a, 0x4c, 0x5e,
	0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98,
	0x76, 0x64, 0x52, 0x40, 0x3e, 0x2c, 0x1a, 0x08,
	0xd4, 0xc6, 0xf0, 0xe2, 0x9c, 0x8e, 0xb8, 0xaa,
	0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e, 0x28, 0x3a,
	0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34,
	0xda, 0xc8, 0xfe, 0xec, 0x92, 0x80, 0xb6, 0xa4,
	0x78, 0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06,
	0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96,
	0x2e, 0x3c, 0x0a, 0x18, 0x66, 0x74, 0x42, 0x50,
	0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0,
	0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70, 0x62,
	0x8c, 0x9e, 0xa8, 0xba, 0xc4, 0xd6, 0xe0, 0xf2,
};

static uint8_t
crc7(uint8_t crc, const uint8_t *buffer, size_t len)
{
	uint8_t data;

	while (len--) {
		data = *buffer++;
		crc = crc7_be_syndrome[crc ^ data];
	}

	return crc;
}

#define dprintf(x, arg...)	printf(x, arg)

#define	READ4(_sc, _reg) \
	bus_read_4((_sc)->res[0], _reg)
#define	WRITE4(_sc, _reg, _val) \
	bus_write_4((_sc)->res[0], _reg, _val)

#define	DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))

#define	DWMMC_LOCK(_sc)			mtx_lock(&(_sc)->sc_mtx)
#define	DWMMC_UNLOCK(_sc)		mtx_unlock(&(_sc)->sc_mtx)
#define	DWMMC_LOCK_INIT(_sc) \
	mtx_init(&_sc->sc_mtx, device_get_nameunit(_sc->dev), \
	    "dwmmc", MTX_DEF)
#define	DWMMC_LOCK_DESTROY(_sc)		mtx_destroy(&_sc->sc_mtx);
#define	DWMMC_ASSERT_LOCKED(_sc)	mtx_assert(&_sc->sc_mtx, MA_OWNED);
#define	DWMMC_ASSERT_UNLOCKED(_sc)	mtx_assert(&_sc->sc_mtx, MA_NOTOWNED);

#define	PENDING_CMD	0x01
#define	PENDING_STOP	0x02
#define	CARD_INIT_DONE	0x04

static int
dwmmc_probe(device_t dev)
{

	//if (!ofw_bus_status_okay(dev))
	//	return (ENXIO);
	//hwtype = ofw_bus_search_compatible(dev, compat_data)->ocd_data;
	//if (hwtype == HWTYPE_NONE)
	//	return (ENXIO);

	device_set_desc(dev, "MMC SPI");
	return (BUS_PROBE_DEFAULT);
}

int
dwmmc_attach(device_t dev)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(dev);
	sc->dev = dev;

	DWMMC_LOCK_INIT(sc);

	sc->bus_hz = 20000000;

	sc->host.f_min = 400000;
	sc->host.f_max = min(200000000, sc->bus_hz);
	sc->host.host_ocr = MMC_OCR_320_330 | MMC_OCR_330_340;
	sc->host.caps = MMC_CAP_4_BIT_DATA | MMC_CAP_SPI;

	device_add_child(dev, "mmc", -1);
	return (bus_generic_attach(dev));
}

static int
dwmmc_update_ios(device_t brdev, device_t reqdev)
{
	struct dwmmc_softc *sc;
	struct mmc_ios *ios;

	sc = device_get_softc(brdev);
	ios = &sc->host.ios;

	dprintf("Setting up clk %u bus_width %d\n",
		ios->clock, ios->bus_width);

	return (0);
}

static uint8_t
xchg_spi(struct dwmmc_softc *sc, uint8_t byte)
{
	struct spi_command spi_cmd;
	uint8_t msg_dinp;

	msg_dinp = 0;

	memset(&spi_cmd, 0, sizeof(spi_cmd));
	spi_cmd.tx_cmd = &byte;
	spi_cmd.rx_cmd = &msg_dinp;
	spi_cmd.tx_cmd_sz = 1;
	spi_cmd.rx_cmd_sz = 1; 

	SPIBUS_TRANSFER(device_get_parent(sc->dev), sc->dev, &spi_cmd);

	return (msg_dinp);
}

static uint8_t
xchg_spi_multi(struct dwmmc_softc *sc, uint8_t *out_bytes,
    uint8_t *in_bytes, uint32_t nbytes)
{
	struct spi_command spi_cmd;

	memset(&spi_cmd, 0, sizeof(spi_cmd));
	spi_cmd.tx_cmd = out_bytes;
	spi_cmd.rx_cmd = in_bytes;
	spi_cmd.tx_cmd_sz = nbytes;
	spi_cmd.rx_cmd_sz = nbytes;

	SPIBUS_TRANSFER(device_get_parent(sc->dev), sc->dev, &spi_cmd);

	return (0);
}

static int
wait_ready(struct dwmmc_softc *sc, int timeout)
{
	int i;

	for (i = 0; i < (timeout * 5000); i++) {
		if (xchg_spi(sc, 0xff) == 0xff)
			return (1);	/* Ready */
	}

	/* Timeout */

	return (0);
}

static int
mmc_spi_req(struct dwmmc_softc *sc, struct mmc_command *cmd)
{
	//struct mmc_command *cmd;
	struct mmc_data *data;
	uint32_t timeout;
	uint8_t *ptr;
	uint32_t reg;
	int success;
	//uint8_t crc;
	uint8_t d;
	uint8_t req_in[7];
	uint8_t req[7];
	int ret;
	int i;
	int j;

	//cmd = req->cmd;
	data = cmd->data;

	req[0] = 0xff;
	req[1] = (0x40 | cmd->opcode);
	req[2] = (cmd->arg >> 24);
	req[3] = (cmd->arg >> 16);
	req[4] = (cmd->arg >> 8);
	req[5] = (cmd->arg >> 0);
	req[6] = crc7(0, &req[1], 5) | 0x01;
	xchg_spi_multi(sc, req, req_in, 7);

	/* Wait response */
	ret = 0;
	cmd->error = MMC_ERR_NONE;
	success = 0;

	for (i = 0; i < 10; i++) {
		ret = xchg_spi(sc, 0xff);
		if ((ret & 0x80) == 0) {
			success = 1;
			printf("SUCCESS(%d): 0x%x\n", i, ret);
			break;
		}
	}
	if (cmd->opcode == 41) {
		if (ret != 0) {
			cmd->error = MMC_ERR_TIMEOUT;
		}
	}
	if (!success) {
		printf("CMD %d FAILED timeout %d\n", cmd->opcode, i);
		cmd->error = MMC_ERR_TIMEOUT;
	}

	if (success && cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {

			/* Wait for data */
			timeout = 20;
			do {
				reg = xchg_spi(sc, 0xff);
				timeout--;
			} while ((reg == 0xff) && timeout);
			if (reg != 0xFE) {
				printf("FAILED to get RSP136\n");
			}

			for (j = 0; j < 4; j++) {
				reg = 0;
				for (i = 3; i >= 0; i--) {
					reg |= (xchg_spi(sc, 0xff) << (i * 8));
				}
				printf("long resp(%d): 0x%08x\n", j, reg);
				cmd->resp[j] = reg;
			}
		//} else if (cmd->flags & MMC_RSP_R1) {
		//	reg = xchg_spi(sc, 0xff);
		//	printf("short resp: 0x%02x\n", reg);
		//	cmd->resp[0] = reg;
		//} else if (cmd->flags & MMC_RSP_R3) {
		} else if (cmd->opcode != 17 && cmd->opcode != 18) {
			reg = 0;
			for (i = 3; i >= 0; i--) {
				reg |= (xchg_spi(sc, 0xff) << (i * 8));
			}
			printf("short resp0: 0x%08x\n", reg);
			cmd->resp[0] = reg;
		}
	}

	int count;
	if (success && (cmd->opcode == 17 || cmd->opcode == 18)) {
		count = (data->len / 512);
		ptr = data->data;

		for (j = 0; j < count; j++) {
			/* Wait for data */
			timeout = 200;
			do {
				reg = xchg_spi(sc, 0xff);
				timeout--;
			} while ((reg == 0xff) && timeout);
			if (reg != 0xFE) {
				printf("FAILED to wait DATA\n");
			}
			for (i = 0; i < 512; i++) {
				d = xchg_spi(sc, 0xff);
				*ptr++ = d;
				//printf("%x ", d);
			}
			xchg_spi(sc, 0xFF);	/* Skip CRC */
			xchg_spi(sc, 0xFF);
		}
		//printf("\n");
	}

	xchg_spi(sc, 0xff);

	return (0);
}

static int
dwmmc_request(device_t brdev, device_t reqdev, struct mmc_request *req)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(brdev);

	DWMMC_LOCK(sc);

	SPIBUS_CHIP_SELECT(device_get_parent(sc->dev), sc->dev);
	if (!wait_ready(sc, 500)) {
		DWMMC_UNLOCK(sc);
		return (1);
	}
	mmc_spi_req(sc, req->cmd);
	if (req->stop) {
		mmc_spi_req(sc, req->stop);
	}
	SPIBUS_CHIP_DESELECT(device_get_parent(sc->dev), sc->dev);

	req->done(req);

	DWMMC_UNLOCK(sc);

	return (0);
}

static int
dwmmc_get_ro(device_t brdev, device_t reqdev)
{

	return (0);
}

static int
dwmmc_acquire_host(device_t brdev, device_t reqdev)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(brdev);

	DWMMC_LOCK(sc);
	while (sc->bus_busy)
		msleep(sc, &sc->sc_mtx, PZERO, "dwmmcah", hz / 5);
	sc->bus_busy++;
	DWMMC_UNLOCK(sc);
	return (0);
}

static int
dwmmc_release_host(device_t brdev, device_t reqdev)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(brdev);

	DWMMC_LOCK(sc);
	sc->bus_busy--;
	wakeup(sc);
	DWMMC_UNLOCK(sc);
	return (0);
}

static int
dwmmc_read_ivar(device_t bus, device_t child, int which, uintptr_t *result)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(bus);

	switch (which) {
	default:
		return (EINVAL);
	case MMCBR_IVAR_BUS_MODE:
		*(int *)result = sc->host.ios.bus_mode;
		break;
	case MMCBR_IVAR_BUS_WIDTH:
		*(int *)result = sc->host.ios.bus_width;
		break;
	case MMCBR_IVAR_CHIP_SELECT:
		*(int *)result = sc->host.ios.chip_select;
		break;
	case MMCBR_IVAR_CLOCK:
		*(int *)result = sc->host.ios.clock;
		break;
	case MMCBR_IVAR_F_MIN:
		*(int *)result = sc->host.f_min;
		break;
	case MMCBR_IVAR_F_MAX:
		*(int *)result = sc->host.f_max;
		break;
	case MMCBR_IVAR_HOST_OCR:
		*(int *)result = sc->host.host_ocr;
		break;
	case MMCBR_IVAR_MODE:
		*(int *)result = sc->host.mode;
		break;
	case MMCBR_IVAR_OCR:
		*(int *)result = sc->host.ocr;
		break;
	case MMCBR_IVAR_POWER_MODE:
		*(int *)result = sc->host.ios.power_mode;
		break;
	case MMCBR_IVAR_VDD:
		*(int *)result = sc->host.ios.vdd;
		break;
	case MMCBR_IVAR_CAPS:
		sc->host.caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;
		*(int *)result = sc->host.caps;
		break;
	case MMCBR_IVAR_MAX_DATA:
		*(int *)result = 65535;
	}
	return (0);
}

static int
dwmmc_write_ivar(device_t bus, device_t child, int which, uintptr_t value)
{
	struct dwmmc_softc *sc;

	sc = device_get_softc(bus);

	switch (which) {
	default:
		return (EINVAL);
	case MMCBR_IVAR_BUS_MODE:
		sc->host.ios.bus_mode = value;
		break;
	case MMCBR_IVAR_BUS_WIDTH:
		sc->host.ios.bus_width = value;
		break;
	case MMCBR_IVAR_CHIP_SELECT:
		sc->host.ios.chip_select = value;
		break;
	case MMCBR_IVAR_CLOCK:
		sc->host.ios.clock = value;
		break;
	case MMCBR_IVAR_MODE:
		sc->host.mode = value;
		break;
	case MMCBR_IVAR_OCR:
		sc->host.ocr = value;
		break;
	case MMCBR_IVAR_POWER_MODE:
		sc->host.ios.power_mode = value;
		break;
	case MMCBR_IVAR_VDD:
		sc->host.ios.vdd = value;
		break;
	/* These are read-only */
	case MMCBR_IVAR_CAPS:
	case MMCBR_IVAR_HOST_OCR:
	case MMCBR_IVAR_F_MIN:
	case MMCBR_IVAR_F_MAX:
	case MMCBR_IVAR_MAX_DATA:
		return (EINVAL);
	}
	return (0);
}

static device_method_t dwmmc_methods[] = {
	DEVMETHOD(device_probe,		dwmmc_probe),
	DEVMETHOD(device_attach,	dwmmc_attach),

	/* Bus interface */
	DEVMETHOD(bus_read_ivar,	dwmmc_read_ivar),
	DEVMETHOD(bus_write_ivar,	dwmmc_write_ivar),

	/* mmcbr_if */
	DEVMETHOD(mmcbr_update_ios,	dwmmc_update_ios),
	DEVMETHOD(mmcbr_request,	dwmmc_request),
	DEVMETHOD(mmcbr_get_ro,		dwmmc_get_ro),
	DEVMETHOD(mmcbr_acquire_host,	dwmmc_acquire_host),
	DEVMETHOD(mmcbr_release_host,	dwmmc_release_host),

	DEVMETHOD_END
};

driver_t dwmmc_driver = {
	"mmc_spi",
	dwmmc_methods,
	sizeof(struct dwmmc_softc),
};

static devclass_t dwmmc_devclass;

DRIVER_MODULE(mmc_spi, spibus, dwmmc_driver, dwmmc_devclass, 0, 0);
MODULE_DEPEND(mmc_spi, spibus, 1, 1, 1);
MODULE_DEPEND(mmc_spi, mmc, 1, 1, 1);
MODULE_VERSION(mmc_spi, 1);
DRIVER_MODULE(mmc, mmc_spi, mmc_driver, mmc_devclass, NULL, NULL);
