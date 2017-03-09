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

#
#	This makefile drives the production of the generic
#	unix kernel module.
#

CODEMGR_WS=$(shell git rev-parse --show-toplevel)
SRC=$(CODEMGR_WS)/usr/src
include $(SRC)/mk/config.mk

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Define the module and object file sets.
#
MODULE		=

OBJECTS		= $(GENUNIX_OBJS:%=$(OBJS_DIR)/%) \
		  $(NOT_YET_KMODS:%=$(OBJS_DIR)/%)

LINTS		= $(GENUNIX_OBJS:%.o=$(LINTS_DIR)/%.ln) \
		  $(NOT_YET_KMODS:%.o=$(LINTS_DIR)/%.ln)

ROOTMODULE	=

LIBGEN		= $(OBJS_DIR)/libgenunix.a
LIBSTUBS	= $(GENSTUBS_OBJS:%=$(OBJS_DIR)/%)

#
#	Include common rules.
#
include $(UTSBASE)/$(MACH)/Makefile.$(MACH)

GENASSYM_CF		= $(UTSBASE)/$(MACH)/ml/genassym.cf
ASSYM_H			= $(OBJS_DIR)/assym.h
AS_INC_PATH		+= -I$(OBJS_DIR)

#
#	Define targets
#
ALL_TARGET	= $(LIBGEN)
LINT_TARGET	= $(MODULE).lint
INSTALL_TARGET	= $(ALL_TARGET) #$(ROOTMODULE)

#
#	Overrides
#
CLEANFILES	+= $(LIBSTUBS) $(LIBGEN) $(ASSYM_H)
BINARY		=

CPPFLAGS	+= -I$(SRC)/common
CPPFLAGS	+= -I$(SRC)/uts/common/fs/zfs

CPPFLAGS	+= -I$(UTSBASE)/$(MACH)pf

#
# For now, disable these lint checks; maintainers should endeavor
# to investigate and remove these for maximum lint coverage.
# Please do not carry these forward to new Makefiles.
#
LINTTAGS	+= -erroff=E_SUSPICIOUS_COMPARISON
LINTTAGS	+= -erroff=E_BAD_PTR_CAST_ALIGN
LINTTAGS	+= -erroff=E_SUPPRESSION_DIRECTIVE_UNUSED
LINTTAGS	+= -erroff=E_STATIC_UNUSED
LINTTAGS	+= -erroff=E_PTRDIFF_OVERFLOW
LINTTAGS	+= -erroff=E_ASSIGN_NARROW_CONV

CERRWARN	+= -_gcc=-Wno-unused-label
CERRWARN	+= -_gcc=-Wno-unused-variable
CERRWARN	+= -_gcc=-Wno-unused-value
CERRWARN	+= -_gcc=-Wno-unused-function
CERRWARN	+= -_gcc=-Wno-parentheses
CERRWARN	+= -_gcc=-Wno-switch
CERRWARN	+= -_gcc=-Wno-type-limits
CERRWARN	+= -_gcc=-Wno-uninitialized
CERRWARN	+= -_gcc=-Wno-clobbered
CERRWARN	+= -_gcc=-Wno-empty-body
CERRWARN	+= -_gcc=-Wno-override-init

#
# Ensure that lint sees 'struct cpu' containing a fully declared
# embedded 'struct machcpu'
#
LINTFLAGS	+= -D_MACHDEP -I../../$(MACH)pf

#
#	Default build targets.
#
.KEEP_STATE:

def:		$(DEF_DEPS)

all:		$(ALL_DEPS)

clean:		$(CLEAN_DEPS)

clobber:	$(CLOBBER_DEPS)

lint:		$(LINT_DEPS)

modlintlib:	$(MODLINTLIB_DEPS)

clean.lint:	$(CLEAN_LINT_DEPS)

install:	$(INSTALL_DEPS)

$(LIBGEN):	$(OBJECTS) $(LIBSTUBS)
	$(AR) $(ARFLAGS) -o $@ $(OBJECTS) $(LIBSTUBS)

#
#	Include common targets.
#
include $(UTSBASE)/$(MACH)/Makefile.targ

#
# Override.
#
$(MODULE).lint :  GEN_LINT_LIB =
