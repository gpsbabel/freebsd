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
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/selinfo.h>
#include <sys/module.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/uio.h>

#include <sys/bio.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <geom/geom_disk.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/md_var.h>
#include <machine/bus.h>
#include <machine/htif.h>
#include <machine/trap.h>
#include <sys/rman.h>

#include "htif.h"

#define	SECTOR_SIZE_SHIFT	9
#define	SECTOR_SIZE		(1 << SECTOR_SIZE_SHIFT)

#define HTIF_BLK_LOCK(_sc)	mtx_lock(&(_sc)->sc_mtx)
#define	HTIF_BLK_UNLOCK(_sc)	mtx_unlock(&(_sc)->sc_mtx)
#define HTIF_BLK_LOCK_INIT(_sc) \
	mtx_init(&_sc->sc_mtx, device_get_nameunit(_sc->dev), \
	    "htif_blk", MTX_DEF)
#define HTIF_BLK_LOCK_DESTROY(_sc)	mtx_destroy(&_sc->sc_mtx);
#define HTIF_BLK_ASSERT_LOCKED(_sc)	mtx_assert(&_sc->sc_mtx, MA_OWNED);
#define HTIF_BLK_ASSERT_UNLOCKED(_sc)	mtx_assert(&_sc->sc_mtx, MA_NOTOWNED);

static void htif_blk_task(void *arg);

static disk_open_t	htif_blk_open;
static disk_close_t	htif_blk_close;
static disk_strategy_t	htif_blk_strategy;
static dumper_t		htif_blk_dump;

struct htif_blk_softc {
	device_t	dev;
	struct disk	*disk;
	struct htif_dev_softc *sc_dev;
	struct mtx	htif_io_mtx;
	struct mtx	sc_mtx;
	struct proc	*p;
	struct bio_queue_head bio_queue;
	int		running;
	int		intr_chan;
	int		cmd_done;
	int		curtag;
	struct bio	*bp;
};

struct htif_blk_request {
	uint64_t addr;
	uint64_t offset;	/* offset in bytes */
	uint64_t size;		/* length in bytes */
	uint64_t tag;
} __packed;

static void
htif_blk_intr(void *arg, uint64_t entry)
{
	struct htif_blk_softc *sc;
	uint64_t devcmd;
	uint64_t data;

	//sc = htif_blk_sc;
	sc = arg;

	devcmd = ((entry >> 48) & 0xff);
	if (devcmd == 0xff)
		return;

	data = (entry & 0xffff);

	//if (sc->cmd_done == 1)
	//	printf("htif_blk_intr: entry 0x%016lx\n", entry);

	if (sc->curtag == data) {
		sc->cmd_done = 1;
		wakeup(&sc->intr_chan);
		//printf(".");
		//biodone(sc->bp);
	}
}

static int
htif_blk_probe(device_t dev)
{

	return (0);
}

static int
htif_blk_attach(device_t dev)
{
	struct htif_blk_softc *sc;
	struct htif_dev_softc *sc_dev;
	long size;
	sc = device_get_softc(dev);
	sc->dev = dev;
	sc_dev = device_get_ivars(dev);
	sc->sc_dev = sc_dev;
	mtx_init(&sc->htif_io_mtx, device_get_nameunit(dev), "htif_blk", MTX_DEF);
	HTIF_BLK_LOCK_INIT(sc);

	//htif_blk_sc = sc;

	char *str;
	char prefix[] = " size=";

	str = strstr(sc_dev->id, prefix);

	size = strtol((str + 6), NULL, 10);
	if (size == 0) {
		return (ENXIO);
	}

	printf("htif blk attach, size %ld\n", size);

	htif_setup_intr(sc_dev->index, htif_blk_intr, sc);

	//printf("disabled\n");
	//return (0);

#if 0
	struct htif_ld_info *ld_info;
	struct htif_blk_pending *ld_pend;
	uint64_t sectors;
	uint32_t secsize;
	char *state;

	ld_info = device_get_ivars(dev);

	sc->ld_dev = dev;
	sc->ld_id = ld_info->ld_config.properties.ld.v.target_id;
	sc->ld_unit = device_get_unit(dev);
	sc->ld_info = ld_info;
	sc->ld_controller = device_get_softc(device_get_parent(dev));
	sc->ld_flags = 0;

	sectors = ld_info->size;
	secsize = MFI_SECTOR_LEN;
	mtx_lock(&sc->ld_controller->htif_io_mtx);
	TAILQ_INSERT_TAIL(&sc->ld_controller->htif_ld_tqh, sc, ld_link);
	TAILQ_FOREACH(ld_pend, &sc->ld_controller->htif_ld_pend_tqh,
	    ld_link) {
		TAILQ_REMOVE(&sc->ld_controller->htif_ld_pend_tqh,
		    ld_pend, ld_link);
		free(ld_pend, M_MFIBUF);
		break;
	}
	mtx_unlock(&sc->ld_controller->htif_io_mtx);

	switch (ld_info->ld_config.params.state) {
	case MFI_LD_STATE_OFFLINE:
		state = "offline";
		break;
	case MFI_LD_STATE_PARTIALLY_DEGRADED:
		state = "partially degraded";
		break;
	case MFI_LD_STATE_DEGRADED:
		state = "degraded";
		break;
	case MFI_LD_STATE_OPTIMAL:
		state = "optimal";
		break;
	default:
		state = "unknown";
		break;
	}

	if ( strlen(ld_info->ld_config.properties.name) == 0 ) {
		device_printf(dev,
		      "%juMB (%ju sectors) RAID volume (no label) is %s\n",
		       sectors / (1024 * 1024 / secsize), sectors, state);
	} else {
		device_printf(dev,
		      "%juMB (%ju sectors) RAID volume '%s' is %s\n",
		      sectors / (1024 * 1024 / secsize), sectors,
		      ld_info->ld_config.properties.name, state);
	}

	sc->ld_disk = disk_alloc();
	sc->ld_disk->d_drv1 = sc;
	sc->ld_disk->d_maxsize = min(sc->ld_controller->htif_max_io * secsize,
	    (sc->ld_controller->htif_max_sge - 1) * PAGE_SIZE);
	sc->ld_disk->d_name = "htifd";
	sc->ld_disk->d_open = htif_blk_open;
	sc->ld_disk->d_close = htif_blk_close;
	sc->ld_disk->d_strategy = htif_blk_strategy;
	sc->ld_disk->d_dump = htif_blk_dump;
	sc->ld_disk->d_unit = sc->ld_unit;
	sc->ld_disk->d_sectorsize = secsize;
	sc->ld_disk->d_mediasize = sectors * secsize;
	if (sc->ld_disk->d_mediasize >= (1 * 1024 * 1024)) {
		sc->ld_disk->d_fwheads = 255;
		sc->ld_disk->d_fwsectors = 63;
	} else {
		sc->ld_disk->d_fwheads = 64;
		sc->ld_disk->d_fwsectors = 32;
	}
	sc->ld_disk->d_flags = DISKFLAG_UNMAPPED_BIO;
	disk_create(sc->ld_disk, DISK_VERSION);
#endif

	sc->disk = disk_alloc();
	sc->disk->d_drv1 = sc;

	//sc->ld_disk->d_maxsize = min(sc->ld_controller->htif_max_io * secsize,
	//    (sc->ld_controller->htif_max_sge - 1) * PAGE_SIZE);

	sc->disk->d_maxsize = 4096; /* Max transfer */
	sc->disk->d_name = "htif_blk";
	sc->disk->d_open = htif_blk_open;
	sc->disk->d_close = htif_blk_close;
	sc->disk->d_strategy = htif_blk_strategy;
	sc->disk->d_dump = htif_blk_dump;
	sc->disk->d_unit = 0;
	sc->disk->d_sectorsize = SECTOR_SIZE;
	sc->disk->d_mediasize = size;
	disk_create(sc->disk, DISK_VERSION);

#if 0
	sc->disk->d_unit = sc->ld_unit;
	sc->disk->d_sectorsize = secsize;
	sc->disk->d_mediasize = sectors * secsize;
	if (sc->ld_disk->d_mediasize >= (1 * 1024 * 1024)) {
		sc->ld_disk->d_fwheads = 255;
		sc->ld_disk->d_fwsectors = 63;
	} else {
		sc->ld_disk->d_fwheads = 64;
		sc->ld_disk->d_fwsectors = 32;
	}
	sc->ld_disk->d_flags = DISKFLAG_UNMAPPED_BIO;

	disk_create(sc->ld_disk, DISK_VERSION);
#endif

	bioq_init(&sc->bio_queue);

	sc->running = 1;

	kproc_create(&htif_blk_task, sc, &sc->p, 0, 0, "%s: transfer", 
	    device_get_nameunit(dev));

	return (0);
}

