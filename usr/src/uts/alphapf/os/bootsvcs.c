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

#include <sys/bootsvcs.h>
#include <sys/pal.h>

extern uint64_t cons_dispatch(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static int initialized;
static int cons_dev;
enum {
	CCB_GETC = 0x01,
	CCB_PUTS, CCB_RESET_TERM, CCB_SET_TERM_INT, CCB_SET_TERM_CTL, CCB_PROCESS_KEYCODE,
	CCB_CONSOLE_OPEN, CCB_CONSOLE_CLOSE,
	CCB_OPEN = 0x10,
	CCB_CLOSE, CCB_IOCTL, CCB_READ, CCB_WRITE,
	CCB_SET_ENV = 0x20,
	CCB_RESET_ENV, CCB_GET_ENV, CCB_SAVE_ENV,
	CCB_PSWITCH = 0x30,
	CCB_BIOS_EMUL = 0x32
};
enum {
	ENV_AUTO_ACTION = 0x01,
	ENV_BOOT_DEV, ENV_BOOTDEF_DEV, ENV_BOOTED_DEV, ENV_BOOT_FILE, ENV_BOOTED_FILE,
	ENV_BOOT_OSFLAGS, ENV_BOOTED_OSFLAGS, ENV_BOOT_RESET, ENV_DUMP_DEV,
	ENV_ENABLE_AUDIT, ENV_LICENSE, ENV_CHAR_SET, ENV_LANGUAGE, ENV_TTY_DEV
};

static int
srm_getenv(int index, char *envval, int maxlen)
{
	union {
		uint64_t ret;
		struct {
			uint32_t len;
			uint32_t rsv: 29;
			uint32_t rst: 3;
		} s;
	} ccb_sts;

	ccb_sts.ret = cons_dispatch(CCB_GET_ENV, index, (uint64_t)envval, maxlen - 1, 0);

	return (ccb_sts.s.rst == 0)? 0: -1;
}

static void
cons_init(void)
{
	union {
		char b[8];
		uint64_t qw;
	} buf;
	srm_getenv(ENV_TTY_DEV, buf.b, sizeof(buf));
	cons_dev = buf.b[0] - '0'; // should be 0..9
	initialized = 1;
}

static int _getchar()
{
	if (!initialized)
		cons_init();

	union {
		uint64_t ret;
		struct {
			uint32_t c;
			uint32_t rsv: 29;
			uint32_t rst: 3;
		} s;
	} ccb_sts;

	ccb_sts.ret = cons_dispatch(CCB_GETC, cons_dev, 0, 0, 0);

	if (ccb_sts.s.rst == 0 || ccb_sts.s.rst == 1) {
		return ccb_sts.s.c & 0xFF;
	} else {
		return -1;
	}
}

static void _putchar(int c)
{
	if (!initialized)
		cons_init();
	char b = c;
	for (;;) {
		union {
			uint64_t ret;
			struct {
				uint32_t len;
				uint32_t rsv: 29;
				uint32_t rst: 3;
			} s;
		} ccb_sts;
		ccb_sts.ret = cons_dispatch(CCB_PUTS, cons_dev, (uint64_t)&b, 1, 0);
		if (ccb_sts.s.len == 1)
			break;
	}
}

static void _reset() __NORETURN;
static void _reset()
{
	pal_swpipl(6);
	_putchar('r');
	_putchar('e');
	_putchar('s');
	_putchar('e');
	_putchar('t');
	_putchar('\r');
	_putchar('\n');
	asm volatile("call_pal %0" :: "i" (PAL_halt):);
	for (;;) {}
}

static struct boot_syscalls _sysp =
{
	.bsvc_getchar = _getchar,
	.bsvc_putchar = _putchar,
	.bsvc_reset = _reset,
};

struct boot_syscalls *sysp = &_sysp;

