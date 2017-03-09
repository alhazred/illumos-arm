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
# Copyright (c) 1989, 2010, Oracle and/or its affiliates. All rights reserved.
# Copyright (c) 2012 by Delphix. All rights reserved.
# Copyright 2020 Joyent, Inc.
# Copyright (c) 2013 Gary Mills
# Copyright 2014 Garrett D'Amore <garrett@damore.org>
# Copyright (c) 2015 Gary Mills
# Copyright 2016 Toomas Soome <tsoome@me.com>
# Copyright 2018 Nexenta Systems, Inc.
# Copyright 2017 Hayashi Naoyuki
# Copyright (c) 2016, Chris Fraire <cfraire@me.com>.
# Copyright 2017 RackTop Systems.
# Copyirght 2018 Jason King
# Copyright 2019 OmniOS Community Edition (OmniOSce) Association.
#

include ../Makefile.master

#	Note that libcurses installs commands along with its library.
#	This is a minor bug which probably should be fixed.
#	Note also that a few extra libraries are kept in cmd source.
#
# Certain libraries are linked with, hence depend on, other libraries.
# Library dependencies are called out explicitly, see "Library
# interdependencies" below.
.PARALLEL:

# Build libc and its dependencies
SUBDIRS=			\
	crt			\
	../cmd/sgs/libconv	\
	../cmd/sgs/libdl	\
	libc			\
	.WAIT

# Build libraries with lots of dependents
SUBDIRS +=		\
	libm		\
	libmd		\
	libmp		\
	libnsl		\
	libnvpair	\
	libsocket	\
	.WAIT

# Build everything else. Aside from explicit dependencies, all libraries
# are built in parallel.
SUBDIRS +=				\
	../cmd/sendmail/libmilter	\
	../cmd/sgs/libelf	\
	../cmd/sgs/librtld_db	\
	abi		\
	auditd_plugins	\
	brand		\
	c_synonyms	\
	cfgadm_plugins	\
	crypt_modules	\
	extendedFILE	\
	fm		\
	getloginx	\
	gss_mechs/mech_dh	\
	gss_mechs/mech_dummy	\
	gss_mechs/mech_krb5	\
	gss_mechs/mech_spnego	\
	hal		\
	hbaapi		\
	iconv_modules	\
	krb5		\
	libadm		\
	libads		\
	libadt_jni	\
	libadutils	\
	libaio		\
	libast		\
	libavl		\
	libbe		\
	libbrand	\
	libbsdmalloc	\
	libbsm		\
	libc_db		\
	libcfgadm	\
	libcmd		\
	libcmdutils	\
	libcommputil	\
	libcontract	\
	libcpc		\
	libcrypt	\
	libcryptoutil	\
	libctf		\
	libcurses	\
	libcustr	\
	libdemangle	\
	libdevice	\
	libdevid	\
	libdevinfo	\
	libdhcpagent	\
	libdhcputil	\
	libdisasm	\
	libdiskmgt	\
	libdladm	\
	libdll		\
	libdlpi		\
	libdns_sd	\
	libdoor		\
	libdtrace	\
	libdtrace_jni	\
	libdwarf	\
	libefi		\
	libelfsign	\
	libeti		\
	libexacct	\
	libexacct/demo	\
	libfakekernel	\
	libfcoe		\
	libficl		\
	libfru		\
	libfruutils	\
	libfsmgt	\
	libfstyp	\
	libgen		\
	libgrubmgmt	\
	libgss		\
	libhotplug	\
	libidmap	\
	libilb		\
	libima		\
	libinetsvc	\
	libinetutil	\
	libinstzones	\
	libintl		\
	libipadm	\
	libipd		\
	libipmi		\
	libipmp		\
	libipp		\
	libipsecutil	\
	libiscsit	\
	libjedec	\
	libkmf		\
	libkrb5		\
	libkstat	\
	libkvm		\
	libldap5	\
	liblgrp		\
	liblm		\
	libm1		\
	libmail		\
	libmalloc	\
	libmapid	\
	libmapmalloc	\
	libmd5		\
	libmlrpc	\
	libmtmalloc	\
	libmvec		\
	libndmp		\
	libnisdb	\
	libnls		\
	libnwam		\
	libofmt		\
	libpam		\
	libpcidb	\
	libpcsc		\
	libpctx		\
	libpicl		\
	libpicltree	\
	libpkg		\
	libpool		\
	libpp		\
	libproc		\
	libproject	\
	libpthread	\
	libraidcfg	\
	librcm		\
	libreparse	\
	libresolv	\
	libresolv2	\
	librestart	\
	librpcsvc	\
	librsm		\
	librstp		\
	librt		\
	libsasl		\
	libscf		\
	libsched	\
	libsctp		\
	libsec		\
	libsecdb	\
	libsendfile	\
	libsff		\
	libshare	\
	libshell	\
	libsip		\
	libsldap	\
	libslp		\
	libsmbfs	\
	libsmbios	\
	libsmedia	\
	libsqlite	\
	libsrpt		\
	libstmf		\
	libstmfproxy	\
	libsum		\
	libsun_ima	\
	libsysevent	\
	libtecla	\
	libtermcap	\
	libthread	\
	libtnf		\
	libtnfctl	\
	libtnfprobe	\
	libtsnet	\
	libtsol		\
	libumem		\
	libutempter	\
	libuuid		\
	libuutil	\
	libvolmgt	\
	libvrrpadm	\
	libvscan	\
	libw		\
	libwrap		\
	libxcurses	\
	libxcurses2	\
	libxnet		\
	libzfs		\
	libzfs_core	\
	libzfs_jni	\
	libzonecfg	\
	libzoneinfo	\
	libzonestat	\
	libzpool	\
	libzutil	\
	madv		\
	mpapi		\
	mpss		\
	nametoaddr	\
	ncad_addr	\
	nsswitch	\
	pam_modules	\
	passwdutil	\
	pkcs11		\
	policykit	\
	print		\
	pylibbe		\
	pysolaris	\
	pyzfs		\
	raidcfg_plugins	\
	rpcsec_gss	\
	sasl_plugins	\
	scsi		\
	smbclnt		\
	smbsrv		\
	smhba		\
	sun_fc		\
	sun_sas		\
	udapl		\
	watchmalloc	\
	$($(MACH)_SUBDIRS)

