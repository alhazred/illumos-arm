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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright 2020 Joyent, Inc.
#

LIBRARY = libfmd_snmp.a
VERS = .1

LIBSRCS = \
	debug_subr.c	\
	init.c		\
	module.c	\
	problem.c	\
	resource.c	\
	scheme.c

OBJECTS = $(LIBSRCS:%.c=%.o)

include ../../../Makefile.lib
include ../../Makefile.lib

SRCS = $(LIBSRCS:%.c=../common/%.c)
LIBS = $(DYNLIB)

SRCDIR =	../common

CSTD= $(CSTD_GNU99)

CPPFLAGS += -I../common -I.
$(NOT_RELEASE_BUILD)CPPFLAGS += -DDEBUG
CFLAGS += $(CCVERBOSE) $(C_BIGPICFLAGS)
CFLAGS64 += $(CCVERBOSE) $(C_BIGPICFLAGS)
CERRWARN +=	-_gcc=-Wno-cast-function-type

SNMPLIBS = -lnetsnmp -lnetsnmpagent
NATIVE_LIBS += libnetsnmp.so libnetsnmpagent.so

LDLIBS += $(MACH_LDLIBS)
LDLIBS += -lfmd_adm -luutil -lnvpair -ltopo
LDLIBS += $(SNMPLIBS)
LDLIBS += -lc

.KEEP_STATE:

all: $(LIBS)

pics/%.o: ../$(MACH)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

%.o: ../common/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

include ../../../Makefile.targ
include ../../Makefile.targ
