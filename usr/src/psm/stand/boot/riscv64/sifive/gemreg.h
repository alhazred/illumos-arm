/*
 * QEMU Cadence GEM emulation
 *
 * Copyright (c) 2011 Xilinx, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <sys/types.h>

#define GEM_NWCTRL        (0x00000000/4) /* Network Control reg */
#define GEM_NWCFG         (0x00000004/4) /* Network Config reg */
#define GEM_NWSTATUS      (0x00000008/4) /* Network Status reg */
#define GEM_USERIO        (0x0000000C/4) /* User IO reg */
#define GEM_DMACFG        (0x00000010/4) /* DMA Control reg */
#define GEM_TXSTATUS      (0x00000014/4) /* TX Status reg */
#define GEM_RXQBASE       (0x00000018/4) /* RX Q Base address reg */
#define GEM_TXQBASE       (0x0000001C/4) /* TX Q Base address reg */
#define GEM_RXSTATUS      (0x00000020/4) /* RX Status reg */
#define GEM_ISR           (0x00000024/4) /* Interrupt Status reg */
#define GEM_IER           (0x00000028/4) /* Interrupt Enable reg */
#define GEM_IDR           (0x0000002C/4) /* Interrupt Disable reg */
#define GEM_IMR           (0x00000030/4) /* Interrupt Mask reg */
#define GEM_PHYMNTNC      (0x00000034/4) /* Phy Maintenance reg */
#define GEM_RXPAUSE       (0x00000038/4) /* RX Pause Time reg */
#define GEM_TXPAUSE       (0x0000003C/4) /* TX Pause Time reg */
#define GEM_TXPARTIALSF   (0x00000040/4) /* TX Partial Store and Forward */
#define GEM_RXPARTIALSF   (0x00000044/4) /* RX Partial Store and Forward */
#define GEM_HASHLO        (0x00000080/4) /* Hash Low address reg */
#define GEM_HASHHI        (0x00000084/4) /* Hash High address reg */
#define GEM_SPADDR1LO     (0x00000088/4) /* Specific addr 1 low reg */
#define GEM_SPADDR1HI     (0x0000008C/4) /* Specific addr 1 high reg */
#define GEM_SPADDR2LO     (0x00000090/4) /* Specific addr 2 low reg */
#define GEM_SPADDR2HI     (0x00000094/4) /* Specific addr 2 high reg */
#define GEM_SPADDR3LO     (0x00000098/4) /* Specific addr 3 low reg */
#define GEM_SPADDR3HI     (0x0000009C/4) /* Specific addr 3 high reg */
#define GEM_SPADDR4LO     (0x000000A0/4) /* Specific addr 4 low reg */
#define GEM_SPADDR4HI     (0x000000A4/4) /* Specific addr 4 high reg */
#define GEM_TIDMATCH1     (0x000000A8/4) /* Type ID1 Match reg */
#define GEM_TIDMATCH2     (0x000000AC/4) /* Type ID2 Match reg */
#define GEM_TIDMATCH3     (0x000000B0/4) /* Type ID3 Match reg */
#define GEM_TIDMATCH4     (0x000000B4/4) /* Type ID4 Match reg */
#define GEM_WOLAN         (0x000000B8/4) /* Wake on LAN reg */
#define GEM_IPGSTRETCH    (0x000000BC/4) /* IPG Stretch reg */
#define GEM_SVLAN         (0x000000C0/4) /* Stacked VLAN reg */
#define GEM_MODID         (0x000000FC/4) /* Module ID reg */
#define GEM_OCTTXLO       (0x00000100/4) /* Octects transmitted Low reg */
#define GEM_OCTTXHI       (0x00000104/4) /* Octects transmitted High reg */
#define GEM_TXCNT         (0x00000108/4) /* Error-free Frames transmitted */
#define GEM_TXBCNT        (0x0000010C/4) /* Error-free Broadcast Frames */
#define GEM_TXMCNT        (0x00000110/4) /* Error-free Multicast Frame */
#define GEM_TXPAUSECNT    (0x00000114/4) /* Pause Frames Transmitted */
#define GEM_TX64CNT       (0x00000118/4) /* Error-free 64 TX */
#define GEM_TX65CNT       (0x0000011C/4) /* Error-free 65-127 TX */
#define GEM_TX128CNT      (0x00000120/4) /* Error-free 128-255 TX */
#define GEM_TX256CNT      (0x00000124/4) /* Error-free 256-511 */
#define GEM_TX512CNT      (0x00000128/4) /* Error-free 512-1023 TX */
#define GEM_TX1024CNT     (0x0000012C/4) /* Error-free 1024-1518 TX */
#define GEM_TX1519CNT     (0x00000130/4) /* Error-free larger than 1519 TX */
#define GEM_TXURUNCNT     (0x00000134/4) /* TX under run error counter */
#define GEM_SINGLECOLLCNT (0x00000138/4) /* Single Collision Frames */
#define GEM_MULTCOLLCNT   (0x0000013C/4) /* Multiple Collision Frames */
#define GEM_EXCESSCOLLCNT (0x00000140/4) /* Excessive Collision Frames */
#define GEM_LATECOLLCNT   (0x00000144/4) /* Late Collision Frames */
#define GEM_DEFERTXCNT    (0x00000148/4) /* Deferred Transmission Frames */
#define GEM_CSENSECNT     (0x0000014C/4) /* Carrier Sense Error Counter */
#define GEM_OCTRXLO       (0x00000150/4) /* Octects Received register Low */
#define GEM_OCTRXHI       (0x00000154/4) /* Octects Received register High */
#define GEM_RXCNT         (0x00000158/4) /* Error-free Frames Received */
#define GEM_RXBROADCNT    (0x0000015C/4) /* Error-free Broadcast Frames RX */
#define GEM_RXMULTICNT    (0x00000160/4) /* Error-free Multicast Frames RX */
#define GEM_RXPAUSECNT    (0x00000164/4) /* Pause Frames Received Counter */
#define GEM_RX64CNT       (0x00000168/4) /* Error-free 64 byte Frames RX */
#define GEM_RX65CNT       (0x0000016C/4) /* Error-free 65-127B Frames RX */
#define GEM_RX128CNT      (0x00000170/4) /* Error-free 128-255B Frames RX */
#define GEM_RX256CNT      (0x00000174/4) /* Error-free 256-512B Frames RX */
#define GEM_RX512CNT      (0x00000178/4) /* Error-free 512-1023B Frames RX */
#define GEM_RX1024CNT     (0x0000017C/4) /* Error-free 1024-1518B Frames RX */
#define GEM_RX1519CNT     (0x00000180/4) /* Error-free 1519-max Frames RX */
#define GEM_RXUNDERCNT    (0x00000184/4) /* Undersize Frames Received */
#define GEM_RXOVERCNT     (0x00000188/4) /* Oversize Frames Received */
#define GEM_RXJABCNT      (0x0000018C/4) /* Jabbers Received Counter */
#define GEM_RXFCSCNT      (0x00000190/4) /* Frame Check seq. Error Counter */
#define GEM_RXLENERRCNT   (0x00000194/4) /* Length Field Error Counter */
#define GEM_RXSYMERRCNT   (0x00000198/4) /* Symbol Error Counter */
#define GEM_RXALIGNERRCNT (0x0000019C/4) /* Alignment Error Counter */
#define GEM_RXRSCERRCNT   (0x000001A0/4) /* Receive Resource Error Counter */
#define GEM_RXORUNCNT     (0x000001A4/4) /* Receive Overrun Counter */
#define GEM_RXIPCSERRCNT  (0x000001A8/4) /* IP header Checksum Error Counter */
#define GEM_RXTCPCCNT     (0x000001AC/4) /* TCP Checksum Error Counter */
#define GEM_RXUDPCCNT     (0x000001B0/4) /* UDP Checksum Error Counter */

