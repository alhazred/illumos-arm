#
# Copyright 2017 Hayashi Naoyuki
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include ../Makefile.cross
include Makefile.common

SRC=../../..

CPPFLAGS += -I. -I$(SRC)/cmd/zic
all install clean: FRC

%.o: $(SRC)/cmd/zic/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(PROG)

$(OBJS): tzfile.h

tzfile.h:
	$(CP) $(SRC)/head/tzfile.h .

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)

clean:
	$(RM) $(PROG) $(OBJS) tzfile.h

clobber: clean

FRC:
