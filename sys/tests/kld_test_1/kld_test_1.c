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

static int kld_test_1_modevent(module_t mod, int type, void *data);


static moduledata_t moddef = {
	.name = "kld_test_1",
	.evhand = kld_test_1_modevent,
	.priv = 0
};

MODULE_DEPEND(kld_test_1_, kld_test_0, 1, 1, 1);
MODULE_VERSION(kld_test_1, 1);
DECLARE_MODULE(kld_test_1, moddef, SI_SUB_PSEUDO, SI_ORDER_ANY);


static int
do_kld_test_1(void)
{
	int n_unexpected = 0;
	volatile int* ext_vp;

	/* 
	 * Check if the variale defined in kld_test_0.ko is unique.
	 */
	ext_vp = kld_test_0_varptr();
	if (ext_vp != &kld_test_0_var) {
		++ n_unexpected;
	}

	/*
	 * check if the extrnal function defined in another module is valid.
	 */
	kld_test_fn_ptr set_magic_fn = kld_test_0_fnaddr();
	if (set_magic_fn == NULL) {
		++ n_unexpected;
		goto test_end;
	}
		
	/*
	 * Preparation for a test in another module.
	 * kld_test_0_var is set to KLD_TEST_0_VAR_MAGIC_1.
	 */
	(*set_magic_fn)();

	/*
	 * Check if this module can read the value defined in another module,
	 * and it reads expected value.
	 */
	printf("%s: kld_test_0_var@%p=%d ",
	       moddef.name, &kld_test_0_var, kld_test_0_var);
	if (kld_test_0_var == KLD_TEST_0_VAR_MAGIC_1) {
		printf("OK\n");
	} else  {
		printf("NG\n");
		++ n_unexpected;
	}

test_end:
	return n_unexpected;
}


static int
kld_test_1_modevent(module_t mod, int type, void *data)
{
	int error = 0;
	switch (type) {
	case MOD_LOAD:
		error = do_kld_test_1();
		if (error != 0)
			printf("%s: MOD_LOAD: NG: error=%d\n", moddef.name, error);
		else
			printf("%s: MOD_LOAD: OK\n", moddef.name);
		break;

	case MOD_UNLOAD:
		kld_test_0_var = KLD_TEST_0_VAR_MAGIC_2;
		break;
	default:
		return (EOPNOTSUPP);
	}

	return (0);
}
