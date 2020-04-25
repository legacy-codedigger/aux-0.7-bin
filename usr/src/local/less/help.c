#include  "less.h"

/*
 * Display some help.
 * Help is in two pages.
 */
	static void
help0()
{
	puts("f, SPACE       Forward one screen.\r\n");
	puts("b              Backward one screen.\r\n");
	puts("e, j, CR    *  Forward N lines, default 1.\r\n");
	puts("y, k        *  Backward N lines, default 1.\r\n");
	puts("d           *  Forward N lines, default 10 or last N to d or u command.\r\n");
	puts("u           *  Backward N lines, default 10 or last N to d or u command.\r\n");
	puts("r              Repaint screen.\r\n");
	puts("g           *  Go to line N, default 1.\r\n");
	puts("G           *  Like g, but default is last line in file.\r\n");
	puts("=              Print current file name\r\n");
	puts("/pattern    *  Search forward for N-th occurence of pattern.\r\n");
	puts("?pattern    *  Search backward for N-th occurence of pattern.\r\n");
	puts("n           *  Repeat previous search (for N-th occurence).\r\n");
	puts("q              Exit.\r\n");
	error("More help...");
}

	static void
help1()
{
	char message[100];
	extern char all_options[];

	puts("R              Repaint screen, discarding buffered input.\r\n");
	puts("p, %        *  Position to N percent into the file.\r\n");
	puts("m<letter>      Mark the current position with <letter>.\r\n");
	puts("'<letter>      Return to a previously marked position.\r\n");
	sprintf(message, 
	     "-X             Toggle a flag (X may be one of \"%s\").\r\n", 
				all_options);
	puts(message);
	puts("E [file]       Examine a new file.\r\n");
	puts("N              Examine the next file (from the command line).\r\n");
	puts("P              Examine the previous file (from the command line).\r\n");
	puts("V              Print version number.\r\n");
#if SHELL_ESCAPE
	puts("!command       Passes the command to a shell to be executed.\r\n");
#endif
#if EDITOR
	sprintf(message,
	     "v              Edit the current file with $EDITOR (default %s).\r\n",
				EDIT_PGM);
	puts(message);
#endif
	error("");
}

	public void
help()
{
	register int i;

	for (i = 0;  i < 2;  i++)
	{
		clear();
		puts("Commands marked with * may be preceeded by a number, N.\r\n\r\n");

		switch (i)
		{
		case 0:		help0();	break;
		case 1:		help1();	break;
		}
	}
}
