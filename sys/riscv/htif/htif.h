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
