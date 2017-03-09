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
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include ../Makefile.nspr

SUBDIRS=	$(MACH)

all :		TARGET = all
clean :		TARGET = clean
clobber :	TARGET = clobber
install :	TARGET = install

#
# Definitions for header files
#

HDRS =\
      prcountr.h prrwlock.h prmwait.h prio.h prlink.h prclist.h pripcsem.h \
      prwin16.h prtrace.h prbit.h prtpool.h prlog.h prvrsion.h prshm.h \
      prtypes.h prinit.h prtime.h prlong.h prlock.h prcmon.h prolock.h \
      prenv.h prproces.h prrng.h prdtoa.h prcvar.h pratom.h prthread.h \
      prmem.h prnetdb.h prerr.h prsystem.h prinet.h prerror.h nspr.h prshma.h \
      prcpucfg.h prprf.h prmon.h prinrval.h prpdce.h \
      obsolete/pralarm.h obsolete/prsem.h \
      obsolete/protypes.h obsolete/probslet.h \
      private/pprthred.h private/pprio.h private/prpriv.h

HDRDIR=		$(NSPR_BASE)/pr/include
ROOTHDRS=	$(HDRS:%=$(ROOTHDRDIR)/%)

all clean clobber install: $(SUBDIRS)
install_h: $(ROOTHDRS)


$(ROOTHDRS): FILEMODE= 644
$(ROOTHDRS): $(ROOTHDRDIR)/obsolete
$(ROOTHDRS): $(ROOTHDRDIR)/private

$(ROOTHDRDIR)/obsolete $(ROOTHDRDIR)/private:
	$(INS.dir)

$(ROOTHDRDIR)/%: $(HDRDIR)/%
	$(INS.file)

$(SUBDIRS):	FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
