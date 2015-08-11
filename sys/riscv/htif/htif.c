#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/pcpu.h>
#include <sys/proc.h>

#include <machine/asm.h>
#include <machine/htif.h>
#include <machine/vmparam.h>

void
htif_test(void)
{
	char id[HTIF_MAX_ID] __aligned(HTIF_ALIGN);
	int i;

	for (i = 0; i < HTIF_MAX_DEV; i++) {

		htif_tohost(i, HTIF_CMD_IDENTIFY, (__pa(id) << 8) | 0xFF);
		htif_fromhost();
	}

}

void
htif_init(void)
{

}

SYSINIT(htif, SI_SUB_CPU, SI_ORDER_ANY, htif_init, NULL);