#define GEM_1588S         (0x000001D0/4) /* 1588 Timer Seconds */
#define GEM_1588NS        (0x000001D4/4) /* 1588 Timer Nanoseconds */
#define GEM_1588ADJ       (0x000001D8/4) /* 1588 Timer Adjust */
#define GEM_1588INC       (0x000001DC/4) /* 1588 Timer Increment */
#define GEM_PTPETXS       (0x000001E0/4) /* PTP Event Frame Transmitted (s) */
#define GEM_PTPETXNS      (0x000001E4/4) /* PTP Event Frame Transmitted (ns) */
#define GEM_PTPERXS       (0x000001E8/4) /* PTP Event Frame Received (s) */
#define GEM_PTPERXNS      (0x000001EC/4) /* PTP Event Frame Received (ns) */
#define GEM_PTPPTXS       (0x000001E0/4) /* PTP Peer Frame Transmitted (s) */
#define GEM_PTPPTXNS      (0x000001E4/4) /* PTP Peer Frame Transmitted (ns) */
#define GEM_PTPPRXS       (0x000001E8/4) /* PTP Peer Frame Received (s) */
#define GEM_PTPPRXNS      (0x000001EC/4) /* PTP Peer Frame Received (ns) */

/* Design Configuration Registers */
#define GEM_DESCONF       (0x00000280/4)
#define GEM_DESCONF2      (0x00000284/4)
#define GEM_DESCONF3      (0x00000288/4)
#define GEM_DESCONF4      (0x0000028C/4)
#define GEM_DESCONF5      (0x00000290/4)
#define GEM_DESCONF6      (0x00000294/4)
#define GEM_DESCONF6_64B_MASK (1U << 23)
#define GEM_DESCONF7      (0x00000298/4)

