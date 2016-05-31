/*- 
 * Copyright (c) 2016 Yukishige Shibata All rights reserved.
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
#include <sys/libkern.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/systm.h>
#include <tests/kld_test.h>

static int kld_test_0_modevent(module_t mod, int type, void *data);

static moduledata_t moddef = {
	.name = "kld_test_0",
	.evhand = kld_test_0_modevent,
	.priv = 0
};

MODULE_VERSION(kld_test_0, 1);
DECLARE_MODULE(kld_test_0, moddef, SI_SUB_PSEUDO, SI_ORDER_ANY);

volatile int kld_test_0_var = 0;

static int
do_kld_test_0(void)
{
	int n_unexpected = 0;

	/* 
	 * Check if this module can get the address of a 
	 * global object in the kernel.
	 */
	printf("%s: null_sysvec @%p \n", moddef.name, &null_sysvec);

	/*
	 * Check if this module can get the address of 
	 * members in the global object, and it
	 * reads exepcted values.
	 */
	printf("%s:\tsv_size=%d ", moddef.name, null_sysvec.sv_size);
	if (null_sysvec.sv_size == 0) {
		printf("OK\n");
	} else {
		printf("NG\n");
		++n_unexpected;
	}
	if (null_sysvec.sv_name &&
	    strncmp(null_sysvec.sv_name, "null", sizeof("null")) == 0) {
		printf("%s: \tsv_name=%s OK\n", moddef.name, null_sysvec.sv_name);
	} else {
		printf("%s: \tsv_name=%s NG\n", moddef.name, null_sysvec.sv_name);
		++n_unexpected;
	}

	/*
	 * Check if this module can access the object
	 * defined in itself, and it reads expected value.
	 */
	printf("%s: kld_test_0_var@%p=%d",
	       moddef.name, &kld_test_0_var, kld_test_0_var);
	if (kld_test_0_var == KLD_TEST_0_VAR_MAGIC_0) {
		printf("OK\n");
	} else {
		printf("NG\n");
		++n_unexpected;
	}

	return n_unexpected;
}


static void
kld_test_0_set_magic_1(void)
{
	kld_test_0_var = KLD_TEST_0_VAR_MAGIC_1;
}

kld_test_fn_ptr
kld_test_0_fnaddr(void)
{
	return &kld_test_0_set_magic_1;
}


volatile int*
kld_test_0_varptr(void)
{
	return &kld_test_0_var;
}

static int
kld_test_0_modevent(module_t mod, int type, void *data)
{
	int error = 0;
	switch (type) {
	case MOD_LOAD:
		/*
		 * Set the expected value to a variable,
		 * that is used for test in another module.
		 */
		kld_test_0_var = KLD_TEST_0_VAR_MAGIC_0;
		error = do_kld_test_0();
		if (error != 0)
			printf("%s: MOD_LOAD: error=%d\n", moddef.name, error);
		else
			printf("%s: MOD_LOAD: OK\n", moddef.name);
		break;

	case MOD_UNLOAD:
		/*
		 * Check if another module writes expected value.
		 */
		if (kld_test_0_var != KLD_TEST_0_VAR_MAGIC_2)
			printf("%s: MOD_UNLOAD: NG\n", moddef.name);
		else
			printf("%s: MOD_UNLOAD: OK\n", moddef.name);
		break;

	default:
		return (EOPNOTSUPP);
	}

	return (0);
}

