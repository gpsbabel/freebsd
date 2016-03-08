/*-
 * Copyright (c) 2016 Ruslan Bukin <br@bsdpad.com>
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");
#include <sys/param.h>

#include <sys/systm.h>
#include <machine/disassem.h>
#include <machine/riscvreg.h>
#include <machine/riscv_opcode.h>
#include <ddb/ddb.h>

struct riscv_op {
	char *name;
	char *type;
	int opcode;
	int funct3;
	int funct7; /* or imm, depending on type */
};

/* Must be sorted by opcode, funct3, funct7 */
static struct riscv_op riscv_opcodes[] = {
	{ "lb",		"I",	  3,  0, -1 },
	{ "lh",		"I",	  3,  1, -1 },
	{ "lw",		"I",	  3,  2, -1 },
	{ "ld",		"I",	  3,  3, -1 },
	{ "lbu",	"I",	  3,  4, -1 },
	{ "lhu",	"I",	  3,  5, -1 },
	{ "lwu",	"I",	  3,  6, -1 },
	{ "ldu",	"I",	  3,  7, -1 },
	{ "fence",	"I",	 15,  0, -1 },
	{ "fence.i",	"I",	 15,  1, -1 },
	{ "addi",	"I",	 19,  0, -1 },
	{ "slli",	"R",	 19,  1,  0 },
	{ "slti",	"I",	 19,  2, -1 },
	{ "sltiu",	"I",	 19,  3, -1 },
	{ "xori",	"I",	 19,  4, -1 },
	{ "srli",	"R",	 19,  5,  0 },
	{ "srai",	"R",	 19,  5,  0b010000 },
	{ "ori",	"I",	 19,  6, -1 },
	{ "andi",	"I",	 19,  7, -1 },
	{ "auipc",	"U",	 23, -1, -1 },
	{ "sext.w",	"I",	 27,  0,  0 },
	{ "addiw",	"I",	 27,  0, -1 },
	{ "sb",		"S",	 35,  0, -1 },
	{ "sh",		"S",	 35,  1, -1 },
	{ "sw",		"S",	 35,  2, -1 },
	{ "sd",		"S",	 35,  3, -1 },
	{ "sbu",	"S",	 35,  4, -1 },
	{ "shu",	"S",	 35,  5, -1 },
	{ "swu",	"S",	 35,  6, -1 },
	{ "sdu",	"S",	 35,  7, -1 },
	{ "lr.w",	"RA",	 47,  2, 0b0001000 },
	{ "sc.w",	"RA",	 47,  2, 0b0001100 },
	{ "amoswap.w",	"RA",	 47,  2, 0b0000100 },
	{ "amoadd.w",	"RA",	 47,  2, 0b0000000 },
	{ "amoxor.w",	"RA",	 47,  2, 0b0010000 },
	{ "amoand.w",	"RA",	 47,  2, 0b0110000 },
	{ "amoor.w",	"RA",	 47,  2, 0b0100000 },
	{ "amomin.w",	"RA",	 47,  2, 0b1000000 },
	{ "amomax.w",	"RA",	 47,  2, 0b1010000 },
	{ "amominu.w",	"RA",	 47,  2, 0b1100000 },
	{ "amomaxu.w",	"RA",	 47,  2, 0b1110000 },
	{ "lr.w.aq",	"RA",	 47,  2, 0b0001000 },
	{ "sc.w.aq",	"RA",	 47,  2, 0b0001100 },
	{ "amoswap.w.aq","RA",	 47,  2, 0b0000110 },
	{ "amoadd.w.aq","RA",	 47,  2, 0b0000010 },
	{ "amoxor.w.aq","RA",	 47,  2, 0b0010010 },
	{ "amoand.w.aq","RA",	 47,  2, 0b0110010 },
	{ "amoor.w.aq",	"RA",	 47,  2, 0b0100010 },
	{ "amomin.w.aq","RA",	 47,  2, 0b1000010 },
	{ "amomax.w.aq","RA",	 47,  2, 0b1010010 },
	{ "amominu.w.aq","RA",	 47,  2, 0b1100010 },
	{ "amomaxu.w.aq","RA",	 47,  2, 0b1110010 },
	{ "amoswap.w.rl","RA",	 47,  2, 0b0000110 },
	{ "amoadd.w.rl","RA",	 47,  2, 0b0000001 },
	{ "amoxor.w.rl","RA",	 47,  2, 0b0010001 },
	{ "amoand.w.rl","RA",	 47,  2, 0b0110001 },
	{ "amoor.w.rl",	"RA",	 47,  2, 0b0100001 },
	{ "amomin.w.rl","RA",	 47,  2, 0b1000001 },
	{ "amomax.w.rl","RA",	 47,  2, 0b1010001 },
	{ "amominu.w.rl","RA",	 47,  2, 0b1100001 },
	{ "amomaxu.w.rl","RA",	 47,  2, 0b1110001 },
	{ "amoswap.d",	"RA",	 47,  3, 0b0000100 },
	{ "amoadd.d",	"RA",	 47,  3, 0b0000000 },
	{ "amoxor.d",	"RA",	 47,  3, 0b0010000 },
	{ "amoand.d",	"RA",	 47,  3, 0b0110000 },
	{ "amoor.d",	"RA",	 47,  3, 0b0100000 },
	{ "amomin.d",	"RA",	 47,  3, 0b1000000 },
	{ "amomax.d",	"RA",	 47,  3, 0b1010000 },
	{ "amominu.d",	"RA",	 47,  3, 0b1100000 },
	{ "amomaxu.d",	"RA",	 47,  3, 0b1110000 },
	{ "lr.d.aq",	"RA",	 47,  3, 0b0001000 },
	{ "sc.d.aq",	"RA",	 47,  3, 0b0001100 },
	{ "amoswap.d.aq","RA",	 47,  3, 0b0000110 },
	{ "amoadd.d.aq","RA",	 47,  3, 0b0000010 },
	{ "amoxor.d.aq","RA",	 47,  3, 0b0010010 },
	{ "amoand.d.aq","RA",	 47,  3, 0b0110010 },
	{ "amoor.d.aq",	"RA",	 47,  3, 0b0100010 },
	{ "amomin.d.aq","RA",	 47,  3, 0b1000010 },
	{ "amomax.d.aq","RA",	 47,  3, 0b1010010 },
	{ "amominu.d.aq","RA",	 47,  3, 0b1100010 },
	{ "amomaxu.d.aq","RA",	 47,  3, 0b1110010 },
	{ "amoswap.d.rl","RA",	 47,  3, 0b0000110 },
	{ "amoadd.d.rl","RA",	 47,  3, 0b0000001 },
	{ "amoxor.d.rl","RA",	 47,  3, 0b0010001 },
	{ "amoand.d.rl","RA",	 47,  3, 0b0110001 },
	{ "amoor.d.rl",	"RA",	 47,  3, 0b0100001 },
	{ "amomin.d.rl","RA",	 47,  3, 0b1000001 },
	{ "amomax.d.rl","RA",	 47,  3, 0b1010001 },
	{ "amominu.d.rl","RA",	 47,  3, 0b1100001 },
	{ "amomaxu.d.rl","RA",	 47,  3, 0b1110001 },
	{ "add",	"R",	 51,  0,  0 },
	{ "sub",	"R",	 51,  0,  0b010000 },
	{ "sll",	"R",	 51,  1,  0 },
	{ "slt",	"R",	 51,  2,  0 },
	{ "sltu",	"R",	 51,  3,  0 },
	{ "xor",	"R",	 51,  4,  0 },
	{ "srl",	"R",	 51,  5,  0 },
	{ "sra",	"R",	 51,  5,  0b010000 },
	{ "or",		"R",	 51,  6,  0 },
	{ "and",	"R",	 51,  7,  0 },
	{ "lui",	"U",	 55, -1, -1 },
	{ "beq",	"SB",	 99,  0, -1 },
	{ "bne",	"SB",	 99,  1, -1 },
	{ "blt",	"SB",	 99,  4, -1 },
	{ "bge",	"SB",	 99,  5, -1 },
	{ "bltu",	"SB",	 99,  6, -1 },
	{ "bgeu",	"SB",	 99,  7, -1 },
	{ "jalr",	"I",	103,  0, -1 },
	{ "jal",	"UJ",	111, -1, -1 },
	{ "sfence.vm",	"I",	115,  0, 0b000100000001 },
	{ "wfi",	"I",	115,  0, 0b000100000010 },
	{ "rdcycle",	"I",	115,  2, 0b110000000000 },
	{ "rdcycleh",	"I",	115,  2, 0b110010000000 },
	{ "rdtime",	"I",	115,  2, 0b110000000001 },
	{ "rdtimeh",	"I",	115,  2, 0b110010000001 },
	{ "rdinstret",	"I",	115,  2, 0b110000000010 },
	{ "rdinstreth",	"I",	115,  2, 0b110010000010 },
	{ NULL, NULL, 0, 0, 0 }, /* terminator */
};

