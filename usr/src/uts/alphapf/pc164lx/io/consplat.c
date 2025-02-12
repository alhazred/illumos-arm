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
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * isa-specific console configuration routines
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/cmn_err.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/esunddi.h>
#include <sys/ddi_impldefs.h>
#include <sys/promif.h>
#include <sys/modctl.h>
#include <sys/termios.h>
#include <sys/pci.h>

int
plat_use_polled_debug()
{
	return (0);
}

int
plat_support_serial_kbd_and_ms()
{
	return (0);
}

#define	A_CNT(arr)	(sizeof (arr) / sizeof (arr[0]))

#define	CONS_INVALID	-1
#define	CONS_SCREEN	0
#define	CONS_TTYA	1
#define	CONS_TTYB	2

char *plat_fbpath(void);

static int
console_type()
{
#if 1
	static int boot_console = CONS_TTYA;
#else
	static int boot_console = CONS_INVALID;

	char *cons;
	dev_info_t *root;

	if (boot_console != CONS_INVALID)
		return (boot_console);

	/*
	 * console is defined by "console" property, with
	 * fallback on the old "input-device" property.
	 * If "input-device" is not defined either, also check "output-device".
	 */
	boot_console = CONS_SCREEN;	/* default is screen/kb */
	root = ddi_root_node();
	if ((ddi_prop_lookup_string(DDI_DEV_T_ANY, root,
	    DDI_PROP_DONTPASS, "console", &cons) == DDI_SUCCESS) ||
	    (ddi_prop_lookup_string(DDI_DEV_T_ANY, root,
	    DDI_PROP_DONTPASS, "input-device", &cons) == DDI_SUCCESS) ||
	    (ddi_prop_lookup_string(DDI_DEV_T_ANY, root,
	    DDI_PROP_DONTPASS, "output-device", &cons) == DDI_SUCCESS)) {
		if (strcmp(cons, "ttya") == 0) {
			boot_console = CONS_TTYA;
		} else if (strcmp(cons, "ttyb") == 0) {
			boot_console = CONS_TTYB;
		}
		ddi_prop_free(cons);
	}

	/*
	 * If the console is configured to use a framebuffer but none
	 * could be found, fallback to "ttya" since it's likely to exist
	 * and it matches longstanding behavior on SPARC.
	 */
	if (boot_console == CONS_SCREEN && plat_fbpath() == NULL)
		boot_console = CONS_TTYA;
#endif

	return (boot_console);
}

int
plat_stdin_is_keyboard(void)
{
	return (console_type() == CONS_SCREEN);
}

int
plat_stdout_is_framebuffer(void)
{
	return (console_type() == CONS_SCREEN);
}

static char *
plat_devpath(char *name, char *path)
{
	major_t major;
	dev_info_t *dip, *pdip;

	if ((major = ddi_name_to_major(name)) == (major_t)-1)
		return (NULL);

	if ((dip = devnamesp[major].dn_head) == NULL)
		return (NULL);

	pdip = ddi_get_parent(dip);
	if (i_ddi_attach_node_hierarchy(pdip) != DDI_SUCCESS)
		return (NULL);
	if (ddi_initchild(pdip, dip) != DDI_SUCCESS)
		return (NULL);

	(void) ddi_pathname(dip, path);

	return (path);
}

/*
 * Return generic path to keyboard device from the alias.
 */
char *
plat_kbdpath(void)
{
	static char kbpath[MAXPATHLEN];

	if (plat_devpath("kb8042", kbpath) == NULL)
		return (NULL);

	return (kbpath);
}

/*
 * NOTE: this function is duplicated here and in gfx_private/vgatext while
 *       we work on a set of commitable interfaces to sunpci.c.
 *
 * Use the class code to determine if the device is a PCI-to-PCI bridge.
 * Returns:  B_TRUE  if the device is a bridge.
 *           B_FALSE if the device is not a bridge or the property cannot be
 *		     retrieved.
 */
static boolean_t
is_pci_bridge(dev_info_t *dip)
{
	uint32_t class_code;

	class_code = (uint32_t)ddi_prop_get_int(DDI_DEV_T_ANY, dip,
	    DDI_PROP_DONTPASS, "class-code", 0xffffffff);

	if (class_code == 0xffffffff || class_code == DDI_PROP_NOT_FOUND)
		return (B_FALSE);

	class_code &= 0x00ffff00;
	if (class_code == ((PCI_CLASS_BRIDGE << 16) | (PCI_BRIDGE_PCI << 8)))
		return (B_TRUE);

	return (B_FALSE);
}

/*
 * Type for the parameter of find_fb_dev()
 */
struct find_fb_dev_param
{
	dev_info_t *found_dip;	/* dip found for VGA console */
	int vga_enable;		/* check PCI_BCNF_BCNTRL_VGA_ENABLE or not */
};

/*
 * The VGA device could be under a subtractive PCI bridge on some systems.
 * Though the PCI_BCNF_BCNTRL_VGA_ENABLE bit is not set on such subtractive
 * PCI bridge, the subtractive PCI bridge can forward VGA access if no other
 * agent claims the access.
 * The vga_enable element in param acts as a flag, if not set, ignore the
 * checking for the PCI_BCNF_BCNTRL_VGA_ENABLE bit of the PCI bridge during
 * the search.
 */
static int
find_fb_dev(dev_info_t *dip, void *param)
{
	struct find_fb_dev_param *p = param;
	char *dev_type;
	dev_info_t *pdip;
	char *parent_type;

	if (dip == ddi_root_node())
		return (DDI_WALK_CONTINUE);

	if (ddi_prop_lookup_string(DDI_DEV_T_ANY, dip, DDI_PROP_DONTPASS,
	    "device_type", &dev_type) != DDI_SUCCESS)
		return (DDI_WALK_PRUNECHILD);

	if ((strcmp(dev_type, "isa") == 0) || (strcmp(dev_type, "eisa") == 0)) {
		ddi_prop_free(dev_type);
		return (DDI_WALK_CONTINUE);
	}

	if (strcmp(dev_type, "pci") == 0) {
		ddi_acc_handle_t pci_conf;
		uint16_t data16;
		char *nodename;

		ddi_prop_free(dev_type);

		if (!p->vga_enable)
			return (DDI_WALK_CONTINUE);

		nodename = ddi_node_name(dip);

		/*
		 * If the node is not a PCI-to-PCI bridge, continue traversing
		 * (it could be the root node), otherwise, check for the
		 * VGAEnable bit to be set in the Bridge Control Register.
		 */
		if (strcmp(nodename, "pci") == 0) {
			if (is_pci_bridge(dip) == B_FALSE)
				return (DDI_WALK_CONTINUE);
		}

		if (i_ddi_attach_node_hierarchy(dip) != DDI_SUCCESS)
			return (DDI_WALK_PRUNECHILD);

		if (pci_config_setup(dip, &pci_conf) != DDI_SUCCESS)
			return (DDI_WALK_PRUNECHILD);

		data16 = pci_config_get16(pci_conf, PCI_BCNF_BCNTRL);
		pci_config_teardown(&pci_conf);

		if (data16 & PCI_BCNF_BCNTRL_VGA_ENABLE)
			return (DDI_WALK_CONTINUE);

		return (DDI_WALK_PRUNECHILD);
	}

	if (strcmp(dev_type, "display") != 0) {
		ddi_prop_free(dev_type);
		return (DDI_WALK_CONTINUE);
	}

	ddi_prop_free(dev_type);

	if ((pdip = ddi_get_parent(dip)) == NULL)
		return (DDI_WALK_PRUNECHILD);

	if (ddi_prop_lookup_string(DDI_DEV_T_ANY, pdip, DDI_PROP_DONTPASS,
	    "device_type", &parent_type) != DDI_SUCCESS)
		return (DDI_WALK_PRUNECHILD);

	if ((strcmp(parent_type, "isa") == 0) ||
	    (strcmp(parent_type, "eisa") == 0)) {
		p->found_dip = dip;
		ddi_prop_free(parent_type);
		return (DDI_WALK_TERMINATE);
	}

	if (strcmp(parent_type, "pci") == 0) {
		ddi_acc_handle_t pci_conf;
		uint16_t data16;

		ddi_prop_free(parent_type);

		if (i_ddi_attach_node_hierarchy(dip) != DDI_SUCCESS)
			return (DDI_WALK_PRUNECHILD);

		if (pci_config_setup(dip, &pci_conf) != DDI_SUCCESS)
			return (DDI_WALK_PRUNECHILD);

		data16 = pci_config_get16(pci_conf, PCI_CONF_COMM);
		pci_config_teardown(&pci_conf);

		if (!(data16 & PCI_COMM_IO))
			return (DDI_WALK_PRUNECHILD);

		p->found_dip = dip;
		return (DDI_WALK_TERMINATE);
	}

	ddi_prop_free(parent_type);
	return (DDI_WALK_PRUNECHILD);
}

/*
 * The first round search is to find:
 * 1) a VGA device.
 * 2) a PCI VGA compatible device whose IO space is enabled
 *    and the VGA Enable bit of any PCI-PCI bridge above it is set.
 * If the first round search succeeds, prune the second round search.
 *
 * The second round seach does not check the VGA Enable bit.
 *
 * Return the device path as the console fb path.
 */
char *
plat_fbpath(void)
{
	struct find_fb_dev_param param;
	static char *fbpath = NULL;
	static char fbpath_buf[MAXPATHLEN];

	/* first round search */
	param.found_dip = NULL;
	param.vga_enable = 1;
	ddi_walk_devs(ddi_root_node(), find_fb_dev, &param);

	if (param.found_dip != NULL) {
		(void) ddi_pathname(param.found_dip, fbpath_buf);
		fbpath = fbpath_buf;
		return (fbpath);
	}

	/*
	 * second round search, do not check the
	 * PCI_BCNF_BCNTRL_VGA_ENABLE bit
	 */
	param.found_dip = NULL;
	param.vga_enable = 0;
	ddi_walk_devs(ddi_root_node(), find_fb_dev, &param);

	if (param.found_dip == NULL)
		return (NULL);

	(void) ddi_pathname(param.found_dip, fbpath_buf);
	fbpath = fbpath_buf;
	return (fbpath);
}

char *
plat_mousepath(void)
{
	static char mpath[MAXPATHLEN];

	/*
	 * Hardcode to isa mouse path
	 * XXX make it settable via bootprop?
	 */
	if (plat_devpath("mouse8042", mpath) == NULL)
		return (NULL);

	return (mpath);
}

static char *
plat_ttypath(int inum)
{
	static char path[MAXPATHLEN];
	char *bp;
	major_t major;
	dev_info_t *dip;

	if ((major = ddi_name_to_major("asy")) == (major_t)-1)
		return (NULL);

	if ((dip = devnamesp[major].dn_head) == NULL)
		return (NULL);

	for (; dip != NULL; dip = ddi_get_next(dip)) {
		if (i_ddi_attach_node_hierarchy(dip) != DDI_SUCCESS)
			return (NULL);

		if (DEVI(dip)->devi_minor->ddm_name[0] == ('a' + (char)inum))
			break;
	}
	if (dip == NULL)
		return (NULL);

	(void) ddi_pathname(dip, path);
	bp = path + strlen(path);
	(void) snprintf(bp, 3, ":%s", DEVI(dip)->devi_minor->ddm_name);

	return (path);
}

/*
 * Lacking support for com2 and com3, if that matters.
 * Another possible enhancement could be to use properties
 * for the port mapping rather than simply hard-code them.
 */
char *
plat_stdinpath(void)
{
	char *path;
	switch (console_type()) {
	case CONS_TTYA:
		path = plat_ttypath(0);
		break;
	case CONS_TTYB:
		path = plat_ttypath(1);
		break;
	case CONS_SCREEN:
	default:
		path = plat_kbdpath();
		break;
	};
	return path;
}

char *
plat_stdoutpath(void)
{
	char *path;
	switch (console_type()) {
	case CONS_TTYA:
		path = plat_ttypath(0);
		break;
	case CONS_TTYB:
		path = plat_ttypath(1);
		break;
	case CONS_SCREEN:
	default:
		path = plat_fbpath();
		break;
	};
	return path;
}

char *
plat_diagpath(void)
{
	char *str = plat_stdoutpath();
	return str;
}

void
plat_tem_get_colors(uint8_t *fg, uint8_t *bg)
{
}

void
plat_tem_get_inverses(int *inverse, int *inverse_screen)
{
	*inverse = 0;
	*inverse_screen = 0;
}

void
plat_tem_get_prom_font_size(int *charheight, int *windowtop)
{
	*charheight = 0;
	*windowtop = 0;
}

/*ARGSUSED*/
void
plat_tem_get_prom_size(size_t *height, size_t *width)
{
	panic("unimplemented at line %d of %s", __LINE__, __FILE__);
}

void
plat_tem_hide_prom_cursor(void)
{
	panic("unimplemented at line %d of %s", __LINE__, __FILE__);
}

/*ARGSUSED*/
void
plat_tem_get_prom_pos(uint32_t *row, uint32_t *col)
{
	panic("unimplemented at line %d of %s", __LINE__, __FILE__);
}