static int
htif_blk_detach(device_t dev)
{

	printf("%s\n", __func__);

#if 0
	struct htif_blk *sc;

	sc = device_get_softc(dev);

	mtx_lock(&sc->ld_controller->htif_io_lock);
	if (((sc->ld_disk->d_flags & DISKFLAG_OPEN) ||
	    (sc->ld_flags & MFI_DISK_FLAGS_OPEN)) &&
	    (sc->ld_controller->htif_keep_deleted_volumes ||
	    sc->ld_controller->htif_detaching)) {
		mtx_unlock(&sc->ld_controller->htif_io_lock);
		return (EBUSY);
	}
	mtx_unlock(&sc->ld_controller->htif_io_lock);

	disk_destroy(sc->ld_disk);
	mtx_lock(&sc->ld_controller->htif_io_lock);
	TAILQ_REMOVE(&sc->ld_controller->htif_ld_tqh, sc, ld_link);
	mtx_unlock(&sc->ld_controller->htif_io_lock);
	free(sc->ld_info, M_MFIBUF);
#endif
	return (0);
}

static int
htif_blk_open(struct disk *dp)
{
	int error;

	//printf("%s\n", __func__);

	error = 0;
#if 0
	struct htif_blk *sc;

	sc = dp->d_drv1;
	mtx_lock(&sc->ld_controller->htif_io_lock);
	if (sc->ld_flags & MFI_DISK_FLAGS_DISABLED)
		error = ENXIO;
	else {
		sc->ld_flags |= MFI_DISK_FLAGS_OPEN;
		error = 0;
	}
	mtx_unlock(&sc->ld_controller->htif_io_lock);
#endif

	return (error);
}

static int
htif_blk_close(struct disk *dp)
{
#if 0
	struct htif_blk *sc;

	sc = dp->d_drv1;
	mtx_lock(&sc->ld_controller->htif_io_lock);
	sc->ld_flags &= ~MFI_DISK_FLAGS_OPEN;
	mtx_unlock(&sc->ld_controller->htif_io_lock);

#endif
	return (0);
}

#if 0
int
htif_blk_disable(struct htif_blk_softc *sc)
{
	mtx_assert(&sc->ld_controller->htif_io_lock, MA_OWNED);
	if (sc->ld_flags & MFI_DISK_FLAGS_OPEN) {
		if (sc->ld_controller->htif_delete_busy_volumes)
			return (0);
		device_printf(sc->ld_dev, "Unable to delete busy ld device\n");
		return (EBUSY);
	}
	sc->ld_flags |= MFI_DISK_FLAGS_DISABLED;
	return (0);
}
#endif

#if 0
void
htif_blk_enable(struct htif_blk_softc *sc)
{

	mtx_assert(&sc->ld_controller->htif_io_lock, MA_OWNED);
	sc->ld_flags &= ~MFI_DISK_FLAGS_DISABLED;
}
#endif

static void
htif_blk_task(void *arg)
{
	struct htif_blk_softc *sc;
	struct bio *bp;
	device_t dev;
	struct htif_dev_softc *sc_dev;
	int block;
	int bcount;
	uint64_t cmd;
	uint64_t paddr;
	struct htif_blk_request req __aligned(HTIF_ALIGN);

	sc = (struct htif_blk_softc *)arg;
	dev = sc->dev;
	sc_dev = sc->sc_dev;

	while (1) {
		HTIF_BLK_LOCK(sc);
		do {
			//if (sc->running == 0)
			//	goto out;
			bp = bioq_takefirst(&sc->bio_queue);
			if (bp == NULL)
				msleep(sc, &sc->sc_mtx, PRIBIO, "jobqueue", 0);
		} while (bp == NULL);
		HTIF_BLK_UNLOCK(sc);

		//mtx_lock(&sc->htif_io_mtx);
		rmb();

		if (bp->bio_cmd == BIO_READ || bp->bio_cmd == BIO_WRITE) {
			block = bp->bio_pblkno;
			bcount = bp->bio_bcount;
			//printf(".");
			//printf("%d block %d count %d\n", bp->bio_cmd, block, bcount);

			rmb();
			req.offset = (bp->bio_pblkno * sc->disk->d_sectorsize);
			req.size = bp->bio_bcount;
			paddr = vtophys(bp->bio_data);
			KASSERT(paddr != 0, ("paddr is 0"));
			req.addr = paddr;

			if (sc->curtag++ >= 65535)
				sc->curtag = 0;
			req.tag = sc->curtag;

			//printf("index %d addr 0x%016lx\n", sc_dev->index, req.addr);
			cmd = sc_dev->index;
			cmd <<= 56;
			if (bp->bio_cmd == BIO_READ)
				cmd |= (HTIF_CMD_READ << 48);
			else
				cmd |= (HTIF_CMD_WRITE << 48);
			paddr = vtophys(&req);
			KASSERT(paddr != 0, ("paddr is 0"));
			cmd |= paddr;

			sc->cmd_done = 0;
			htif_command(cmd, ECALL_HTIF_CMD);

			/* Wait for interrupt */
			HTIF_BLK_LOCK(sc);
			int i = 0;
			while (sc->cmd_done == 0) {
				msleep(&sc->intr_chan, &sc->sc_mtx, PRIBIO, "intr", hz/2);
				i+=1;
				if ( i > 2 ) {
					printf("Err %d\n", i);

					bp->bio_error = EIO;
					//bp->bio_resid = (end - block) * sz;
					bp->bio_flags |= BIO_ERROR;

					break;
				}
			}
			HTIF_BLK_UNLOCK(sc);

			//printf(".");

			biodone(bp);
			//biofinish(bp, NULL, ENXIO);
		} else {
			printf("unknown op %d\n", bp->bio_cmd);
		}

		//mtx_unlock(&sc->htif_io_mtx);
	}
}

