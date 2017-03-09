#
# Copyright 2017 Hayashi Naoyuki
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include ../Makefile.cross
PROG=		vtfontcvt

OBJS=		vtfontcvt.o \
		lz4.o
SRCS=		$(OBJS:%.o=%.c)

SRC=../../..

all install clean: FRC

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)

clean:
	$(RM) $(PROG) $(OBJS)

clobber: clean

FRC:
