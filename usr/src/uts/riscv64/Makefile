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
# uts/$(MACH)/Makefile
#
# Copyright (c) 1999, 2010, Oracle and/or its affiliates. All rights reserved.
# Copyright 2015 Igor Kozhukhov <ikozhukhov@gmail.com>
# Copyright 2017 Hayashi Naoyuki
#
#	This makefile drives the production of all implementation architecture
#	independent modules for Intel processors.

CODEMGR_WS=$(shell git rev-parse --show-toplevel)
SRC=$(CODEMGR_WS)/usr/src
include $(SRC)/mk/config.mk

UTSBASE = ..

include Makefile.$(MACH)

LINT_LIBS	=

#
# dprov is delivered in the SUNWcryptoint package.
#

#
#
def		: 	TARGET= def
def.prereq	: 	TARGET= def
all		: 	TARGET= all
all.prereq	: 	TARGET= all
install		: 	TARGET= install
install.prereq	: 	TARGET= install
clean		: 	TARGET= clean
clean.prereq	: 	TARGET= clean
clobber		: 	TARGET= clobber
clobber.prereq	: 	TARGET= clobber
lint		: 	TARGET= lint
lint.prereq	: 	TARGET= lint
modlintlib	: 	TARGET= modlintlib
modlist		: 	TARGET= modlist
modlist		: 	NO_STATE= -K $$MODSTATE$$$$
clean.lint	: 	TARGET= clean.lint
check		: 	TARGET= check
install_h	: 	TARGET= install_h
install_h.prereq	: 	TARGET= install_h

.KEEP_STATE:

.PARALLEL:	$(PARALLEL_KMODS) $(XMODS) config $(LINT_DEPS)

def all install clean clobber modlist: $(KMODS) $(XMODS) config

clobber: clobber.targ

#
# Privilege constants
#
# NOTE: The rules for generating priv_const.c file are shared between all
# processor architectures and and should be kept in sync. If they are changed in
# this file make sure that sparc rules are updated as well.
#
PRIVS_C = $(SRC)/uts/common/os/priv_const.c

$(PRIVS_C): $(PRIVS_AWK) $(PRIVS_DEF)
	$(AWK) -f $(PRIVS_AWK) < $(PRIVS_DEF) cfile=$@

CLOBBERFILES += $(PRIVS_C)

#
# Prerequisites
#
all.prereq install.prereq install_h.prereq def.prereq clean.prereq clobber.prereq: genunix FRC

lint.prereq: FRC

#
# Nothing to do for any other prerequisite targets.
#
%.prereq:

install_h: $(PRIVS_C)

modlintlib clean.lint: $(LINT_KMODS) $(XMODS)

$(KMODS) $(SUBDIRS) config:	FRC
	@cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET)

$(XMODS):	FRC
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(NO_STATE) $(TARGET); \
	else \
		true; \
	fi

install_h check:	FRC
	@cd sys; pwd; $(MAKE) $(TARGET)
	@cd asm; pwd; $(MAKE) $(TARGET)

#
# Work-around to disable acpica global crosscheck lint warnings
#
LGREP.$(MACH) =	grep -v '$(MACH)/io/acpica'

#
#	Full kernel lint target.
#
LINT_TARGET	= globallint

# workaround for multiply defined errors
globallint :  LINTFLAGS += -erroff=E_NAME_MULTIPLY_DEF2

globallint:
	@pwd
	@-$(ECHO) "\nFULL KERNEL: global crosschecks:"
	@-$(LINT) $(LINTFLAGS) $(LINT_LIBS) 2>&1 | $(LGREP.$(MACH)) | $(LGREP.2)

lint:	modlintlib .WAIT $(LINT_DEPS)

include ../Makefile.targ
