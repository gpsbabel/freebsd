/* bus softc */
struct htif_softc {
	device_t	dev;
	struct mtx	sc_mtx;
};

/* device private data */
struct htif_dev_softc {
	char			*id;
	int			index;
	device_t		dev;
	struct htif_softc	*sc;
};

uint64_t htif_command(uint64_t, uint64_t);
