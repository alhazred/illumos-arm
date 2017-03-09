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

SOURCE=perl-$(PERL_VERSION).tar.gz
CROSS_SOURCE=perl-cross-1.3.tar.gz
SOURCE_DIR=$(SOURCE:%.tar.gz=%)
#PATCH=$(SOURCE:%.tar.gz=%.patch)

CONFIG_ARGS= --target=$(MACH)-solaris2.11
CONFIG_ARGS+= -Dprefix=/usr/perl5/$(PERL_VERSION)
CONFIG_ARGS+= -Dprivlib=/usr/perl5/$(PERL_VERSION)/lib
CONFIG_ARGS+= -Dsitelib=/usr/perl5/site_perl/$(PERL_VERSION)
CONFIG_ARGS+= -Dvendorprefix=/usr/perl5/$(PERL_VERSION)
CONFIG_ARGS+= -Dvendorlib=/usr/perl5/vendor_perl/$(PERL_VERSION)
CONFIG_ARGS+= -Dlibperl=libperl.so \
CONFIG_ARGS+= -Duseshrplib \
CONFIG_ARGS+= -Duse64bitint \
CONFIG_ARGS+= -Dcf_email="perl-bugs@sun.com" \
CONFIG_ARGS+= -Dcf_by="perl-bugs" \
CONFIG_ARGS+= -Dmyhostname="localhost" \
CONFIG_ARGS+= -Dcc="$(GNU_CC)"
CONFIG_ARGS+= -Dld="$(GNU_CC)"
CONFIG_ARGS+= -Dar="$(AR)"
CONFIG_ARGS+= -Dranlib="$(RANLIB)"
CONFIG_ARGS+= -Dobjdump="$(OD)"
CONFIG_ARGS+= -Dreadelf="$(READELF)"
CONFIG_ARGS+= -Dccflags="-W -fPIC -isystem $(ROOT)/usr/include --sysroot=$(ROOT)"
CONFIG_ARGS+= -Dldflags="-lnsl -lsocket  -lresolv -lm --sysroot=$(ROOT)"
CONFIG_ARGS+= -Dlddlflags="-shared -lnsl -lsocket -lresolv -lm --sysroot=$(ROOT)"
CONFIG_ARGS+= --disable-mod=ext/Errno
BUILDDIR=$(PICDIR)/$(SOURCE_DIR)
PATCHDIR?=$(CURDIR)/patch
SRCDIR=$(EXTRA)/distfiles
INSTALL_TARGET=install.perl
TMPINSTALLDIR=$(PICDIR)/install

.PHONY: all expand post_expand install build config install_h clean clobber install_dst post_install
.PHONY: pre_config post_patch apply_patch post_expand expand patch

all: build

install: post_install

post_install: install_dst
post_install: ${ROOT}/usr/perl5/bin
post_install: ${ROOT}/usr/perl5/lib
post_install: ${ROOT}/usr/bin/perl

.PHONY: $(PERL_VERSION)/bin
.PHONY: $(PERL_VERSION)/lib
.PHONY: ../perl5/$(PERL_VERSION)/bin/perl
${ROOT}/usr/perl5/bin: $(PERL_VERSION)/bin install_dst
	${INS.symlink}

${ROOT}/usr/perl5/lib: $(PERL_VERSION)/lib install_dst
	${INS.symlink}

${ROOT}/usr/bin/perl: ../perl5/$(PERL_VERSION)/bin/perl install_dst
	${INS.symlink}

install_dst: all
	cd $(BUILDDIR) ; $(MAKE) $(MAKE_ARGS) -j 1 DESTDIR=$(TMPINSTALLDIR) $(INSTALL_TARGET)
	find $(TMPINSTALLDIR) -name "*.la" -exec rm {} \;
	[ x$(WITH_STATIC) = x1 ] || find $(TMPINSTALLDIR) -name "*.a" -exec rm -f {} \;
	tar -cf - -C $(TMPINSTALLDIR) . | tar -xf - -C $(ROOT)

build: config
	cd $(BUILDDIR); $(MAKE) || $(MAKE) || $(MAKE) || $(MAKE) -j 1

config: pre_config
	cd $(BUILDDIR); \
		$(PICDIR)/$(SOURCE_DIR)/configure $(CONFIG_ARGS)

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

patch:
	diff -cNr orig/$(SOURCE_DIR) $(SOURCE_DIR) > patch/$(PATCH); echo

post_expand:
	cd $(PICDIR); tar -xf $(CROSS_SOURCE:%=$(SRCDIR)/%)
	cd $(PICDIR)/$(CROSS_SOURCE:%.tar.gz=%); tar -cf - . | tar -xf - -C $(PICDIR)/$(SOURCE_DIR)

