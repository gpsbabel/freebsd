struct htif_blk_request {
	uint64_t addr;
	uint64_t offset;	/* offset in bytes */
	uint64_t size;		/* length in bytes */
	uint64_t tag;
} __packed;

#define	SECTOR_SIZE_SHIFT	9
#define	SECTOR_SIZE		(1 << SECTOR_SIZE_SHIFT)

void htif_blk_intr(uint64_t entry);
