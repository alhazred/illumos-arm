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

PROG=		yacc
LIBRARY=	liby.a

MACHOBJS=	y1.o y2.o y3.o y4.o
OBJECTS=	libmai.o libzer.o

CPPFLAGS+=	-DPARSER='"$(LIBDIR)/yaccpar"'
CFLAGS+=	-std=gnu99

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
	$(INSTALL) -m 644 yaccpar $(LIBDIR)/yaccpar
	$(INSTALL) -m 644 $(LIBRARY) $(LIBDIR)/$(LIBRARY)

clobber: clean

clean:
	$(RM) $(PROG) $(LIBRARY) $(OBJECTS) $(MACHOBJS) $(OBJECTS:%.o=%.d) $(MACHOBJS:%.o=%.d)

FRC:
