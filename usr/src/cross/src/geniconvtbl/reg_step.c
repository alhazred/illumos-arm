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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <widec.h>
#include "_regexp.h"

#define	ecmp(s1, s2, n)	(strncmp(s1, s2, n) == 0)
#define	Popwchar(p, l) mbtowc(&l, p, MB_LEN_MAX)
#define	uletter(c) (isalpha(c) || c == '_')
#define	_NBRA	128

char	*loc1 = (char *)0, *loc2 = (char *)0, *locs = (char *)0;
char	*braslist[_NBRA] = { (char *)0};
char	*braelist[_NBRA] = { (char *)0};

#ifdef  _REENTRANT
static thread_key_t key = THR_ONCE_KEY;
typedef struct _vars_storage {
	char	*loc1, *loc2, *locs;
	char    *braslist[_NBRA];
	char    *braelist[_NBRA];
} vars_storage;
#endif

static unsigned char _bittab[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
static void getrnge(char *);
static int cclass(char *, char **, int);
static int	low;
static int	size;
static int _advance(char *, char *);
static char *start;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; 

int
step(char *p1, char *p2)
{
	int	c;
	wchar_t cl;
	int	n;
	int	ret;

	/* check if match is restricted to beginning of string */
	(void) pthread_mutex_lock(&lock);
	start = p1;
	if (*p2++) {
		loc1 = p1;
		ret = _advance(p1, p2);
		(void) pthread_mutex_unlock(&lock);
		return (ret);
	}
	if (*p2 == CCHR) {
	/* fast check for first character */
		c = p2[1];
		do {
			if (*p1 != c)
				continue;
			if (_advance(p1, p2)) {
				loc1 = p1;
				(void) pthread_mutex_unlock(&lock);
				return (1);
			}
		} while (*p1++);
	}
	else
		/* regular algorithm */
		do {
			if (_advance(p1, p2)) {
				loc1 = p1;
				(void) pthread_mutex_unlock(&lock);
				return (1);
			}
		} while (*p1++);
	(void) pthread_mutex_unlock(&lock);
	return (0);
}

int
advance(char *lp, char *ep)
{
	int ret;

	(void) pthread_mutex_lock(&lock);
	/* ignore flag to see if expression is anchored */
	start = lp;
	ret = _advance(lp, ++ep);
	(void) pthread_mutex_unlock(&lock);
	return (ret);
}

static int
_advance(char *lp, char *ep)
{
	char 	*rp;
	char 	*curlp;
	wchar_t	c, d;
	int 	n;
	wchar_t cl;
	int 	neg;
	char 	*bbeg;
	int 	ct;

	for (;;) {
		neg = 0;
		switch (*ep++) {

		case CCHR:
			if (*ep++ == *lp++)
				continue;
			return (0);

		case MCCHR:
			ep += Popwchar(ep, cl);
			c = cl;
			if ((n = Popwchar(lp, cl)) <= 0 || c !=  cl)
				return (0);
			lp += n;
			continue;

		case CDOT:
			/*
			 * match any characters except NULL
			 */
			if ((n = Popwchar(lp, cl)) > 0) {
				lp += n;
				continue;
			} else if (n < 0) {
				lp++;
				continue;
			} else {
				return (0);
			}
		case CDOL:
			if (*lp == 0)
				continue;
			return (0);

		case CCEOF:
			loc2 = lp;
			return (1);

		case CCL:
			c = (unsigned char)*lp++;
			if (ISTHERE(c)) {
				ep += 32;
				continue;
			}
			return (0);

		case NMCCL:
			neg = 1;
			/* FALLTHRU */

		case MCCL:
			rp = lp;
			if (cclass(ep, &rp, neg) != 1)
				return (0);
			ep += *(ep + 32) + 32;
			lp = rp;
			continue;

		case CBRA:
			braslist[*ep++] = lp;
			continue;

		case CKET:
			braelist[*ep++] = lp;
			continue;

		case MCCHR | RNGE:
			ep += Popwchar(ep, cl);
			c = cl;
			getrnge(ep);
			while (low--) {
				if ((n = Popwchar(lp, cl)) <= 0 || cl != c)
					return (0);
				lp += n;
			}
			curlp = lp;
			while (size--) {
				if ((n = Popwchar(lp, cl)) <= 0 || cl != c)
					break;
				lp += n;
			}
			if (size < 0)
				n = Popwchar(lp, cl);
			if (n == -1)
				return (0);
			lp += (n ? n : 1);
			ep += 2;
			goto mstar;

		case CCHR | RNGE:
			c = *ep++;
			getrnge(ep);
			while (low--)
				if (*lp++ != c)
					return (0);
			curlp = lp;
			while (size--)
				if (*lp++ != c)
					break;
			if (size < 0)
				lp++;
			ep += 2;
			goto star;

		case CDOT | RNGE:
			getrnge(ep);
			while (low--) {
				if ((n = Popwchar(lp, cl)) > 0) {
					lp += n;
				} else if (n < 0) {
					lp++;
				} else {
					return (0);
				}
			}
			curlp = lp;
			while (size--) {
				if ((n = Popwchar(lp, cl)) > 0) {
					lp += n;
				} else if (n < 0) {
					lp++;
				} else {
					break;
				}
			}
			if (size < 0)
				n = Popwchar(lp, cl);
			if (n > 0) {
				lp += n;
			} else {
				lp++;
			}
			ep += 2;
			goto mstar;

		case NMCCL | RNGE:
			neg = 1;
			/* FALLTHRU */

		case MCCL | RNGE:
			getrnge(ep + *(ep + 32) + 32);
			rp = lp;
			while (low--) {
				if (cclass(ep, &rp, neg) != 1)
					return (0);
			}
			curlp = rp;
			while (size-- && (c = (cclass(ep, &rp, neg))) == 1)
				;
			if (c == -1)
				return (0);
			lp = rp;
			if (size < 0) {
				if ((n = Popwchar(lp, cl)) == -1)
					return (0);
				lp += (n ? n : 1);
			}
			ep += *(ep + 32) + 34;
			goto mstar;

		case CCL | RNGE:
			getrnge(ep + 32);
			while (low--) {
				c = (unsigned char)*lp++;
				if (!ISTHERE(c))
					return (0);
			}
			curlp = lp;
			while (size--) {
				c = (unsigned char)*lp++;
				if (!ISTHERE(c))
					break;
			}
			if (size < 0)
				lp++;
			ep += 34;		/* 32 + 2 */
			goto star;

		case CBACK:
			bbeg = braslist[*ep];
			ct = (int)(braelist[*ep++] - bbeg);

			if (ecmp(bbeg, lp, ct)) {
				lp += ct;
				continue;
			}
			return (0);

		case CBACK | STAR:
			bbeg = braslist[*ep];
			ct = (int)(braelist[*ep++] - bbeg);
			curlp = lp;
			while (ecmp(bbeg, lp, ct))
				lp += ct;

			while (lp >= curlp) {
				if (_advance(lp, ep))
					return (1);
				lp -= ct;
			}
			return (0);

		case CDOT | STAR:
			curlp = lp;
			while (*lp++)
				;
			goto mstar;

		case CCHR | STAR:
			curlp = lp;
			while (*lp++ == *ep)
				;
			ep++;
			goto star;

		case MCCHR | STAR:
			curlp = lp;
			ep += Popwchar(ep, cl);
			c = cl;
			while ((n = Popwchar(lp, cl))  > 0 && cl == c)
				lp += n;
			if (n == -1)
				return (0);
			lp += (n ? n : 1);
			goto mstar;

		case NMCCL | STAR:
			neg = 1;
			/* FALLTHRU */

		case MCCL | STAR:
			curlp = rp = lp;
			while ((d = cclass(ep, &rp, neg)) == 1)
				;
			if (d == -1)
				return (0);
			lp = rp;
			ep += *(ep + 32) + 32;
			goto mstar;

		case CCL | STAR:
			curlp = lp;
			do {
				c = (unsigned char)*lp++;
			} while (ISTHERE(c));
			ep += 32;
			goto star;

		case CBRC:
			if (lp == start && locs == (char *)0)
				continue;
			c = (unsigned char)*lp;
			d = (unsigned char)*(lp-1);
			if ((isdigit((int)c) || uletter((int)c) || c >= 0200 &&
			    MB_CUR_MAX > 1) && !isdigit((int)d) &&
			    !uletter((int)d) &&
			    (d < 0200 || MB_CUR_MAX == 1))
				continue;
			return (0);

		case CLET:
			d = (unsigned char)*lp;
			if (!isdigit((int)d) && !uletter((int)d) && (d < 0200 ||
			    MB_CUR_MAX == 1))
				continue;
			return (0);

		default:
			return (0);
		}
	}

mstar:
star:
	do {
		if (--lp == locs)
			break;
		if (_advance(lp, ep))
			return (1);
	} while (lp > curlp);
	return (0);
}

static void
getrnge(char *str)
{
	low = *str++ & 0377;
	size = (*str == (char)255)? 20000: (*str &0377) - low;
}

static int
cclass(char *ep, char **rp, int neg)
{
	char	*lp;
	wchar_t	c, d, f = 0;
	int	n;
	wchar_t	cl;
	char	*endep;

	lp = *rp;
	if ((n = Popwchar(lp, cl)) == -1)
		return (-1);
	*rp = lp + (n ? n : 1);
	c = cl;
	/* look for eight bit characters in bitmap */
	if (c <= 0177 || c <= 0377 && iscntrl((int)c))
		return (ISTHERE(c) && !neg || !ISTHERE(c) && neg);
	else {
		/* look past bitmap for multibyte characters */
		endep = *(ep + 32) + ep + 32;
		ep += 33;
		for (;;) {
			if (ep >= endep)
				return (neg);
			ep += Popwchar(ep, cl);
			d = cl;
			if (d == '-') {
				ep += Popwchar(ep, cl);
				d = cl;
				if (f <= c && c <= d)
					return (!neg);
			}
			if (d == c)
				return (!neg);
			f = d;
		}
	}
	/*NOTREACHED*/
}
