#
# This file and its contents are supplied under the terms of the
# Common Development and Distribution License ("CDDL"), version 1.0.
# You may only use this file in accordance with the terms of version
# 1.0 of the CDDL.
#
# A full copy of the text of the CDDL should have accompanied this
# source.  A copy of the CDDL is also available via the Internet at
# http://www.illumos.org/license/CDDL.
#

#
# Copyright (c) 2012 by Delphix. All rights reserved.
#

all     :       TARGET = all
install :       TARGET = install
clean   :       TARGET = clean
clobber :       TARGET = clobber
lint    :       TARGET = lint

.KEEP_STATE:

all clean clobber install lint: $(SUBDIRS)

lint_PROG:
	$(LINT.c) $(PROG).c $(LDLIBS)

lint_SRCS:
	$(LINT.c) $(SRCS) $(LDLIBS)

$(SUBDIRS): FRC
	@if [ -f $@/Makefile  ]; then \
		cd $@; pwd; $(MAKE) $(TARGET); \
	else \
		true; \
	fi

FRC:
