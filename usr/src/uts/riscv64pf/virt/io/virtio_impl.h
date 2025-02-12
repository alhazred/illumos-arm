/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2019 Joyent, Inc.
 */

#ifndef _VIRTIO_IMPL_H
#define	_VIRTIO_IMPL_H

/*
 * VIRTIO FRAMEWORK: FRAMEWORK-PRIVATE DEFINITIONS
 *
 * For design and usage documentation, see the comments in "virtio.h".
 *
 * NOTE: Client drivers should not use definitions from this file.
 */

#include <sys/types.h>
#include <sys/dditypes.h>
#include <sys/list.h>
#include <sys/ccompile.h>

#include "virtio.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ddi_device_acc_attr_t virtio_acc_attr;
extern ddi_dma_attr_t virtio_dma_attr;

typedef struct virtio_vq_desc virtio_vq_desc_t;
typedef struct virtio_vq_driver virtio_vq_driver_t;
typedef struct virtio_vq_device virtio_vq_device_t;
typedef struct virtio_vq_elem virtio_vq_elem_t;

int virtio_dma_init(virtio_t *, virtio_dma_t *, size_t, const ddi_dma_attr_t *,
    int, int);
void virtio_dma_fini(virtio_dma_t *);



typedef enum virtio_dma_level {
	VIRTIO_DMALEVEL_HANDLE_ALLOC =	(1ULL << 0),
	VIRTIO_DMALEVEL_MEMORY_ALLOC =	(1ULL << 1),
	VIRTIO_DMALEVEL_HANDLE_BOUND =	(1ULL << 2),
	VIRTIO_DMALEVEL_COOKIE_ARRAY =	(1ULL << 3),
} virtio_dma_level_t;

struct virtio_dma {
	virtio_dma_level_t		vidma_level;
	virtio_t			*vidma_virtio;
	caddr_t				vidma_va;
	size_t				vidma_size;
	size_t				vidma_real_size;
	ddi_dma_handle_t		vidma_dma_handle;
	ddi_acc_handle_t		vidma_acc_handle;
	uint_t				vidma_dma_ncookies;
	ddi_dma_cookie_t		*vidma_dma_cookies;
};

typedef enum virtio_initlevel {
	VIRTIO_INITLEVEL_REGS =		(1ULL << 0),
	VIRTIO_INITLEVEL_PROVIDER =	(1ULL << 1),
	VIRTIO_INITLEVEL_INT_ALLOC =	(1ULL << 2),
	VIRTIO_INITLEVEL_INT_ADDED =	(1ULL << 3),
	VIRTIO_INITLEVEL_INT_ENABLED =	(1ULL << 4),
	VIRTIO_INITLEVEL_SHUTDOWN =	(1ULL << 5),
} virtio_initlevel_t;

struct virtio {
	dev_info_t			*vio_dip;

	kmutex_t			vio_mutex;

	virtio_initlevel_t		vio_initlevel;

	list_t				vio_queues;

	ddi_acc_handle_t		vio_barh;
	caddr_t				vio_bar;
	uint_t				vio_config_offset;

	uint32_t			vio_features;
	uint32_t			vio_features_device;

	ddi_intr_handle_t		*vio_interrupts;
	int				vio_ninterrupts;
	int				vio_interrupt_type;
	int				vio_interrupt_cap;
	uint_t				vio_interrupt_priority;
};

struct virtio_queue {
	virtio_t			*viq_virtio;
	kmutex_t			viq_mutex;
	const char			*viq_name;
	list_node_t			viq_link;

	boolean_t			viq_shutdown;
	boolean_t			viq_indirect;
	uint_t				viq_max_segs;

	/*
	 * Each Virtio device type has some set of queues for data transfer to
	 * and from the host.  This index is described in the specification for
	 * the particular device and queue type, and written to QUEUE_SELECT to
	 * allow interaction with the queue.  For example, a network device has
	 * at least a receive queue with index 0, and a transmit queue with
	 * index 1.
	 */
	uint16_t			viq_index;

	/*
	 * For legacy Virtio devices, the size and shape of the queue is
	 * determined entirely by the number of queue entries.
	 */
	uint16_t			viq_size;
	id_space_t			*viq_descmap;

