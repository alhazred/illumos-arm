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
# Copyright 2019 Hayashi Naoyuki
#

TOPDIR =	../../../../..
DTS=hifive-unleashed-a00.dts
OBJS += gem.o sifive_gpio.o mmc.o
BOARD=	sifive

include ../Makefile.com
ROOT_PSM_DIR=$(ROOT_PLAT_DIR)/SUNW,sifive

.DELETE_ON_ERROR:

all: $(NFSBOOT_BIN) $(DTB)
$(NFS_SRT0) $(NFS_OBJS):  $(ASSYM_H)

install: all $(ROOT_PSM_NFSBOOT) $(ROOT_PSM_DTB)

CFLAGS += -I. -_gcc=-Wno-error=unused-function -_gcc=-Wno-error=unused-variable -_gcc=-Wno-error=unused-but-set-variable

include $(TOPDIR)/psm/Makefile.psm.64
include $(BOOTSRCDIR)/Makefile.rules

include ../Makefile.targ

VPATH += dts

%.dtb: %.dts
	cpp -nostdinc -I dts $(CPPINCS) -undef -x assembler-with-cpp $< | $(DTC) -o $@ -O dtb

$(ROOT_PSM_NFSBOOT): $(NFSBOOT_BIN) $(ROOT_PSM_DIR)
	mkimage -A riscv -O linux -C none  -T kernel -a 0x80400000 -e 0x80400000 -n 'OpenSolaris' -d $(NFSBOOT_BIN) $@

$(ROOT_PSM_DTB): $(DTB) $(ROOT_PSM_DIR)
	$(INS.file)
