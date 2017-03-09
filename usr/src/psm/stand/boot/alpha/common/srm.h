/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2017 Hayashi Naoyuki
 */
#ifndef _SRM_H
#define _SRM_H

#ifdef __cplusplus
extern "C" {
#endif

extern int srm_getenv(int index, char *envval, int maxlen);

enum {
	ENV_AUTO_ACTION = 0x01,
	ENV_BOOT_DEV, ENV_BOOTDEF_DEV, ENV_BOOTED_DEV, ENV_BOOT_FILE, ENV_BOOTED_FILE,
	ENV_BOOT_OSFLAGS, ENV_BOOTED_OSFLAGS, ENV_BOOT_RESET, ENV_DUMP_DEV,
	ENV_ENABLE_AUDIT, ENV_LICENSE, ENV_CHAR_SET, ENV_LANGUAGE, ENV_TTY_DEV
};

#ifdef __cplusplus
}
#endif

#endif /* _SRM_H */