#define GEM_INT_Q1_STATUS               (0x00000400 / 4)
#define GEM_INT_Q1_MASK                 (0x00000640 / 4)

#define GEM_TRANSMIT_Q1_PTR             (0x00000440 / 4)
#define GEM_TRANSMIT_Q7_PTR             (GEM_TRANSMIT_Q1_PTR + 6)

#define GEM_RECEIVE_Q1_PTR              (0x00000480 / 4)
#define GEM_RECEIVE_Q7_PTR              (GEM_RECEIVE_Q1_PTR + 6)

#define GEM_TBQPH                       (0x000004C8 / 4)
#define GEM_RBQPH                       (0x000004D4 / 4)

#define GEM_INT_Q1_ENABLE               (0x00000600 / 4)
#define GEM_INT_Q7_ENABLE               (GEM_INT_Q1_ENABLE + 6)

#define GEM_INT_Q1_DISABLE              (0x00000620 / 4)
#define GEM_INT_Q7_DISABLE              (GEM_INT_Q1_DISABLE + 6)

#define GEM_INT_Q1_MASK                 (0x00000640 / 4)
#define GEM_INT_Q7_MASK                 (GEM_INT_Q1_MASK + 6)

#define GEM_SCREENING_TYPE1_REGISTER_0  (0x00000500 / 4)

#define GEM_ST1R_UDP_PORT_MATCH_ENABLE  (1 << 29)
#define GEM_ST1R_DSTC_ENABLE            (1 << 28)
#define GEM_ST1R_UDP_PORT_MATCH_SHIFT   (12)
#define GEM_ST1R_UDP_PORT_MATCH_WIDTH   (27 - GEM_ST1R_UDP_PORT_MATCH_SHIFT + 1)
#define GEM_ST1R_DSTC_MATCH_SHIFT       (4)
#define GEM_ST1R_DSTC_MATCH_WIDTH       (11 - GEM_ST1R_DSTC_MATCH_SHIFT + 1)
#define GEM_ST1R_QUEUE_SHIFT            (0)
#define GEM_ST1R_QUEUE_WIDTH            (3 - GEM_ST1R_QUEUE_SHIFT + 1)

#define GEM_SCREENING_TYPE2_REGISTER_0  (0x00000540 / 4)

#define GEM_ST2R_COMPARE_A_ENABLE       (1 << 18)
#define GEM_ST2R_COMPARE_A_SHIFT        (13)
#define GEM_ST2R_COMPARE_WIDTH          (17 - GEM_ST2R_COMPARE_A_SHIFT + 1)
#define GEM_ST2R_ETHERTYPE_ENABLE       (1 << 12)
#define GEM_ST2R_ETHERTYPE_INDEX_SHIFT  (9)
#define GEM_ST2R_ETHERTYPE_INDEX_WIDTH  (11 - GEM_ST2R_ETHERTYPE_INDEX_SHIFT \
                                            + 1)
#define GEM_ST2R_QUEUE_SHIFT            (0)
#define GEM_ST2R_QUEUE_WIDTH            (3 - GEM_ST2R_QUEUE_SHIFT + 1)

#define GEM_SCREENING_TYPE2_ETHERTYPE_REG_0     (0x000006e0 / 4)
#define GEM_TYPE2_COMPARE_0_WORD_0              (0x00000700 / 4)

#define GEM_T2CW1_COMPARE_OFFSET_SHIFT  (7)
#define GEM_T2CW1_COMPARE_OFFSET_WIDTH  (8 - GEM_T2CW1_COMPARE_OFFSET_SHIFT + 1)
#define GEM_T2CW1_OFFSET_VALUE_SHIFT    (0)
#define GEM_T2CW1_OFFSET_VALUE_WIDTH    (6 - GEM_T2CW1_OFFSET_VALUE_SHIFT + 1)

/*****************************************/
#define GEM_NWCTRL_TXSTART     0x00000200 /* Transmit Enable */
#define GEM_NWCTRL_CLRSTAT     0x00000020 /* Clear Status Registers */
#define GEM_NWCTRL_MPENA       0x00000010 /* Management Port Enable */
#define GEM_NWCTRL_TXENA       0x00000008 /* Transmit Enable */
#define GEM_NWCTRL_RXENA       0x00000004 /* Receive Enable */
#define GEM_NWCTRL_LOCALLOOP   0x00000002 /* Local Loopback */

#define GEM_NWCFG_MDC_CLK_DIV96 0x00140000 /* mdc clk div 96 */
#define GEM_NWCFG_STRIP_FCS    0x00020000 /* Strip FCS field */
#define GEM_NWCFG_LERR_DISC    0x00010000 /* Discard RX frames with len err */
#define GEM_NWCFG_BUFF_OFST_M  0x0000C000 /* Receive buffer offset mask */
#define GEM_NWCFG_BUFF_OFST_S  14         /* Receive buffer offset shift */
#define GEM_NWCFG_GIGA         0x00000400 /* giga bit ethernet  */
#define GEM_NWCFG_UCAST_HASH   0x00000080 /* accept unicast if hash match */
#define GEM_NWCFG_MCAST_HASH   0x00000040 /* accept multicast if hash match */
#define GEM_NWCFG_BCAST_REJ    0x00000020 /* Reject broadcast packets */
#define GEM_NWCFG_PROMISC      0x00000010 /* Accept all packets */
#define GEM_NWCFG_FDX          0x00000002
#define GEM_NWCFG_100BASE      0x00000001

#define GEM_NWSTATUS_IDLE      0x00000004 /* Phy Management idle */

#define GEM_DMACFG_ADDR_64B    (1U << 30)
#define GEM_DMACFG_TX_BD_EXT   (1U << 29)
#define GEM_DMACFG_RX_BD_EXT   (1U << 28)
#define GEM_DMACFG_RBUFSZ_M    0x00FF0000 /* DMA RX Buffer Size mask */
#define GEM_DMACFG_RBUFSZ_S    16         /* DMA RX Buffer Size shift */
#define GEM_DMACFG_RBUFSZ_MUL  64         /* DMA RX Buffer Size multiplier */
#define GEM_DMACFG_TXCSUM_OFFL 0x00000800 /* Transmit checksum offload */

#define GEM_TXSTATUS_TXCMPL    0x00000020 /* Transmit Complete */
#define GEM_TXSTATUS_USED      0x00000001 /* sw owned descriptor encountered */

#define GEM_RXSTATUS_FRMRCVD   0x00000002 /* Frame received */
#define GEM_RXSTATUS_NOBUF     0x00000001 /* Buffer unavailable */

/* GEM_ISR GEM_IER GEM_IDR GEM_IMR */
#define GEM_INT_TXCMPL        0x00000080 /* Transmit Complete */
#define GEM_INT_TXUSED         0x00000008
#define GEM_INT_RXUSED         0x00000004
#define GEM_INT_RXCMPL        0x00000002

#define GEM_PHYMNTNC_OP_R      0x20000000 /* read operation */
#define GEM_PHYMNTNC_OP_W      0x10000000 /* write operation */
#define GEM_PHYMNTNC_ADDR      0x0F800000 /* Address bits */
#define GEM_PHYMNTNC_ADDR_SHFT 23
#define GEM_PHYMNTNC_REG       0x007C0000 /* register bits */
#define GEM_PHYMNTNC_REG_SHIFT 18
#define GEM_PHYMNTNC_CODE      0x00020000
#define GEM_PHYMNTNC_SOF       0x40000000

/* Marvell PHY definitions */
#define BOARD_PHY_ADDRESS    23 /* PHY address we will emulate a device at */

#define PHY_REG_CONTROL      0
#define PHY_REG_STATUS       1
#define PHY_REG_PHYID1       2
#define PHY_REG_PHYID2       3
#define PHY_REG_ANEGADV      4
#define PHY_REG_LINKPABIL    5
#define PHY_REG_ANEGEXP      6
#define PHY_REG_NEXTP        7
#define PHY_REG_LINKPNEXTP   8
#define PHY_REG_100BTCTRL    9
#define PHY_REG_1000BTSTAT   10
#define PHY_REG_EXTSTAT      15
#define PHY_REG_PHYSPCFC_CTL 16
#define PHY_REG_PHYSPCFC_ST  17
#define PHY_REG_INT_EN       18
#define PHY_REG_INT_ST       19
#define PHY_REG_EXT_PHYSPCFC_CTL  20
#define PHY_REG_RXERR        21
#define PHY_REG_EACD         22
#define PHY_REG_LED          24
#define PHY_REG_LED_OVRD     25
#define PHY_REG_EXT_PHYSPCFC_CTL2 26
#define PHY_REG_EXT_PHYSPCFC_ST   27
#define PHY_REG_CABLE_DIAG   28

#define PHY_REG_CONTROL_RST  0x8000
#define PHY_REG_CONTROL_LOOP 0x4000
#define PHY_REG_CONTROL_ANEG 0x1000

#define PHY_REG_STATUS_LINK     0x0004
#define PHY_REG_STATUS_ANEGCMPL 0x0020

#define PHY_REG_INT_ST_ANEGCMPL 0x0800
#define PHY_REG_INT_ST_LINKC    0x0400
#define PHY_REG_INT_ST_ENERGY   0x0010

/***********************************************************************/
#define GEM_RX_REJECT                   (-1)
#define GEM_RX_PROMISCUOUS_ACCEPT       (-2)
#define GEM_RX_BROADCAST_ACCEPT         (-3)
#define GEM_RX_MULTICAST_HASH_ACCEPT    (-4)
#define GEM_RX_UNICAST_HASH_ACCEPT      (-5)

#define GEM_RX_SAR_ACCEPT               0

/***********************************************************************/

#define DESC_1_USED 0x80000000
#define DESC_1_LENGTH 0x00001FFF

#define DESC_1_TX_WRAP 0x40000000
#define DESC_1_TX_LAST 0x00008000

#define DESC_0_RX_WRAP 0x00000002
#define DESC_0_RX_OWNERSHIP 0x00000001

#define R_DESC_1_RX_SAR_SHIFT           25
#define R_DESC_1_RX_SAR_LENGTH          2
#define R_DESC_1_RX_SAR_MATCH           (1 << 27)
#define R_DESC_1_RX_UNICAST_HASH        (1 << 29)
#define R_DESC_1_RX_MULTICAST_HASH      (1 << 30)
#define R_DESC_1_RX_BROADCAST           (1 << 31)

#define DESC_1_RX_SOF 0x00004000
#define DESC_1_RX_EOF 0x00008000

#define GEM_MODID_VALUE 0x00020118

