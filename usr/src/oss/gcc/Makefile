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

SOURCE_DIR=$(PICDIR)/gcc

MPC=mpc-1.1.0
GMP=gmp-6.1.2
MPFR=mpfr-4.0.2

BUILDDIR=$(PICDIR)/build

CONFIG_ADDITIONAL_ARGS+= --target=$(MACH)-solaris2.11
CONFIG_ADDITIONAL_ARGS+= --enable-languages=c,c++ --with-stabs --enable-threads --disable-tls
CONFIG_ADDITIONAL_ARGS+= --enable-libssp --disable-multilib
CONFIG_ADDITIONAL_ARGS+= --with-mpfr-include=$(SOURCE_DIR)/mpfr/src
CONFIG_ADDITIONAL_ARGS+= --with-mpfr-lib=$(BUILDDIR)/mpfr/src/.libs
CONFIG_ADDITIONAL_ARGS+= --enable-initfini-array --enable-__cxa_atexit
CONFIG_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no
MAKE_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no

WITH_STATIC=1

include ../Makefile.oss
post_expand:
	cd $(EXTRA); tar -cf - gcc | tar -xf - -C $(PICDIR)
	tar -xf $(DISTFILES)/$(MPC).tar.gz  -C $(PICDIR); ln -s $(PICDIR)/$(MPC)  $(SOURCE_DIR)/mpc
	tar -xf $(DISTFILES)/$(GMP).tar.xz  -C $(PICDIR); ln -s $(PICDIR)/$(GMP)  $(SOURCE_DIR)/gmp
	tar -xf $(DISTFILES)/$(MPFR).tar.xz -C $(PICDIR); ln -s $(PICDIR)/$(MPFR) $(SOURCE_DIR)/mpfr

