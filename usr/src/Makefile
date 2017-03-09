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

include Makefile.master
include Targetdirs

.DEFAULT_GOAL := def

.PHONY: pre_build def install_h clean post_install clean_head tools
def: pre_build cross post_install

pre_build: $(ROOTDIRS)
	$(INS) -d -m 775 $(ROOT)/var/mail/:saved

tools:
	$(MAKE) -C $(CROSS_SRCS) install

.PHONY: install_h install_h.head install_h.uts install_h.lib
install_h: install_h.head install_h.uts install_h.lib pre_build
install_h.head install_h.uts install_h.lib: tools
install_h.lib: install_h.head
install_h.uts: install_h.head

install_h.head:
	make install_h -C head

install_h.lib:
	make install_h -C lib

install_h.uts:
	make install_h -C uts
	make install_h -C uts

cross: install_h
	$(MAKE) -C $(CROSS_SRCS)/binutils install
	$(MAKE) -C $(CROSS_SRCS)/gcc install
	$(MAKE) -C $(CROSS_SRCS)/file install


.PHONY: install install.stand install.uts install.psm install.cmd install.lib install.data

install.stand install.uts install.psm install.cmd install.lib: cross install_h
install.psm: install.stand
install.lib: install.data
install.cmd: install.lib install.data
install.uts: install.data
install: install.stand install.uts install.psm install.cmd install.lib install.data

install.stand:
	$(MAKE) -C stand install

install.psm:
	$(MAKE) -C psm install

install.cmd:
	$(MAKE) -C cmd install

install.lib:
	$(MAKE) -C lib install

install.data:
	$(MAKE) -C data install

install.uts:
	$(MAKE) -C uts install


.PHONY: clobber clobber.stand clobber.uts clobber.psm clobber.cmd clobber.lib clobber.tools clobber.head clobber.data
clobber: clobber.stand clobber.uts clobber.psm clobber.cmd clobber.lib clobber.tools clobber.head clobber.data

clobber.head:
	$(MAKE) -C head clobber

clobber.tools:
	$(MAKE) -C cross clobber

clobber.stand:
	$(MAKE) -C stand clobber

clobber.psm:
	$(MAKE) -C psm clobber

clobber.cmd:
	$(MAKE) -C cmd clobber

clobber.lib:
	$(MAKE) -C lib clobber

clobber.data:
	$(MAKE) -C data clobber

clobber.uts:
	$(MAKE) -C uts clobber

post_install: ${ROOT}/var/svc/profile/name_service.xml
post_install: ${ROOT}/var/svc/profile/generic.xml
post_install: ${ROOT}/var/svc/profile/inetd_services.xml

.PHONY: ns_dns.xml
${ROOT}/var/svc/profile/name_service.xml: ns_dns.xml install
	${INS.symlink}

.PHONY: generic_minimum.xml
${ROOT}/var/svc/profile/generic.xml: generic_minimum.xml install
	${INS.symlink}

.PHONY: inetd_generic.xml
${ROOT}/var/svc/profile/inetd_services.xml: inetd_generic.xml install
	${INS.symlink}

post_install: install
	tar czf ${ROOT}/../illumos-`date -I`.tar.gz -C ${ROOT} .
	cp ${ROOT}/../illumos-`date -I`.tar.gz  ${ROOT}/var/illumos.tar.gz

$(ROOTDIRS) $(MSGDIRS):
	$(INS.dir)
