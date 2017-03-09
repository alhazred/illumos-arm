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
/*
 * HWRPB (Hardware Restart Parameter Block).
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define HWRPB_ADDR	0x10000000	/* virtual address, at boot */

/* Alpha CPU ID */
#define EV3_CPU		1	/* EV3 */
#define EV4_CPU		2	/* EV4 (21064) */
#define LCA4_CPU	4	/* LCA4 (21066/21068) */
#define EV5_CPU		5	/* EV5 (21164) */
#define EV45_CPU	6	/* EV4.5 (21064A) */
#define EV56_CPU	7	/* EV5.6 (21164A) */
#define EV6_CPU		8	/* EV6 (21264) */
#define PCA56_CPU	9	/* EV5.6 (21164PC) */
#define PCA57_CPU	10	/* EV5.7 (21164PC) */
#define EV67_CPU	11	/* EV6.7 (21264A) */
#define EV68CB_CPU	12	/* EV6.8CB (21264C) */
#define EV68AL_CPU	13	/* EV6.8AL (21264B) */
#define EV68CX_CPU	14	/* EV6.8CX (21264D) */
#define EV69A_CPU	17	/* EV6.9 (21264/EV69A) */
#define EV7_CPU		15	/* EV7 (21364) */
#define EV79_CPU	16	/* EV7.9 (21364a) */

/* System Type */
#define ST_ADU			1	/* Alpha ADU */
#define ST_DEC_4000		2	/* Cobra */
#define ST_DEC_7000		3	/* Ruby */
#define ST_DEC_3000_500		4	/* Flamingo */
#define ST_DEC_2000_300		6	/* Jensen */
#define ST_DEC_3000_300		7	/* Pelican */
#define ST_DEC_2100_A500	9	/* Sable */
#define ST_DEC_AXPVME_64	10	/* AXPvme system type */
#define ST_DEC_AXPPCI_33	11	/* NoName system type */
#define ST_DEC_21000		12	/* Turbolaser */
#define ST_DEC_2100_A50		13	/* Avanti */
#define ST_DEC_MUSTANG		14	/* Mustang */
#define ST_DEC_KN20AA		15	/* kn20aa */
#define ST_DEC_1000		17	/* Mikasa */
#define ST_EB66			19	/* EB66 */
#define ST_EB64P		20	/* EB64+ */
#define ST_ALPHABOOK1		21	/* Alphabook1 */
#define ST_DEC_4100		22	/* RAWHIDE */
#define ST_DEC_EV45_PBP		23	/* LEGO K2 Passive SBC */
#define ST_DEC_2100A_A500	24	/* LYNX */
#define ST_EB164		26	/* EB164 */
#define ST_DEC_1000A		27	/* Noritake */
#define ST_DEC_ALPHAVME_224	28	/* Cortex */
#define ST_DEC_550		30	/* miata */
#define ST_DEC_EV56_PBP		32	/* Takara systpe */
#define ST_DEC_ALPHAVME_320	33	/* Yukon */
#define ST_DEC_6600		34	/* Tsunami */
#define ST_ALPHA_WILDFIRE	35	/* Wildfire */
#define ST_DMCC_EV6		37	/* Eiger */
#define ST_ALPHA_TITAN		38	/* Titan */
#define ST_ALPHA_MARVEL		39	/* Marvel */
#define	ST_API_NAUTILUS		201	/* API UP1000, UP1100, UP1500 */

/* System Variation */
#define SV_MPCAP	0x00000001	/* MP capable */
#define SV_PF_UNITED	0x00000020	/* United */
#define SV_PF_SEPARATE	0x00000040	/* Separate */
#define SV_PF_FULLBB	0x00000060	/* Full battery backup */
#define SV_POWERFAIL	0x000000e0	/* Powerfail implementation */
#define SV_PF_RESTART	0x00000100	/* Powerfail restart */
#define SV_GRAPHICS	0x00000200	/* Embedded graphics processor */

#define SV_STS_MASK	0x0000fc00	/* STS bits - system and I/O board */

#define SV_STS_SANDPIPER	0x00000400	/* Sandpiper 3000/400 */
#define SV_STS_FLAMINGO		0x00000800	/* Flamingo 3000/500 */
#define SV_STS_HOTPINK		0x00000c00	/* Hot Pink 3000/500X */
#define SV_STS_FLAMINGOPLUS	0x00001000	/* Flamingo+ 3000/800 */
#define SV_STS_ULTRA		0x00001400	/* Ultra(Flamingo+) */
#define SV_STS_SANDPLUS		0x00001800	/* Sandpiper+ 3000/600 */
#define SV_STS_SANDPIPER45	0x00001c00	/* Sandpiper45 3000/700 */
#define SV_STS_FLAMINGO45	0x00002000	/* Flamingo45 3000/900 */
#define SV_STS_SABLE		0x00000400
#define SV_STS_PELICAN		0x00000000	/* Pelican 3000/300 */
#define SV_STS_PELICA		0x00000400	/* Pelica 3000/300L */
#define SV_STS_PELICANPLUS	0x00000800	/* Pelican+ 3000/300X */
#define SV_STS_PELICAPLUS	0x00000c00	/* Pelica+ 3000/300LX */
#define SV_STS_AVANTI		0x00000000	/* Avanti 400 4/233 */
#define SV_STS_MUSTANG2_4_166	0x00000800	/* Mustang II 200 4/166 */
#define SV_STS_MUSTANG2_4_233	0x00001000	/* Mustang II 200 4/233 */
#define SV_STS_AVANTI_XXX	0x00001400	/* Avanti 400 4/233 */
#define SV_STS_AVANTI_4_266	0x00002000	/* Avanti 400 4/266 */
#define SV_STS_MUSTANG2_4_100	0x00002400	/* Mustang II 200 4/100 */
#define SV_STS_AVANTI_4_233	0x0000a800	/* AlphaStation 255/233 */
#define SV_STS_KN20AA		0x00000400	/* AlphaStation 500/600 */
#define SV_STS_RAWHIDE		0x00000865	/* RAWHIDE */
#define SV_STS_TINCUP		0x00001c65
#define SV_STS_AXPVME_64	0x00000000	/* AXPvme  64 MHz */
#define SV_STS_AXPVME_160	0x00000400	/* AXPvme 160 MHz */
#define SV_STS_AXPVME_100	0x00000c00	/* AXPvme  99 MHz */
#define SV_STS_AXPVME_230	0x00001000	/* AXPvme 231 MHz */
#define SV_STS_AXPVME_66	0x00001400	/* AXPvme  66 MHz */
#define SV_STS_AXPVME_166	0x00001800	/* AXPvme 165 MHz */
#define SV_STS_AXPVME_264	0x00001c00	/* AXPvme 264 MHz */
#define SV_STS_EB164_266	0x00000400	/* EB164 266MHz */
#define SV_STS_EB164_300	0x00000800	/* EB164 300MHz */
#define SV_STS_ALPHAPC164_366	0x00000c00	/* AlphaPC164 366MHz */
#define SV_STS_ALPHAPC164_400	0x00001000	/* AlphaPC164 400MHz */
#define SV_STS_ALPHAPC164_433	0x00001400	/* AlphaPC164 433MHz */
#define SV_STS_ALPHAPC164_466	0x00001800	/* AlphaPC164 466MHz */
#define SV_STS_ALPHAPC164_500	0x00001c00	/* AlphaPC164 500MHz */
#define SV_STS_ALPHAPC164LX_400 0x00002000	/* AlphaPC164LX 400MHz */
#define SV_STS_ALPHAPC164LX_466 0x00002400	/* AlphaPC164LX 466MHz */
#define SV_STS_ALPHAPC164LX_533 0x00002800	/* AlphaPC164LX 533MHz */
#define SV_STS_ALPHAPC164LX_600 0x00002c00	/* AlphaPC164LX 600MHz */
#define SV_STS_ALPHAPC164SX_400 0x00003000	/* AlphaPC164SX 400MHz */
#define SV_STS_ALPHAPC164SX_466 0x00003400	/* AlphaPC164SX 433MHz */
#define SV_STS_ALPHAPC164SX_533 0x00003800	/* AlphaPC164SX 533MHz */
#define SV_STS_ALPHAPC164SX_600 0x00003c00	/* AlphaPC164SX 600MHz */

#define PCS_STAT_BIP		0x00000001	/* bootstrap in progress */
#define PCS_STAT_RC		0x00000002	/* restart capable */
#define PCS_STAT_PA		0x00000004	/* processor available to OS */
#define PCS_STAT_PP		0x00000008	/* processor present */
#define PCS_STAT_OH		0x00000010	/* operator halted */
#define PCS_STAT_CV		0x00000020	/* context valid */
#define PCS_STAT_PV		0x00000040	/* PALcode valid */
#define PCS_STAT_PMV		0x00000080	/* PALcode memory valid */
#define PCS_STAT_PL		0x00000100	/* PALcode loaded */
#define PCS_STAT_HALT_MASK	0x00ff0000	/* Mask for Halt Requested field */
#define PCS_STAT_DEFAULT	0x00000000	/* Default (no specific action) */
#define PCS_STAT_SVRS_TERM	0x00010000	/* SAVE_TERM/RESTORE_TERM Exit */
#define PCS_STAT_COLD_BOOT	0x00020000	/* Cold Bootstrap Requested */
#define PCS_STAT_WARM_BOOT	0x00030000	/* Warm Bootstrap Requested */
#define PCS_STAT_HALT		0x00040000	/* Remain halted (no restart) */
#define PCS_STAT_POWER_OFF	0x00050000	/* Power off system on Halt */

#ifndef _ASM
#include <sys/types.h>
#include <sys/int_types.h>

struct rpb {
	uint64_t phys_addr;	/*   0: Physical Address of HWRPB */
	char	string[8];	/*   8: "HWRPB" */
	uint64_t revision;	/*  10: HWRPB Revision */
	uint64_t size;		/*  18: HWRPB size */
	uint64_t cpu_id;	/*  20: Primary CPU ID */
	uint64_t page_size;	/*  28: Page Size (=8192) */
	uint32_t pa_size;	/*  30: PA Bits */
	uint32_t ext_va_size;	/*  34: Extention VA Bits */
	uint64_t max_asn;	/*  38: Max Vaild ASN */
	char	ssn[16];	/*  40: System Serial Number */
	uint64_t systype;	/*  50: System Type */
	uint64_t sysvar;	/*  58: System Variation */
	uint64_t sysrev;	/*  60: System Rev */
	uint64_t intr_clock;	/*  68; Interval Clock Interrupt Freq */
	uint64_t counter;	/*  70: Cycle Counter Freq */
	uint64_t vptb;		/*  78: Virtual Page Table base */
	uint64_t reserved_arch;	/*  80: Reserved for Architecture */
	uint64_t tbhint_off;	/*  88: Offset to Translation Buffer Hint */
	uint64_t pcs_num;	/*  90: Number of Processor Slots */
	uint64_t pcs_size;	/*  98; Per-CPU Slot Size */
	uint64_t pcs_off;	/*  A0: Offset to Per-CPU Slots */
	uint64_t ctb_num;	/*  A8: Number of CTBs */
	uint64_t ctb_size;	/*  B0: CTB Size */
	uint64_t ctb_off;	/*  B8: Offset to CTB */
	uint64_t crb_off;	/*  C0: Offset to CRB */
	uint64_t mddt_off;	/*  C8: Offset to Memory Data Descriptor */
	uint64_t cfg_off;	/*  D0: Offset to Config Data */
	uint64_t fru_off;	/*  D8: Offset to FRU */
	uint64_t save_term;	/*  E0: terminal save */
	uint64_t save_term_pv;	/*  E8: */
	uint64_t rest_term;	/*  F0: terminal restore */
	uint64_t rest_term_pv;	/*  F8: */
	uint64_t restart;	/* 100: restart */
	uint64_t restart_va;	/* 108: */
	uint64_t reserved_os;	/* 110: */
	uint64_t reserved_hw;	/* 118: */
	uint64_t checksum;	/* 120: HWRPB checksum */
	uint64_t rxrdy;		/* 128: receive ready */
	uint64_t txrdy;		/* 130: transmit ready */
	uint64_t dsr_off;	/* 138: HWRPB + DSRDB offset */
};

struct hwpcb {
	uint64_t ksp;		/* 000: Kernel Stack Pointer */
	uint64_t esp;		/* 008: Excution Stack Pointer */
	uint64_t ssp;		/* 010: Supervisor Stack Pointer */
	uint64_t usp;		/* 018: User Stack Pointer */
	uint64_t ptbr;		/* 020: Page Pable Base Register */
	uint64_t asn;		/* 028: Address Space Number */
	uint64_t asten;		/* 030: AST enable / AST SR */
	uint64_t fen;		/* 038: Floating point enable */
	uint64_t cc;		/* 040: Cycle Counter */
	uint64_t scratch[7];	/* 048: Scratch area */
};

struct iccb {
	uint32_t iccb_rxlen;
	uint32_t iccb_txlen;
	uint8_t  iccb_rxbuf[80];
	uint8_t  iccb_txbuf[80];
};

struct pcs {
	struct hwpcb hwpcb;		/*   0: Bootstrap/Restart HWPCB */
	uint64_t stat_flags;		/*  80: Per-CPU State Flags */
	uint64_t pal_mem_size;		/*  88: PAL memory size */
	uint64_t pal_scr_size;		/*  90: PAL scratch size */
	uint64_t pal_mem_addr;		/*  98: PAL memory addr */
	uint64_t pal_scr_addr;		/*  A0: PAL scratch addr */
	uint64_t pal_rev;		/*  A8: PAL Rev */
	uint64_t proc_type;		/*  B0: Processor type */
	uint64_t proc_var;		/*  B8: Processor variation. */
	uint8_t proc_rev[8];		/*  C0: Processor revision */
	uint8_t proc_sn[16];		/*  C8: Processor serial Number */
	uint64_t logout_pa;		/*  D8: Logout Area Physical Address */
	uint64_t logout_len;		/*  E0: Logout Area Len */
	uint64_t halt_pcbb;		/*  E8: Halt PCB */
	uint64_t halt_pc;		/*  F0: Halt PC */
	uint64_t halt_ps;		/*  F8: Halt PS */
	uint64_t halt_al;		/* 100: Halt argument list(r25) */
	uint64_t halt_ra;		/* 108: Halt return addr(r26) */
	uint64_t halt_pv;		/* 110: Halt procedure value(r27) */
	uint64_t halt_code;		/* 118: reason for halt */
	uint64_t reserved_soft;		/* 120: reserved software */
	struct iccb iccb;		/* 128: inter-console buffers */
	uint64_t pal_rev_avl[16];	/* 1D0: PAL available */
	uint64_t proc_cmp;		/* 250: Processor software compatibility field */
	uint64_t haltdata_pa;		/* 258: Halt data log physical Address */
	uint64_t haltdata_sz;		/* 260: Halt data log length */
	uint64_t cache_sz;		/* 268: Cache size */
	uint64_t counter;		/* 270: Cycle counter */
	uint64_t reserved_arch;		/* 278: Reserved for arch use */
};

struct mddt_cluster {
	uint64_t pfn;		/* 000: Starting PFN */
	uint64_t pfncount;	/* 008: Count of PFNs */
	uint64_t pfntested;	/* 010: Count of tested PFNs */
	uint64_t bitmap_va;	/* 018: Physical Address of bitmap */
	uint64_t bitmap_pa;	/* 018: Virtual Address of bitmap */
	uint64_t bitmap_cksum;	/* 028: Checksum of bitmap */
	uint64_t usage;		/* 030: usage of cluster */
};

struct mddt {
	uint64_t checksum;	/* 000: Checksum of mddt */
	uint64_t imp_pa;	/* 008: Physical Address of implementation-specific info */
	uint64_t cluster_num;	/* 010: Number of clusters in mddt */
	struct mddt_cluster cluster[0];
};

struct dsr {
	uint64_t smm;		/* 000: SMM nubber used by LMF */
	uint64_t lurt_off;	/* 008: Offset to LURT table */
	uint64_t sysname_off;	/* 010: Offset to System name */
};

/*
 * The physical/virtual map for the console routine block.
 */
struct crb_map {
	uint64_t	va;	/* virtual address for map entry */
	uint64_t	pa;	/* phys address for map entry */
	uint64_t	num;	/* page count for map entry */
};

/*
 * The "Console Routine Block" portion of the HWRPB.
 * Note: the "offsets" are all relative to the start of the HWRPB (HWRPB_ADDR).
 */
struct crblk {
	uint64_t	va_disp;	/* va of call-back dispatch rtn */
	uint64_t	pa_disp;	/* pa of call-back dispatch rtn */
	uint64_t	va_fixup;	/* va of call-back fixup rtn */
	uint64_t	pa_fixup;	/* pa of call-back fixup rtn */
	uint64_t	num;		/* number of entries in phys/virt map */
	uint64_t	mapped_pages;	/* Number of pages to be mapped */
	struct crb_map	map[0];		/* first instance of a map entry */
};

extern struct rpb *hwrpb;

extern void console_fixup(uintptr_t new_base_va, uintptr_t new_hwrpb_va);

#endif /* _ASM */
#ifdef __cplusplus
}
#endif
