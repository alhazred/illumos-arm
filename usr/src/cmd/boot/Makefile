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
# Copyright 2017 Hayashi Naoyuki
# Copyright 2016 Toomas Soome <tsoome@me.com>
# Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
#

include ../Makefile.cmd

.PARALLEL:

COMMON_SUBDIRS=		\
	filelist	\
	fiocompress	\
	scripts		\
	bootadm

i386_SUBDIRS=		\
	installboot \
	installgrub	\
	symdef

alpha_SUBDIRS=		\
	installboot_alpha

aarch64_SUBDIRS=

riscv64_SUBDIRS=

sparc_SUBDIRS=		\
	installboot

COMMON_LINTSUBDIRS=	\
	bootadm		\
	installboot

i386_LINTSUBDIRS=	\
	fiocompress	\
	installboot \
	installgrub	\
	symdef

sparc_LINTSUBDIRS=	\
	installboot

COMMON_MSGSUBDIRS=	\
	bootadm		\
	fiocompress

i386_MSGSUBDIRS=	\
	installboot \
	installgrub

sparc_MSGSUBDIRS=	\
	installboot


all: 		TARGET= all
install: 	TARGET= install
clean: 		TARGET= clean
clobber: 	TARGET= clobber
lint: 		TARGET= lint
_msg: 		TARGET= _msg

.KEEP_STATE:

SUBDIRS = $(COMMON_SUBDIRS) $($(MACH)_SUBDIRS)
LINTSUBDIRS = $(COMMON_LINTSUBDIRS) $($(MACH)_LINTSUBDIRS)
MSGSUBDIRS = $(COMMON_MSGSUBDIRS) $($(MACH)_MSGSUBDIRS)

all clean clobber: $(SUBDIRS)

install: $(SUBDIRS)

lint: $(LINTSUBDIRS)

_msg: $(MSGSUBDIRS)

FRC:

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)
