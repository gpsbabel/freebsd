/*-
 * Copyright (c) 2014 Andrew Turner
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
 *
 */

#include "opt_platform.h"

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/cons.h>
#include <sys/cpu.h>
#include <sys/efi.h>
#include <sys/exec.h>
#include <sys/imgact.h>
#include <sys/kdb.h> 
#include <sys/kernel.h>
#include <sys/limits.h>
#include <sys/linker.h>
#include <sys/msgbuf.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/signalvar.h>
#include <sys/syscallsubr.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/ucontext.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_pager.h>

#include <machine/armreg.h>
#include <machine/cpu.h>
#include <machine/debug_monitor.h>
#include <machine/kdb.h>
#include <machine/devmap.h>
#include <machine/machdep.h>
#include <machine/metadata.h>
#include <machine/pcb.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

#include <machine/asm.h>
#include <machine/htif.h>

#ifdef VFP
#include <machine/vfp.h>
#endif

#ifdef FDT
#include <dev/fdt/fdt_common.h>
#include <dev/ofw/openfirm.h>
#endif

struct pcpu __pcpu[MAXCPU];

static struct trapframe proc0_tf;

vm_paddr_t phys_avail[PHYS_AVAIL_SIZE + 2];
vm_paddr_t dump_avail[PHYS_AVAIL_SIZE + 2];

int early_boot = 1;
int cold = 1;
long realmem = 0;
long Maxmem = 0;

#define	PHYSMAP_SIZE	(2 * (VM_PHYSSEG_MAX - 1))
vm_paddr_t physmap[PHYSMAP_SIZE];
u_int physmap_idx;

struct kva_md_info kmi;

int64_t dcache_line_size;	/* The minimum D cache line size */
int64_t icache_line_size;	/* The minimum I cache line size */
int64_t idcache_line_size;	/* The minimum cache line size */

extern int *end;
extern int *initstack_end;

struct pcpu *pcpup;

/*
void __attribute__((noreturn)) bad_trap(void);
void __attribute__((noreturn)) bad_trap(void)
{
	while (1);
}
*/

uintptr_t mcall_trap(uintptr_t mcause, uintptr_t* regs);

uintptr_t
mcall_trap(uintptr_t mcause, uintptr_t* regs)
{

	return (0);
}

#ifdef	EARLY_PRINTF
static void 
spike_early_putc(int c)
{

	//__asm __volatile("ecall");

	__asm __volatile(
		"mv	t5, %0\n"
		"ecall" :: "r"(ECALL_LOW_PRINTC)
	);

	//__asm __volatile(
	//	"mv	t5, %1\n"
	//	"mv	t6, %0\n"
	//	"ecall" :: "r"(c), "r"(ECALL_LOW_PRINTC)
	//);
}
early_putc_t *early_putc = spike_early_putc;
#endif

static void
cpu_startup(void *dummy)
{

	identify_cpu();

	vm_ksubmap_init(&kmi);
	bufinit();
	vm_pager_bufferinit();
}

SYSINIT(cpu, SI_SUB_CPU, SI_ORDER_FIRST, cpu_startup, NULL);

int
cpu_idle_wakeup(int cpu)
{

	return (0);
}

void
bzero(void *buf, size_t len)
{
	uint8_t *p;

	p = buf;
	while(len-- > 0)
		*p++ = 0;
}

int
fill_regs(struct thread *td, struct reg *regs)
{
	struct trapframe *frame;

	frame = td->td_frame;
	regs->sepc = frame->tf_sepc;
	regs->sstatus = frame->tf_sstatus;

	//regs->sp = frame->tf_sp;
	//regs->lr = frame->tf_lr;
	//regs->elr = frame->tf_elr;
	//regs->spsr = frame->tf_spsr;

	memcpy(regs->x, frame->tf_x, sizeof(regs->x));

	return (0);
}

