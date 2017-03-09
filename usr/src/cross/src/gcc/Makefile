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

SOURCE_DIR=$(BUILD_BASE)/gcc

MPC=mpc-1.1.0
GMP=gmp-6.1.2
MPFR=mpfr-4.0.2

CONFIG_CFLAGS=
CONFIG_ARGS+= --target=$(MACH)-solaris2.11
CONFIG_ARGS+= --disable-nls --enable-languages=c,c++ --disable-shared \
	      --enable-c99 --enable-long-long \
	      --disable-libquadmath --disable-libmudflap --disable-libgomp \
	      --disable-decimal-float --disable-libatomic --disable-libitm \
	      --disable-libquadmath --disable-libsanitizer --disable-libvtv --disable-libcilkrts \
	      --with-system-zlib --enable-__cxa_atexit

CONFIG_ARGS+= --with-mpfr-include=$(SOURCE_DIR)/mpfr/src
CONFIG_ARGS+= --with-mpfr-lib=$(BUILDDIR)/mpfr/src/.libs

CONFIG_ARGS_alpha += --with-cpu=$(MCPU) --with-tune=$(MCPU)
CONFIG_ARGS_arm += --enable-sjlj-exceptions --enable-interwork --enable-multilib
CONFIG_ARGS_arm += --with-cpu=$(MCPU) --with-fpu=$(MFPU)
CONFIG_ARGS+= --with-headers=$(ROOT)/usr/include --with-gnu-as --with-gnu-ld --enable-threads --disable-tls
CONFIG_ARGS+= --prefix=$(CROSS_TOOLS)
CONFIG_ARGS+= --enable-initfini-array

CFLAGS_FOR_TARGET=-I$(ROOT)/usr/include

CONFIG_ARGS += $(CONFIG_ARGS_$(MACH))
CONFIG_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no
MAKE_ENV= \
	    gcc_cv_objdump=$(OD) \
	    gcc_cv_c_no_fpie=no \
	    ac_cv_prog_XSLTPROC=no

include ../Makefile.oss
post_expand:
	cd $(EXTRA); tar -cf - gcc | tar -xf - -C $(BUILD_BASE)
	cd $(BUILD_BASE); tar -xf $(DISTFILES)/$(MPC).tar.gz;  ln -s $(BUILD_BASE)/$(MPC)  $(SOURCE_DIR)/mpc
	cd $(BUILD_BASE); tar -xf $(DISTFILES)/$(GMP).tar.xz;  ln -s $(BUILD_BASE)/$(GMP)  $(SOURCE_DIR)/gmp
	cd $(BUILD_BASE); tar -xf $(DISTFILES)/$(MPFR).tar.xz; ln -s $(BUILD_BASE)/$(MPFR) $(SOURCE_DIR)/mpfr
