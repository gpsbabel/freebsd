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
 *
 * $FreeBSD$
 */

#ifndef	_MACHINE_PCB_H_
#define	_MACHINE_PCB_H_

#ifndef LOCORE

struct trapframe;

#define	PCB_RA		1
#define	PCB_SP		2
#define	PCB_GP		3
#define	PCB_TP		4

struct pcb {
	uint64_t	pcb_x[32];
	uint64_t	pcb_sepc;
	vm_offset_t	pcb_l1addr;

	/* Fault handler, the error value is passed in a0 */
	vm_offset_t	pcb_onfault;

	/* Place last to simplify the asm to access the rest if the struct */
	__uint128_t	pcb_vfp[32];
	uint32_t	pcb_fpcr;
	uint32_t	pcb_fpsr;
	u_int		pcb_fpflags;
#define	PCB_FP_STARTED	0x01
	u_int		pcb_vfpcpu;	/* Last cpu this thread ran VFP code */
};

#ifdef _KERNEL
void	makectx(struct trapframe *tf, struct pcb *pcb);
int	savectx(struct pcb *pcb) __returns_twice;
#endif

#endif /* !LOCORE */

#endif /* !_MACHINE_PCB_H_ */
