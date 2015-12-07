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
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/pioctl.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#ifdef KDB
#include <sys/kdb.h>
#endif

#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_param.h>
#include <vm/vm_extern.h>

#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/pcpu.h>
#include <machine/vmparam.h>

#include <machine/resource.h>
#include <machine/intr.h>

#ifdef KDTRACE_HOOKS
#include <sys/dtrace_bsd.h>
#endif

#ifdef VFP
#include <machine/vfp.h>
#endif

#ifdef KDB
#include <machine/db_machdep.h>
#endif

#ifdef DDB
#include <ddb/db_output.h>
#endif

extern register_t fsu_intr_fault;

/* Called from exception.S */
void do_trap(struct trapframe *);
void do_trap_user(struct trapframe *);
void do_el0_sync(struct trapframe *);
void do_el0_error(struct trapframe *);

int (*dtrace_invop_jump_addr)(struct trapframe *);

//extern uint64_t console_data;

static __inline void
call_trapsignal(struct thread *td, int sig, int code, void *addr)
{
	ksiginfo_t ksi;

	//panic("%s: implement me\n", __func__);

	ksiginfo_init_trap(&ksi);
	ksi.ksi_signo = sig;
	ksi.ksi_code = code;
	ksi.ksi_addr = addr;
	trapsignal(td, &ksi);
}

int
cpu_fetch_syscall_args(struct thread *td, struct syscall_args *sa)
{
	struct proc *p;
	register_t *ap;
	int nap;

	nap = 8;
	p = td->td_proc;
	ap = &td->td_frame->tf_x[10];

	//sa->code = td->td_frame->tf_x[8];
	sa->code = td->td_frame->tf_x[5];

	if (sa->code == SYS_syscall || sa->code == SYS___syscall) {
		sa->code = *ap++;
		nap--;

		//printf("syscall 0(%d)\n", sa->code);
	} else {
		//printf("syscall %d\n", sa->code);
	}

	if (p->p_sysent->sv_mask)
		sa->code &= p->p_sysent->sv_mask;
	if (sa->code >= p->p_sysent->sv_size)
		sa->callp = &p->p_sysent->sv_table[0];
	else
		sa->callp = &p->p_sysent->sv_table[sa->code];

	sa->narg = sa->callp->sy_narg;
	//printf("sa->narg %d\n", sa->narg);
	memcpy(sa->args, ap, nap * sizeof(register_t));
	if (sa->narg > nap)
		panic("TODO: Could we have more then 8 args?");

	td->td_retval[0] = 0;
	td->td_retval[1] = 0;

	return (0);
}

#include "../../kern/subr_syscall.c"

static void
svc_handler(struct trapframe *frame)
{
	struct syscall_args sa;
	struct thread *td;
	int error;

	td = curthread;
	td->td_frame = frame;

	error = syscallenter(td, &sa);
	syscallret(td, error, &sa);
}

