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

MANIFEST = ssh.xml
SVCMETHOD = sshd

include ../../cmd/Makefile.cmd

SOURCE=openssh-8.0p1.tar.gz
SOURCE_DIR=$(SOURCE:%.tar.gz=%)
#PATCH=$(SOURCE:%.tar.gz=%.patch)

CONFIG_ADDITIONAL_ARGS+= --sysconfdir=/etc/ssh --libexecdir=/usr/lib/ssh --sbindir=/usr/lib/ssh
CONFIG_ADDITIONAL_ARGS+= --with-privsep-user=nobody --with-privsep-path=/var/empty
CONFIG_ADDITIONAL_CFLAGS= -std=gnu90
INSTALL_TARGET= install-nosysconf
#CONFIG_ENV = ac_cv_func_arc4random=no ac_cv_func_arc4random_buf=no ac_cv_func_arc4random_uniform=no

include ../Makefile.oss

ETCSSHDIR=	$(ROOTETC)/ssh
ROOTMANIFESTDIR = $(ROOTSVCNETWORK)
FILES= sshd_config ssh_config

ETCSSHFILES=	$(FILES:%=$(ETCSSHDIR)/%)

post_install: $(ROOTMANIFEST) $(ROOTSVCMETHOD) $(ETCSSHFILES)

$(ROOTMANIFESTDIR)/%: $(SRC)/cmd/ssh/etc/%
	$(INS.file)

$(ROOTLIBSVCMETHOD)/%: $(SRC)/cmd/ssh/etc/%
	$(INS.file)

$(ETCSSHDIR)/%: etc/% $(ETCSSHDIR)
	$(INS.file)

$(ETCSSHDIR):
	$(INS.dir)
