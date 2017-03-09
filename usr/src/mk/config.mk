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

ifndef _CONFIG_MK
_CONFIG_MK=1

GCC_MAJOR_VER=10
GCC_MINOR_VER=1
GCC_PATCH_VER=0
GCC_VER=$(GCC_MAJOR_VER).$(GCC_MINOR_VER).$(GCC_PATCH_VER)
FILE_VER=5.36

#####
MACH=aarch64
#MACH=alpha
#MACH=riscv64

#####
MK_HOME=$(SRC)/mk

CROSS_TOOLS=$(SRC)/cross
CROSS_SRCS=$(SRC)/cross/src

#####
ROOT=		/data/proto/root_$(MACH)
PIC_BASE=	/tmp/solaris/$(MACH)/pic

#####
PICDIR=		$(PIC_BASE)$(subst $(abspath $(CODEMGR_WS)),,$(CURDIR))

export LANG=C
export LC_ALL=C
export LC_COLLATE=C
export LC_CTYPE=C
export LC_MESSAGES=C
export LC_MONETARY=C
export LC_NUMERIC=C
export LC_TIME=C

include ${MK_HOME}/${MACH}.mk

endif # ifndef _CONFIG_MK
