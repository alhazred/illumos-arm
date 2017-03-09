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

#include <sys/sbi.h>
#include "prom_dev.h"

static void initialize()
{
	static int initialized = 0;
	if (initialized == 0)
	{
		// initialized by u-boot
		initialized = 1;
	}
}

static ssize_t console_gets(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	int i;
	for (i = 0; i < len; i++) {
		int c = sbi_console_getchar();
		if (c < 0)
			break;
		buf[i] = c;
	}
	return i;
}

static void console_putc(int c)
{
	sbi_console_putchar(c);

	if (c == '\n')
		console_putc('\r');
}
static ssize_t console_puts(int dev, caddr_t buf, size_t len, uint_t startblk)
{
	for (int i = 0; i < len; i++)
		console_putc(buf[i]);
	return len;
}

static int console_open(const char *name)
{
	initialize();
	return 0;
}

static int
stdin_match(const char *name)
{
	return !strcmp(name, "stdin");
}

static int
stdout_match(const char *name)
{
	return !strcmp(name, "stdout");
}

static struct prom_dev stdin_dev =
{
	.match = stdin_match,
	.open = console_open,
	.read = console_gets,
};

static struct prom_dev stdout_dev =
{
	.match = stdout_match,
	.open = console_open,
	.write = console_puts,
};

void init_console(void)
{
	prom_register(&stdout_dev);
	prom_register(&stdin_dev);
}