int
set_regs(struct thread *td, struct reg *regs)
{
	//struct trapframe *frame;

	panic("%s", __func__);

	//frame = td->td_frame;
	//frame->tf_sp = regs->sp;
	//frame->tf_lr = regs->lr;
	//frame->tf_elr = regs->elr;
	//frame->tf_spsr = regs->spsr;

	//memcpy(frame->tf_x, regs->x, sizeof(frame->tf_x));

	return (0);
}

int
fill_fpregs(struct thread *td, struct fpreg *regs)
{
#ifdef VFP
	struct pcb *pcb;

	pcb = td->td_pcb;
	if ((pcb->pcb_fpflags & PCB_FP_STARTED) != 0) {
		/*
		 * If we have just been running VFP instructions we will
		 * need to save the state to memcpy it below.
		 */
		vfp_save_state(td);

		memcpy(regs->fp_q, pcb->pcb_vfp, sizeof(regs->fp_q));
		regs->fp_cr = pcb->pcb_fpcr;
		regs->fp_sr = pcb->pcb_fpsr;
	} else
#endif
		memset(regs->fp_q, 0, sizeof(regs->fp_q));
	return (0);
}

int
set_fpregs(struct thread *td, struct fpreg *regs)
{
#ifdef VFP
	struct pcb *pcb;

	pcb = td->td_pcb;
	memcpy(pcb->pcb_vfp, regs->fp_q, sizeof(regs->fp_q));
	pcb->pcb_fpcr = regs->fp_cr;
	pcb->pcb_fpsr = regs->fp_sr;
#endif
	return (0);
}

int
fill_dbregs(struct thread *td, struct dbreg *regs)
{

	panic("fill_dbregs");
}

int
set_dbregs(struct thread *td, struct dbreg *regs)
{

	panic("set_dbregs");
}

int
ptrace_set_pc(struct thread *td, u_long addr)
{

	panic("ptrace_set_pc");
	return (0);
}

int
ptrace_single_step(struct thread *td)
{

	/* TODO; */
	return (0);
}

int
ptrace_clear_single_step(struct thread *td)
{

	/* TODO; */
	return (0);
}

void
exec_setregs(struct thread *td, struct image_params *imgp, u_long stack)
{
	struct trapframe *tf = td->td_frame;

	memset(tf, 0, sizeof(struct trapframe));

	/*
	 * We need to set a0 for init as it doesn't call
	 * cpu_set_syscall_retval to copy the value. We also
	 * need to set td_retval for the cases where we do.
	 */
	tf->tf_x[10] = td->td_retval[0] = stack;
	tf->tf_x[2] = STACKALIGN(stack);
	tf->tf_x[1] = imgp->entry_addr;
	tf->tf_sepc = imgp->entry_addr;
}

/* Sanity check these are the same size, they will be memcpy'd to and fro */
CTASSERT(sizeof(((struct trapframe *)0)->tf_x) ==
    sizeof((struct gpregs *)0)->gp_x);
CTASSERT(sizeof(((struct trapframe *)0)->tf_x) ==
    sizeof((struct reg *)0)->x);

int
get_mcontext(struct thread *td, mcontext_t *mcp, int clear_ret)
{
	struct trapframe *tf = td->td_frame;

	memcpy(&mcp->mc_gpregs.gp_x[1], &tf->tf_x[1],
	    sizeof(mcp->mc_gpregs.gp_x[1]) * (nitems(mcp->mc_gpregs.gp_x) - 1));

	if (clear_ret & GET_MC_CLEAR_RET) {
		mcp->mc_gpregs.gp_x[10] = 0;
		mcp->mc_gpregs.gp_x[5] = 0; /* syscall error == 0 */
		//mcp->mc_gpregs.gp_spsr = tf->tf_spsr & ~PSR_C;
	} else {
		mcp->mc_gpregs.gp_x[10] = tf->tf_x[10];
		//mcp->mc_gpregs.gp_spsr = tf->tf_spsr;
	}

	mcp->mc_gpregs.gp_x[2] = tf->tf_x[2]; /* sp */
	mcp->mc_gpregs.gp_x[1] = tf->tf_x[1]; /* ra */
	mcp->mc_gpregs.gp_sepc = tf->tf_sepc;
	mcp->mc_gpregs.gp_sstatus = tf->tf_sstatus;

	return (0);
}

int
set_mcontext(struct thread *td, mcontext_t *mcp)
{
	struct trapframe *tf;

	tf = td->td_frame;

	memcpy(tf->tf_x, mcp->mc_gpregs.gp_x, sizeof(tf->tf_x));

	tf->tf_sepc = mcp->mc_gpregs.gp_sepc;
	tf->tf_sstatus = mcp->mc_gpregs.gp_sstatus;

	//tf->tf_sp = mcp->mc_gpregs.gp_sp;
	//tf->tf_lr = mcp->mc_gpregs.gp_lr;
	//tf->tf_elr = mcp->mc_gpregs.gp_elr;
	//tf->tf_spsr = mcp->mc_gpregs.gp_spsr;

	return (0);
}

static void
get_fpcontext(struct thread *td, mcontext_t *mcp)
{
#ifdef VFP
	struct pcb *curpcb;

	critical_enter();

	curpcb = curthread->td_pcb;

	if ((curpcb->pcb_fpflags & PCB_FP_STARTED) != 0) {
		/*
		 * If we have just been running VFP instructions we will
		 * need to save the state to memcpy it below.
		 */
		vfp_save_state(td);

		memcpy(mcp->mc_fpregs.fp_q, curpcb->pcb_vfp,
		    sizeof(mcp->mc_fpregs));
		mcp->mc_fpregs.fp_cr = curpcb->pcb_fpcr;
		mcp->mc_fpregs.fp_sr = curpcb->pcb_fpsr;
		mcp->mc_fpregs.fp_flags = curpcb->pcb_fpflags;
		mcp->mc_flags |= _MC_FP_VALID;
	}

	critical_exit();
#endif
}

static void
set_fpcontext(struct thread *td, mcontext_t *mcp)
{
#ifdef VFP
	struct pcb *curpcb;

	critical_enter();

	if ((mcp->mc_flags & _MC_FP_VALID) != 0) {
		curpcb = curthread->td_pcb;

		/*
		 * Discard any vfp state for the current thread, we
		 * are about to override it.
		 */
		vfp_discard(td);

		memcpy(curpcb->pcb_vfp, mcp->mc_fpregs.fp_q,
		    sizeof(mcp->mc_fpregs));
		curpcb->pcb_fpcr = mcp->mc_fpregs.fp_cr;
		curpcb->pcb_fpsr = mcp->mc_fpregs.fp_sr;
		curpcb->pcb_fpflags = mcp->mc_fpregs.fp_flags;
	}

	critical_exit();
#endif
}

void
cpu_idle(int busy)
{

	spinlock_enter();
	if (!busy)
		cpu_idleclock();
	if (!sched_runnable())
		__asm __volatile(
		    "fence \n"
		    "wfi   \n");
#if 0
		__asm __volatile(
		    "dsb sy \n"
		    "wfi    \n");
#endif
	if (!busy)
		cpu_activeclock();
	spinlock_exit();
}

void
cpu_halt(void)
{

	panic("cpu_halt");
}

/*
 * Flush the D-cache for non-DMA I/O so that the I-cache can
 * be made coherent later.
 */
void
cpu_flush_dcache(void *ptr, size_t len)
{

	/* TBD */
}

/* Get current clock frequency for the given CPU ID. */
int
cpu_est_clockrate(int cpu_id, uint64_t *rate)
{

	panic("cpu_est_clockrate");
}

void
cpu_pcpu_init(struct pcpu *pcpu, int cpuid, size_t size)
{

	pcpu->pc_acpi_id = 0xffffffff;
}

void
spinlock_enter(void)
{
	struct thread *td;
	register_t daif;

	td = curthread;
	if (td->td_md.md_spinlock_count == 0) {
		daif = intr_disable();
		td->td_md.md_spinlock_count = 1;
		td->td_md.md_saved_daif = daif;
	} else
		td->td_md.md_spinlock_count++;
	critical_enter();
}

void
spinlock_exit(void)
{
	struct thread *td;
	register_t daif;

	td = curthread;
	critical_exit();
	daif = td->td_md.md_saved_daif;
	td->td_md.md_spinlock_count--;
	if (td->td_md.md_spinlock_count == 0)
		intr_restore(daif);
}

#ifndef	_SYS_SYSPROTO_H_
struct sigreturn_args {
	ucontext_t *ucp;
};
#endif

int
sys_sigreturn(struct thread *td, struct sigreturn_args *uap)
{
	ucontext_t uc;
#if 0
	uint32_t spsr;
#endif

	if (uap == NULL)
		return (EFAULT);
	if (copyin(uap->sigcntxp, &uc, sizeof(uc)))
		return (EFAULT);

#if 0
	/* RISCVTODO */

	/*
	 * Make sure the processor mode has not been tampered with and
	 * interrupts have not been disabled.
	 */
	spsr = uc.uc_mcontext.mc_gpregs.gp_spsr;
	if ((spsr & PSR_M_MASK) != PSR_M_EL0t ||
	    (spsr & (PSR_F | PSR_I | PSR_A | PSR_D)) != 0)
		return (EINVAL); 
#endif

	set_mcontext(td, &uc.uc_mcontext);
	set_fpcontext(td, &uc.uc_mcontext);

	/* Restore signal mask. */
	kern_sigprocmask(td, SIG_SETMASK, &uc.uc_sigmask, NULL, 0);

	return (EJUSTRETURN);
}

/*
 * Construct a PCB from a trapframe. This is called from kdb_trap() where
 * we want to start a backtrace from the function that caused us to enter
 * the debugger. We have the context in the trapframe, but base the trace
 * on the PCB. The PCB doesn't have to be perfect, as long as it contains
 * enough for a backtrace.
 */
void
makectx(struct trapframe *tf, struct pcb *pcb)
{
	int i;

	panic("%s: implement me\n", __func__);

	for (i = 0; i < PCB_LR; i++)
		pcb->pcb_x[i] = tf->tf_x[i];

	//pcb->pcb_x[PCB_LR] = tf->tf_lr;
	//pcb->pcb_pc = tf->tf_elr;
	//pcb->pcb_sp = tf->tf_sp;
	pcb->pcb_x[2] = tf->tf_x[2];
}

void
sendsig(sig_t catcher, ksiginfo_t *ksi, sigset_t *mask)
{
	struct thread *td;
	struct proc *p;
	struct trapframe *tf;
	struct sigframe *fp, frame;
	struct sigacts *psp;
	int code, onstack, sig;

	//panic("%s: implement me\n", __func__);

	td = curthread;
	p = td->td_proc;
	PROC_LOCK_ASSERT(p, MA_OWNED);

	sig = ksi->ksi_signo;
	code = ksi->ksi_code;
	psp = p->p_sigacts;
	mtx_assert(&psp->ps_mtx, MA_OWNED);

	tf = td->td_frame;
	onstack = sigonstack(tf->tf_x[2]);

	CTR4(KTR_SIG, "sendsig: td=%p (%s) catcher=%p sig=%d", td, p->p_comm,
	    catcher, sig);

	/* Allocate and validate space for the signal handler context. */
	if ((td->td_pflags & TDP_ALTSTACK) != 0 && !onstack &&
	    SIGISMEMBER(psp->ps_sigonstack, sig)) {
		fp = (struct sigframe *)(td->td_sigstk.ss_sp +
		    td->td_sigstk.ss_size);
#if defined(COMPAT_43)
		td->td_sigstk.ss_flags |= SS_ONSTACK;
#endif
	} else {
		fp = (struct sigframe *)td->td_frame->tf_x[2];
	}

	/* Make room, keeping the stack aligned */
	fp--;
	fp = (struct sigframe *)STACKALIGN(fp);

	/* Fill in the frame to copy out */
	get_mcontext(td, &frame.sf_uc.uc_mcontext, 0);
	get_fpcontext(td, &frame.sf_uc.uc_mcontext);
	frame.sf_si = ksi->ksi_info;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_stack.ss_flags = (td->td_pflags & TDP_ALTSTACK) ?
	    ((onstack) ? SS_ONSTACK : 0) : SS_DISABLE;
	frame.sf_uc.uc_stack = td->td_sigstk;
	mtx_unlock(&psp->ps_mtx);
	PROC_UNLOCK(td->td_proc);

	/* Copy the sigframe out to the user's stack. */
	if (copyout(&frame, fp, sizeof(*fp)) != 0) {
		/* Process has trashed its stack. Kill it. */
		CTR2(KTR_SIG, "sendsig: sigexit td=%p fp=%p", td, fp);
		PROC_LOCK(p);
		sigexit(td, SIGILL);
	}

	tf->tf_x[10] = sig;
	tf->tf_x[11] = (register_t)&fp->sf_si;
	tf->tf_x[12] = (register_t)&fp->sf_uc;

	tf->tf_sepc = (register_t)catcher;
	tf->tf_x[2] = (register_t)fp;
	tf->tf_x[1] = (register_t)(PS_STRINGS - *(p->p_sysent->sv_szsigcode));

	CTR3(KTR_SIG, "sendsig: return td=%p pc=%#x sp=%#x", td, tf->tf_elr,
	    tf->tf_sp);

	PROC_LOCK(p);
	mtx_lock(&psp->ps_mtx);
}

static void
init_proc0(vm_offset_t kstack)
{
	pcpup = &__pcpu[0];

	proc_linkup0(&proc0, &thread0);
	//printf("thread0 is 0x%016lx\n", thread0);
	//printf("proc0 is 0x%016lx\n", (uint64_t)&proc0);

	thread0.td_kstack = kstack;
	thread0.td_pcb = (struct pcb *)(thread0.td_kstack) - 1;
	thread0.td_pcb->pcb_fpflags = 0;
	thread0.td_pcb->pcb_vfpcpu = UINT_MAX;
	thread0.td_frame = &proc0_tf;
	pcpup->pc_curpcb = thread0.td_pcb;
}

typedef struct {
	uint32_t type;
	uint64_t phys_start;
	uint64_t virt_start;
	uint64_t num_pages;
	uint64_t attr;
} EFI_MEMORY_DESCRIPTOR;

#if 1
static int
add_physmap_entry(uint64_t base, uint64_t length, vm_paddr_t *physmap,
    u_int *physmap_idxp)
{
	u_int i, insert_idx, _physmap_idx;

	_physmap_idx = *physmap_idxp;

	if (length == 0)
		return (1);

	/*
	 * Find insertion point while checking for overlap.  Start off by
	 * assuming the new entry will be added to the end.
	 */
	insert_idx = _physmap_idx;
	for (i = 0; i <= _physmap_idx; i += 2) {
		if (base < physmap[i + 1]) {
			if (base + length <= physmap[i]) {
				insert_idx = i;
				break;
			}
			if (boothowto & RB_VERBOSE)
				printf(
		    "Overlapping memory regions, ignoring second region\n");
			return (1);
		}
	}

	/* See if we can prepend to the next entry. */
	if (insert_idx <= _physmap_idx &&
	    base + length == physmap[insert_idx]) {
		physmap[insert_idx] = base;
		return (1);
	}

	/* See if we can append to the previous entry. */
	if (insert_idx > 0 && base == physmap[insert_idx - 1]) {
		physmap[insert_idx - 1] += length;
		return (1);
	}

	_physmap_idx += 2;
	*physmap_idxp = _physmap_idx;
	if (_physmap_idx == PHYSMAP_SIZE) {
		printf(
		"Too many segments in the physical address map, giving up\n");
		return (0);
	}

	/*
	 * Move the last 'N' entries down to make room for the new
	 * entry if needed.
	 */
	for (i = _physmap_idx; i > insert_idx; i -= 2) {
		physmap[i] = physmap[i - 2];
		physmap[i + 1] = physmap[i - 1];
	}

	/* Insert the new entry. */
	physmap[insert_idx] = base;
	physmap[insert_idx + 1] = base + length;

	printf("physmap[%d] = 0x%016lx\n", insert_idx, base);
	printf("physmap[%d] = 0x%016lx\n", insert_idx + 1, base + length);
	return (1);
}
#endif

#define efi_next_descriptor(ptr, size) \
	((struct efi_md *)(((uint8_t *) ptr) + size))

#if 0
static void
add_efi_map_entries(struct efi_map_header *efihdr, vm_paddr_t *physmap,
    u_int *physmap_idxp)
{
	struct efi_md *map, *p;
	const char *type;
	size_t efisz;
	int ndesc, i;

	static const char *types[] = {
		"Reserved",
		"LoaderCode",
		"LoaderData",
		"BootServicesCode",
		"BootServicesData",
		"RuntimeServicesCode",
		"RuntimeServicesData",
		"ConventionalMemory",
		"UnusableMemory",
		"ACPIReclaimMemory",
		"ACPIMemoryNVS",
		"MemoryMappedIO",
		"MemoryMappedIOPortSpace",
		"PalCode"
	};

	/*
	 * Memory map data provided by UEFI via the GetMemoryMap
	 * Boot Services API.
	 */
	efisz = (sizeof(struct efi_map_header) + 0xf) & ~0xf;
	map = (struct efi_md *)((uint8_t *)efihdr + efisz); 

	if (efihdr->descriptor_size == 0)
		return;
	ndesc = efihdr->memory_size / efihdr->descriptor_size;

	if (boothowto & RB_VERBOSE)
		printf("%23s %12s %12s %8s %4s\n",
		    "Type", "Physical", "Virtual", "#Pages", "Attr");

	for (i = 0, p = map; i < ndesc; i++,
	    p = efi_next_descriptor(p, efihdr->descriptor_size)) {
		if (boothowto & RB_VERBOSE) {
			if (p->md_type <= EFI_MD_TYPE_PALCODE)
				type = types[p->md_type];
			else
				type = "<INVALID>";
			printf("%23s %012lx %12p %08lx ", type, p->md_phys,
			    p->md_virt, p->md_pages);
			if (p->md_attr & EFI_MD_ATTR_UC)
				printf("UC ");
			if (p->md_attr & EFI_MD_ATTR_WC)
				printf("WC ");
			if (p->md_attr & EFI_MD_ATTR_WT)
				printf("WT ");
			if (p->md_attr & EFI_MD_ATTR_WB)
				printf("WB ");
			if (p->md_attr & EFI_MD_ATTR_UCE)
				printf("UCE ");
			if (p->md_attr & EFI_MD_ATTR_WP)
				printf("WP ");
			if (p->md_attr & EFI_MD_ATTR_RP)
				printf("RP ");
			if (p->md_attr & EFI_MD_ATTR_XP)
				printf("XP ");
			if (p->md_attr & EFI_MD_ATTR_RT)
				printf("RUNTIME");
			printf("\n");
		}

		switch (p->md_type) {
		case EFI_MD_TYPE_CODE:
		case EFI_MD_TYPE_DATA:
		case EFI_MD_TYPE_BS_CODE:
		case EFI_MD_TYPE_BS_DATA:
		case EFI_MD_TYPE_FREE:
			/*
			 * We're allowed to use any entry with these types.
			 */
			break;
		default:
			continue;
		}

		if (!add_physmap_entry(p->md_phys, (p->md_pages * PAGE_SIZE),
		    physmap, physmap_idxp))
			break;
	}
}
#endif

#ifdef FDT
static void
try_load_dtb(caddr_t kmdp)
{
	vm_offset_t dtbp;

	//dtbp = MD_FETCH(kmdp, MODINFOMD_DTBP, vm_offset_t);
	dtbp = (vm_offset_t)&fdt_static_dtb;
	if (dtbp == (vm_offset_t)NULL) {
		printf("ERROR loading DTB\n");
		return;
	}

	if (OF_install(OFW_FDT, 0) == FALSE)
		panic("Cannot install FDT");

	if (OF_init((void *)dtbp) != 0)
		panic("OF_init failed with the found device tree");
}
#endif

static void
cache_setup(void)
{
	int dcache_line_shift, icache_line_shift;
	uint32_t ctr_el0;

	ctr_el0 = 0; //READ_SPECIALREG(ctr_el0);

	/* Read the log2 words in each D cache line */
	dcache_line_shift = CTR_DLINE_SIZE(ctr_el0);
	/* Get the D cache line size */
	dcache_line_size = sizeof(int) << dcache_line_shift;

	/* And the same for the I cache */
	icache_line_shift = CTR_ILINE_SIZE(ctr_el0);
	icache_line_size = sizeof(int) << icache_line_shift;

	idcache_line_size = MIN(dcache_line_size, icache_line_size);
}

/*
 * Fake up a boot descriptor table
 */
vm_offset_t
fake_preload_metadata(struct riscv_bootparams *rvbp __unused)
{
#ifdef DDB
	vm_offset_t zstart = 0, zend = 0;
#endif
	vm_offset_t lastaddr;
	int i = 0;
	static uint32_t fake_preload[35];

	fake_preload[i++] = MODINFO_NAME;
	fake_preload[i++] = strlen("kernel") + 1;
	strcpy((char*)&fake_preload[i++], "kernel");
	i += 1;
	fake_preload[i++] = MODINFO_TYPE;
	fake_preload[i++] = strlen("elf64 kernel") + 1;
	strcpy((char*)&fake_preload[i++], "elf64 kernel");
	i += 3;
	fake_preload[i++] = MODINFO_ADDR;
	fake_preload[i++] = sizeof(vm_offset_t);
	fake_preload[i++] = (uint64_t)(KERNBASE + 0x200);
	i += 1;
	fake_preload[i++] = MODINFO_SIZE;
	fake_preload[i++] = sizeof(uint64_t);
	printf("end is 0x%016lx\n", (uint64_t)&end);
	fake_preload[i++] = (uint64_t)&end - (uint64_t)(KERNBASE + 0x200);
	i += 1;
#ifdef DDBremoveme
	if (*(uint32_t *)KERNVIRTADDR == MAGIC_TRAMP_NUMBER) {
		fake_preload[i++] = MODINFO_METADATA|MODINFOMD_SSYM;
		fake_preload[i++] = sizeof(vm_offset_t);
		fake_preload[i++] = *(uint32_t *)(KERNVIRTADDR + 4);
		fake_preload[i++] = MODINFO_METADATA|MODINFOMD_ESYM;
		fake_preload[i++] = sizeof(vm_offset_t);
		fake_preload[i++] = *(uint32_t *)(KERNVIRTADDR + 8);
		lastaddr = *(uint32_t *)(KERNVIRTADDR + 8);
		zend = lastaddr;
		zstart = *(uint32_t *)(KERNVIRTADDR + 4);
		db_fetch_ksymtab(zstart, zend);
	} else
#endif
		lastaddr = (vm_offset_t)&end;
	fake_preload[i++] = 0;
	fake_preload[i] = 0;
	preload_metadata = (void *)fake_preload;

	return (lastaddr);
}

void
initriscv(struct riscv_bootparams *rvbp)
{
	//struct efi_map_header *efihdr;
	//struct pcpu *pcpup;
	vm_offset_t lastaddr;
	caddr_t kmdp;
	//vm_paddr_t mem_len;
	//int i;

	printf("preload metadata\n");

	/* Set the module data location */
	lastaddr = fake_preload_metadata(rvbp);
	//preload_metadata = (caddr_t)(uintptr_t)(rvbp->modulep);

	printf("preload metadata done\n");

	/* Find the kernel address */
	kmdp = preload_search_by_type("elf kernel");
	if (kmdp == NULL)
		kmdp = preload_search_by_type("elf64 kernel");

	if (kmdp == NULL)
		printf("elf kernel not found\n");
	else
		printf("elf kernel found\n");

	printf("boothowto\n");

	boothowto = 0; //MD_FETCH(kmdp, MODINFOMD_HOWTO, int);
	boothowto = RB_SINGLE;

	printf("kern_envp\n");
	kern_envp = NULL; //MD_FETCH(kmdp, MODINFOMD_ENVP, char *);

	printf("try load dtb\n");

#ifdef FDT
	try_load_dtb(kmdp);
#endif

	/* Find the address to start allocating from */
	//lastaddr = MD_FETCH(kmdp, MODINFOMD_KERNEND, vm_offset_t);

	printf("Load the physical memory ranges\n");

	/* Load the physical memory ranges */
	physmap_idx = 0;
#if 0
	efihdr = (struct efi_map_header *)preload_search_info(kmdp,
	    MODINFO_METADATA | MODINFOMD_EFI_MAP);
	add_efi_map_entries(efihdr, physmap, &physmap_idx);

	/* Print the memory map */
	mem_len = 0;
	for (i = 0; i < physmap_idx; i += 2)
		mem_len += physmap[i + 1] - physmap[i];
#endif

	add_physmap_entry(0, 0x8000000, physmap, &physmap_idx);
	printf("physmap_idx %d\n", physmap_idx);

	/* Set the pcpu data, this is needed by pmap_bootstrap */
	pcpup = &__pcpu[0];
	pcpu_init(pcpup, 0, sizeof(struct pcpu));

	/*
	 * Set the pcpu pointer with a backup in tpidr_el1 to be
	 * loaded when entering the kernel from userland.
	 */
#if 0
	__asm __volatile(
	    "mov x18, %0 \n"
	    "msr tpidr_el1, %0" :: "r"(pcpup));
#endif
#if 0
	__asm __volatile(
	    "mv gp, %0" :: "r"(pcpup));
#endif

	PCPU_SET(curthread, &thread0);

	printf("init_param1\n");

	/* Do basic tuning, hz etc */
	init_param1();

	printf("rvbp->kern_stack 0x%016lx\n", rvbp->kern_stack);
	printf("rvbp->kern_l1pt 0x%016lx\n", rvbp->kern_l1pt);
	printf("rvbp->kern_delta 0x%016lx\n", rvbp->kern_delta);
	printf("lastaddr 0x%016lx KERNBASE 0x%016lx\n", lastaddr, KERNBASE);

	printf("cache_setup\n");
	cache_setup();

	//while (1);

	// pmap_bootstrap(vm_offset_t l1pt, vm_paddr_t kernstart, vm_size_t kernlen)

	/* Bootstrap enough of pmap to enter the kernel proper */

	vm_paddr_t kernstart;
	vm_size_t kernlen;

	kernstart = KERNBASE - rvbp->kern_delta;
	//kernstart = (KERNBASE + 0x200);
	kernlen = (lastaddr - KERNBASE);
	pmap_bootstrap(rvbp->kern_l1pt, kernstart, kernlen);

#if 0
	/* Bootstrap enough of pmap  to enter the kernel proper */
	pmap_bootstrap(rvbp->kern_l1pt, KERNBASE - rvbp->kern_delta,
	    lastaddr - KERNBASE);

	arm_devmap_bootstrap(0, NULL);
#endif

	printf("cninit\n");
	cninit();

	printf("init proc0 kernstack 0x%016lx\n", rvbp->kern_stack);
	init_proc0(rvbp->kern_stack);

	/* set page table base register for thread0 */
	thread0.td_pcb->pcb_l1addr = (rvbp->kern_l1pt - KERNBASE);
	printf("thread0 0x%016lx pcb_l1addr 0x%016lx\n",
		&thread0, thread0.td_pcb->pcb_l1addr);

	printf("msgbuf init\n");
	msgbufinit(msgbufp, msgbufsize);

	printf("mutex init\n");
	mutex_init();

	printf("init param2\n");
	init_param2(physmem);

	printf("dbg monitor init\n");
	dbg_monitor_init();

	printf("kdb_init\n");
	kdb_init();

	early_boot = 0;

	printf("initriscv done\n");
	//while (1);
}
