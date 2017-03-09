#
# Copyright 2017 Hayashi Naoyuki
# Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

# include global definitions
include ../../Makefile.master
plugins: dyn kdb kadm5 ss
kadm5: dyn kdb
SUBDIRS= \
	dyn	\
	\
	kdb	\
	\
	kadm5	\
	\
	ss	\
	plugins

all : 		TARGET= all
install : 	TARGET= install
clean : 	TARGET= clean
clobber : 	TARGET= clobber
_msg : 		TARGET= _msg

.KEEP_STATE:

all install clean clobber _msg: $(SUBDIRS)

$(SUBDIRS): FRC
	@cd $@; pwd; $(MAKE) $(TARGET)

FRC:
