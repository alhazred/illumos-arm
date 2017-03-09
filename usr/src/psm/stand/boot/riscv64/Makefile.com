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

include $(TOPDIR)/psm/stand/boot/Makefile.boot

CSTD= $(CSTD_GNU99)

BOOTSRCDIR	= ../..
MACH_DIR	= $(BOOTSRCDIR)/$(MACH)/common
PORT_DIR	= $(BOOTSRCDIR)/port
TOP_CMN_DIR	= $(SRC)/common
CMN_DIR		= $(BOOTSRCDIR)/common
BOOT_DIR	= $(SRC)/psm/stand/boot
DTC_BASE	= $(EXTRA)/dtc

SRT0_O = srt0.o
OBJS += \
	   nfsconf.o \
	   heap_kmem.o readfile.o  sscanf.o strtoul.o strtol.o \
	   get.o standalloc.o memlist.o memmove.o memchr.o ddi_subr.o \
	   boot_plat.o uname-m.o uname-i.o machdep.o \
	   prom_exit.o prom_gettime.o prom_printf.o prom_string.o \
	   prom_getchar.o prom_init.o prom_panic.o prom_putchar.o \
	   prom_wrtestr.o prom_node.o prom_node_init.o \
	   getoptstr.o bootflags.o ramdisk.o \
	   boot_riscv64.o riscv64_subr.o console.o prom_utils.o \
	   assfail.o avl.o

DTEXTDOM=
DTS_ERRNO=

CFLAGS +=	$(STAND_FLAGS_$(CLASS)) -_gcc=-mcmodel=medany
ASFLAGS+=	$(STAND_FLAGS_$(CLASS)) -_gcc=-mcmodel=medany

SRT0_OBJ = $(SRT0_O)

LIBSYS_DIR = $(ROOT)/stand/lib

NFSBOOT = inetboot
DTB = $(DTS:%.dts=%.dtb)
ROOT_PSM_NFSBOOT = $(ROOT_PSM_DIR)/$(NFSBOOT)
ROOT_PSM_DTB = $(DTB:%=$(ROOT_PSM_DIR)/%)

PSMSTANDDIR =	$(SRC)/psm/stand
STANDDIR =	$(SRC)/stand
CMNNETDIR =	$(SRC)/common/net
CMNDIR =	$(SRC)/common
CMNUTILDIR =	$(SRC)/common/util
SYSDIR	=	$(SRC)/uts
CPPDEFS	= 	-D$(MACH) -D_BOOT -D_KERNEL -D_MACHDEP -D_ELF64_SUPPORT -D_SYSCALL32
CPPINCS	= 	-I$(PORT_DIR) \
		-I$(PSMSTANDDIR) \
		-I$(STANDDIR)/lib/sa \
		-I$(STANDDIR) -I$(CMNDIR) -I$(MACHDIR) \
		-I$(STANDDIR)/$(MACH) \
		-I$(SYSDIR)/$(MACH)pf/$(BOARD) \
		-I$(SYSDIR)/$(MACH)pf \
		-I$(SYSDIR)/$(MACH) \
		-I$(SYSDIR)/common \
		-I$(TOP_CMN_DIR) \
		-I$(DTC_BASE)/libfdt

CPPFLAGS	+= $(CPPDEFS) $(CPPINCS) $(STAND_CPPFLAGS)
AS_CPPFLAGS	+= $(CPPDEFS) $(CPPINCS) $(STAND_CPPFLAGS)

LIBNFS_LIBS     = libnfs.a libxdr.a libsock.a libinet.a libtcp.a libfdt.a libhsfs.a libufs.a libzfs.a libsa.a
NFS_LIBS        = $(LIBNFS_LIBS:lib%.a=-l%)
NFS_DIRS        = $(LIBSYS_DIR:%=-L%)
LIBDEPS		= $(LIBNFS_LIBS:%=$(LIBSYS_DIR)/%)
NFS_MAPFILE	= mapfile
NFS_LDFLAGS	= $(STAND_MARCH:%=-_gcc=-march=%) $(STAND_MABI:%=-_gcc=-mabi=%) -xnolib -_gcc=-nostdlib -_gcc="-Wl,--gc-sections,-T,$(NFS_MAPFILE),$(NFS_DIRS)"

NFS_SRT0        = $(SRT0_OBJ)
NFS_OBJS        = $(OBJS)

NFSBOOT_OUT	= $(NFSBOOT).out
NFSBOOT_BIN	= $(NFSBOOT).bin

AS_CPPFLAGS += -I. -D_ASM -c

MACHDIR = ../common
GENASSYM_CF = $(MACHDIR)/genassym.cf
ASSYM_H		= assym.h


