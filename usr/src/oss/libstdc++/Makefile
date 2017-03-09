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

SOURCE_DIR=$(EXTRA)/gcc

SRCDIR=$(CROSS_SRCS)/gcc/src
PATCHDIR=$(CROSS_SRCS)/gcc/patch
BUILDDIR=$(PICDIR)/build

CONFIG_ADDITIONAL_ARGS+= --with-stabs --enable-threads --disable-tls --disable-multilib
CONFIG_SUBDIR=libstdc++-v3
CONFIG_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no
MAKE_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no

include ../Makefile.oss

