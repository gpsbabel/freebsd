/* $FreeBSD$ */

#define	ECALL_LOW_PRINTC	1
#define	ECALL_MTIMECMP		2
#define	ECALL_MTIMECMP2		3
#define	ECALL_CLEAR_PENDING	4
#define	ECALL_HTIF_CMD		5
#define	ECALL_HTIF_GET_ENTRY	6
#define	ECALL_LOW_GETC		7

#define	MSTATUS_MPRV		(1 << 16)
#define	MSTATUS_PRV_SHIFT	1
#define	MSTATUS_PRV1_SHIFT	4
#define	MSTATUS_PRV2_SHIFT	7
#define	MSTATUS_PRV_MASK	(0x3 << MSTATUS_PRV_SHIFT)
#define	MSTATUS_PRV_U		0
#define	MSTATUS_PRV_S		1
#define	MSTATUS_PRV_H		2
#define	MSTATUS_PRV_M		3

#define	MSTATUS_VM_SHIFT	17
#define	MSTATUS_VM_MASK		0x1f
#define	MSTATUS_VM_MBARE	0
#define	MSTATUS_VM_MBB		1
#define	MSTATUS_VM_MBBID	2
#define	MSTATUS_VM_SV32		8
#define	MSTATUS_VM_SV39		9
#define	MSTATUS_VM_SV48		10

#define	MIE_SSIE	(1 << 1)
#define	MIE_HSIE	(1 << 2)
#define	MIE_MSIE	(1 << 3)

#define	MIE_STIE	(1 << 5)
#define	MIE_HTIE	(1 << 6)
#define	MIE_MTIE	(1 << 7)

#define	MIP_SSIP	(1 << 1)
#define	MIP_HSIP	(1 << 2)
#define	MIP_MSIP	(1 << 3)

#define	MIP_STIP	(1 << 5)
#define	MIP_HTIP	(1 << 6)
#define	MIP_MTIP	(1 << 7)

#define	SR_IE		(1 << 0)
#define	SR_IE1		(1 << 3)
#define	SR_IE2		(1 << 6)
#define	SR_IE3		(1 << 9)
