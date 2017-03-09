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
 */

#define FPCR_SUM  (1ul<<63)
#define FPCR_INED (1ul<<62)
#define FPCR_UNFD (1ul<<61)
#define FPCR_UNDZ (1ul<<60)
#define FPCR_DYN_MASK  (3ul<<58)
#define FPCR_DYN(x)  ((x)<<58)
#define FPCR_IOV  (1ul<<57)
#define FPCR_INE  (1ul<<56)
#define FPCR_UNF  (1ul<<55)
#define FPCR_OVF  (1ul<<54)
#define FPCR_DZE  (1ul<<53)
#define FPCR_INV  (1ul<<52)
#define FPCR_OVFD (1ul<<51)
#define FPCR_DZED (1ul<<50)
#define FPCR_INVD (1ul<<49)
#define FPCR_DNZ  (1ul<<48)
#define FPCR_DNOD (1ul<<47)

#define FPCR_TRAP_DISABLE \
    (FPCR_DNOD|FPCR_INVD|FPCR_DZED|FPCR_OVFD|FPCR_UNFD|FPCR_INED)

#define FPCR_TRAP_STATUS \
    (FPCR_INV|FPCR_DZE|FPCR_OVF|FPCR_UNF|FPCR_INE|FPCR_IOV|FPCR_SUM)

void __get_fpcr(unsigned long *fpcr);
void __set_fpcr(unsigned long *fpcr);
