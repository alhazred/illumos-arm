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
include ../../Makefile.master

COMMON_SUBDIRS += FX FX_DPTBL IA TS TS_DPTBL SDC

COMMON_SUBDIRS += clone options pseudo
COMMON_SUBDIRS += dev nulldriver strplumb hook
COMMON_SUBDIRS += zcons srn
COMMON_SUBDIRS += scsi sd inst_sync kaio
COMMON_SUBDIRS += sata ahci si3124 cpuid
COMMON_SUBDIRS += sda

COMMON_SUBDIRS += arp ip ip6 tcp tcp6 udp udp6 icmp icmp6
COMMON_SUBDIRS += cc cc_cubic cc_newreno cc_sunreno
COMMON_SUBDIRS += dld dls ipnet neti
COMMON_SUBDIRS += mac mac_ether mac_ipv4 mac_ipv6 softmac mii net_dacf
COMMON_SUBDIRS += rpcmod rpcsec tlimod
COMMON_SUBDIRS += random kcf md4 md5 swrand sha1 sha2
COMMON_SUBDIRS += timod idmap cmlb sad elfexec kssl ldterm ttcompat
COMMON_SUBDIRS += ptsl ptc ipsecesp ipsecah tl keysock spdsock sysmsg
COMMON_SUBDIRS += cn mm pipe intpexec log sysevent cryptoadm devinfo
COMMON_SUBDIRS += sy rts fm ramdisk poll lofi ippctl iptun crypto
COMMON_SUBDIRS += bl dlpistub physmem pts ptm logindmux dump des
COMMON_SUBDIRS += sppp sppptun pool ipf connld ptem redirmod shmsys ipc
COMMON_SUBDIRS += exacctsys msgsys semsys ksocket bridge aggr
COMMON_SUBDIRS += sockpfp sockrds socksctp socksdp sdp bpf
COMMON_SUBDIRS += pset iscsi iscsit idm
#COMMON_SUBDIRS += lockstat

COMMON_SUBDIRS += fifofs specfs devfs procfs sockfs nfs nfs_dlboot ufs
COMMON_SUBDIRS += doorfs ctfs mntfs tmpfs objfs sharefs namefs portfs
COMMON_SUBDIRS += lofs fssnap fssnap_if fdfs autofs hsfs zfs
COMMON_SUBDIRS += edonr skein
COMMON_SUBDIRS += pcfs dcfs smbfs udfs
COMMON_SUBDIRS += winlock llc1 stmf stmf_sbd strategy simnet vnic
COMMON_SUBDIRS += consconfig conskbd kbtrans consms
COMMON_SUBDIRS += wc tem iwscn kstat
COMMON_SUBDIRS += klmmod  klmops
COMMON_SUBDIRS += hidparser usb_ac usb_ah usb_as usb_ia usb_mid usba
COMMON_SUBDIRS += usba10 usbecm usbftdi usbkbm usbms usbprn usbs49_fw
COMMON_SUBDIRS += usbsacm usbser usbskel usbsksp usbsprl usbvc usbwcm
COMMON_SUBDIRS += mac_wifi hubd net80211 gld scsa2usb

alpha_SUBDIRS += dnet asy ife vel dadk gda ata pci-ide cmdk
#alpha_SUBDIRS += iprb

COMMON_SUBDIRS += pipemod pfmod ksyms blkdev bufmod cryptmod
COMMON_SUBDIRS += rge pcihp busra hpcsvc

SUBDIRS = $(COMMON_SUBDIRS) $($(MACH)_SUBDIRS)

all install clean clobber : $(SUBDIRS)

.PHONY: all install clean clobber install_h $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

install_h:
