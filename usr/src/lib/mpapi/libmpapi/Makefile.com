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
# Copyright 2017 Hayashi Naoyuki
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.

LIBRARY=	libMPAPI.a
VERS=		.1
OBJECTS=	mpapi.o mpapi-sun.o

include ../../../Makefile.lib
include ../../../Makefile.rootfs

SRCDIR =	../common

LIBS =		$(DYNLIB)
LDLIBS +=	-lc


CFLAGS +=	$(CCVERBOSE)
CPPFLAGS +=	-I$(SRCDIR) -mt $(CCVERBOSE) -D_POSIX_PTHREAD_SEMANTICS
CPPFLAGS +=	-DBUILD_TIME='"Wed Sep 24 12:00:00 2008"'
DYNFLAGS +=	-Wl,-zfiniarray=ExitLibrary
DYNFLAGS +=	-Wl,-zinitarray=InitLibrary

CERRWARN +=	-_gcc=-Wno-type-limits
CERRWARN +=	-_gcc=-Wno-unused-variable
CERRWARN +=	$(CNOWARN_UNINIT)

# not linted
SMATCH=off

ROOTETC =	$(ROOT)/etc
CONFDIR =	../common
CONFFILE =	$(CONFDIR)/mpapi.conf
IETCFILES =     $(CONFFILE:$(CONFDIR)/%=$(ROOTETC)/%)
$(IETCFILES) :  FILEMODE = 0644

.KEEP_STATE:

all: $(LIBS) $(IETCFILES)


$(ROOTETC)/%:	$(CONFDIR)/%
	$(INS.file)

include ../../../Makefile.targ
