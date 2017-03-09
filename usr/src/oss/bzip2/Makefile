#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
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

include ../../Makefile.master

SOURCE=bzip2-1.0.6.tar.gz
SOURCE_DIR=$(SOURCE:%.tar.gz=%)
#PATCH=$(SOURCE:%.tar.gz=%.patch)

CC_FOR_TARGET=  $(GNU_CC) -fPIC --sysroot=$(ROOT) -isystem $(ROOT)/usr/include

SRCDIR?=$(DISTFILES)
PATCHDIR?=$(CURDIR)/patch
BUILDDIR?=$(PICDIR)/$(SOURCE_DIR)

.PHONY: all expand post_expand install build config install_h clean clobber install_dst post_install
.PHONY: pre_config post_patch apply_patch post_expand expand patch

all: build

install: all install_dst post_install

install_dst: all
	cd $(BUILDDIR) ; \
		$(MAKE) $(MAKE_ARGS) PREFIX=$(ROOT)/usr install

build: expand
	cd $(BUILDDIR); \
		$(MAKE) CC="$(CC_FOR_TARGET)" $(MAKE_ARGS) bzip2

clean clobber:
	$(RM) -rf $(PICDIR)

pre_config: post_patch

post_patch: apply_patch

apply_patch: post_expand
	cd $(PICDIR)/$(SOURCE_DIR); for p in $(PATCH:%=$(PATCHDIR)/%) ; do patch -p0 < $${p}; done

post_expand: expand

expand:
	$(RM) -rf $(PICDIR)
	mkdir -p $(BUILDDIR)
	cd $(PICDIR); tar -xf $(SOURCE:%=$(SRCDIR)/%)

install_h:
post_install: install_dst

patch:
	diff -cNr orig/$(SOURCE_DIR) $(SOURCE_DIR) > patch/$(PATCH); echo

