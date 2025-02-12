/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef _SYS_MACHCLOCK_H
#define	_SYS_MACHCLOCK_H

#include <sys/time.h>

#ifdef	__cplusplus
extern "C" {
#endif

struct tod_ops;
typedef struct tod_ops tod_ops_t;

/*
 * tod module name and operations
 */
struct tod_ops {
	timestruc_t	(*tod_get)(void);
	void		(*tod_set)(timestruc_t);
	uint_t		(*tod_set_watchdog_timer)(uint_t);
	uint_t		(*tod_clear_watchdog_timer)(void);
	void		(*tod_set_power_alarm)(timestruc_t);
	void		(*tod_clear_power_alarm)(void);
};
extern struct tod_ops	tod_ops;
extern char		*tod_module_name;

/*
 * These defines allow common code to use TOD functions independant
 * of hardware platform.
 */
#define	TODOP_GET(top)		((top).tod_get())
#define	TODOP_SET(top, ts)	((top).tod_set(ts))
#define	TODOP_SETWD(top, nsec)	((top).tod_set_watchdog_timer(nsec))
#define	TODOP_CLRWD(top)	((top).tod_clear_watchdog_timer())
#define	TODOP_SETWAKE(top, ts)	((top).tod_set_power_alarm(ts))
#define	TODOP_CLRWAKE(top)	((top).tod_clear_power_alarm())

#ifdef	__cplusplus
}
#endif

#endif	/* !_SYS_MACHCLOCK_H */
