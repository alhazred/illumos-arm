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

PROG=		ndrgen1

MACHOBJS=	ndr_main.o ndr_lex.o ndr_anal.o \
	ndr_gen.o ndr_parse.o ndr_print.o string.o

CPPFLAGS+=	-D__sun -I.
CFLAGS+=	-std=gnu89 -fno-builtin
YACC=		$(BINDIR)/yacc

all install clean: FRC

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(PROG) $(LIBRARY)

$(PROG): $(MACHOBJS)
	$(CC) -o $@ $(MACHOBJS) -L$(LIBDIR) -ly

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)
	$(INSTALL) -m 755 ndrgen.sh $(BINDIR)/ndrgen

clobber: clean

clean:
	$(RM) $(PROG) $(MACHOBJS) $(MACHOBJS:%.o=%.d) y.tab.c y.tab.h

$(MACHOBJS): y.tab.c

y.tab.c: ndr_parse.y
	$(YACC) -d ndr_parse.y

ndr_parse.o : y.tab.c
	$(CC) -c -o $@ $<

FRC:
