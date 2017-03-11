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

#pragma once

#include <sys/types.h>
#include <sys/pci.h>

extern uintptr_t get_config_base(int hose, int bus, int dev, int func);
extern uint8_t   pci_conf_read8(uintptr_t conf_base, int offset);
extern uint16_t  pci_conf_read16(uintptr_t conf_base, int offset);
extern uint32_t  pci_conf_read32(uintptr_t conf_base, int offset);
extern void      pci_conf_write8(uintptr_t conf_base, int offset,  uint8_t v);
extern void      pci_conf_write16(uintptr_t conf_base, int offset, uint16_t v);
extern void      pci_conf_write32(uintptr_t conf_base, int offset, uint32_t v);

extern uint8_t   pci_read8(int hose, uintptr_t addr);
extern uint16_t  pci_read16(int hose, uintptr_t addr);
extern uint32_t  pci_read32(int hose, uintptr_t addr);
extern void      pci_write8(int hose, uintptr_t addr,  uint8_t v);
extern void      pci_write16(int hose, uintptr_t addr, uint16_t v);
extern void      pci_write32(int hose, uintptr_t addr, uint32_t v);
extern uint64_t  virt_to_pci(void *va);
extern void      prom_pci_setup(void);
