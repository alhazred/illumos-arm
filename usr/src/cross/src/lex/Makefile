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

PROG=		lex
LIBRARY=	libl.a

MACHOBJS=	main.o sub1.o sub2.o sub3.o header.o parser.o
LIBOBJS=	allprint.o libmain.o reject.o yyless.o yywrap.o
LIBOBJS_W=	allprint_w.o reject_w.o yyless_w.o
LIBOBJS_E=	reject_e.o yyless_e.o
OBJECTS=	$(LIBOBJS) $(LIBOBJS_W) $(LIBOBJS_E)

LEXDIR=		$(LIBDIR)/lex

CPPFLAGS+=	-DELF
CPPFLAGS+=	-DCNAME='"$(LEXDIR)/ncform"'
CPPFLAGS+=	-DRATNAME='"$(LEXDIR)/nrform"'
CPPFLAGS+=	-DEUCNAME='"$(LEXDIR)/nceucform"'
CFLAGS+=	-std=gnu99 -fno-builtin -Wno-unprototyped-calls
YACC=		$(BINDIR)/yacc

all install clean: FRC

%_w.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DEUC -DJLSLEX  -DWOPTION -D$*=$*_w -c -o $@ $<

%_e.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DEUC -DJLSLEX  -DEOPTION -D$*=$*_e -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

all: $(PROG) $(LIBRARY)

$(PROG): $(MACHOBJS)
	$(CC) -o $@ $(MACHOBJS) -L$(LIBDIR) -ly

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

install: all
	test -d $(BINDIR) || mkdir -p $(BINDIR)
	test -d $(LIBDIR) || mkdir -p $(LIBDIR)
	test -d $(LEXDIR) || mkdir -p $(LEXDIR)
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)
	$(INSTALL) -m 644 $(LIBRARY) $(LIBDIR)/$(LIBRARY)
	$(INSTALL) -c -m 644 ncform $(LEXDIR)/ncform
	$(INSTALL) -c -m 644 nceucform $(LEXDIR)/nceucform
	$(INSTALL) -c -m 644 nrform $(LEXDIR)/nrform

clean:
	$(RM) $(PROG) $(LIBRARY) $(OBJECTS) $(MACHOBJS) $(OBJECTS:%.o=%.d) $(MACHOBJS:%.o=%.d)

clobber: clean

FRC:
