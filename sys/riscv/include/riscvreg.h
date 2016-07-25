/*-
 * Copyright (c) 2015-2016 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Portions of this software were developed by SRI International and the
 * University of Cambridge Computer Laboratory under DARPA/AFRL contract
 * FA8750-10-C-0237 ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Portions of this software were developed by the University of Cambridge
 * Computer Laboratory as part of the CTSRD Project, with support from the
 * UK Higher Education Innovation Fund (HEIF).
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
 *
 * $FreeBSD$
 */

#ifndef _MACHINE_RISCVREG_H_
#define	_MACHINE_RISCVREG_H_

/* Machine mode requests */
#define	ECALL_MTIMECMP		0x01
#define	ECALL_CLEAR_PENDING	0x02
#define	ECALL_HTIF_CMD		0x03
#define	ECALL_HTIF_GET_ENTRY	0x04
#define	ECALL_MCPUID_GET	0x05
#define	ECALL_MIMPID_GET	0x06
#define	ECALL_SEND_IPI		0x07
#define	ECALL_CLEAR_IPI		0x08
#define	ECALL_HTIF_LOWPUTC	0x09
#define	ECALL_MIE_SET		0x0a
#define	ECALL_IO_IRQ_MASK	0x0b
#define	ECALL_HTIF_GETC		0x0c

#define	EXCP_SHIFT			0
#define	EXCP_MASK			(0xf << EXCP_SHIFT)
#define	EXCP_INSTR_ADDR_MISALIGNED	0
#define	EXCP_INSTR_ACCESS_FAULT		1
#define	EXCP_INSTR_ILLEGAL		2
#define	EXCP_INSTR_BREAKPOINT		3
#define	EXCP_LOAD_ADDR_MISALIGNED	4
#define	EXCP_LOAD_ACCESS_FAULT		5
#define	EXCP_STORE_ADDR_MISALIGNED	6
#define	EXCP_STORE_ACCESS_FAULT		7
#define	EXCP_UMODE_ENV_CALL		8
#define	EXCP_SMODE_ENV_CALL		9
#define	EXCP_HMODE_ENV_CALL		10
#define	EXCP_MMODE_ENV_CALL		11
#define	EXCP_INTR			(1ul << 63)
#define	EXCP_INTR_SOFTWARE		0
#define	EXCP_INTR_TIMER			1
#define	EXCP_INTR_HTIF			2

#define	SSTATUS_UIE			(1 << 0)
#define	SSTATUS_SIE			(1 << 1)
#define	SSTATUS_UPIE			(1 << 4)
#define	SSTATUS_SPIE			(1 << 5)
#define	SSTATUS_SPIE_SHIFT		5
#define	SSTATUS_SPP			(1 << 8)
#define	SSTATUS_SPP_SHIFT		8

#define	MSTATUS_SPP			(1 << 8)
#define	MSTATUS_SPP_SHIFT		8
#define	MSTATUS_SPIE			(1 << 5)
#define	MSTATUS_SPIE_SHIFT		5

#if 0
#define	MSTATUS_MPRV		(1 << 16)
#define	MSTATUS_PRV_SHIFT	1
#define	MSTATUS_PRV1_SHIFT	4
#define	MSTATUS_PRV2_SHIFT	7

#define	MSTATUS_PRV_MASK	(0x3 << MSTATUS_PRV_SHIFT)
#define	MSTATUS_VM_SHIFT	17
#endif

#if 0
#define MSTATUS_UIE         0x00000001
#define MSTATUS_SIE         0x00000002
#define MSTATUS_HIE         0x00000004
#define MSTATUS_MIE         0x00000008
#define MSTATUS_UPIE        0x00000010
#define MSTATUS_SPIE        0x00000020
#define MSTATUS_HPIE        0x00000040
#define MSTATUS_MPIE        0x00000080
#define MSTATUS_SPP         0x00000100
#define MSTATUS_HPP         0x00000600
#define MSTATUS_MPP         0x00001800
#define MSTATUS_FS          0x00006000
#define MSTATUS_XS          0x00018000
#define MSTATUS_MPRV        0x00020000
#define MSTATUS_PUM         0x00040000
#define MSTATUS_VM          0x1F000000
#define MSTATUS32_SD        0x80000000
#define MSTATUS64_SD        0x8000000000000000
#endif

#define	MSTATUS_UIE		(1 << 0)
#define	MSTATUS_SIE		(1 << 1)
#define	MSTATUS_HIE		(1 << 2)
#define	MSTATUS_MIE		(1 << 3)

#define	MSTATUS_SPIE		(1 << 5)
#define	MSTATUS_MPIE		(1 << 7)
#define	MSTATUS_MPIE_SHIFT	7

#define	MSTATUS_PRV_U		0	/* user */
#define	MSTATUS_PRV_S		1	/* supervisor */
#define	MSTATUS_PRV_H		2	/* hypervisor */
#define	MSTATUS_PRV_M		3	/* machine */

#define	MSTATUS_MPP_SHIFT	11

#define	MSTATUS_VM_SHIFT	24
#define	MSTATUS_VM_MASK		0x1f
#define	 MSTATUS_VM_MBARE	0
#define	 MSTATUS_VM_MBB		1
#define	 MSTATUS_VM_MBBID	2
#define	 MSTATUS_VM_SV32	8
#define	 MSTATUS_VM_SV39	9
#define	 MSTATUS_VM_SV48	10
#define	 MSTATUS_VM_SV57	11
#define	 MSTATUS_VM_SV64	12

#define	MIE_USIE	(1 << 0)
#define	MIE_SSIE	(1 << 1)
#define	MIE_HSIE	(1 << 2)
#define	MIE_MSIE	(1 << 3)
#define	MIE_UTIE	(1 << 4)
#define	MIE_STIE	(1 << 5)
#define	MIE_HTIE	(1 << 6)
#define	MIE_MTIE	(1 << 7)

#define	MIP_USIP	(1 << 0)
#define	MIP_SSIP	(1 << 1)
#define	MIP_HSIP	(1 << 2)
#define	MIP_MSIP	(1 << 3)
#define	MIP_UTIP	(1 << 4)
#define	MIP_STIP	(1 << 5)
#define	MIP_HTIP	(1 << 6)
#define	MIP_MTIP	(1 << 7)

#if 0
#define	SR_IE		(1 << 0)
#define	SR_IE1		(1 << 3)
#define	SR_IE2		(1 << 6)
#define	SR_IE3		(1 << 9)
#endif

#define	SIE_USIE	(1 << 0)
#define	SIE_SSIE	(1 << 1)
#define	SIE_UTIE	(1 << 4)
#define	SIE_STIE	(1 << 5)

#define	MIP_SEIP	(1 << 9)

/* Note: sip register has no SIP_STIP bit in Spike simulator */
#define	SIP_SSIP	(1 << 1)
#define	SIP_STIP	(1 << 5)

#define CAUSE_MISALIGNED_FETCH 0x0
#define CAUSE_FAULT_FETCH 0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT 0x3
#define CAUSE_MISALIGNED_LOAD 0x4
#define CAUSE_FAULT_LOAD 0x5
#define CAUSE_MISALIGNED_STORE 0x6
#define CAUSE_FAULT_STORE 0x7
#define CAUSE_USER_ECALL 0x8
#define CAUSE_SUPERVISOR_ECALL 0x9
#define CAUSE_HYPERVISOR_ECALL 0xa
#define CAUSE_MACHINE_ECALL 0xb


#define	NCSRS		4096
#define	CSR_IPI		0x783
#define	CSR_IO_IRQ	0x7c0	/* lowRISC only? */
#define	XLEN		8
#define	INSN_SIZE	4

#define	RISCV_INSN_NOP		0x00000013
#define	RISCV_INSN_BREAK	0x00100073
#define	RISCV_INSN_RET		0x00008067

#define	CSR_ZIMM(val)							\
	(__builtin_constant_p(val) && ((u_long)(val) < 32))

#define	csr_swap(csr, val)						\
({	if (CSR_ZIMM(val))  						\
		__asm __volatile("csrrwi %0, " #csr ", %1"		\
				: "=r" (val) : "i" (val));		\
	else 								\
		__asm __volatile("csrrw %0, " #csr ", %1"		\
				: "=r" (val) : "r" (val));		\
	val;								\
})

#define	csr_write(csr, val)						\
({	if (CSR_ZIMM(val)) 						\
		__asm __volatile("csrwi " #csr ", %0" :: "i" (val));	\
	else 								\
		__asm __volatile("csrw " #csr ", %0" ::  "r" (val));	\
})

#define	csr_set(csr, val)						\
({	if (CSR_ZIMM(val)) 						\
		__asm __volatile("csrsi " #csr ", %0" :: "i" (val));	\
	else								\
		__asm __volatile("csrs " #csr ", %0" :: "r" (val));	\
})

#define	csr_clear(csr, val)						\
({	if (CSR_ZIMM(val))						\
		__asm __volatile("csrci " #csr ", %0" :: "i" (val));	\
	else								\
		__asm __volatile("csrc " #csr ", %0" :: "r" (val));	\
})

#define	csr_read(csr)							\
({	u_long val;							\
	__asm __volatile("csrr %0, " #csr : "=r" (val));		\
	val;								\
})

#endif /* !_MACHINE_RISCVREG_H_ */
