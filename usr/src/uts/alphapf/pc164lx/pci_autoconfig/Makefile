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
#

UTSBASE	= ../../..
include $(UTSBASE)/../Makefile.master

BOARD		= pc164lx

MODULE		= pci_autoconfig
OBJECTS		= $(PCI_AUTOCONFIG_OBJS:%=$(OBJS_DIR)/%)
GENASSYM_CF	= $(UTSBASE)/$(MACH)pf/ml/genassym.cf
ROOTMODULE	= $(ROOT_PSM_MISC_DIR)/$(MODULE)

include ../Makefile.$(BOARD)

ALL_TARGET	= $(BINARY)
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

install:	$(INSTALL_DEPS)

include ../Makefile.targ
