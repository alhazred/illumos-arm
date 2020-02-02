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

#include <sys/types.h>
#include <sys/machclock.h>
#include <sys/platform.h>
#include <sys/modctl.h>
#include <sys/platmod.h>
#include <sys/promif.h>
#include <sys/errno.h>
#include <sys/byteorder.h>
#include <sys/gpio.h>
#include <sys/sysmacros.h>
#include "boot_plat.h"

#define GPIO_INPUT_VAL	0x00
#define GPIO_INPUT_EN	0x04
#define GPIO_OUTPUT_EN	0x08
#define GPIO_OUTPUT_VAL	0x0C
#define GPIO_PUE	0x10
#define GPIO_DS		0x14
#define GPIO_RISE_IE	0x18
#define GPIO_RISE_IP	0x1C
#define GPIO_FALL_IE	0x20
#define GPIO_FALL_IP	0x24
#define GPIO_HIGH_IE	0x28
#define GPIO_HIGH_IP	0x2C
#define GPIO_LOW_IE	0x30
#define GPIO_LOW_IP	0x34
#define GPIO_OUT_XOR	0x40

static uintptr_t
gpio_base_addr(pnode_t node)
{
	static uintptr_t base;
	static pnode_t nodeid;
	if (nodeid == node)
		return base;

	uint64_t regbase;
	uint64_t regsize;
	if (prom_get_reg_address(node, 0, &regbase) != 0)
		return 0;
	if (prom_get_reg_size(node, 0, &regsize) != 0)
		return 0;

	regsize = roundup(regsize, MMU_PAGESIZE);
	base = memlist_get(regsize, MMU_PAGESIZE, &ptmplistp);
	map_phys(PTE_A | PTE_D | PTE_G | PTE_W | PTE_R, (caddr_t)base, regbase, regsize);
	nodeid = node;

	return base;
}

int plat_gpio_direction_output(struct gpio_ctrl *gpio, int value)
{
	if (!prom_is_compatible(gpio->node, "sifive,gpio0"))
		return -1;
	uintptr_t base = gpio_base_addr(gpio->node);
	*(volatile uint32_t *)(base + GPIO_INPUT_EN) &= ~(1u << gpio->pin);
	if (value)
		*(volatile uint32_t *)(base + GPIO_OUTPUT_VAL) |= (1u << gpio->pin);
	else
		*(volatile uint32_t *)(base + GPIO_OUTPUT_VAL) &= ~(1u << gpio->pin);
	*(volatile uint32_t *)(base + GPIO_OUTPUT_EN) |= (1u << gpio->pin);
	return 0;
}

int plat_gpio_direction_input(struct gpio_ctrl *gpio)
{
	if (!prom_is_compatible(gpio->node, "sifive,gpio0"))
		return -1;
	uintptr_t base = gpio_base_addr(gpio->node);
	*(volatile uint32_t *)(base + GPIO_OUTPUT_EN) &= ~(1u << gpio->pin);
	*(volatile uint32_t *)(base + GPIO_INPUT_EN) |= (1u << gpio->pin);
	return 0;
}

int plat_gpio_get(struct gpio_ctrl *gpio)
{
	if (!prom_is_compatible(gpio->node, "sifive,gpio0"))
		return -1;
	uintptr_t base = gpio_base_addr(gpio->node);
	return ((*(volatile uint32_t *)(base + GPIO_INPUT_VAL)) >> gpio->pin) & 1;
}

int plat_gpio_set(struct gpio_ctrl *gpio, int value)
{
	if (!prom_is_compatible(gpio->node, "sifive,gpio0"))
		return -1;
	uintptr_t base = gpio_base_addr(gpio->node);
	if (value)
		*(volatile uint32_t *)(base + GPIO_OUTPUT_VAL) |= (1u << gpio->pin);
	else
		*(volatile uint32_t *)(base + GPIO_OUTPUT_VAL) &= ~(1u << gpio->pin);
	return 0;
}

int plat_gpio_set_pullup(struct gpio_ctrl *gpio, int value)
{
	return -1;
}

