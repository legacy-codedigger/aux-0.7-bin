/*
 *	Loader command file for producing unshared executables
 *	with text relocation at address 0.
 *
 *	Text starts at 0, data and bss immediately thereafter.
 *
 *	The -N flag (same as if specified on the ld command line)
 *	tells ld to place the magic number for unshared binaries
 *	(407) in the optional file header, instead of the default
 *	magic number (410).
 *
 */

-N

SECTIONS
{
	.text: {}
	.data: {}
	.bss: {}
}
