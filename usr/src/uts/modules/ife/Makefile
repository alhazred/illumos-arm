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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
#

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..
include $(UTSBASE)/../Makefile.master

IFE_OBJS=gem.o ife_gem.o
#
#	Define the module and object file sets.
#
MODULE		= ife
OBJECTS		= $(IFE_OBJS:%=$(OBJS_DIR)/%)
ROOTMODULE	= $(ROOT_DRV_DIR)/$(MODULE)

include $(UTSBASE)/$(MACH)/Makefile.$(MACH)

CFLAGS		+= -D__INLINE__=inline -DGEM_GCC_RUNTIME \
		   -DVERSION='"2.6.2"' \
		   -DGEM_CONFIG_POLLING -DGEM_CONFIG_GLDv3 -DGEM_CONFIG_VLAN_HW \
		   -DGEM_CONFIG_CKSUM_OFFLOAD -DSOLARIS10 -DGEM_CONFIG_JUMBO_FRAME \
		   -UGEM_CONFIG_ND -DGEM_CONFIG_MAC_PROP \
		   -Unotdef -UNEVER -UGEM_GCC_RUNTIME \
		   -UGEM_COMPAT -USANITY -UGEM_CONFIG_FMA -UMODULE \
		   -UGEM_CONFIG_RX_DIRECT -DGEM_CONFIG_TX_DIRECT \
		   -DGEM_CONFIG_TX_HEAD_PTR \
		   -UOS_PUTBACK \
		   -DEXTEND_TCB -DMAP_MEM -DCONFIG_OPT_IO \
		   -UGEM_CONFIG_TXINTR_OPT -DGEM_CONFIG_TXSCHED_OPT

#
#	Include common rules.
#

#
#	Define targets
#
ALL_TARGET	= $(BINARY)
INSTALL_TARGET	= $(BINARY) $(ROOTMODULE)

#
#	Overrides
#
CERRWARN	+= -_gcc=-Wno-unused-label
CERRWARN	+= -_gcc=-Wno-unused-variable
CERRWARN	+= -_gcc=-Wno-unused-function
CERRWARN	+= -_gcc=-Wno-parentheses
CERRWARN	+= -_gcc=-Wno-format
CERRWARN	+= -_gcc=-Wno-switch

#
# Driver depends on GLD
#
NEEDED_MODULES	+=   misc/mac misc/mii
MAPFILES	+= mapfile-mod

#
#	Default build targets.
#

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

install:	$(INSTALL_DEPS)

#
#	Include common targets.
#
include $(UTSBASE)/$(MACH)/Makefile.targ
$(OBJS_DIR)/%.o: %.c
	$(COMPILE.c) -o $@ $<
