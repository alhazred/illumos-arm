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

DTC_BASE= $(EXTRA)/dtc

PROG=		dtc
LIBRARY=	libfdt.a

MACHOBJS=	dtc.o flattree.o fstree.o data.o livetree.o treesource.o srcpos.o \
		checks.o util.o dtc-lexer.lex.o dtc-parser.tab.o
OBJECTS=	fdt.o fdt_ro.o fdt_wip.o fdt_sw.o fdt_rw.o fdt_strerror.o fdt_empty_tree.o

CPPFLAGS+=	-D__sun -I$(DTC_BASE)/libfdt -I$(DTC_BASE) -I.
CFLAGS+=	-std=gnu99 -fno-builtin
YACC=		bison
LEX=		flex

all install clean: FRC

%.o: $(DTC_BASE)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: $(DTC_BASE)/libfdt/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(PROG) $(LIBRARY)

$(PROG): $(MACHOBJS) $(LIBRARY)
	$(CC) -o $@ $(MACHOBJS) -ly -L. -lfdt

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)

clean:
	$(RM) $(PROG) $(LIBRARY) $(OBJECTS) $(MACHOBJS) $(OBJECTS:%.o=%.d) $(MACHOBJS:%.o=%.d) \
		dtc-parser.tab.c dtc-parser.tab.h dtc-lexer.lex.c

dtc-parser.tab.c: $(DTC_BASE)/dtc-parser.y
	$(YACC) -d $<

dtc-lexer.lex.c: $(DTC_BASE)/dtc-lexer.l dtc-parser.tab.c
	$(LEX) -o $@ $<

clobber: clean

FRC:
