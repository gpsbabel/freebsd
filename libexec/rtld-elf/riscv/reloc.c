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

#include <sys/param.h>
#include <sys/mman.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <machine/cpu.h>
#include <machine/md_var.h>

#include "debug.h"
#include "rtld.h"

struct funcdesc {
	Elf_Addr addr;
	Elf_Addr toc;
	Elf_Addr env;
};

/*
 * Process the R_RISCV_COPY relocations
 */
int
do_copy_relocations(Obj_Entry *dstobj)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;

	/*
	 * COPY relocs are invalid outside of the main program
	 */
	assert(dstobj->mainprog);

	relalim = (const Elf_Rela *) ((caddr_t) dstobj->rela +
	    dstobj->relasize);
	for (rela = dstobj->rela;  rela < relalim;  rela++) {
		void *dstaddr;
		const Elf_Sym *dstsym;
		const char *name;
		size_t size;
		const void *srcaddr;
		const Elf_Sym *srcsym = NULL;
		const Obj_Entry *srcobj, *defobj;
		SymLook req;
		int res;

		if (ELF_R_TYPE(rela->r_info) != R_RISCV_COPY) {
			continue;
		}

		dstaddr = (void *) (dstobj->relocbase + rela->r_offset);
		dstsym = dstobj->symtab + ELF_R_SYM(rela->r_info);
		name = dstobj->strtab + dstsym->st_name;
		size = dstsym->st_size;
		symlook_init(&req, name);
		req.ventry = fetch_ventry(dstobj, ELF_R_SYM(rela->r_info));
		req.flags = SYMLOOK_EARLY;

		for (srcobj = dstobj->next;  srcobj != NULL;
		     srcobj = srcobj->next) {
			res = symlook_obj(&req, srcobj);
			if (res == 0) {
				srcsym = req.sym_out;
				defobj = req.defobj_out;
				break;
			}
		}

		if (srcobj == NULL) {
			_rtld_error("Undefined symbol \"%s\" "
				    " referenced from COPY"
				    " relocation in %s", name, dstobj->path);
			return (-1);
		}

		srcaddr = (const void *) (defobj->relocbase+srcsym->st_value);
		memcpy(dstaddr, srcaddr, size);
		dbg("copy_reloc: src=%p,dst=%p,size=%zd\n",srcaddr,dstaddr,size);

		//uint64_t *addr = (uint64_t *)(dstobj->relocbase + rela->r_offset);
		//if (size == 8)
		//	dbg("val 0x%016lx\n", *addr);
	}

	return (0);
}

/*
 * Relocate a non-PLT object with addend.
 */
static int
reloc_nonplt_object(Obj_Entry *obj_rtld, Obj_Entry *obj, const Elf_Rela *rela,
    SymCache *cache, int flags, RtldLockState *lockstate)
{
	Elf_Addr        *where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
	const Elf_Sym   *def;
	const Obj_Entry *defobj;
	Elf_Addr         tmp;

	switch (ELF_R_TYPE(rela->r_info)) {

	case R_RISCV_NONE:
		break;

        case R_RISCV_64:
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    flags, cache, lockstate);
		if (def == NULL) {
			return (-1);
		}

                tmp = (Elf_Addr)(defobj->relocbase + def->st_value +
                    rela->r_addend);

		/* Don't issue write if unnecessary; avoid COW page fault */
                if (*where != tmp) {
                        *where = tmp;
		}
                break;

        case R_RISCV_RELATIVE:  /* doubleword64 B + A */
		tmp = (Elf_Addr)(obj->relocbase + rela->r_addend);

		/* As above, don't issue write unnecessarily */
		if (*where != tmp) {
			*where = tmp;
		}
		break;

	case R_RISCV_COPY:
		/*
		 * These are deferred until all other relocations
		 * have been done.  All we do here is make sure
		 * that the COPY relocation is not in a shared
		 * library.  They are allowed only in executable
		 * files.
		 */
		if (!obj->mainprog) {
			_rtld_error("%s: Unexpected R_COPY "
				    " relocation in shared library",
				    obj->path);
			return (-1);
		}
		break;

	case R_RISCV_JUMP_SLOT:
		/*
		 * These will be handled by the plt/jmpslot routines
		 */
		break;

	case R_RISCV_TLS_DTPMOD64:
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    flags, cache, lockstate);

		if (def == NULL)
			return (-1);

		*where = (Elf_Addr) defobj->tlsindex;

		break;

	case R_RISCV_TLS_TPREL64:
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    flags, cache, lockstate);

		if (def == NULL)
			return (-1);

		/*
		 * We lazily allocate offsets for static TLS as we
		 * see the first relocation that references the
		 * TLS block. This allows us to support (small
		 * amounts of) static TLS in dynamically loaded
		 * modules. If we run out of space, we generate an
		 * error.
		 */
		if (!defobj->tls_done) {
			if (!allocate_tls_offset((Obj_Entry*) defobj)) {
				_rtld_error("%s: No space available for static "
				    "Thread Local Storage", obj->path);
				return (-1);
			}
		}

		*(Elf_Addr **)where = *where * sizeof(Elf_Addr)
		    + (Elf_Addr *)(def->st_value + rela->r_addend
		    + defobj->tlsoffset - TLS_TP_OFFSET);

		break;

	case R_RISCV_TLS_DTPREL64:
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    flags, cache, lockstate);

		if (def == NULL)
			return (-1);

		*where += (Elf_Addr)(def->st_value + rela->r_addend
		    - TLS_DTV_OFFSET);

		break;

	default:
		_rtld_error("%s: Unsupported relocation type %ld"
			    " in non-PLT relocations\n", obj->path,
			    ELF_R_TYPE(rela->r_info));
		return (-1);
        }
	return (0);
}


/*
 * Process non-PLT relocations
 */
int
reloc_non_plt(Obj_Entry *obj, Obj_Entry *obj_rtld, int flags,
    RtldLockState *lockstate)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	SymCache *cache;
	int bytes = obj->dynsymcount * sizeof(SymCache);
	int r = -1;

	if ((flags & SYMLOOK_IFUNC) != 0)
		/* XXX not implemented */
		return (0);

	/*
	 * The dynamic loader may be called from a thread, we have
	 * limited amounts of stack available so we cannot use alloca().
	 */
	if (obj != obj_rtld) {
		cache = mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_ANON,
		    -1, 0);
		if (cache == MAP_FAILED)
			cache = NULL;
	} else
		cache = NULL;

	/*
	 * From the SVR4 PPC ABI:
	 * "The PowerPC family uses only the Elf32_Rela relocation
	 *  entries with explicit addends."
	 */
	relalim = (const Elf_Rela *)((caddr_t)obj->rela + obj->relasize);
	for (rela = obj->rela; rela < relalim; rela++) {
		if (reloc_nonplt_object(obj_rtld, obj, rela, cache, flags,
		    lockstate) < 0)
			goto done;
	}
	r = 0;
done:
	if (cache)
		munmap(cache, bytes);

	/* Synchronize icache for text seg in case we made any changes */
	//__syncicache(obj->mapbase, obj->textsize);

	return (r);
}

/*
 * Process the PLT relocations.
 */
int
reloc_plt(Obj_Entry *obj)
{
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	Elf_Addr *where;

	if (obj->pltrelasize != 0) {
		relalim = (const Elf_Rela *)((char *)obj->pltrela +
		    obj->pltrelasize);
		for (rela = obj->pltrela;  rela < relalim;  rela++) {
			assert(ELF_R_TYPE(rela->r_info) == R_RISCV_JUMP_SLOT);

			where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
			*where += (Elf_Addr)obj->relocbase;
		}
	}

	return (0);
}


/*
 * LD_BIND_NOW was set - force relocation for all jump slots
 */
int
reloc_jmpslots(Obj_Entry *obj, int flags, RtldLockState *lockstate)
{
	const Obj_Entry *defobj;
	const Elf_Rela *relalim;
	const Elf_Rela *rela;
	const Elf_Sym *def;
	Elf_Addr *where;
	Elf_Addr target;

	relalim = (const Elf_Rela *)((char *)obj->pltrela + obj->pltrelasize);
	for (rela = obj->pltrela; rela < relalim; rela++) {
		assert(ELF_R_TYPE(rela->r_info) == R_RISCV_JUMP_SLOT);
		where = (Elf_Addr *)(obj->relocbase + rela->r_offset);
		def = find_symdef(ELF_R_SYM(rela->r_info), obj, &defobj,
		    SYMLOOK_IN_PLT | flags, NULL, lockstate);
		if (def == NULL) {
			dbg("reloc_jmpslots: sym not found");
			return (-1);
		}

		target = (Elf_Addr)(defobj->relocbase + def->st_value);

		//*where = target;
		//continue;

#if 0
		/* PG XXX */
		dbg("\"%s\" in \"%s\" --> %p in \"%s\"",
		    defobj->strtab + def->st_name, basename(obj->path),
		    (void *)target, basename(defobj->path));
#endif

		if (def == &sym_zero) {
			/* Zero undefined weak symbols */
			bzero(where, sizeof(struct funcdesc));
		} else {
			reloc_jmpslot(where, target, defobj, obj,
			    (const Elf_Rel *) rela);
		}
	}

	obj->jmpslots_done = true;

	return (0);
}


/*
 * Update the value of a PLT jump slot.
 */
Elf_Addr
reloc_jmpslot(Elf_Addr *wherep, Elf_Addr target, const Obj_Entry *defobj,
	      const Obj_Entry *obj, const Elf_Rel *rel)
{

#if 0
	dbg(" reloc_jmpslot: where=%p, target=%p (%#lx + %#lx)",
	    (void *)wherep, (void *)target, *(Elf_Addr *)target,
	    (Elf_Addr)defobj->relocbase);
#endif

	if (*wherep != target)
                *wherep = target;

	//__asm __volatile("dcbst 0,%0; sync" :: "r"(wherep) : "memory");

	return (target);
}

int
reloc_iresolve(Obj_Entry *obj, struct Struct_RtldLockState *lockstate)
{

	/* XXX not implemented */
	return (0);
}

int
reloc_gnu_ifunc(Obj_Entry *obj, int flags,
    struct Struct_RtldLockState *lockstate)
{

	/* XXX not implemented */
	return (0);
}

void
init_pltgot(Obj_Entry *obj)
{

	dbg("pltgot relocbase 0x%016lx obj->pltgot 0x%016lx",
			(uint64_t)obj->relocbase, (uint64_t)obj->pltgot);
	if (obj->pltgot != NULL) {
		obj->pltgot[0] = (Elf_Addr) &_rtld_bind_start;
		obj->pltgot[1] = (Elf_Addr) obj;
	}
}

void
allocate_initial_tls(Obj_Entry *list)
{
	Elf_Addr **tp;

	/*
	* Fix the size of the static TLS block by using the maximum
	* offset allocated so far and adding a bit for dynamic modules to
	* use.
	*/

	tls_static_space = tls_last_offset + tls_last_size + RTLD_STATIC_TLS_EXTRA;

	tp = (Elf_Addr **) ((char *)allocate_tls(list, NULL, TLS_TCB_SIZE, 16) 
	    + TLS_TP_OFFSET + TLS_TCB_SIZE);

	__asm __volatile("mv tp, %0" :: "r"(tp));
}

void*
__tls_get_addr(tls_index* ti)
{
	Elf_Addr **tp;
	char *p;

	__asm __volatile("mv %0, tp" : "=r"(tp));

	p = tls_get_addr_common((Elf_Addr**)((Elf_Addr)tp - TLS_TP_OFFSET 
	    - TLS_TCB_SIZE), ti->ti_module, ti->ti_offset);

	return (p + TLS_DTV_OFFSET);
}