static void
data_abort(struct trapframe *frame, uint64_t esr, int lower)
{
	struct vm_map *map;
	struct thread *td;
	struct proc *p;
	struct pcb *pcb;
	vm_prot_t ftype;
	vm_offset_t va;
	uint64_t far;
	int error, sig, ucode;
	int i;

	td = curthread;
	pcb = td->td_pcb;

	//printf("data_abort: sbadaddr 0x%016lx\n", frame->tf_sbadaddr);
	//while (1);

	/*
	 * Special case for fuswintr and suswintr. These can't sleep so
	 * handle them early on in the trap handler.
	 */
	if (__predict_false(pcb->pcb_onfault == (vm_offset_t)&fsu_intr_fault)) {
		frame->tf_sepc = pcb->pcb_onfault;
		//frame->tf_elr = pcb->pcb_onfault;
		return;
	}

	//far = READ_SPECIALREG(far_el1);
	far = frame->tf_sbadaddr;

	p = td->td_proc;

	if (lower)
		map = &td->td_proc->p_vmspace->vm_map;
	else {
		/* The top bit tells us which range to use */
		if ((far >> 63) == 1)
			map = kernel_map;
		else
			map = &td->td_proc->p_vmspace->vm_map;
	}

	va = trunc_page(far);

	//if (va == 0)
	//	panic("Va == 0\n");
	//ftype = ((esr >> 6) & 1) ? VM_PROT_READ | VM_PROT_WRITE : VM_PROT_READ;
	//ftype = frame->tf_scause == EXCP_LOAD_ACCESS_FAULT ? VM_PROT_READ : (VM_PROT_READ | VM_PROT_WRITE);

	if (frame->tf_scause == EXCP_STORE_ACCESS_FAULT) {
		ftype = (VM_PROT_READ | VM_PROT_WRITE);
	} else {
		ftype = (VM_PROT_READ);
	}

	if (map != kernel_map) {
		/*
		 * Keep swapout from messing with us during this
		 *	critical time.
		 */
		PROC_LOCK(p);
		++p->p_lock;
		PROC_UNLOCK(p);

		/* Fault in the user page: */
		error = vm_fault(map, va, ftype, VM_FAULT_NORMAL);

		PROC_LOCK(p);
		--p->p_lock;
		PROC_UNLOCK(p);
	} else {
		/*
		 * Don't have to worry about process locking or stacks in the
		 * kernel.
		 */
		//printf("map is kernel one: 0x%016lx\n", map);
		error = vm_fault(map, va, ftype, VM_FAULT_NORMAL);
	}

	if (error != KERN_SUCCESS) {
		if (lower) {
			sig = SIGSEGV;
			if (error == KERN_PROTECTION_FAILURE)
				ucode = SEGV_ACCERR;
			else
				ucode = SEGV_MAPERR;
			call_trapsignal(td, sig, ucode, (void *)far);
		} else {
			if (td->td_intr_nesting_level == 0 &&
			    pcb->pcb_onfault != 0) {
				frame->tf_x[10] = error;
				frame->tf_sepc = pcb->pcb_onfault;
				//frame->tf_x[0] = error;
				//frame->tf_elr = pcb->pcb_onfault;
				return;
			}
			for (i = 0; i < 32; i++) {
				printf("x[%d] == 0x%016lx\n", i, frame->tf_x[i]);
			}

			panic("vm_fault failed: %lx, va 0x%016lx",
				frame->tf_sepc, far);
			//panic("vm_fault failed: %lx", frame->tf_elr);
		}
	}

	if (lower)
		userret(td, frame);
}