static void
htif_blk_strategy(struct bio *bp)
{
	struct htif_blk_softc *sc;

	sc = bp->bio_disk->d_drv1;

	HTIF_BLK_LOCK(sc);
	if (sc->running > 0) {
		bioq_disksort(&sc->bio_queue, bp);
		HTIF_BLK_UNLOCK(sc);
		wakeup(sc);
	} else {
		HTIF_BLK_UNLOCK(sc);
		biofinish(bp, NULL, ENXIO);
	}

	//printf("%s\n", __func__);
	//printf("%s done\n", __func__);
#if 0
	struct htif_blk *sc;
	struct htif_softc *controller;

	sc = bio->bio_disk->d_drv1;

	if (sc == NULL) {
		bio->bio_error = EINVAL;
		bio->bio_flags |= BIO_ERROR;
		bio->bio_resid = bio->bio_bcount;
		biodone(bio);
		return;
	}

	controller = sc->ld_controller;
	if (controller->adpreset) {
		bio->bio_error = EBUSY;
		return;
	}

	if (controller->hw_crit_error) {
		bio->bio_error = EBUSY;
		return;
	}

	if (controller->issuepend_done == 0) {
		bio->bio_error = EBUSY;
		return;
	}

	bio->bio_driver1 = (void *)(uintptr_t)sc->ld_id;
	/* Mark it as LD IO */
	bio->bio_driver2 = (void *)MFI_LD_IO;
	mtx_lock(&controller->htif_io_lock);
	htif_enqueue_bio(controller, bio);
	htif_startio(controller);
	mtx_unlock(&controller->htif_io_lock);
#endif
	return;
}

#if 0
void
htif_blk_complete(struct bio *bio)
{
	struct htif_blk_softc *sc;
	struct htif_frame_header *hdr;

	sc = bio->bio_disk->d_drv1;
	hdr = bio->bio_driver1;

	if (bio->bio_flags & BIO_ERROR) {
		bio->bio_resid = bio->bio_bcount;
		if (bio->bio_error == 0)
			bio->bio_error = EIO;
		disk_err(bio, "hard error", -1, 1);
	} else {
		bio->bio_resid = 0;
	}
	biodone(bio);
}
#endif

static int
htif_blk_dump(void *arg, void *virt, vm_offset_t phys, off_t offset, size_t len)
{
	printf("%s\n", __func__);
#if 0
	struct htif_blk_softc *sc;
	struct htif_softc *parent_sc;
	struct disk *dp;
	int error;

	dp = arg;
	sc = dp->d_drv1;
	parent_sc = sc->ld_controller;

	if (len > 0) {
		if ((error = htif_dump_blocks(parent_sc, sc->ld_id, offset /
		    MFI_SECTOR_LEN, virt, len)) != 0)
			return (error);
	} else {
		/* htif_sync_cache(parent_sc, sc->ld_id); */
	}
#endif

	return (0);
}

static device_method_t htif_blk_methods[] = {
	DEVMETHOD(device_probe,		htif_blk_probe),
	DEVMETHOD(device_attach,	htif_blk_attach),
	DEVMETHOD(device_detach,	htif_blk_detach),
	{ 0, 0 }
};

static driver_t htif_blk_driver = {
	"htif_blk",
	htif_blk_methods,
	sizeof(struct htif_blk_softc)
};

static devclass_t	htif_blk_devclass;

DRIVER_MODULE(htif_blk, htif, htif_blk_driver, htif_blk_devclass, 0, 0);
