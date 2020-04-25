/************************************************************************/
/*									*/
/*	Loader Script for linking a Mac Unix program with low memory	*/
/*		globals defined as externs.				*/
/*									*/
/************************************************************************/

MEMORY {
	low:	org=0,		len=0x4000
	valid:	org=0x400000,	len=0x400000
	}

-N

SECTIONS
	{
	.text :{} > valid
        .data :{} > valid
        .bss  :{} > valid

	.foo 0000 (DSECT): { 
			/usr/lib/low.o (.data)
			/usr/lib/low.o (.bss) }

	}