	/*
	 * The memory shared between the device and the driver is allocated as
	 * a large phyisically contiguous chunk.  Access to this area is
	 * through three pointers to packed structures.
	 */
	virtio_dma_t			viq_dma;
	virtio_vq_desc_t		*viq_dma_descs;
	virtio_vq_driver_t		*viq_dma_driver;
	virtio_vq_device_t		*viq_dma_device;

	uint16_t			viq_device_index;
	uint16_t			viq_driver_index;

	/*
	 * Interrupt handler function, or NULL if not provided.
	 */
	ddi_intr_handler_t		*viq_func;
	void				*viq_funcarg;
	boolean_t			viq_handler_added;
	uint_t				viq_handler_index;

	/*
	 * When a chain is submitted to the queue, it is also stored in this
	 * AVL tree keyed by the index of the first descriptor in the chain.
	 */
	avl_tree_t			viq_inflight;
};

struct virtio_chain {
	virtio_queue_t			*vic_vq;
	avl_node_t			vic_node;

	void				*vic_data;

	uint16_t			vic_head;
	uint32_t			vic_received_length;

	virtio_dma_t			vic_indirect_dma;
	uint_t				vic_indirect_capacity;
	uint_t				vic_indirect_used;

	uint_t				vic_direct_capacity;
	uint_t				vic_direct_used;
	uint16_t			vic_direct[];
};

/*
 * PACKED STRUCTS FOR DEVICE ACCESS
 */

struct virtio_vq_desc {
	/*
	 * Buffer physical address and length.
	 */
	uint64_t			vqd_addr;
	uint32_t			vqd_len;

	/*
	 * Flags.  Use with the VIRTQ_DESC_F_* family of constants.  See below.
	 */
	uint16_t			vqd_flags;

	/*
	 * If VIRTQ_DESC_F_NEXT is set in flags, this refers to the next
	 * descriptor in the chain by table index.
	 */
	uint16_t			vqd_next;
} __packed;

/*
 * VIRTIO DESCRIPTOR FLAGS (vqd_flags)
 */

/*
 * NEXT:
 *	Signals that this descriptor (direct or indirect) is part of a chain.
 *	If populated, "vqd_next" names the next descriptor in the chain by its
 *	table index.
 */
#define	VIRTQ_DESC_F_NEXT		(1 << 0)

/*
 * WRITE:
 *	Determines whether this buffer is to be written by the device (WRITE is
 *	set) or by the driver (WRITE is not set).
 */
#define	VIRTQ_DESC_F_WRITE		(1 << 1)

/*
 * INDIRECT:
 *	This bit signals that a direct descriptor refers to an indirect
 *	descriptor list, rather than directly to a buffer.  This bit may only
 *	be used in a direct descriptor; indirect descriptors are not allowed to
 *	refer to additional layers of indirect tables.  If this bit is set,
 *	NEXT must be clear; indirect descriptors may not be chained.
 */
#define	VIRTQ_DESC_F_INDIRECT		(1 << 2)

/*
 * This structure is variously known as the "available" or "avail" ring, or the
 * driver-owned portion of the queue structure.  It is used by the driver to
 * submit descriptor chains to the device.
 */
struct virtio_vq_driver {
	uint16_t			vqdr_flags;
	uint16_t			vqdr_index;
	uint16_t			vqdr_ring[];
} __packed;

#define	VIRTQ_AVAIL_F_NO_INTERRUPT	(1 << 0)

/*
 * We use the sizeof operator on this packed struct to calculate the offset of
 * subsequent structs.  Ensure the compiler is not adding any padding to the
 * end of the struct.
 */
CTASSERT(sizeof (virtio_vq_driver_t) ==
    offsetof(virtio_vq_driver_t, vqdr_ring));

struct virtio_vq_elem {
	/*
	 * The device returns chains of descriptors by specifying the table
	 * index of the first descriptor in the chain.
	 */
	uint32_t			vqe_start;
	uint32_t			vqe_len;
} __packed;

/*
 * This structure is variously known as the "used" ring, or the device-owned
 * portion of the queue structure.  It is used by the device to return
 * completed descriptor chains to the driver.
 */
struct virtio_vq_device {
	uint16_t			vqde_flags;
	uint16_t			vqde_index;
	virtio_vq_elem_t		vqde_ring[];
} __packed;

#define	VIRTQ_USED_F_NO_NOTIFY		(1 << 0)

