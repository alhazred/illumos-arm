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
# Copyright 2017 Hayashi Naoyuki
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright 2018 Jason King
#
# Copyright (c) 2018, Joyent, Inc.

LIBRARY =	pkcs11_tpm.a
VERS =		.1

RSA_DIR =		$(SRC)/common/crypto/rsa
RSA_FLAGS =		-I$(RSA_DIR)

BIGNUM_DIR =		$(SRC)/common/bignum
BIGNUM_FLAGS =		-I$(BIGNUM_DIR)

PADDING_DIR =		$(SRC)/common/crypto/padding
PADDING_FLAGS =		-I$(PADDING_DIR)

SOFTCRYPTOFLAGS =	$(RSA_FLAGS) $(PADDING_FLAGS) $(BIGNUM_FLAGS)

OBJECTS= api_interface.o \
	apiutil.o \
	asn1.o \
	cert.o \
	data_obj.o \
	decr_mgr.o \
	dig_mgr.o \
	encr_mgr.o \
	globals.o \
	hwf_obj.o \
	key.o \
	key_mgr.o \
	loadsave.o \
	log.o \
	mech_md5.o \
	mech_rsa.o \
	mech_sha.o \
	new_host.o \
	obj_mgr.o \
	object.o \
	sess_mgr.o \
	sign_mgr.o \
	template.o \
	tpm_specific.o \
	utility.o \
	verify_mgr.o


include ../../../Makefile.lib

SRCDIR= ../common

SRCS=	$(OBJECTS:%.o=$(SRCDIR)/%.c)

#       set signing mode
POST_PROCESS_SO +=      ; $(ELFSIGN_CRYPTO)

ROOTLIBDIR=$(ROOT)/usr/lib/security
ROOTLIBDIR64=$(ROOT)/usr/lib/security/$(MACH64)

LIBS=$(DYNLIB) $(DYNLIB64)

TSSROOT=$(ADJUNCT_PROTO)
TSPILIBDIR=$(TSSROOT)/usr/lib
TSPIINCDIR=$(TSSROOT)/usr/include
TSSLIB=-L$(TSPILIBDIR)
TSSLIB64=-L$(TSPILIBDIR)/$(MACH64)
TSSINC=-I$(TSPIINCDIR)

LDLIBS += -Bsymbolic $(TSSLIB) -L$(ADJUNCT_PROTO)/lib -lc -luuid -lmd -ltspi -lsoftcrypto
CPPFLAGS += -xCC -D_POSIX_PTHREAD_SEMANTICS $(TSSINC) $(SOFTCRYPTOFLAGS)
CPPFLAGS64 += $(CPPFLAGS)
CSTD=        $(CSTD_GNU99)

CFLAGS  +=	-_gcc=-fvisibility=protected

CERRWARN +=	-_gcc=-Wno-parentheses
CERRWARN +=	-_gcc=-Wno-unused-label
CERRWARN +=	$(CNOWARN_UNINIT)

# not linted
SMATCH=off

.KEEP_STATE:

all: $(LIBS)


pics/%.o: $(SRCDIR)/%.c
	$(COMPILE.c) -o $@ $<
	$(POST_PROCESS_O)

include $(SRC)/lib/Makefile.targ