alpha_SUBDIRS=		\
	libfdisk

aarch64_SUBDIRS=	\
	libfdisk

riscv64_SUBDIRS=	\
	libfdisk

i386_SUBDIRS=		\
	libfdisk	\
	libppt		\
	libsaveargs	\
	libvmm		\
	libvmmapi

sparc_SUBDIRS=		\
	efcode		\
	libds		\
	libdscp		\
	libprtdiag	\
	libprtdiag_psr	\
	libpri		\
	librsc		\
	storage		\
	libpcp		\
	libtsalarm	\
	libv12n

#
# Create a special version of $(SUBDIRS) with no .WAIT's, for use with the
# clean and clobber targets (for more information, see those targets, below).
#
NOWAIT_SUBDIRS= $(SUBDIRS:.WAIT=)

DCSUBDIRS=

MSGSUBDIRS=		\
	abi		\
	auditd_plugins	\
	brand		\
	cfgadm_plugins	\
	gss_mechs/mech_dh	\
	gss_mechs/mech_krb5	\
	krb5		\
	libast		\
	libbsm		\
	libc		\
	libcfgadm	\
	libcmd		\
	libcontract	\
	libcurses	\
	libdhcputil	\
	libdiskmgt	\
	libdladm	\
	libdll		\
	libgrubmgmt	\
	libgss		\
	libidmap	\
	libilb		\
	libinstzones	\
	libipadm	\
	libipmp		\
	libipsecutil	\
	libldap5	\
	libnsl		\
	libnwam		\
	libofmt		\
	libpam		\
	libpicl		\
	libpkg		\
	libpool		\
	libpp		\
	libreparse	\
	libsasl		\
	libscf		\
	libsecdb	\
	libshare	\
	libshell	\
	libsldap	\
	libslp		\
	libsmbfs	\
	libsmedia	\
	libsum		\
	libtsol		\
	libuutil	\
	libvrrpadm	\
	libvscan	\
	libzfs		\
	libzonecfg	\
	madv		\
	mpss		\
	pam_modules	\
	pyzfs		\
	rpcsec_gss	\
	$($(MACH)_MSGSUBDIRS)

sparc_MSGSUBDIRS=	\
	libprtdiag	\
	libprtdiag_psr

