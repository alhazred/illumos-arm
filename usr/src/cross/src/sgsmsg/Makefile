#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Copyright 2017 Hayashi Naoyuki
#

include ../Makefile.cross

PROG=		sgsmsg

MACHOBJS=	sgsmsg.o string_table.o avl.o findprime.o

CPPFLAGS+=	 -D__sun -I.
CFLAGS+=	-std=gnu89 -fno-builtin

all install clean: FRC

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(PROG) $(LIBRARY)

$(PROG): $(MACHOBJS)
	$(CC) -o $@ $(MACHOBJS)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)

clobber: clean

clean:
	$(RM) $(PROG) $(MACHOBJS) $(MACHOBJS:%.o=%.d)

FRC:
