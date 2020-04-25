# makefile to build sample applications

# The next entry builds the "sample" program.  Note that to use this program,
# the file "sample.res", which contains the resource fork of sample, must be
# copied to the directory containing sample.

sample:		sample.o
		ld /usr/lib/maccrt0.o -o sample sample.o \
		/usr/lib/low.o -lmac -lc /usr/lib/low.ld
sample.o:	sample.c
		cc ${CFLAGS} -I/usr/include/mac -DUNIX -c sample.c

# The next entry builds the "qdsamp" program.  This program does not have
# a resource fork.

qdsamp:		qdsamp.o
		ld /usr/lib/maccrt0.o -o qdsamp qdsamp.o \
		/usr/lib/low.o -lmac -lc /usr/lib/low.ld
qdsamp.o:	qdsamp.c
		cc ${CFLAGS} -I/usr/include/mac -c qdsamp.c