alpha_MSGSUBDIRS= libfdisk
aarch64_MSGSUBDIRS= libfdisk
riscv64_MSGSUBDIRS= libfdisk
i386_MSGSUBDIRS= libfdisk

HDRSUBDIRS=				\
	../cmd/sendmail/libmilter	\
	auditd_plugins	\
	fm		\
	hal		\
	hbaapi		\
	libads		\
	libast		\
	libbrand	\
	libbsm		\
	libc		\
	libcmd		\
	libcmdutils	\
	libcommputil	\
	libcontract	\
	libcpc		\
	libcryptoutil	\
	libctf		\
	libcurses	\
	libcustr	\
	libdemangle	\
	libdevice	\
	libdevid	\
	libdevinfo	\
	libdhcpagent	\
	libdhcputil	\
	libdisasm	\
	libdiskmgt	\
	libdladm	\
	libdll		\
	libdlpi		\
	libdns_sd	\
	libdtrace	\
	libdtrace_jni	\
	libelfsign	\
	libeti		\
	libfakekernel	\
	libfcoe		\
	libfru		\
	libfstyp	\
	libgen		\
	libgrubmgmt	\
	libidmap	\
	libilb		\
	libima		\
	libinetsvc	\
	libinetutil	\
	libinstzones	\
	libipadm	\
	libipd		\
	libipmi		\
	libipmp		\
	libipp		\
	libipsecutil	\
	libiscsit	\
	libkrb5		\
	libkstat	\
	libkvm		\
	libmail		\
	libmapid	\
	libmd		\
	libmlrpc	\
	libmtmalloc	\
	libndmp		\
	libnsl		\
	libnvpair	\
	libnwam		\
	libofmt		\
	libpam		\
	libpcidb	\
	libpcsc		\
	libpctx		\
	libpicl		\
	libpicltree	\
	libpool		\
	libpp		\
	libproc		\
	libraidcfg	\
	librcm		\
	libreparse	\
	librestart	\
	librpcsvc	\
	librsm		\
	librstp		\
	libsasl		\
	libscf		\
	libsec		\
	libsff		\
	libshare	\
	libshell	\
	libsip		\
	libslp		\
	libsmbfs	\
	libsmbios	\
	libsmedia	\
	libsocket	\
	libsqlite	\
	libsrpt		\
	libstmf		\
	libstmfproxy	\
	libsum		\
	libsun_ima	\
	libsysevent	\
	libtecla	\
	libtermcap	\
	libtnf		\
	libtnfctl	\
	libtnfprobe	\
	libtsnet	\
	libtsol		\
	libumem		\
	libutempter	\
	libuutil	\
	libvolmgt	\
	libvrrpadm	\
	libvscan	\
	libwrap		\
	libxcurses2	\
	libzfs		\
	libzfs_core	\
	libzfs_jni	\
	libzoneinfo	\
	libzonestat	\
	mpapi		\
	passwdutil	\
	pkcs11		\
	policykit	\
	scsi		\
	smbsrv		\
	smhba		\
	udapl		\
	$($(MACH)_HDRSUBDIRS)

alpha_HDRSUBDIRS=	\
	libfdisk	\
	../oss/nss		\
	../oss/nspr

aarch64_HDRSUBDIRS=	\
	libfdisk	\
	../oss/nss		\
	../oss/nspr

riscv64_HDRSUBDIRS=	\
	libfdisk	\
	../oss/nss		\
	../oss/nspr

i386_HDRSUBDIRS=	\
	libfdisk	\
	libppt		\
	libsaveargs	\
	libvmm		\
	libvmmapi

sparc_HDRSUBDIRS=	\
	libds		\
	libdscp		\
	libpri		\
	libv12n		\
	storage

all : 		TARGET= all
check : 	TARGET= check
clean : 	TARGET= clean
clobber : 	TARGET= clobber
install : 	TARGET= install
install_h : 	TARGET= install_h
_dc : 		TARGET= _dc
_msg : 		TARGET= _msg

.KEEP_STATE:

SUBDIRS+=	\
	../oss/libiconv	\
	../oss/gettext	\
	../oss/ncurses	\
	../oss/pcre	\
	../oss/net-snmp	\
	../oss/trousers	\
	../oss/libstdc++ \
	../oss/libssp	\
	../oss/xz	\
	../oss/libxml2	\
	../oss/zlib	\
	../oss/libressl	\
	../oss/libxslt	\
	../oss/nss	\
	../oss/nspr	\
	../oss/idnkit

