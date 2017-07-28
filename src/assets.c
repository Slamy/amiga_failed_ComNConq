

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <intuition/screens.h>
//#include <intuition/intuition.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/blit.h>
#include <hardware/cia.h>
#include <graphics/display.h>
#include <exec/interrupts.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "assets.h"

struct assets *assets;

int readAssets()
{
	BPTR fh;

	printf("Reading assets...\n");

	assets=AllocMem(sizeof(struct assets), MEMF_CHIP);
	if (assets==NULL)
	{
		printf("Couldn't allocate %d bytes chip mem for assets...\n",sizeof(struct assets));
		return 1;
	}

	fh = Open((CONST_STRPTR)"assets.bin", MODE_OLDFILE);
	if (!fh)
		return 1;


	int anz = Read( fh, (APTR)assets, sizeof(struct assets) );

	printf("Read %d bytes...\n",anz);
	printf("Assets are expected to be %d bytes...\n",sizeof(struct assets));

	printf("", assets->sprite_mouseCursor1);
	Close(fh);

	if (anz != sizeof(struct assets))
		return 1;

	return 0;

}

