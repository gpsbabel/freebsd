#define HTIF_DEV_SHIFT      (56)
#define HTIF_CMD_SHIFT      (48)

#define HTIF_CMD_READ       (0x00UL)
#define HTIF_CMD_WRITE      (0x01UL)
#define HTIF_CMD_IDENTIFY   (0xFFUL)

#define HTIF_MAX_DEV        (256)
#define HTIF_MAX_ID         (64)

#define HTIF_ALIGN          (64)

#define HTIF_DEV_CMD(entry)	((entry >> 48) & 0xff)
#define HTIF_DEV_ID(entry)	((entry >> 56) & 0xff)
#define HTIF_DEV_DATA(entry)	(entry & 0xffffffff)

/* bus softc */
struct htif_softc {
	struct resource		*res[1];
	void			*ihl[1];
	device_t		dev;
	//struct mtx		sc_mtx;
	uint64_t		identify_id;
	uint64_t		identify_done;
};

/* device private data */
struct htif_dev_softc {
	char			*id;
	int			index;
	device_t		dev;
	struct htif_softc	*sc;
};

uint64_t htif_command(uint64_t, uint64_t);

int htif_setup_intr(int id, void *func, void *arg);
