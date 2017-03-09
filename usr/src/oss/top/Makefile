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

SOURCE=top-3.7.tar.gz
SOURCE_DIR=$(SOURCE:%.tar.gz=%)
PATCH=$(SOURCE:%.tar.gz=%.patch)

MAKE_ARGS = "SIGNAL=$(ROOT)/usr/include/sys/signal.h"
BUILDDIR=$(SOURCE_DIR)

include ../Makefile.oss

post_config:
	cd $(BUILDDIR); \
		$(SED) -e 's/OSREV.*$$/OSREV	511/' -e's/OSMAJOR.*$$/OSMAJOR	5/' config.h > config.h.new
	cd $(BUILDDIR); \
		$(MV) config.h.new config.h