void
do_trap(struct trapframe *frame)
{
	uint64_t exception;
	struct thread *td;
	uint64_t esr;
	int i;

	td = curthread;

	/* Read the esr register to get the exception details */
	esr = 0;//READ_SPECIALREG(esr_el1);

	exception = (frame->tf_scause & 0xf);
	if (frame->tf_scause & (1 << 31)) {
		/* Interrupt */
		riscv_cpu_intr(frame);
		return;
	}

	//printf("trap: scause 0x%016lx sbadaddr 0x%016lx sepc 0x%016lx sstatus 0x%016lx\n",
	//		exception, frame->tf_sbadaddr, frame->tf_sepc, frame->tf_sstatus);
	for (i = 0; i < 32; i++) {
		//printf("tf_x[%d] == 0x%016lx\n", i, frame->tf_x[i]);
	}

#ifdef KDTRACE_HOOKS
	if (dtrace_trap_func != NULL && (*dtrace_trap_func)(frame, exception))
		return;
#endif

	/*
	 * Sanity check we are in an exception er can handle. The IL bit
	 * is used to indicate the instruction length, except in a few
	 * exceptions described in the ARMv8 ARM.
	 *
	 * It is unclear in some cases if the bit is implementation defined.
	 * The Foundation Model and QEMU disagree on if the IL bit should
	 * be set when we are in a data fault from the same EL and the ISV
	 * bit (bit 24) is also set.
	 */
	//KASSERT((esr & ESR_ELx_IL) == ESR_ELx_IL ||
	//    (exception == EXCP_DATA_ABORT && ((esr & ISS_DATA_ISV) == 0)),
	//    ("Invalid instruction length in exception"));

	//CTR4(KTR_TRAP,
	//    "do_el1_sync: curthread: %p, esr %lx, elr: %lx, frame: %p",
	//    curthread, esr, frame->tf_elr, frame);

	switch(exception) {
	case EXCP_LOAD_ACCESS_FAULT:
	case EXCP_STORE_ACCESS_FAULT:
	case EXCP_INSTR_ACCESS_FAULT:
		data_abort(frame, esr, 0);
		break;
	//case EXCP_FP_SIMD:
	//case EXCP_TRAP_FP:
	//	panic("VFP exception in the kernel");
	//case EXCP_DATA_ABORT:
	//	data_abort(frame, esr, 0);
	//	break;
	//case EXCP_ENV_CALL:
	//	frame->tf_sepc += 4;
	//	svc_handler(frame);
	//	break;
	//case EXCP_BRK:
#ifdef KDTRACE_HOOKS
#endif
	//case EXCP_WATCHPT_EL1:
	//case EXCP_SOFTSTP_EL1:
#ifdef KDB
	//	kdb_trap(exception, 0, frame);
#else
	//	panic("No debugger in kernel.\n");
#endif
	//	break;
	default:
		for (i = 0; i < 32; i++) {
			printf("x[%d] == 0x%016lx\n", i, frame->tf_x[i]);
		}

		printf("sepc == 0x%016lx\n", frame->tf_sepc);
		printf("sstatus == 0x%016lx\n", frame->tf_sstatus);

		panic("Unknown kernel exception %x esr_el1 %lx\n", exception,
		    esr);
	}

	//printf("trap finished\n");
}

void
do_trap_user(struct trapframe *frame)
{
	uint64_t exception;
	uint64_t esr;
	int i;

	/* Check we have a sane environment when entering from userland */
	//KASSERT((uintptr_t)get_pcpu() >= VM_MIN_KERNEL_ADDRESS,
	 //   ("Invalid pcpu address from userland: %p (tpidr %lx)",
	 //    get_pcpu(), READ_SPECIALREG(tpidr_el1)));

	esr = 0;//READ_SPECIALREG(esr_el1);
	exception = 0;//ESR_ELx_EXCEPTION(esr);

	exception = (frame->tf_scause & 0xf);
	if (frame->tf_scause & (1 << 31)) {
		/* Interrupt */
		riscv_cpu_intr(frame);
		return;
	}

	CTR4(KTR_TRAP,
	    "do_el0_sync: curthread: %p, esr %lx, elr: %lx, frame: %p",
	    curthread, esr, frame->tf_elr, frame);

	switch(exception) {
	case EXCP_LOAD_ACCESS_FAULT:
	case EXCP_STORE_ACCESS_FAULT:
	case EXCP_INSTR_ACCESS_FAULT:
		data_abort(frame, esr, 1);
		break;
	case EXCP_ENV_CALL:
		frame->tf_sepc += 4;
		svc_handler(frame);
		break;
	//case EXCP_FP_SIMD:
	//case EXCP_TRAP_FP:
#ifdef VFP
	//	vfp_restore_state();
#else
	//	panic("VFP exception in userland");
#endif
	//	break;
	//case EXCP_SVC:
	//	svc_handler(frame);
	//	break;
	//case EXCP_INSN_ABORT_L:
	//case EXCP_DATA_ABORT_L:
	//	data_abort(frame, esr, 1);
	//	break;
	default:
		for (i = 0; i < 32; i++) {
			printf("x[%d] == 0x%016lx\n", i, frame->tf_x[i]);
		}
		printf("tf_sepc 0x%016lx\n", frame->tf_sepc);

		panic("Unknown userland exception %x badaddr %lx\n",
			exception, frame->tf_sbadaddr);
	}

	//if ((frame->tf_sstatus & (1 << 4)) == 0) {
	//	/* trap originated from user mode */
	//	userret(td, frame);
	//}
}

void
do_el0_error(struct trapframe *frame)
{

	panic("do_el0_error");
}