/*
 * BASIC CONFIGURATION
 */
enum {
	VirtIoQueueNumMax = 1024,
	VirtIoQueueAlign = 64,
};

#define VIRTIO_MMIO_MAGIC_VALUE		0x000
#define VIRTIO_MMIO_VERSION		0x004
#define VIRTIO_MMIO_DEVICE_ID		0x008
#define VIRTIO_MMIO_VENDOR_ID		0x00c
#define VIRTIO_MMIO_HOST_FEATURES	0x010
#define VIRTIO_MMIO_HOST_FEATURES_SEL	0x014
#define VIRTIO_MMIO_GUEST_FEATURES	0x020
#define VIRTIO_MMIO_GUEST_FEATURES_SEL	0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE	0x028
#define VIRTIO_MMIO_QUEUE_SEL		0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX	0x034
#define VIRTIO_MMIO_QUEUE_NUM		0x038
#define VIRTIO_MMIO_QUEUE_ALIGN		0x03c
#define VIRTIO_MMIO_QUEUE_PFN		0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY	0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060
#define VIRTIO_MMIO_INTERRUPT_ACK	0x064
#define VIRTIO_MMIO_STATUS		0x070
#define VIRTIO_MMIO_CONFIG		0x100



/*
 * Bits in the Device Status byte (VIRTIO_LEGACY_DEVICE_STATUS):
 */
#define	VIRTIO_STATUS_RESET		0
#define	VIRTIO_STATUS_ACKNOWLEDGE	(1 << 0)
#define	VIRTIO_STATUS_DRIVER		(1 << 1)
#define	VIRTIO_STATUS_DRIVER_OK		(1 << 2)
#define	VIRTIO_STATUS_FAILED		(1 << 7)

/*
 * Bits in the Interrupt Service Routine Status byte
 * (VIRTIO_LEGACY_ISR_STATUS):
 */
#define	VIRTIO_ISR_CHECK_QUEUES		(1 << 0)
#define	VIRTIO_ISR_CHECK_CONFIG		(1 << 1)

/*
 * Bits in the Features fields (VIRTIO_LEGACY_FEATURES_DEVICE,
 * VIRTIO_LEGACY_FEATURES_DRIVER):
 */
#define	VIRTIO_F_RING_INDIRECT_DESC	(1ULL << 28)

/*
 * For devices operating in the legacy mode, virtqueues must be aligned on a
 * "page size" of 4096 bytes; this is also called the "Queue Align" value in
 * newer versions of the specification.
 */
#define	VIRTIO_PAGE_SHIFT		12
#define	VIRTIO_PAGE_SIZE		(1 << VIRTIO_PAGE_SHIFT)
CTASSERT(VIRTIO_PAGE_SIZE == 4096);
CTASSERT(ISP2(VIRTIO_PAGE_SIZE));

/*
 * DMA SYNCHRONISATION WRAPPERS
 */

/*
 * Synchronise the driver-owned portion of the queue so that the device can see
 * our writes.  This covers the memory accessed via the "viq_dma_descs" and
 * "viq_dma_device" members.
 */
#define	VIRTQ_DMA_SYNC_FORDEV(viq)	VERIFY0(ddi_dma_sync( \
					    (viq)->viq_dma.vidma_dma_handle, \
					    0, \
					    (uintptr_t)(viq)->viq_dma_device - \
					    (uintptr_t)(viq)->viq_dma_descs, \
					    DDI_DMA_SYNC_FORDEV))

/*
 * Synchronise the device-owned portion of the queue so that we can see any
 * writes from the device.  This covers the memory accessed via the
 * "viq_dma_device" member.
 */
#define	VIRTQ_DMA_SYNC_FORKERNEL(viq)	VERIFY0(ddi_dma_sync( \
					    (viq)->viq_dma.vidma_dma_handle, \
					    (uintptr_t)(viq)->viq_dma_device - \
					    (uintptr_t)(viq)->viq_dma_descs, \
					    (viq)->viq_dma.vidma_size - \
					    (uintptr_t)(viq)->viq_dma_device - \
					    (uintptr_t)(viq)->viq_dma_descs, \
					    DDI_DMA_SYNC_FORKERNEL))

#ifdef __cplusplus
}
#endif

#endif /* _VIRTIO_IMPL_H */