NOWAIT_SUBDIRS= $(SUBDIRS:.WAIT=)
#
# For the all and install targets, we clearly must respect library
# dependencies so that the libraries link correctly.  However, for
# the remaining targets (check, clean, clobber, install_h, _dc
# and _msg), libraries do not have any dependencies on one another
# and thus respecting dependencies just slows down the build.
# As such, for these rules, we use pattern replacement to explicitly
# avoid triggering the dependency information.  Note that for clean and
# clobber, we must use $(NOWAIT_SUBDIRS) rather than
# $(SUBDIRS), to prevent `.WAIT' from expanding to `.WAIT-nodepend'.
#
aarch64_NOSUPPORT=	\
	libpp		\
	libcurses	\
	libfru		\
	libunistat	\
	libxcurses	\
	libxcurses2	\
	c_synonyms	\
	libmd5		\
	libw		\
	libintl		\
	libaio		\
	libdisasm	\
	libdtrace	\
	libdtrace_jni	\
	libtermcap	\
	libeti		\
	libthread	\
	libslp		\
	libcrypt	\
	libdoor		\
	libsys		\
	librdc		\
	libm1		\
	libmvec		\
	libadt_jni	\
	abi		\
	libtnf		\
	libtnfctl	\
	print		\
	libsched	\
	libpctx		\
	libcpc		\
	../cmd/sendmail/libmilter	\
	pylibbe		\
	pyzfs		\
	pysolaris	\
	libzfs_jni	\
	brand		\
	policykit	\
	hal		\
	libgrubmgmt	\
	libfruutils	\
	libdscfg

alpha_NOSUPPORT= $(aarch64_NOSUPPORT)
riscv64_NOSUPPORT= $(aarch64_NOSUPPORT)

all:			$(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS))

install:		$(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS)) .WAIT install_extra

# extra libraries kept in other source areas
install_extra: $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS))
install_extra:
	@cd ../cmd/sgs; pwd; $(MAKE) install_lib
	@pwd

clean clobber:		$(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(NOWAIT_SUBDIRS)))


install_h check:	$(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(HDRSUBDIRS)))

_msg:			$(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(MSGSUBDIRS))) .WAIT _dc

_dc:			$(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(DCSUBDIRS)))

lint:

# Library interdependencies are called out explicitly here.
libdemangle: crt
../cmd/sgs/libconv: crt libdemangle
../cmd/sgs/libdl: crt
#libc:		../cmd/sgs/libconv ../cmd/sgs/libdl

libm:		libc
libmd:		libc
libmp:		libc
libnsl:		libc libmd libmp
libnvpair:	libc libnsl
libsocket:	libc libnsl

basedeps:	libc libm libmd libmp libnsl libnvpair libsocket
basedeps :	TARGET=install

# The following dependencies are currently required by libdbus-1 and should
# be used for anything linking against it.
dbusdeps:	libsecdb libtsol libinetutil libscf libuutil libgen libsmbios

# The following dependencies don't need to be listed below as they are built
# before everything else:
# libc libm libmd libmp libnsl libnvpair libsocket
../cmd/sendmail/libmilter:	libsocket libnsl
abi:		libctf libmapmalloc libproc
auditd_plugins: libbsm libsecdb libgss libmtmalloc
brand:		libzonecfg libmapmalloc
cfgadm_plugins:	libdevice libdevinfo libhotplug librcm hbaapi libkstat libscf
fm:		libexacct libipmi libzfs scsi libdevinfo libdevid libcfgadm \
		libcontract libsysevent ../cmd/sgs/libelf libdladm libsff \
		libpcidb libdiskmgt
