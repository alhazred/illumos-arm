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
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
# Copyright (c) 2019, Joyent, Inc.
#

include ../Makefile.cross

ITM	= geniconvtbl.so
PROG	= geniconvtbl

SRCSH1  = iconv_tm.h hash.h
SRCCH1  = itmcomp.h itm_util.h maptype.h
SRCSC1  = itmcomp.c assemble.c disassemble.c itm_util.c reg_compile.c  reg_step.c
SRCY1   = itm_comp.y
SRCL1   = itm_comp.l
SRCI1   = geniconvtbl.c


YTABC   = y.tab.c
YTABH   = y.tab.h
LEXYY   = lex.yy.c
YOUT    = y.output

SRCSH	= $(SRCSH1)
SRCCH	= $(SRCCH1)
SRCSC	= $(SRCSC1)
SRCI	= $(SRCI1:)
SRCY    = $(SRCY1)
SRCL    = $(SRCL1)

SRCYC	= $(SRCY:%.y=%.c)
SRCLC	= $(SRCL:%.l=%.c)

SRCS    = $(SRCSC) $(YTABC) $(LEXYY)
HDRS	= $(SRCCH1) $(ERNOSTRH)

SED	= sed
LEXSED	= lex.sed
YACCSED	= yacc.sed

#
# definition for some useful target like clean, 
OBJS	= $(SRCSC1:%.c=%.o) $(YTABC:.c=.o) $(LEXYY:.c=.o)

CLEANFILES = 	$(OBJS) $(YTABC) $(YTABH) $(LEXYY) $(YOUT) \
		$(POFILES) $(POFILE) $(PROG)

CLOBBERFILES=	$(CLEANFILES) $(SRCYC)

CPPFLAGS	+= -I. -I.. -g
CERRWARN	+= -Wno-uninitialized
CERRWARN	+= -Wno-unused-label
CERRWARN	+= -Wno-switch
CERRWARN	+= -Wno-unused-variable
CERRWARN	+= -Wno-implicit-function-declaration
YFLAGS		+= -d -v
CFLAGS 		+= -D_FILE_OFFSET_BITS=64

.NOTPARALLEL:
.KEEP_STATE:
all: $(PROG)

install: all
	$(INSTALL) -m 755 $(PROG) $(BINDIR)/$(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(YTABC) $(YTABH): $(SRCY)
	$(YACC) $(YFLAGS) $(SRCY)
	@ $(MV) $(YTABC) $(YTABC)~
	@ $(SED) -f $(YACCSED) $(YTABC)~ > $(YTABC)
	@ $(RM) $(YTABC)~

$(LEXYY): $(SRCL) $(YTABH)
	$(LEX) -t $(SRCL) | $(SED) -f $(LEXSED) > $(LEXYY)

clean:
	$(RM) $(CLEANFILES)

clobber: clean

%.o:	%.c
	$(COMPILE.c) $<

%.o:	../%.c
	$(COMPILE.c) $<

