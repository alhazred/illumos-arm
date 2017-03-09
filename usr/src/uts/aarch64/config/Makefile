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
# uts/$(MACH)/config/Makefile
# Copyright 2017 Hayashi Naoyuki
# Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
# Copyright 2015 Igor Kozhukhov <ikozhukhov@gmail.com>
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#	This makefile drives the production of the kernel configuration files.
#
#	$(MACH) architecture dependent
#

CODEMGR_WS=$(shell git rev-parse --show-toplevel)
SRC=$(CODEMGR_WS)/usr/src
include $(SRC)/mk/config.mk

#
#	Path to the base of the uts directory tree (usually /usr/src/uts).
#
UTSBASE	= ../..

#
#	Include common rules.
#
include $(UTSBASE)/$(MACH)/Makefile.$(MACH)

#
#	Define the module and object file sets.
#
SRCDIR		= $(UTSBASE)/$(MACH)/os
PATH2INST	= path_to_inst
CONFIGFILES	= name_to_major minor_perm driver_aliases name_to_sysnum \
		  driver_classes dacf.conf $(PATH2INST) mach
SECCONFIG	= device_policy priv_names
BOOTENVRC	= bootenv.rc
DEVMASTER	= master
FILEMODE	= 644
ROOTETC		= $(ROOT)/etc
ROOTETCSEC	= $(ROOTETC)/security
ROOTBOOT	= $(ROOT)/boot
BOOTSOLARIS	= $(ROOTBOOT)/solaris
BOOTDEVICEDB	= $(BOOTSOLARIS)/devicedb

SRCFILES	= $(CONFIGFILES:%=$(SRCDIR)/%) $(SECCONFIG:%=$(SRCDIR)/%)
SRCFILES	+= $(BOOTENVRC:%=$(SRCDIR)/%)
SRCFILES	+= $(DEVMASTER:%=$(SRCDIR)/%)
ROOTCONFIGFILES	= $(CONFIGFILES:%=$(ROOTETC)/%) $(SECCONFIG:%=$(ROOTETCSEC)/%)
ROOTCONFIGFILES	+= $(BOOTENVRC:%=$(BOOTSOLARIS)/%)
ROOTCONFIGFILES	+= $(DEVMASTER:%=$(BOOTDEVICEDB)/%)

#
#	Default build targets.
#
.KEEP_STATE:

def all: $(SRCFILES)

install: all $(ROOTCONFIGFILES) $(ROOTALTBOOTIMAGE)

#
#	The ROOTETC directory is made by ../../../Targetdirs
#	through the rootdirs target in ../../../Makefile.
#
#	etc/path_to_inst has permissions different from the other config files
#
$(ROOTETC)/$(PATH2INST):  FILEMODE=444

.PHONY: $(ROOTETC)
$(ROOTETC):
	$(INS) -s -d -m $(DIRMODE) $@

$(ROOTETC)/%: $(SRCDIR)/% $(ROOTETC)
	$(INS.file)

.PHONY: $(ROOTETCSEC)
$(ROOTETCSEC):
	$(INS) -s -d -m $(DIRMODE) $@

$(ROOTETCSEC)/%: $(SRCDIR)/% $(ROOTETCSEC)
	$(INS.file)

$(ROOTBOOT):
	$(INS) -s -d -m $(DIRMODE) $@

$(ROOTBOOT)/%: $(SRCDIR)/% $(ROOTBOOT)
	$(INS.file)

.PHONY: $(BOOTSOLARIS)
$(BOOTSOLARIS):
	$(INS) -s -d -m $(DIRMODE) $@

$(BOOTSOLARIS)/%: $(SRCDIR)/% $(BOOTSOLARIS)
	$(INS.file)

.PHONY: $(BOOTDEVICEDB)
$(BOOTDEVICEDB):
	$(INS) -s -d -m $(DIRMODE) $@

$(BOOTDEVICEDB)/%: $(SRCDIR)/% $(BOOTDEVICEDB)
	$(INS.file)

$(SRCDIR)/priv_names: $(PRIVS_DEF) $(PRIVS_AWK)
	$(AWK) -f $(PRIVS_AWK) < $(PRIVS_DEF) -v pnamesfile=$@

clean lint:
clobber: clobber.targ

# 
#	No lints, so keep the clobber and clean targets reasonable. 
# 
CLEANLINTFILES	= Nothing_to_remove
CLEANFILES	= Nothing_to_remove
CLOBBERFILES	= $(SRCDIR)/priv_names

#
#	Include common targets.
#
include $(UTSBASE)/$(MACH)/Makefile.targ
