#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright 2012 Joyent, Inc.  All rights reserved.
# Copyright 2017 Hayashi Naoyuki
#

LIBRARY =	libipd.a
VERS =		.1
OBJECTS =	libipd.o

include ../../Makefile.lib

LIBS =		$(DYNLIB)
LDLIBS +=	-lc
CPPFLAGS +=	-I../common

SRCDIR =	../common

.KEEP_STATE:

all:	$(LIBS)


include ../../Makefile.targ
