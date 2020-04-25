CFLAGS	      =
DEST	      = .
EXTHDRS	      =
HDRS	      =
LIBRARY	      = lib.a
MAKE	      = make
MAKEFILE      = Makefile
OBJS	      =
PRINT	      = pr
SHELL	      = /bin/sh
SRCS	      =

all:		$(LIBRARY)

$(LIBRARY):	$(OBJS)
		@echo  "Loading $(LIBRARY) ... \c"
		@ar cru $(LIBRARY) $(OBJS)
		@echo "done"

clean:;		@rm -f $(OBJS)

depend:;	@mkmf -f $(MAKEFILE) LIBRARY=$(LIBRARY) DEST=$(DEST)

extract:;	@ar xo $(DEST)/$(LIBRARY)

index:;		@ctags -wx $(HDRS) $(SRCS)

install:	$(LIBRARY)
		@echo Installing $(LIBRARY) in $(DEST)
		@cp $(LIBRARY) $(DEST)

library:        $(LIBRARY)

print:;		@$(PRINT) $(HDRS) $(SRCS)

tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

update:         $(DEST)/$(LIBRARY)

$(DEST)/$(LIBRARY): $(SRCS) $(HDRS) $(EXTHDRS)
		@-ar xo $(DEST)/$(LIBRARY)
		@$(MAKE) -f $(MAKEFILE) DEST=$(DEST) install clean
