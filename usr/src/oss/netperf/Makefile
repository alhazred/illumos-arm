#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
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

SOURCE=netperf-2.7.0.tar.gz
SOURCE_DIR=$(SOURCE:%.tar.gz=%)
PATCH=$(SOURCE:%.tar.gz=%.patch)
CONFIG_ADDITIONAL_CFLAGS= -fcommon -lxnet -lsocket -lnsl -lkstat -D_XOPEN_SOURCE=500 -D__EXTENSIONS__ -std=gnu90
CONFIG_ENV=ac_cv_func_setpgrp_void=yes
include ../Makefile.oss