$(SPARC_BLD)fm:	libpri
gss_mechs/mech_dh: libgss
gss_mechs/mech_dummy: libgss
gss_mechs/mech_krb5: libgss libresolv2 pkcs11 libkstat
gss_mechs/mech_spnego: gss_mechs/mech_krb5
hal:		dbusdeps
krb5:		gss_mechs/mech_krb5 libtecla libldap5
libads:		libnsl
libadt_jni:	libbsm
libadutils:	libldap5 libresolv2
libbe:		libzfs libinstzones libuuid libgen libdevinfo libefi libficl
libbsm:		libinetutil libscf libsecdb libtsol
libcfgadm:	libdevinfo
libcontract:	libnvpair
libcmd:		libsum libast
libcmdutils:	libavl libnvpair
libcpc:		libpctx
libcrypt:	libgen
libctf:		libdwarf
libdemangle:	libcustr
libdevice:	libnvpair
libdevid:	libdevinfo
libdevinfo:	libsec libgen
libdhcpagent:	libdhcputil libuuid libdlpi libcontract
libdhcputil:	libgen libinetutil libdlpi
libdiskmgt:	libdevid libdevinfo libadm libefi libkstat libsysevent
$(INTEL_BLD)libdiskmgt: libfdisk
libdladm:	libdevinfo libinetutil libscf librcm libexacct libkstat \
		libpool
libdll:		libast
libdlpi:	libinetutil libdladm
libds:		libsysevent
libdtrace:	libproc libgen libctf libmapmalloc
libdtrace_jni:	libuutil libdtrace
libdwarf:	../cmd/sgs/libelf
libefi:		libuuid libsmbios
libelfsign:	libcryptoutil libkmf ../cmd/sgs/libelf
libeti:		libcurses
libexacct/demo:	libexacct libproject
libfakekernel:	libumem libcryptoutil
libfcoe:	libdladm
libficl:	libuuid libumem
libfru:		libfruutils
libfsmgt:	libkstat
libgrubmgmt:	libdevinfo libzfs libfstyp libefi
$(INTEL_BLD)libgrubmgmt: libfdisk
libidmap:	libavl libuutil
libinetsvc:	libscf
libinetutil:	libsocket
libinstzones:	libzonecfg libcontract
libipadm:	libinetutil libdlpi libdhcpagent libdladm libsecdb libdhcputil
libipmi:	libm libnvpair libsocket libnsl
libipmp:	libinetutil
libipsecutil:	libtecla libtsol libkmf
libiscsit:	libstmf libuuid
libhotplug:	libnvpair
libkmf:		libcryptoutil pkcs11 libcustr
libkvm:		../cmd/sgs/libelf
libldap5:	libsasl
libmapid:	libresolv2 libscf
libmlrpc:	libsmbfs libuuid
libndmp:	libscf
libnisdb:	libldap5
libnwam:	libscf libbsm libdladm libipadm
libpcp:		libumem libdevinfo
libpctx:	libproc
libpkg:		libscf libadm
libpool:	libscf libexacct
libpp:		libast
libppt:		libpcidb libdevinfo libcmdutils
libproc:	../cmd/sgs/librtld_db ../cmd/sgs/libelf libctf
$(INTEL_BLD)libproc: libsaveargs
libproject:	libpool libproc libsecdb
libprtdiag:	libkstat libdevinfo
libprtdiag_psr:	libprtdiag
libraidcfg:	libdevinfo
librcm:		libnvpair
librestart:	libuutil libscf libpool libproject libsecdb libsysevent
libsasl:	libgss pkcs11
libsaveargs:	libdisasm
libscf:		libuutil libgen libnsl libnvpair libsmbios
libsec:		libavl libidmap
libsecdb:	libnsl
libsff:		libnvpair
libshare:	libscf libzfs libuuid libfsmgt libsecdb libumem libsmbfs
libshell:	libast libcmd libdll libsecdb
libsip:		libmd5
libsldap:	libldap5 libscf
libsmbfs:	libkrb5 libsec libidmap pkcs11
libsmbios:	libdevinfo
libsrpt:	libstmf
libstmf:	libscf
libstmfproxy:	libstmf libpthread
libsum:		libast
libsun_ima:	libdevinfo libsysevent
libsysevent:	libsecdb
#libtecla:	libcurses
#libtermcap:	libcurses
libtnfctl:	../cmd/sgs/libelf
libtsalarm:	libpcp
libtsnet:	libtsol libsecdb
libtsol:	libsecdb
libuuid:	libdlpi
libv12n:	libds libuuid
libvmm:		libvmmapi
libvolmgt:	libadm
libvrrpadm:	libdladm libscf
libvscan:	libscf libsecdb
libzfs:		libdevid libgen libuutil libavl libefi libidmap \
		libumem libtsol libzfs_core libcryptoutil pkcs11 libmd libzutil
libzfs_core:	libnvpair
libzfs_jni:	libdiskmgt libzfs libzutil
libzonecfg:	libuuid libsysevent libsec libbrand libpool libscf libproc \
		libuutil libbsm libsecdb
libzonestat:	libcmdutils libumem
libzpool:	libavl libumem libcmdutils libsysevent libfakekernel libzutil
libzutil:	libadm libavl libdevid libefi
madv:		libgen
mpapi:		libpthread libdevinfo libsysevent
mpss:		libgen
nsswitch:	libadutils libidmap libdns_sd libscf libldap5 libsldap
pam_modules:	libproject passwdutil smbsrv libtsnet libpam libbsm libsecdb
passwdutil:	libsldap
pkcs11:		libcryptoutil libgen libuuid
policykit:	dbusdeps
print:		libldap5 libmd5 libsendfile
pylibbe:	libbe libzfs
pysolaris:	libsec libidmap
pyzfs:		libzfs
raidcfg_plugins: libraidcfg librcm libcfgadm libpicl libpicltree
rpcsec_gss:	libgss
sasl_plugins:	pkcs11 libgss libsasl
scsi:		libfru libumem libdevid libdevinfo
smbclnt:	libfakekernel pkcs11
smbsrv:		libxnet libpthread librt libshare libidmap pkcs11 libsqlite \
		libcryptoutil libreparse libcmdutils libresolv2 libsmbfs \
		libuuid libfakekernel libads libgss libldap5 krb5 libmlrpc
storage:	libdevice libdevinfo libdevid
sun_fc:		libdevinfo libsysevent
sun_sas:	libdevinfo libsysevent libkstat libdevid
udapl:		libdevinfo libdladm



# GROUP1
libc:			crt
libpthread:		libc
../oss/libiconv:	libc libm libpthread
../oss/gettext:		../oss/libiconv
../oss/ncurses:		../oss/gettext
SUBDIR_GROUP1= crt libc libpthread ../oss/libiconv ../oss/gettext ../oss/ncurses libmd libnsl libmp libsocket libm libnvpair
$(filter-out .WAIT $($(MACH)_NOSUPPORT) $(SUBDIR_GROUP1), $(SUBDIRS)): $(SUBDIR_GROUP1)

../oss/libxml2:		../oss/xz ../oss/zlib
../oss/libxslt:		../oss/libxml2 ../oss/libressl
../oss/libressl:	../oss/zlib ../oss/libssp
../cmd/sgs/libdl:	../cmd/sgs/libconv libc
../cmd/sgs/libelf:	../cmd/sgs/libconv libc
../cmd/sgs/librtld_db:	../cmd/sgs/libconv libc
../oss/idnkit:		libc

../oss/pcre:		../oss/zlib ../oss/libstdc++
../oss/net-snmp:	libresolv2 libbsm ../oss/pcre
../oss/trousers:	../oss/libressl libresolv2 libbsm

libpool:		../oss/libxml2
pkcs11:			../oss/trousers ../oss/libressl
libipsecutil:		../oss/libressl
libkrb5:		gss_mechs/mech_krb5
libdiskmgt:		libfdisk
../oss/nss:		libbsm ../oss/zlib ../oss/nspr ../oss/libressl
../oss/nspr:		../cmd/sgs/libdl
libnisdb sun_fc:	../oss/libstdc++
libshare libkmf fm libzonecfg libbrand:	../oss/libxml2
libzfs libzpool:		../oss/zlib
libkmf libldap5:		../oss/nss
libpkg libwanboot libkmf krb5:	../oss/libressl
fm:				../oss/net-snmp
../oss/libstdc++:	librt ../oss/libssp
../oss/libssp:		librt
iconv_modules:	../oss/idnkit

#
# The reason this rule checks for the existence of the
# Makefile is that some of the directories do not exist
# in certain situations (e.g., exportable source builds,
# OpenSolaris).
#

$(info $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS)))

.PHONY: $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS))

$(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS)): FRC
	@echo Start $@
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(TARGET); \
	else \
		echo Missing files $@/Makefile; \
		true; \
	fi
	@echo End $@

.PHONY: $(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS)))
$(patsubst %,%-nodepend, $(filter-out .WAIT $($(MACH)_NOSUPPORT), $(SUBDIRS))): FRC
	@if [ -f $(@:%-nodepend=%)/Makefile  ]; then \
		cd $(@:%-nodepend=%); pwd; $(MAKE) $(TARGET); \
	else \
		true; \
	fi

FRC:
