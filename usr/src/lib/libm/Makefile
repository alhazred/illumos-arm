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
# Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
# Copyright 2017 Hayashi Naoyuki
#

LIBRARY=	libm.a
VERS=		.2

# include common library definitions
include ../Makefile.lib

SUBDIRS_i386 = $(MACH)
SUBDIRS_sparc = $(MACH)
SUBDIRS_alpha = $(SRC)/oss/msun
SUBDIRS_aarch64 = $(SRC)/oss/msun
SUBDIRS_riscv64 = $(SRC)/oss/msun
SUBDIRS = $(SUBDIRS_$(MACH))
$(BUILD64)SUBDIRS += $(MACH64)

all	: 	TARGET= all
install	: 	TARGET= install
clean	: 	TARGET= clean
clobber	: 	TARGET= clobber

.KEEP_STATE:

.PARALLEL: $(SUBDIRS)

all clean clobber install: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; VERSION='$(VERSION)' $(MAKE) $(TARGET)

FRC:

include $(SRC)/lib/Makefile.targ
