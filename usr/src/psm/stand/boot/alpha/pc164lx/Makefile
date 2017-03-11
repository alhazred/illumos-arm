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

TOPDIR =	../../../../..
DTS=
OBJS += pci_dev.o sil3124.o
BOARD= pc164lx

include ../Makefile.com
ROOT_PSM_DIR=$(ROOT_PLAT_DIR)/SUNW,pc164lx

.DELETE_ON_ERROR:

all: $(NFSBOOT) $(DTB)
$(NFS_SRT0) $(NFS_OBJS):  $(ASSYM_H)

install: all $(ROOT_PSM_NFSBOOT)

include $(TOPDIR)/psm/Makefile.psm.64
include $(BOOTSRCDIR)/Makefile.rules

include ../Makefile.targ

$(NFSBOOT): $(NFSBOOT_BIN)
	$(RM) $@
	$(CP) $< $@

$(ROOT_PSM_NFSBOOT): $(NFSBOOT) $(ROOT_PSM_DIR)
	$(INS.file)