static char *reg_name[32] = {
	"zero",	"ra",	"sp",	"gp",	"tp",	"t0",	"t1",	"t2",
	"s0",	"s1",	"a0",	"a1",	"a2",	"a3",	"a4",	"a5",
	"a6",	"a7",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7",
	"s8",	"s9",	"s10",	"s11",	"t3",	"t4",	"t5",	"t6"
};

static int32_t
get_imm(InstFmt i, char *type)
{
	int imm;

	imm = 0;

	if (strcmp(type, "I") == 0) {
		imm = i.IType.imm;
		if (imm & (1 << 11))
			imm |= (0xfffff << 12);	/* sign extend */
	} else if (strcmp(type, "S") == 0) {
		imm = i.SType.imm0_4;
		imm |= (i.SType.imm5_11 << 5);
		if (imm & (1 << 11))
			imm |= (0xfffff << 12); /* sign extend */
	} else if (strcmp(type, "U") == 0) {
		imm = i.UType.imm12_31;
	} else if (strcmp(type, "UJ") == 0) {
		imm = i.UJType.imm12_19 << 12;
		imm |= i.UJType.imm11 << 11;
		imm |= i.UJType.imm1_10 << 1;
		imm |= i.UJType.imm20 << 20;
		if (imm & (1 << 20))
			imm |= (0xfff << 21);	/* sign extend */
	} else if (strcmp(type, "SB") == 0) {
		imm = i.SBType.imm11 << 11;
		imm |= i.SBType.imm1_4 << 1;
		imm |= i.SBType.imm5_10 << 5;
		imm |= i.SBType.imm12 << 12;
		if (imm & (1 << 12))
			imm |= (0xfffff << 12);	/* sign extend */
	}

	return (imm);
}

static int
match_opcode(InstFmt i, struct riscv_op *op, vm_offset_t loc)
{
	int imm;

	imm = get_imm(i, op->type);

	if (strcmp(op->type, "U") == 0) {
		/* Match */
		db_printf("%s\t%s,0x%x", op->name,
		    reg_name[i.UType.rd], imm);
		return (1);
	}
	if (strcmp(op->type, "UJ") == 0) {
		/* Match */
		db_printf("%s\t0x%lx", op->name, (loc + imm));
		return (1);
	}
	if ((strcmp(op->type, "I") == 0) && \
	    (op->funct3 == i.IType.funct3)) {

		if (op->funct7 != -1) {
			if (op->funct7 == i.IType.imm) {
				/* Match */
				db_printf("%s\t%s, %s, %d", op->name,
				    reg_name[i.IType.rd],
				    reg_name[i.IType.rs1], imm);
				return (1);
			}
		} else {
			/* Match */
			db_printf("%s\t%s, %s, %d", op->name,
			    reg_name[i.IType.rd],
			    reg_name[i.IType.rs1], imm);
			return (1);
		}
	}
	if ((strcmp(op->type, "S") == 0) && \
	    (op->funct3 == i.SType.funct3)) {
		/* Match */
		db_printf("%s\t%s, %s, %d", op->name,
		    reg_name[i.SType.rs1],
		    reg_name[i.SType.rs2], imm);
		return (1);
	}
	if ((strcmp(op->type, "SB") == 0) && \
	    (op->funct3 == i.SType.funct3)) {
		/* Match */
		db_printf("%s\t%s, %s, 0x%lx", op->name,
		    reg_name[i.SBType.rs1],
		    reg_name[i.SBType.rs2], (loc + imm));
		return (1);
	}
	if ((strcmp(op->type, "R") == 0) && \
	    (op->funct3 == i.RType.funct3) && \
	    (op->funct7 == i.RType.funct7)) {
		/* Match */
		db_printf("%s\t%s, %s, 0x%x", op->name,
		    reg_name[i.RType.rd],
		    reg_name[i.RType.rs1], i.RType.rs2);
		return (1);
	}
	if ((strcmp(op->type, "RA") == 0) && \
	    (op->funct3 == i.RAType.funct3) && \
	    (op->funct7 == i.RAType.funct7)) {
		/* Match */
		db_printf("%s\t%s, %s, %s", op->name,
		    reg_name[i.RAType.rd],
		    reg_name[i.RAType.rs2],
		    reg_name[i.RAType.rs1]);
		return (1);
	}

	return (0);
}

vm_offset_t
disasm(const struct disasm_interface *di, vm_offset_t loc, int altfmt)
{
	struct riscv_op *op;
	InstFmt i;
	int j;

	i.word = di->di_readword(loc);

	/* First match opcode */
	for (j = 0; riscv_opcodes[j].name != NULL; j++) {
		op = &riscv_opcodes[j];
		if (op->opcode == i.RType.opcode) {
			if (match_opcode(i, op, loc))
				break;
		}
	}

	di->di_printf("\n");
	return(loc + INSN_SIZE);
}
