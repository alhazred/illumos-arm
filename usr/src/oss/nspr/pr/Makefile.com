#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
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
# Copyright 2017 Hayashi Naoyuki
#

include $(dir $(lastword $(MAKEFILE_LIST)))../../../Makefile.master

LIBRARY = libnspr4.a
VERS = .1
OBJECTS = \
	  prfdcach.o prmwait.o priometh.o pripv6.o prmapopt.o prlayer.o \
	  prlog.o prmmap.o prpolevt.o prprf.o prscanf.o prstdio.o \
	  prlink.o prmalloc.o prmem.o \
	  prosdep.o \
	  unix.o unix_errors.o uxproces.o uxrng.o uxshm.o uxwrap.o \
	  solaris.o os_SunOS_$(MACH).o \
	  prseg.o prshm.o prshma.o \
	  pralarm.o pratom.o prcountr.o prdtoa.o prenv.o prerr.o prerror.o \
	  prerrortable.o prinit.o prinrval.o pripc.o prlog2.o prlong.o \
	  prnetdb.o praton.o prolock.o prrng.o prsystem.o prtime.o \
	  prthinfo.o prtpool.o prtrace.o  \
	  prcmon.o prrwlock.o prtpd.o \
	  ptio.o ptsynch.o ptthread.o ptmisc.o
include ../../Makefile.nspr

HDRDIR=		$(NSPR_BASE)/pr/include
SRCDIR=		$(NSPR_BASE)/pr/src

LIBS =		$(DYNLIB)

ASFLAGS+= -D_ASM
CFLAGS += -_gcc=-fvisibility=hidden
CERRWARN +=	-_gcc=-Wno-unused-variable
CERRWARN +=	-_gcc=-Wno-unused-but-set-variable
LDLIBS += -lpthread -lsocket -lnsl -ldl
MAPFILE=$(SRCDIR)/nspr.def
MAPFILES=mapfile-vers

all: $(LIBS)
install_h: $(ROOTHDRS)
install: all $(ROOTLIBS) $(ROOTLINKS)

include $(SRC)/lib/Makefile.targ

$(LIBS): $(MAPFILES)
$(MAPFILES): $(MAPFILE)
	grep -v ';-' $< | sed -e 's,;+,,' -e 's; DATA ;;' -e 's,;;,,' -e 's,;.*,;,' > $@

CLEANFILES += $(MAPFILES)

pics/%.o: $(SRCDIR)/io/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/linking/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/malloc/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/md/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/md/unix/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/md/unix/%.s
	$(COMPILE.s) -c -o $@ $<

pics/%.o: $(SRCDIR)/memory/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/misc/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/threads/%.c
	$(COMPILE.c) -o $@ $<

pics/%.o: $(SRCDIR)/pthreads/%.c
	$(COMPILE.c) -o $@ $<

$(ROOTLIBDIR):
	$(INS.dir)
$(ROOTLIBS): $(ROOTLIBDIR)
