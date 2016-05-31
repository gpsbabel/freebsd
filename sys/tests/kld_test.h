/*- 
 * Copyright (c) 2016 Yukishige Shibata <y-shibat@mtd.biglobe.ne.jp>
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
 *__FBSDID("$FreeBSD$");
 */

#ifndef __kld_test_h__
#define __kld_test_h__

#include <sys/sysent.h>

enum {
	KLD_TEST_0_VAR_MAGIC_0 = 17,
	KLD_TEST_0_VAR_MAGIC_1 = 19,
	KLD_TEST_0_VAR_MAGIC_2 = 23,
};

typedef void (*kld_test_fn_ptr)(void);

extern volatile int kld_test_0_var;

kld_test_fn_ptr kld_test_0_fnaddr(void);
volatile int* kld_test_0_varptr(void);


extern struct sysentvec null_sysvec; /* Defined in the kernel, sys/kern/init_main.c */

#endif /* ! __kld_test_h__ */
