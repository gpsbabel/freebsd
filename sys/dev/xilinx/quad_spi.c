/*-
 * Copyright (c) 2014 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
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
 * Xilinx AXI_QUAD_SPI
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
#include <sys/watchdog.h>

#include <dev/spibus/spi.h>
#include <dev/spibus/spibusvar.h>

#include "spibus_if.h"

#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#include <dev/ofw/ofw_bus.h>
#include <dev/ofw/ofw_bus_subr.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#define	READ4(_sc, _reg)	\
	bus_space_read_4(_sc->bst, _sc->bsh, _reg)
#define	WRITE4(_sc, _reg, _val)	\
	bus_space_write_4(_sc->bst, _sc->bsh, _reg, _val)
#define	READ2(_sc, _reg)	\
	bus_space_read_2(_sc->bst, _sc->bsh, _reg)
#define	WRITE2(_sc, _reg, _val)	\
	bus_space_write_2(_sc->bst, _sc->bsh, _reg, _val)
#define	READ1(_sc, _reg)	\
	bus_space_read_1(_sc->bst, _sc->bsh, _reg)
#define	WRITE1(_sc, _reg, _val)	\
	bus_space_write_1(_sc->bst, _sc->bsh, _reg, _val)

#if 0
#define	SPI_FIFO_SIZE	4

#define	SPI_MCR		0x00		/* Module Configuration */
#define	 MCR_MSTR	(1 << 31)	/* Master/Slave Mode Select */
#define	 MCR_CONT_SCKE	(1 << 30)	/* Continuous SCK Enable */
#define	 MCR_FRZ	(1 << 27)	/* Freeze */
#define	 MCR_PCSIS_S	16		/* Peripheral Chip Select */
#define	 MCR_PCSIS_M	0x3f
#define	 MCR_MDIS	(1 << 14)	/* Module Disable */
#define	 MCR_CLR_TXF	(1 << 11)	/* Clear TX FIFO */
#define	 MCR_CLR_RXF	(1 << 10)	/* Clear RX FIFO */
#define	 MCR_HALT	(1 << 0)	/* Starts and stops SPI transfers */
#define	SPI_TCR		0x08		/* Transfer Count */
#define	SPI_CTAR0	0x0C		/* Clock and Transfer Attributes */
#define	SPI_CTAR0_SLAVE	0x0C		/* Clock and Transfer Attributes */
#define	SPI_CTAR1	0x10		/* Clock and Transfer Attributes */
#define	SPI_CTAR2	0x14		/* Clock and Transfer Attributes */
#define	SPI_CTAR3	0x18		/* Clock and Transfer Attributes */
#define	 CTAR_FMSZ_M	0xf
#define	 CTAR_FMSZ_S	27		/* Frame Size */
#define	 CTAR_FMSZ_8	0x7		/* 8 bits */
#define	 CTAR_CPOL	(1 << 26)	/* Clock Polarity */
#define	 CTAR_CPHA	(1 << 25)	/* Clock Phase */
#define	 CTAR_LSBFE	(1 << 24)	/* Less significant bit first */
#define	 CTAR_PCSSCK_M	0x3
#define	 CTAR_PCSSCK_S	22		/* PCS to SCK Delay Prescaler */
#define	 CTAR_PBR_M	0x3
#define	 CTAR_PBR_S	16		/* Baud Rate Prescaler */
#define	 CTAR_PBR_7	0x3		/* Divide by 7 */
#define	 CTAR_CSSCK_M	0xf
#define	 CTAR_CSSCK_S	12		/* PCS to SCK Delay Scaler */
#define	 CTAR_BR_M	0xf
#define	 CTAR_BR_S	0		/* Baud Rate Scaler */
#define	SPI_SR		0x2C		/* Status Register */
#define	 SR_TCF		(1 << 31)	/* Transfer Complete Flag */
#define	 SR_EOQF	(1 << 28)	/* End of Queue Flag */
#define	 SR_TFFF	(1 << 25)	/* Transmit FIFO Fill Flag */
#define	 SR_RFDF	(1 << 17)	/* Receive FIFO Drain Flag */
#define	SPI_RSER	0x30		/* DMA/Interrupt Select */
#define	 RSER_EOQF_RE	(1 << 28)	/* Finished Request Enable */
#define	SPI_PUSHR	0x34		/* PUSH TX FIFO In Master Mode */
#define	 PUSHR_CONT	(1 << 31)	/* Continuous Peripheral CS */
#define	 PUSHR_EOQ	(1 << 27)	/* End Of Queue */
#define	 PUSHR_CTCNT	(1 << 26)	/* Clear Transfer Counter */
#define	 PUSHR_PCS_M	0x3f
#define	 PUSHR_PCS_S	16		/* Select PCS signals */

#define	SPI_PUSHR_SLAVE	0x34	/* PUSH TX FIFO Register In Slave Mode */
#define	SPI_POPR	0x38	/* POP RX FIFO Register */
#define	SPI_TXFR0	0x3C	/* Transmit FIFO Registers */
#define	SPI_TXFR1	0x40
#define	SPI_TXFR2	0x44
#define	SPI_TXFR3	0x48
#define	SPI_RXFR0	0x7C	/* Receive FIFO Registers */
#define	SPI_RXFR1	0x80
#define	SPI_RXFR2	0x84
#define	SPI_RXFR3	0x88

#define	SPI_GIER	0x07	/* Global interrupt enable register */
#define	SPI_ISR		0x08	/* IP interrupt status register */
#define	SPI_IER		0x0A	/* IP interrupt enable register */
#define	SPI_SRR		0x10	/* Software reset register */
#define	SPI_CR		0x18	/* SPI control register */
#define	SPI_SR		0x19	/* SPI status register */
#define	SPI_DTR		0x1A	/* SPI data transmit register */
#define	SPI_DRR		0x1B	/* SPI data receive register */
#define	SPI_SSR		0x1C	/* SPI Slave select register */
#define	SPI_TFOR	0x1D	/* Transmit FIFO occupancy register */
#define	SPI_RFROR	0x1E	/* Receive FIFO occupancy register */
#endif

#define	SPI_SRR		0x40		/* Software reset register */
#define	 SRR_RESET	0x0A
#define	SPI_CR		0x60		/* Control register */
#define	 CR_LSB_FIRST	(1 << 9)	/* LSB first */
#define	 CR_MASTER_TI	(1 << 8)	/* Master Transaction Inhibit */
#define	 CR_MSS		(1 << 7)	/* Manual Slave Select */
#define	 CR_RST_RX	(1 << 6)	/* RX FIFO Reset */
#define	 CR_RST_TX	(1 << 5)	/* TX FIFO Reset */
#define	 CR_CPHA	(1 << 4)	/* Clock phase */
#define	 CR_CPOL	(1 << 3)	/* Clock polarity */
#define	 CR_MASTER	(1 << 2)	/* Master (SPI master mode) */
#define	 CR_SPE		(1 << 1)	/* SPI system enable */
#define	 CR_LOOP	(1 << 0)	/* Local loopback mode */
#define	SPI_SR		0x64		/* Status register */
#define	 SR_TX_FULL	(1 << 3)	/* Transmit full */
#define	 SR_TX_EMPTY	(1 << 2)	/* Transmit empty */
#define	 SR_RX_FULL	(1 << 1)	/* Receive full */
#define	 SR_RX_EMPTY	(1 << 0)	/* Receive empty */
#define	SPI_DTR		0x68		/* Data transmit register */
#define	SPI_DRR		0x6C		/* Data receive register */
#define	SPI_SSR		0x70		/* Slave select register */
#define	SPI_TFOR	0x74		/* Transmit FIFO Occupancy Register */
#define	SPI_RFOR	0x78		/* Receive FIFO Occupancy Register */
#define	SPI_DGIER	0x1C		/* Device global interrupt enable register */
#define	SPI_IPISR	0x20		/* IP interrupt status register */
#define	SPI_IPIER	0x28		/* IP interrupt enable register */

struct spi_softc {
	struct resource		*res[1];
	bus_space_tag_t		bst;
	bus_space_handle_t	bsh;
	void			*ih;
};

//struct spi_softc *spi_sc;

static struct resource_spec spi_spec[] = {
	{ SYS_RES_MEMORY,	0,	RF_ACTIVE },
	{ -1, 0 }
};

static int
spi_probe(device_t dev)
{

	if (!ofw_bus_status_okay(dev))
		return (ENXIO);

	if (!ofw_bus_is_compatible(dev, "xlnx,xps-spi-3.2"))
		return (ENXIO);

	device_set_desc(dev, "Xilinx Quad SPI");
	return (BUS_PROBE_DEFAULT);
}

static int
spi_chip_select(device_t dev, device_t child)
{
	struct spi_softc *sc;
	uint32_t cs;

	sc = device_get_softc(dev);

	spibus_get_cs(child, &cs);

	WRITE4(sc, SPI_SSR, ~(1 << cs));

	return (0);
}

static int
spi_chip_deselect(device_t dev, device_t child)
{
	struct spi_softc *sc;

	sc = device_get_softc(dev);

	WRITE4(sc, SPI_SSR, 0xFFFFFFFF);

	return (0);
}

static int
spi_attach(device_t dev)
{
	struct spi_softc *sc;
	uint32_t reg;

	sc = device_get_softc(dev);
	//spi_sc = sc;

	if (bus_alloc_resources(dev, spi_spec, sc->res)) {
		device_printf(dev, "could not allocate resources\n");
		return (ENXIO);
	}

	/* Memory interface */
	sc->bst = rman_get_bustag(sc->res[0]);
	sc->bsh = rman_get_bushandle(sc->res[0]);

	printf("SPI_CR is 0x%08x\n", READ4(sc, SPI_CR));

#if 1
	WRITE4(sc, SPI_SRR, SRR_RESET);
	DELAY(10000);

	reg = (CR_MASTER | CR_MSS | CR_RST_RX | CR_RST_TX);
	WRITE4(sc, SPI_CR, reg);
	WRITE4(sc, SPI_DGIER, 0);	/* Disable interrupts */

	reg = (CR_MASTER | CR_SPE | CR_MSS);
	WRITE4(sc, SPI_CR, reg);
#endif

	device_add_child(dev, "spibus", 0);
	return (bus_generic_attach(dev));
}

static int
spi_txrx(struct spi_softc *sc, uint8_t *out_buf,
    uint8_t *in_buf, int bufsz, int cs)
{
	//uint32_t reg, wreg;
	//uint32_t txcnt;
	uint32_t i;
	uint32_t data;

	//txcnt = 0;

	for (i = 0; i < bufsz; i++) {

		//printf("write: %x\n", out_buf[i]);
		WRITE4(sc, SPI_DTR, out_buf[i]);
		while(!(READ4(sc, SPI_SR) & SR_TX_EMPTY))
			continue;

		/* Reset RX */
		//WRITE4(sc, SPI_CR, (CR_MASTER | CR_MSS | CR_SPE | CR_RST_RX));

		//printf("reading\n");
		data = READ4(sc, SPI_DRR);
		//printf("read: %x\n", data);

		in_buf[i] = (data & 0xff);
		//printf("read %x\n", in_buf[i]);
	}

	return (0);
}

static int
spi_transfer(device_t dev, device_t child, struct spi_command *cmd)
{
	struct spi_softc *sc;
	uint32_t cs;

	sc = device_get_softc(dev);

	//printf("spi_transfer\n");

	KASSERT(cmd->tx_cmd_sz == cmd->rx_cmd_sz,
	    ("%s: TX/RX command sizes should be equal", __func__));
	KASSERT(cmd->tx_data_sz == cmd->rx_data_sz,
	    ("%s: TX/RX data sizes should be equal", __func__));

	/* get the proper chip select */
	spibus_get_cs(child, &cs);

	/* Command */
	spi_txrx(sc, cmd->tx_cmd, cmd->rx_cmd, cmd->tx_cmd_sz, cs);

	/* Data */
	spi_txrx(sc, cmd->tx_data, cmd->rx_data, cmd->tx_data_sz, cs);

	return (0);
}

static device_method_t spi_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		spi_probe),
	DEVMETHOD(device_attach,	spi_attach),
	/* SPI interface */
	DEVMETHOD(spibus_transfer,	spi_transfer),
	DEVMETHOD(spibus_chip_select,	spi_chip_select),
	DEVMETHOD(spibus_chip_deselect,	spi_chip_deselect),
	{ 0, 0 }
};

static driver_t spi_driver = {
	"spi",
	spi_methods,
	sizeof(struct spi_softc),
};

static devclass_t spi_devclass;

DRIVER_MODULE(spi, simplebus, spi_driver, spi_devclass, 0, 0);
