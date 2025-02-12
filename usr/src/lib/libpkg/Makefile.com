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
# Copyright 2017 Peter Tribble.

#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2018, Joyent, Inc.

LIBRARY= libpkg.a
VERS=	.1

# include library definitions
OBJECTS=	\
		canonize.o   ckparam.o    ckvolseq.o \
		devtype.o    dstream.o    gpkglist.o \
		gpkgmap.o    isdir.o      logerr.o \
		mappath.o    ncgrpw.o     nhash.o \
		path_valid.o pkgexecl.o   pkgexecv.o \
		pkgmount.o   pkgtrans.o   ppkgmap.o \
		progerr.o    putcfile.o   rrmdir.o \
		runcmd.o     srchcfile.o  tputcfent.o \
		verify.o \
		vfpops.o     fmkdir.o     pkgstr.o \
		handlelocalfs.o	pkgserv.o


# include library definitions
include ../../Makefile.lib

SRCDIR=		../common

POFILE =	libpkg.po
MSGFILES =	$(OBJECTS:%.o=../common/%.i)
CLEANFILES +=   $(MSGFILES)



LIBS = $(DYNLIB)


LDLIBS +=	-lc -lscf -ladm

CFLAGS +=	$(CCVERBOSE)
CERRWARN +=	-_gcc=-Wno-parentheses
CERRWARN +=	$(CNOWARN_UNINIT)
CERRWARN +=	-_gcc=-Wno-clobbered
CERRWARN +=	-_gcc=-Wno-switch
pics/gpkgmap.o: CERRWARN += -_gcc=-Wno-overflow

# not linted
SMATCH=off

CPPFLAGS +=	-I$(SRCDIR) -D_FILE_OFFSET_BITS=64

.KEEP_STATE:

all:	$(LIBS)

$(POFILE): $(MSGFILES)
	$(BUILDPO.msgfiles)

_msg: $(MSGDOMAINPOFILE)


# include library targets
include $(SRC)/lib/Makefile.targ
include $(SRC)/Makefile.msg.targ
