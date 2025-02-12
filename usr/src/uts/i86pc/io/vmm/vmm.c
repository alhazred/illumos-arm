/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2011 NetApp, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NETAPP, INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NETAPP, INC OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */
/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * Copyright 2015 Pluribus Networks Inc.
 * Copyright 2018 Joyent, Inc.
 * Copyright 2020 Oxide Computer Company
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/pcpu.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/rwlock.h>
#include <sys/sched.h>
#include <sys/smp.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>
#include <vm/vm_param.h>

#ifdef __FreeBSD__
#include <machine/cpu.h>
#endif
#include <machine/pcb.h>
#include <machine/smp.h>
#include <machine/md_var.h>
#include <x86/psl.h>
#include <x86/apicreg.h>

#include <machine/vmm.h>
#include <machine/vmm_dev.h>
#include <sys/vmm_instruction_emul.h>

#include "vmm_ioport.h"
#include "vmm_ktr.h"
#include "vmm_host.h"
#include "vmm_mem.h"
#include "vmm_util.h"
#include "vatpic.h"
#include "vatpit.h"
#include "vhpet.h"
#include "vioapic.h"
#include "vlapic.h"
#include "vpmtmr.h"
#include "vrtc.h"
#include "vmm_stat.h"
#include "vmm_lapic.h"

#include "io/ppt.h"
#include "io/iommu.h"

struct vlapic;

/*
 * Initialization:
 * (a) allocated when vcpu is created
 * (i) initialized when vcpu is created and when it is reinitialized
 * (o) initialized the first time the vcpu is created
 * (x) initialized before use
 */
struct vcpu {
	struct mtx	mtx;		/* (o) protects 'state' and 'hostcpu' */
	enum vcpu_state	state;		/* (o) vcpu state */
#ifndef __FreeBSD__
	kcondvar_t	vcpu_cv;	/* (o) cpu waiter cv */
	kcondvar_t	state_cv;	/* (o) IDLE-transition cv */
#endif /* __FreeBSD__ */
	int		hostcpu;	/* (o) vcpu's current host cpu */
#ifndef __FreeBSD__
	int		lastloccpu;	/* (o) last host cpu localized to */
#endif
	u_int		runblock;	/* (i) block vcpu from run state */
	int		reqidle;	/* (i) request vcpu to idle */
	struct vlapic	*vlapic;	/* (i) APIC device model */
	enum x2apic_state x2apic_state;	/* (i) APIC mode */
	uint64_t	exitintinfo;	/* (i) events pending at VM exit */
	int		nmi_pending;	/* (i) NMI pending */
	int		extint_pending;	/* (i) INTR pending */
	int	exception_pending;	/* (i) exception pending */
	int	exc_vector;		/* (x) exception collateral */
	int	exc_errcode_valid;
	uint32_t exc_errcode;
	struct savefpu	*guestfpu;	/* (a,i) guest fpu state */
	uint64_t	guest_xcr0;	/* (i) guest %xcr0 register */
	void		*stats;		/* (a,i) statistics */
	struct vm_exit	exitinfo;	/* (x) exit reason and collateral */
	uint64_t	nextrip;	/* (x) next instruction to execute */
	struct vie	*vie_ctx;	/* (x) instruction emulation context */
#ifndef __FreeBSD__
	uint64_t	tsc_offset;	/* (x) offset from host TSC */
#endif
};

#define	vcpu_lock_initialized(v) mtx_initialized(&((v)->mtx))
#define	vcpu_lock_init(v)	mtx_init(&((v)->mtx), "vcpu lock", 0, MTX_SPIN)
#define	vcpu_lock(v)		mtx_lock_spin(&((v)->mtx))
#define	vcpu_unlock(v)		mtx_unlock_spin(&((v)->mtx))
#define	vcpu_assert_locked(v)	mtx_assert(&((v)->mtx), MA_OWNED)

struct mem_seg {
	size_t	len;
	bool	sysmem;
	struct vm_object *object;
};
#ifdef __FreeBSD__
#define	VM_MAX_MEMSEGS	3
#else
#define	VM_MAX_MEMSEGS	4
#endif

struct mem_map {
	vm_paddr_t	gpa;
	size_t		len;
	vm_ooffset_t	segoff;
	int		segid;
	int		prot;
	int		flags;
};
#define	VM_MAX_MEMMAPS	8

/*
 * Initialization:
 * (o) initialized the first time the VM is created
 * (i) initialized when VM is created and when it is reinitialized
 * (x) initialized before use
 */
struct vm {
	void		*cookie;		/* (i) cpu-specific data */
	void		*iommu;			/* (x) iommu-specific data */
	struct vhpet	*vhpet;			/* (i) virtual HPET */
	struct vioapic	*vioapic;		/* (i) virtual ioapic */
	struct vatpic	*vatpic;		/* (i) virtual atpic */
	struct vatpit	*vatpit;		/* (i) virtual atpit */
	struct vpmtmr	*vpmtmr;		/* (i) virtual ACPI PM timer */
	struct vrtc	*vrtc;			/* (o) virtual RTC */
	volatile cpuset_t active_cpus;		/* (i) active vcpus */
	volatile cpuset_t debug_cpus;		/* (i) vcpus stopped for debug */
	int		suspend;		/* (i) stop VM execution */
	volatile cpuset_t suspended_cpus;	/* (i) suspended vcpus */
	volatile cpuset_t halted_cpus;		/* (x) cpus in a hard halt */
	struct mem_map	mem_maps[VM_MAX_MEMMAPS]; /* (i) guest address space */
	struct mem_seg	mem_segs[VM_MAX_MEMSEGS]; /* (o) guest memory regions */
	struct vmspace	*vmspace;		/* (o) guest's address space */
	char		name[VM_MAX_NAMELEN];	/* (o) virtual machine name */
	struct vcpu	vcpu[VM_MAXCPU];	/* (i) guest vcpus */
	/* The following describe the vm cpu topology */
	uint16_t	sockets;		/* (o) num of sockets */
	uint16_t	cores;			/* (o) num of cores/socket */
	uint16_t	threads;		/* (o) num of threads/core */
	uint16_t	maxcpus;		/* (o) max pluggable cpus */
#ifndef __FreeBSD__
	list_t		ioport_hooks;
#endif /* __FreeBSD__ */
	bool		sipi_req;		/* (i) SIPI requested */
	int		sipi_req_vcpu;		/* (i) SIPI destination */
	uint64_t	sipi_req_rip;		/* (i) SIPI start %rip */

	/* Miscellaneous VM-wide statistics and counters */
	struct vm_wide_stats {
		uint64_t sipi_supersede;
	} stats;
};

static int vmm_initialized;


static void
nullop_panic(void)
{
	panic("null vmm operation call");
}

/* Do not allow use of an un-set `ops` to do anything but panic */
static struct vmm_ops vmm_ops_null = {
	.init		= (vmm_init_func_t)nullop_panic,
	.cleanup	= (vmm_cleanup_func_t)nullop_panic,
	.resume		= (vmm_resume_func_t)nullop_panic,
	.vminit		= (vmi_init_func_t)nullop_panic,
	.vmrun		= (vmi_run_func_t)nullop_panic,
	.vmcleanup	= (vmi_cleanup_func_t)nullop_panic,
	.vmgetreg	= (vmi_get_register_t)nullop_panic,
	.vmsetreg	= (vmi_set_register_t)nullop_panic,
	.vmgetdesc	= (vmi_get_desc_t)nullop_panic,
	.vmsetdesc	= (vmi_set_desc_t)nullop_panic,
	.vmgetcap	= (vmi_get_cap_t)nullop_panic,
	.vmsetcap	= (vmi_set_cap_t)nullop_panic,
	.vmspace_alloc	= (vmi_vmspace_alloc)nullop_panic,
	.vmspace_free	= (vmi_vmspace_free)nullop_panic,
	.vlapic_init	= (vmi_vlapic_init)nullop_panic,
	.vlapic_cleanup	= (vmi_vlapic_cleanup)nullop_panic,
	.vmsavectx	= (vmi_savectx)nullop_panic,
	.vmrestorectx	= (vmi_restorectx)nullop_panic,
};

static struct vmm_ops *ops = &vmm_ops_null;

#define	VMM_INIT(num)			((*ops->init)(num))
#define	VMM_CLEANUP()			((*ops->cleanup)())
#define	VMM_RESUME()			((*ops->resume)())

#define	VMINIT(vm, pmap)		((*ops->vminit)(vm, pmap))
#define	VMRUN(vmi, vcpu, rip, pmap, evinfo) \
	((*ops->vmrun)(vmi, vcpu, rip, pmap, evinfo) )
#define	VMCLEANUP(vmi)			((*ops->vmcleanup)(vmi) )
#define	VMSPACE_ALLOC(min, max)		((*ops->vmspace_alloc)(min, max))
#define	VMSPACE_FREE(vmspace)		((*ops->vmspace_free)(vmspace))

#define	VMGETREG(vmi, vcpu, num, rv)	((*ops->vmgetreg)(vmi, vcpu, num, rv))
#define	VMSETREG(vmi, vcpu, num, val)	((*ops->vmsetreg)(vmi, vcpu, num, val))
#define	VMGETDESC(vmi, vcpu, num, dsc)	((*ops->vmgetdesc)(vmi, vcpu, num, dsc))
#define	VMSETDESC(vmi, vcpu, num, dsc)	((*ops->vmsetdesc)(vmi, vcpu, num, dsc))
#define	VMGETCAP(vmi, vcpu, num, rv)	((*ops->vmgetcap)(vmi, vcpu, num, rv))
#define	VMSETCAP(vmi, vcpu, num, val)	((*ops->vmsetcap)(vmi, vcpu, num, val))
#define	VLAPIC_INIT(vmi, vcpu)		((*ops->vlapic_init)(vmi, vcpu))
#define	VLAPIC_CLEANUP(vmi, vlapic)	((*ops->vlapic_cleanup)(vmi, vlapic))

#define	fpu_start_emulating()	load_cr0(rcr0() | CR0_TS)
#define	fpu_stop_emulating()	clts()

SDT_PROVIDER_DEFINE(vmm);

static MALLOC_DEFINE(M_VM, "vm", "vm");

/* statistics */
static VMM_STAT(VCPU_TOTAL_RUNTIME, "vcpu total runtime");

SYSCTL_NODE(_hw, OID_AUTO, vmm, CTLFLAG_RW | CTLFLAG_MPSAFE, NULL,
    NULL);

/*
 * Halt the guest if all vcpus are executing a HLT instruction with
 * interrupts disabled.
 */
static int halt_detection_enabled = 1;
SYSCTL_INT(_hw_vmm, OID_AUTO, halt_detection, CTLFLAG_RDTUN,
    &halt_detection_enabled, 0,
    "Halt VM if all vcpus execute HLT with interrupts disabled");

static int vmm_ipinum;
SYSCTL_INT(_hw_vmm, OID_AUTO, ipinum, CTLFLAG_RD, &vmm_ipinum, 0,
    "IPI vector used for vcpu notifications");

static int trace_guest_exceptions;
SYSCTL_INT(_hw_vmm, OID_AUTO, trace_guest_exceptions, CTLFLAG_RDTUN,
    &trace_guest_exceptions, 0,
    "Trap into hypervisor on all guest exceptions and reflect them back");

static void vm_free_memmap(struct vm *vm, int ident);
static bool sysmem_mapping(struct vm *vm, struct mem_map *mm);
static void vcpu_notify_event_locked(struct vcpu *vcpu, bool lapic_intr);

#ifndef __FreeBSD__
static void vm_clear_memseg(struct vm *, int);

typedef struct vm_ioport_hook {
	list_node_t	vmih_node;
	uint_t		vmih_ioport;
	void		*vmih_arg;
	vmm_rmem_cb_t	vmih_rmem_cb;
	vmm_wmem_cb_t	vmih_wmem_cb;
} vm_ioport_hook_t;

/* Flags for vtc_status */
#define	VTCS_FPU_RESTORED	1 /* guest FPU restored, host FPU saved */
#define	VTCS_FPU_CTX_CRITICAL	2 /* in ctx where FPU restore cannot be lazy */

typedef struct vm_thread_ctx {
	struct vm	*vtc_vm;
	int		vtc_vcpuid;
	uint_t		vtc_status;
} vm_thread_ctx_t;
#endif /* __FreeBSD__ */

#ifdef KTR
static const char *
vcpu_state2str(enum vcpu_state state)
{

	switch (state) {
	case VCPU_IDLE:
		return ("idle");
	case VCPU_FROZEN:
		return ("frozen");
	case VCPU_RUNNING:
		return ("running");
	case VCPU_SLEEPING:
		return ("sleeping");
	default:
		return ("unknown");
	}
}
#endif

static void
vcpu_cleanup(struct vm *vm, int i, bool destroy)
{
	struct vcpu *vcpu = &vm->vcpu[i];

	VLAPIC_CLEANUP(vm->cookie, vcpu->vlapic);
	if (destroy) {
		vmm_stat_free(vcpu->stats);
		fpu_save_area_free(vcpu->guestfpu);
		vie_free(vcpu->vie_ctx);
		vcpu->vie_ctx = NULL;
	}
}

static void
vcpu_init(struct vm *vm, int vcpu_id, bool create)
{
	struct vcpu *vcpu;

	KASSERT(vcpu_id >= 0 && vcpu_id < vm->maxcpus,
	    ("vcpu_init: invalid vcpu %d", vcpu_id));

	vcpu = &vm->vcpu[vcpu_id];

	if (create) {
#ifdef __FreeBSD__
		KASSERT(!vcpu_lock_initialized(vcpu), ("vcpu %d already "
		    "initialized", vcpu_id));
#endif
		vcpu_lock_init(vcpu);
		vcpu->state = VCPU_IDLE;
		vcpu->hostcpu = NOCPU;
#ifndef __FreeBSD__
		vcpu->lastloccpu = NOCPU;
#endif
		vcpu->guestfpu = fpu_save_area_alloc();
		vcpu->stats = vmm_stat_alloc();
		vcpu->vie_ctx = vie_alloc();
	} else {
		vie_reset(vcpu->vie_ctx);
		bzero(&vcpu->exitinfo, sizeof (vcpu->exitinfo));
	}

	vcpu->vlapic = VLAPIC_INIT(vm->cookie, vcpu_id);
	vm_set_x2apic_state(vm, vcpu_id, X2APIC_DISABLED);
	vcpu->runblock = 0;
	vcpu->reqidle = 0;
	vcpu->exitintinfo = 0;
	vcpu->nmi_pending = 0;
	vcpu->extint_pending = 0;
	vcpu->exception_pending = 0;
	vcpu->guest_xcr0 = XFEATURE_ENABLED_X87;
	fpu_save_area_reset(vcpu->guestfpu);
	vmm_stat_init(vcpu->stats);
}

int
vcpu_trace_exceptions(struct vm *vm, int vcpuid)
{

	return (trace_guest_exceptions);
}

struct vm_exit *
vm_exitinfo(struct vm *vm, int cpuid)
{
	struct vcpu *vcpu;

	if (cpuid < 0 || cpuid >= vm->maxcpus)
		panic("vm_exitinfo: invalid cpuid %d", cpuid);

	vcpu = &vm->vcpu[cpuid];

	return (&vcpu->exitinfo);
}

struct vie *
vm_vie_ctx(struct vm *vm, int cpuid)
{
	if (cpuid < 0 || cpuid >= vm->maxcpus)
		panic("vm_vie_ctx: invalid cpuid %d", cpuid);

	return (vm->vcpu[cpuid].vie_ctx);
}

static int
vmm_init(void)
{
	int error;

	vmm_host_state_init();

#ifdef __FreeBSD__
	vmm_ipinum = lapic_ipi_alloc(pti ? &IDTVEC(justreturn1_pti) :
	    &IDTVEC(justreturn));
	if (vmm_ipinum < 0)
		vmm_ipinum = IPI_AST;
#else
	/* We use cpu_poke() for IPIs */
	vmm_ipinum = 0;
#endif

	error = vmm_mem_init();
	if (error)
		return (error);

	if (vmm_is_intel())
		ops = &vmm_ops_intel;
	else if (vmm_is_svm())
		ops = &vmm_ops_amd;
	else
		return (ENXIO);

#ifdef __FreeBSD__
	vmm_resume_p = vmm_resume;
#endif

	return (VMM_INIT(vmm_ipinum));
}

int
vmm_mod_load()
{
	int	error;

	VERIFY(vmm_initialized == 0);

	error = vmm_init();
	if (error == 0)
		vmm_initialized = 1;

	return (error);
}

int
vmm_mod_unload()
{
	int	error;

	VERIFY(vmm_initialized == 1);

	iommu_cleanup();
	error = VMM_CLEANUP();
	if (error)
		return (error);
	vmm_initialized = 0;

	return (0);
}

static void
vm_init(struct vm *vm, bool create)
{
	int i;
#ifndef __FreeBSD__
	uint64_t tsc_off;
#endif

	vm->cookie = VMINIT(vm, vmspace_pmap(vm->vmspace));
	vm->iommu = NULL;
	vm->vioapic = vioapic_init(vm);
	vm->vhpet = vhpet_init(vm);
	vm->vatpic = vatpic_init(vm);
	vm->vatpit = vatpit_init(vm);
	vm->vpmtmr = vpmtmr_init(vm);
	if (create)
		vm->vrtc = vrtc_init(vm);
#ifndef __FreeBSD__
	if (create) {
		list_create(&vm->ioport_hooks, sizeof (vm_ioport_hook_t),
		    offsetof (vm_ioport_hook_t, vmih_node));
	} else {
		VERIFY(list_is_empty(&vm->ioport_hooks));
	}
#endif /* __FreeBSD__ */

	CPU_ZERO(&vm->active_cpus);
	CPU_ZERO(&vm->debug_cpus);

	vm->suspend = 0;
	CPU_ZERO(&vm->suspended_cpus);

	for (i = 0; i < vm->maxcpus; i++)
		vcpu_init(vm, i, create);

#ifndef __FreeBSD__
	tsc_off = (uint64_t)(-(int64_t)rdtsc());
	for (i = 0; i < vm->maxcpus; i++) {
		vm->vcpu[i].tsc_offset = tsc_off;
	}
#endif /* __FreeBSD__ */
}

/*
 * The default CPU topology is a single thread per package.
 */
u_int cores_per_package = 1;
u_int threads_per_core = 1;

int
vm_create(const char *name, struct vm **retvm)
{
	struct vm *vm;
	struct vmspace *vmspace;

	/*
	 * If vmm.ko could not be successfully initialized then don't attempt
	 * to create the virtual machine.
	 */
	if (!vmm_initialized)
		return (ENXIO);

	if (name == NULL || strlen(name) >= VM_MAX_NAMELEN)
		return (EINVAL);

	vmspace = VMSPACE_ALLOC(0, VM_MAXUSER_ADDRESS);
	if (vmspace == NULL)
		return (ENOMEM);

	vm = malloc(sizeof(struct vm), M_VM, M_WAITOK | M_ZERO);
	strcpy(vm->name, name);
	vm->vmspace = vmspace;

	vm->sockets = 1;
	vm->cores = cores_per_package;	/* XXX backwards compatibility */
	vm->threads = threads_per_core;	/* XXX backwards compatibility */
	vm->maxcpus = VM_MAXCPU;	/* XXX temp to keep code working */

	vm_init(vm, true);

	*retvm = vm;
	return (0);
}

void
vm_get_topology(struct vm *vm, uint16_t *sockets, uint16_t *cores,
    uint16_t *threads, uint16_t *maxcpus)
{
	*sockets = vm->sockets;
	*cores = vm->cores;
	*threads = vm->threads;
	*maxcpus = vm->maxcpus;
}

uint16_t
vm_get_maxcpus(struct vm *vm)
{
	return (vm->maxcpus);
}

int
vm_set_topology(struct vm *vm, uint16_t sockets, uint16_t cores,
    uint16_t threads, uint16_t maxcpus)
{
	if (maxcpus != 0)
		return (EINVAL);	/* XXX remove when supported */
	if ((sockets * cores * threads) > vm->maxcpus)
		return (EINVAL);
	/* XXX need to check sockets * cores * threads == vCPU, how? */
	vm->sockets = sockets;
	vm->cores = cores;
	vm->threads = threads;
	vm->maxcpus = VM_MAXCPU;	/* XXX temp to keep code working */
	return(0);
}

static void
vm_cleanup(struct vm *vm, bool destroy)
{
	struct mem_map *mm;
	int i;

	ppt_unassign_all(vm);

	if (vm->iommu != NULL)
		iommu_destroy_domain(vm->iommu);

	if (destroy)
		vrtc_cleanup(vm->vrtc);
	else
		vrtc_reset(vm->vrtc);
	vpmtmr_cleanup(vm->vpmtmr);
	vatpit_cleanup(vm->vatpit);
	vhpet_cleanup(vm->vhpet);
	vatpic_cleanup(vm->vatpic);
	vioapic_cleanup(vm->vioapic);

	for (i = 0; i < vm->maxcpus; i++)
		vcpu_cleanup(vm, i, destroy);

	VMCLEANUP(vm->cookie);

	/*
	 * System memory is removed from the guest address space only when
	 * the VM is destroyed. This is because the mapping remains the same
	 * across VM reset.
	 *
	 * Device memory can be relocated by the guest (e.g. using PCI BARs)
	 * so those mappings are removed on a VM reset.
	 */
	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (destroy || !sysmem_mapping(vm, mm))
			vm_free_memmap(vm, i);
#ifndef __FreeBSD__
		else {
			/*
			 * We need to reset the IOMMU flag so this mapping can
			 * be reused when a VM is rebooted. Since the IOMMU
			 * domain has already been destroyed we can just reset
			 * the flag here.
			 */
			mm->flags &= ~VM_MEMMAP_F_IOMMU;
		}
#endif
	}

	if (destroy) {
		for (i = 0; i < VM_MAX_MEMSEGS; i++)
			vm_free_memseg(vm, i);

		VMSPACE_FREE(vm->vmspace);
		vm->vmspace = NULL;
	}
#ifndef __FreeBSD__
	else {
		/*
		 * Clear the first memory segment (low mem), old memory contents
		 * could confuse the UEFI firmware.
		 */
		vm_clear_memseg(vm, 0);
	}
#endif
}

void
vm_destroy(struct vm *vm)
{
	vm_cleanup(vm, true);
	free(vm, M_VM);
}

int
vm_reinit(struct vm *vm)
{
	int error;

	/*
	 * A virtual machine can be reset only if all vcpus are suspended.
	 */
	if (CPU_CMP(&vm->suspended_cpus, &vm->active_cpus) == 0) {
		vm_cleanup(vm, false);
		vm_init(vm, false);
		error = 0;
	} else {
		error = EBUSY;
	}

	return (error);
}

const char *
vm_name(struct vm *vm)
{
	return (vm->name);
}

int
vm_map_mmio(struct vm *vm, vm_paddr_t gpa, size_t len, vm_paddr_t hpa)
{
	vm_object_t obj;

	if ((obj = vmm_mmio_alloc(vm->vmspace, gpa, len, hpa)) == NULL)
		return (ENOMEM);
	else
		return (0);
}

int
vm_unmap_mmio(struct vm *vm, vm_paddr_t gpa, size_t len)
{

	vmm_mmio_free(vm->vmspace, gpa, len);
	return (0);
}

/*
 * Return 'true' if 'gpa' is allocated in the guest address space.
 *
 * This function is called in the context of a running vcpu which acts as
 * an implicit lock on 'vm->mem_maps[]'.
 */
bool
vm_mem_allocated(struct vm *vm, int vcpuid, vm_paddr_t gpa)
{
	struct mem_map *mm;
	int i;

#ifdef INVARIANTS
	int hostcpu, state;
	state = vcpu_get_state(vm, vcpuid, &hostcpu);
	KASSERT(state == VCPU_RUNNING && hostcpu == curcpu,
	    ("%s: invalid vcpu state %d/%d", __func__, state, hostcpu));
#endif

	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (mm->len != 0 && gpa >= mm->gpa && gpa < mm->gpa + mm->len)
			return (true);		/* 'gpa' is sysmem or devmem */
	}

	if (ppt_is_mmio(vm, gpa))
		return (true);			/* 'gpa' is pci passthru mmio */

	return (false);
}

int
vm_alloc_memseg(struct vm *vm, int ident, size_t len, bool sysmem)
{
	struct mem_seg *seg;
	vm_object_t obj;

#ifndef __FreeBSD__
	extern pgcnt_t get_max_page_get(void);
#endif

	if (ident < 0 || ident >= VM_MAX_MEMSEGS)
		return (EINVAL);

	if (len == 0 || (len & PAGE_MASK))
		return (EINVAL);

#ifndef __FreeBSD__
	if (len > ptob(get_max_page_get()))
		return (EINVAL);
#endif

	seg = &vm->mem_segs[ident];
	if (seg->object != NULL) {
		if (seg->len == len && seg->sysmem == sysmem)
			return (EEXIST);
		else
			return (EINVAL);
	}

	obj = vm_object_allocate(OBJT_DEFAULT, len >> PAGE_SHIFT);
	if (obj == NULL)
		return (ENOMEM);

	seg->len = len;
	seg->object = obj;
	seg->sysmem = sysmem;
	return (0);
}

int
vm_get_memseg(struct vm *vm, int ident, size_t *len, bool *sysmem,
    vm_object_t *objptr)
{
	struct mem_seg *seg;

	if (ident < 0 || ident >= VM_MAX_MEMSEGS)
		return (EINVAL);

	seg = &vm->mem_segs[ident];
	if (len)
		*len = seg->len;
	if (sysmem)
		*sysmem = seg->sysmem;
	if (objptr)
		*objptr = seg->object;
	return (0);
}

#ifndef __FreeBSD__
static void
vm_clear_memseg(struct vm *vm, int ident)
{
	struct mem_seg *seg;

	KASSERT(ident >= 0 && ident < VM_MAX_MEMSEGS,
	    ("%s: invalid memseg ident %d", __func__, ident));

	seg = &vm->mem_segs[ident];

	if (seg->object != NULL)
		vm_object_clear(seg->object);
}
#endif

void
vm_free_memseg(struct vm *vm, int ident)
{
	struct mem_seg *seg;

	KASSERT(ident >= 0 && ident < VM_MAX_MEMSEGS,
	    ("%s: invalid memseg ident %d", __func__, ident));

	seg = &vm->mem_segs[ident];
	if (seg->object != NULL) {
		vm_object_deallocate(seg->object);
		bzero(seg, sizeof(struct mem_seg));
	}
}

int
vm_mmap_memseg(struct vm *vm, vm_paddr_t gpa, int segid, vm_ooffset_t first,
    size_t len, int prot, int flags)
{
	struct mem_seg *seg;
	struct mem_map *m, *map;
	vm_ooffset_t last;
	int i, error;

	if (prot == 0 || (prot & ~(VM_PROT_ALL)) != 0)
		return (EINVAL);

	if (flags & ~VM_MEMMAP_F_WIRED)
		return (EINVAL);

	if (segid < 0 || segid >= VM_MAX_MEMSEGS)
		return (EINVAL);

	seg = &vm->mem_segs[segid];
	if (seg->object == NULL)
		return (EINVAL);

	last = first + len;
	if (first < 0 || first >= last || last > seg->len)
		return (EINVAL);

	if ((gpa | first | last) & PAGE_MASK)
		return (EINVAL);

	map = NULL;
	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		m = &vm->mem_maps[i];
		if (m->len == 0) {
			map = m;
			break;
		}
	}

	if (map == NULL)
		return (ENOSPC);

	error = vm_map_find(&vm->vmspace->vm_map, seg->object, first, &gpa,
	    len, 0, VMFS_NO_SPACE, prot, prot, 0);
	if (error != KERN_SUCCESS)
		return (EFAULT);

	vm_object_reference(seg->object);

	if ((flags & VM_MEMMAP_F_WIRED) != 0) {
		error = vm_map_wire(&vm->vmspace->vm_map, gpa, gpa + len,
		    VM_MAP_WIRE_USER | VM_MAP_WIRE_NOHOLES);
		if (error != KERN_SUCCESS) {
			vm_map_remove(&vm->vmspace->vm_map, gpa, gpa + len);
			return (error == KERN_RESOURCE_SHORTAGE ? ENOMEM :
			    EFAULT);
		}
	}

	map->gpa = gpa;
	map->len = len;
	map->segoff = first;
	map->segid = segid;
	map->prot = prot;
	map->flags = flags;
	return (0);
}

int
vm_mmap_getnext(struct vm *vm, vm_paddr_t *gpa, int *segid,
    vm_ooffset_t *segoff, size_t *len, int *prot, int *flags)
{
	struct mem_map *mm, *mmnext;
	int i;

	mmnext = NULL;
	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (mm->len == 0 || mm->gpa < *gpa)
			continue;
		if (mmnext == NULL || mm->gpa < mmnext->gpa)
			mmnext = mm;
	}

	if (mmnext != NULL) {
		*gpa = mmnext->gpa;
		if (segid)
			*segid = mmnext->segid;
		if (segoff)
			*segoff = mmnext->segoff;
		if (len)
			*len = mmnext->len;
		if (prot)
			*prot = mmnext->prot;
		if (flags)
			*flags = mmnext->flags;
		return (0);
	} else {
		return (ENOENT);
	}
}

static void
vm_free_memmap(struct vm *vm, int ident)
{
	struct mem_map *mm;
	int error;

	mm = &vm->mem_maps[ident];
	if (mm->len) {
		error = vm_map_remove(&vm->vmspace->vm_map, mm->gpa,
		    mm->gpa + mm->len);
		KASSERT(error == KERN_SUCCESS, ("%s: vm_map_remove error %d",
		    __func__, error));
		bzero(mm, sizeof(struct mem_map));
	}
}

static __inline bool
sysmem_mapping(struct vm *vm, struct mem_map *mm)
{

	if (mm->len != 0 && vm->mem_segs[mm->segid].sysmem)
		return (true);
	else
		return (false);
}

vm_paddr_t
vmm_sysmem_maxaddr(struct vm *vm)
{
	struct mem_map *mm;
	vm_paddr_t maxaddr;
	int i;

	maxaddr = 0;
	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (sysmem_mapping(vm, mm)) {
			if (maxaddr < mm->gpa + mm->len)
				maxaddr = mm->gpa + mm->len;
		}
	}
	return (maxaddr);
}

static void
vm_iommu_modify(struct vm *vm, bool map)
{
	int i, sz;
	vm_paddr_t gpa, hpa;
	struct mem_map *mm;
#ifdef __FreeBSD__
	void *vp, *cookie, *host_domain;
#else
	void *vp, *cookie, *host_domain __unused;
#endif

	sz = PAGE_SIZE;
	host_domain = iommu_host_domain();

	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (!sysmem_mapping(vm, mm))
			continue;

		if (map) {
			KASSERT((mm->flags & VM_MEMMAP_F_IOMMU) == 0,
			    ("iommu map found invalid memmap %lx/%lx/%x",
			    mm->gpa, mm->len, mm->flags));
			if ((mm->flags & VM_MEMMAP_F_WIRED) == 0)
				continue;
			mm->flags |= VM_MEMMAP_F_IOMMU;
		} else {
			if ((mm->flags & VM_MEMMAP_F_IOMMU) == 0)
				continue;
			mm->flags &= ~VM_MEMMAP_F_IOMMU;
			KASSERT((mm->flags & VM_MEMMAP_F_WIRED) != 0,
			    ("iommu unmap found invalid memmap %lx/%lx/%x",
			    mm->gpa, mm->len, mm->flags));
		}

		gpa = mm->gpa;
		while (gpa < mm->gpa + mm->len) {
			vp = vm_gpa_hold(vm, -1, gpa, PAGE_SIZE, VM_PROT_WRITE,
					 &cookie);
			KASSERT(vp != NULL, ("vm(%s) could not map gpa %lx",
			    vm_name(vm), gpa));

			vm_gpa_release(cookie);

			hpa = DMAP_TO_PHYS((uintptr_t)vp);
			if (map) {
				iommu_create_mapping(vm->iommu, gpa, hpa, sz);
#ifdef __FreeBSD__
				iommu_remove_mapping(host_domain, hpa, sz);
#endif
			} else {
				iommu_remove_mapping(vm->iommu, gpa, sz);
#ifdef __FreeBSD__
				iommu_create_mapping(host_domain, hpa, hpa, sz);
#endif
			}

			gpa += PAGE_SIZE;
		}
	}

	/*
	 * Invalidate the cached translations associated with the domain
	 * from which pages were removed.
	 */
#ifdef __FreeBSD__
	if (map)
		iommu_invalidate_tlb(host_domain);
	else
		iommu_invalidate_tlb(vm->iommu);
#else
	iommu_invalidate_tlb(vm->iommu);
#endif
}

#define	vm_iommu_unmap(vm)	vm_iommu_modify((vm), false)
#define	vm_iommu_map(vm)	vm_iommu_modify((vm), true)

#ifdef __FreeBSD__
int
vm_unassign_pptdev(struct vm *vm, int bus, int slot, int func)
#else
int
vm_unassign_pptdev(struct vm *vm, int pptfd)
#endif /* __FreeBSD__ */
{
	int error;

#ifdef __FreeBSD__
	error = ppt_unassign_device(vm, bus, slot, func);
#else
	error = ppt_unassign_device(vm, pptfd);
#endif /* __FreeBSD__ */
	if (error)
		return (error);

	if (ppt_assigned_devices(vm) == 0)
		vm_iommu_unmap(vm);

	return (0);
}

#ifdef __FreeBSD__
int
vm_assign_pptdev(struct vm *vm, int bus, int slot, int func)
#else
int
vm_assign_pptdev(struct vm *vm, int pptfd)
#endif /* __FreeBSD__ */
{
	int error;
	vm_paddr_t maxaddr;

	/* Set up the IOMMU to do the 'gpa' to 'hpa' translation */
	if (ppt_assigned_devices(vm) == 0) {
		KASSERT(vm->iommu == NULL,
		    ("vm_assign_pptdev: iommu must be NULL"));
		maxaddr = vmm_sysmem_maxaddr(vm);
		vm->iommu = iommu_create_domain(maxaddr);
		if (vm->iommu == NULL)
			return (ENXIO);
		vm_iommu_map(vm);
	}

#ifdef __FreeBSD__
	error = ppt_assign_device(vm, bus, slot, func);
#else
	error = ppt_assign_device(vm, pptfd);
#endif /* __FreeBSD__ */
	return (error);
}

void *
vm_gpa_hold(struct vm *vm, int vcpuid, vm_paddr_t gpa, size_t len, int reqprot,
	    void **cookie)
{
	int i, count, pageoff;
	struct mem_map *mm;
	vm_page_t m;
#ifdef INVARIANTS
	/*
	 * All vcpus are frozen by ioctls that modify the memory map
	 * (e.g. VM_MMAP_MEMSEG). Therefore 'vm->memmap[]' stability is
	 * guaranteed if at least one vcpu is in the VCPU_FROZEN state.
	 */
	int state;
	KASSERT(vcpuid >= -1 && vcpuid < vm->maxcpus, ("%s: invalid vcpuid %d",
	    __func__, vcpuid));
	for (i = 0; i < vm->maxcpus; i++) {
		if (vcpuid != -1 && vcpuid != i)
			continue;
		state = vcpu_get_state(vm, i, NULL);
		KASSERT(state == VCPU_FROZEN, ("%s: invalid vcpu state %d",
		    __func__, state));
	}
#endif
	pageoff = gpa & PAGE_MASK;
	if (len > PAGE_SIZE - pageoff)
		panic("vm_gpa_hold: invalid gpa/len: 0x%016lx/%lu", gpa, len);

	count = 0;
	for (i = 0; i < VM_MAX_MEMMAPS; i++) {
		mm = &vm->mem_maps[i];
		if (mm->len == 0) {
			continue;
		}
		if (gpa >= mm->gpa && gpa < mm->gpa + mm->len) {
			count = vm_fault_quick_hold_pages(&vm->vmspace->vm_map,
			    trunc_page(gpa), PAGE_SIZE, reqprot, &m, 1);
			break;
		}
	}

	if (count == 1) {
		*cookie = m;
		return ((void *)(PHYS_TO_DMAP(VM_PAGE_TO_PHYS(m)) + pageoff));
	} else {
		*cookie = NULL;
		return (NULL);
	}
}

void
vm_gpa_release(void *cookie)
{
	vm_page_t m = cookie;

	vm_page_unwire(m, PQ_ACTIVE);
}

int
vm_get_register(struct vm *vm, int vcpu, int reg, uint64_t *retval)
{

	if (vcpu < 0 || vcpu >= vm->maxcpus)
		return (EINVAL);

	if (reg >= VM_REG_LAST)
		return (EINVAL);

	return (VMGETREG(vm->cookie, vcpu, reg, retval));
}

int
vm_set_register(struct vm *vm, int vcpuid, int reg, uint64_t val)
{
	struct vcpu *vcpu;
	int error;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (reg >= VM_REG_LAST)
		return (EINVAL);

	error = VMSETREG(vm->cookie, vcpuid, reg, val);
	if (error || reg != VM_REG_GUEST_RIP)
		return (error);

	/* Set 'nextrip' to match the value of %rip */
	VCPU_CTR1(vm, vcpuid, "Setting nextrip to %lx", val);
	vcpu = &vm->vcpu[vcpuid];
	vcpu->nextrip = val;
	return (0);
}

static bool
is_descriptor_table(int reg)
{
	switch (reg) {
	case VM_REG_GUEST_IDTR:
	case VM_REG_GUEST_GDTR:
		return (true);
	default:
		return (false);
	}
}

static bool
is_segment_register(int reg)
{
	switch (reg) {
	case VM_REG_GUEST_ES:
	case VM_REG_GUEST_CS:
	case VM_REG_GUEST_SS:
	case VM_REG_GUEST_DS:
	case VM_REG_GUEST_FS:
	case VM_REG_GUEST_GS:
	case VM_REG_GUEST_TR:
	case VM_REG_GUEST_LDTR:
		return (true);
	default:
		return (false);
	}
}

int
vm_get_seg_desc(struct vm *vm, int vcpu, int reg,
		struct seg_desc *desc)
{

	if (vcpu < 0 || vcpu >= vm->maxcpus)
		return (EINVAL);

	if (!is_segment_register(reg) && !is_descriptor_table(reg))
		return (EINVAL);

	return (VMGETDESC(vm->cookie, vcpu, reg, desc));
}

int
vm_set_seg_desc(struct vm *vm, int vcpu, int reg,
		struct seg_desc *desc)
{
	if (vcpu < 0 || vcpu >= vm->maxcpus)
		return (EINVAL);

	if (!is_segment_register(reg) && !is_descriptor_table(reg))
		return (EINVAL);

	return (VMSETDESC(vm->cookie, vcpu, reg, desc));
}

static void
restore_guest_fpustate(struct vcpu *vcpu)
{

	/* flush host state to the pcb */
	fpuexit(curthread);

	/* restore guest FPU state */
	fpu_stop_emulating();
	fpurestore(vcpu->guestfpu);

	/* restore guest XCR0 if XSAVE is enabled in the host */
	if (rcr4() & CR4_XSAVE)
		load_xcr(0, vcpu->guest_xcr0);

	/*
	 * The FPU is now "dirty" with the guest's state so turn on emulation
	 * to trap any access to the FPU by the host.
	 */
	fpu_start_emulating();
}

static void
save_guest_fpustate(struct vcpu *vcpu)
{

	if ((rcr0() & CR0_TS) == 0)
		panic("fpu emulation not enabled in host!");

	/* save guest XCR0 and restore host XCR0 */
	if (rcr4() & CR4_XSAVE) {
		vcpu->guest_xcr0 = rxcr(0);
		load_xcr(0, vmm_get_host_xcr0());
	}

	/* save guest FPU state */
	fpu_stop_emulating();
	fpusave(vcpu->guestfpu);
#ifdef __FreeBSD__
	fpu_start_emulating();
#else
	/*
	 * When the host state has been restored, we should not re-enable
	 * CR0.TS on illumos for eager FPU.
	 */
#endif
}

static VMM_STAT(VCPU_IDLE_TICKS, "number of ticks vcpu was idle");

static int
vcpu_set_state_locked(struct vm *vm, int vcpuid, enum vcpu_state newstate,
    bool from_idle)
{
	struct vcpu *vcpu;
	int error;

	vcpu = &vm->vcpu[vcpuid];
	vcpu_assert_locked(vcpu);

	/*
	 * State transitions from the vmmdev_ioctl() must always begin from
	 * the VCPU_IDLE state. This guarantees that there is only a single
	 * ioctl() operating on a vcpu at any point.
	 */
	if (from_idle) {
		while (vcpu->state != VCPU_IDLE) {
			vcpu->reqidle = 1;
			vcpu_notify_event_locked(vcpu, false);
			VCPU_CTR1(vm, vcpuid, "vcpu state change from %s to "
			    "idle requested", vcpu_state2str(vcpu->state));
#ifdef __FreeBSD__
			msleep_spin(&vcpu->state, &vcpu->mtx, "vmstat", hz);
#else
			cv_wait(&vcpu->state_cv, &vcpu->mtx.m);
#endif
		}
	} else {
		KASSERT(vcpu->state != VCPU_IDLE, ("invalid transition from "
		    "vcpu idle state"));
	}

	if (vcpu->state == VCPU_RUNNING) {
		KASSERT(vcpu->hostcpu == curcpu, ("curcpu %d and hostcpu %d "
		    "mismatch for running vcpu", curcpu, vcpu->hostcpu));
	} else {
		KASSERT(vcpu->hostcpu == NOCPU, ("Invalid hostcpu %d for a "
		    "vcpu that is not running", vcpu->hostcpu));
	}

	/*
	 * The following state transitions are allowed:
	 * IDLE -> FROZEN -> IDLE
	 * FROZEN -> RUNNING -> FROZEN
	 * FROZEN -> SLEEPING -> FROZEN
	 */
	switch (vcpu->state) {
	case VCPU_IDLE:
	case VCPU_RUNNING:
	case VCPU_SLEEPING:
		error = (newstate != VCPU_FROZEN);
		break;
	case VCPU_FROZEN:
		error = (newstate == VCPU_FROZEN);
		break;
	default:
		error = 1;
		break;
	}

	if (newstate == VCPU_RUNNING) {
		while (vcpu->runblock != 0) {
#ifdef __FreeBSD__
			msleep_spin(&vcpu->state, &vcpu->mtx, "vcpublk", 0);
#else
			cv_wait(&vcpu->state_cv, &vcpu->mtx.m);
#endif
		}
	}

	if (error)
		return (EBUSY);

	VCPU_CTR2(vm, vcpuid, "vcpu state changed from %s to %s",
	    vcpu_state2str(vcpu->state), vcpu_state2str(newstate));

	vcpu->state = newstate;
	if (newstate == VCPU_RUNNING)
		vcpu->hostcpu = curcpu;
	else
		vcpu->hostcpu = NOCPU;

	if (newstate == VCPU_IDLE ||
	    (newstate == VCPU_FROZEN && vcpu->runblock != 0)) {
#ifdef __FreeBSD__
		wakeup(&vcpu->state);
#else
		cv_broadcast(&vcpu->state_cv);
#endif
	}

	return (0);
}

static void
vcpu_require_state(struct vm *vm, int vcpuid, enum vcpu_state newstate)
{
	int error;

	if ((error = vcpu_set_state(vm, vcpuid, newstate, false)) != 0)
		panic("Error %d setting state to %d\n", error, newstate);
}

static void
vcpu_require_state_locked(struct vm *vm, int vcpuid, enum vcpu_state newstate)
{
	int error;

	if ((error = vcpu_set_state_locked(vm, vcpuid, newstate, false)) != 0)
		panic("Error %d setting state to %d", error, newstate);
}

/*
 * Emulate a guest 'hlt' by sleeping until the vcpu is ready to run.
 */
static int
vm_handle_hlt(struct vm *vm, int vcpuid, bool intr_disabled)
{
	struct vcpu *vcpu;
#ifdef __FreeBSD__
	const char *wmesg;
#else
	const char *wmesg __unused;
#endif
	int t, vcpu_halted, vm_halted;

	KASSERT(!CPU_ISSET(vcpuid, &vm->halted_cpus), ("vcpu already halted"));

	vcpu = &vm->vcpu[vcpuid];
	vcpu_halted = 0;
	vm_halted = 0;

	vcpu_lock(vcpu);
	while (1) {
		/*
		 * Do a final check for pending NMI or interrupts before
		 * really putting this thread to sleep. Also check for
		 * software events that would cause this vcpu to wakeup.
		 *
		 * These interrupts/events could have happened after the
		 * vcpu returned from VMRUN() and before it acquired the
		 * vcpu lock above.
		 */
		if (vm->suspend || vcpu->reqidle)
			break;
		if (vm_nmi_pending(vm, vcpuid))
			break;
		if (!intr_disabled) {
			if (vm_extint_pending(vm, vcpuid) ||
			    vlapic_pending_intr(vcpu->vlapic, NULL)) {
				break;
			}
		}

		/* Don't go to sleep if the vcpu thread needs to yield */
		if (vcpu_should_yield(vm, vcpuid))
			break;

		if (vcpu_debugged(vm, vcpuid))
			break;

		/*
		 * Some Linux guests implement "halt" by having all vcpus
		 * execute HLT with interrupts disabled. 'halted_cpus' keeps
		 * track of the vcpus that have entered this state. When all
		 * vcpus enter the halted state the virtual machine is halted.
		 */
		if (intr_disabled) {
			wmesg = "vmhalt";
			VCPU_CTR0(vm, vcpuid, "Halted");
			if (!vcpu_halted && halt_detection_enabled) {
				vcpu_halted = 1;
				CPU_SET_ATOMIC(vcpuid, &vm->halted_cpus);
			}
			if (CPU_CMP(&vm->halted_cpus, &vm->active_cpus) == 0) {
				vm_halted = 1;
				break;
			}
		} else {
			wmesg = "vmidle";
		}

		t = ticks;
		vcpu_require_state_locked(vm, vcpuid, VCPU_SLEEPING);
#ifdef __FreeBSD__
		/*
		 * XXX msleep_spin() cannot be interrupted by signals so
		 * wake up periodically to check pending signals.
		 */
		msleep_spin(vcpu, &vcpu->mtx, wmesg, hz);
#else
		/*
		 * Fortunately, cv_wait_sig can be interrupted by signals, so
		 * there is no need to periodically wake up.
		 */
		(void) cv_wait_sig(&vcpu->vcpu_cv, &vcpu->mtx.m);
#endif
		vcpu_require_state_locked(vm, vcpuid, VCPU_FROZEN);
		vmm_stat_incr(vm, vcpuid, VCPU_IDLE_TICKS, ticks - t);
	}

	if (vcpu_halted)
		CPU_CLR_ATOMIC(vcpuid, &vm->halted_cpus);

	vcpu_unlock(vcpu);

	if (vm_halted)
		vm_suspend(vm, VM_SUSPEND_HALT);

	return (0);
}

static int
vm_handle_paging(struct vm *vm, int vcpuid)
{
	int rv, ftype;
	struct vm_map *map;
	struct vcpu *vcpu;
	struct vm_exit *vme;

	vcpu = &vm->vcpu[vcpuid];
	vme = &vcpu->exitinfo;

	KASSERT(vme->inst_length == 0, ("%s: invalid inst_length %d",
	    __func__, vme->inst_length));

	ftype = vme->u.paging.fault_type;
	KASSERT(ftype == VM_PROT_READ ||
	    ftype == VM_PROT_WRITE || ftype == VM_PROT_EXECUTE,
	    ("vm_handle_paging: invalid fault_type %d", ftype));

	if (ftype == VM_PROT_READ || ftype == VM_PROT_WRITE) {
		rv = pmap_emulate_accessed_dirty(vmspace_pmap(vm->vmspace),
		    vme->u.paging.gpa, ftype);
		if (rv == 0) {
			VCPU_CTR2(vm, vcpuid, "%s bit emulation for gpa %lx",
			    ftype == VM_PROT_READ ? "accessed" : "dirty",
			    vme->u.paging.gpa);
			goto done;
		}
	}

	map = &vm->vmspace->vm_map;
	rv = vm_fault(map, vme->u.paging.gpa, ftype, VM_FAULT_NORMAL);

	VCPU_CTR3(vm, vcpuid, "vm_handle_paging rv = %d, gpa = %lx, "
	    "ftype = %d", rv, vme->u.paging.gpa, ftype);

	if (rv != KERN_SUCCESS)
		return (EFAULT);
done:
	return (0);
}

int
vm_service_mmio_read(struct vm *vm, int cpuid, uint64_t gpa, uint64_t *rval,
    int rsize)
{
	int err = ESRCH;

	if (gpa >= DEFAULT_APIC_BASE && gpa < DEFAULT_APIC_BASE + PAGE_SIZE) {
		err = lapic_mmio_read(vm, cpuid, gpa, rval, rsize);
	} else if (gpa >= VIOAPIC_BASE && gpa < VIOAPIC_BASE + VIOAPIC_SIZE) {
		err = vioapic_mmio_read(vm, cpuid, gpa, rval, rsize);
	} else if (gpa >= VHPET_BASE && gpa < VHPET_BASE + VHPET_SIZE) {
		err = vhpet_mmio_read(vm, cpuid, gpa, rval, rsize);
	}

	return (err);
}

int
vm_service_mmio_write(struct vm *vm, int cpuid, uint64_t gpa, uint64_t wval,
    int wsize)
{
	int err = ESRCH;

	if (gpa >= DEFAULT_APIC_BASE && gpa < DEFAULT_APIC_BASE + PAGE_SIZE) {
		err = lapic_mmio_write(vm, cpuid, gpa, wval, wsize);
	} else if (gpa >= VIOAPIC_BASE && gpa < VIOAPIC_BASE + VIOAPIC_SIZE) {
		err = vioapic_mmio_write(vm, cpuid, gpa, wval, wsize);
	} else if (gpa >= VHPET_BASE && gpa < VHPET_BASE + VHPET_SIZE) {
		err = vhpet_mmio_write(vm, cpuid, gpa, wval, wsize);
	}

	return (err);
}

static int
vm_handle_mmio_emul(struct vm *vm, int vcpuid)
{
	struct vie *vie;
	struct vcpu *vcpu;
	struct vm_exit *vme;
	uint64_t inst_addr;
	int error, fault, cs_d;

	vcpu = &vm->vcpu[vcpuid];
	vme = &vcpu->exitinfo;
	vie = vcpu->vie_ctx;

	KASSERT(vme->inst_length == 0, ("%s: invalid inst_length %d",
	    __func__, vme->inst_length));

	inst_addr = vme->rip + vme->u.mmio_emul.cs_base;
	cs_d = vme->u.mmio_emul.cs_d;

	VCPU_CTR1(vm, vcpuid, "inst_emul fault accessing gpa %lx",
	    vme->u.mmio_emul.gpa);

	/* Fetch the faulting instruction */
	if (vie_needs_fetch(vie)) {
		error = vie_fetch_instruction(vie, vm, vcpuid, inst_addr,
		    &fault);
		if (error != 0) {
			return (error);
		} else if (fault) {
			/*
			 * If a fault during instruction fetch was encounted, it
			 * will have asserted that the appropriate exception be
			 * injected at next entry.  No further work is required.
			 */
			return (0);
		}
	}

	if (vie_decode_instruction(vie, vm, vcpuid, cs_d) != 0) {
		VCPU_CTR1(vm, vcpuid, "Error decoding instruction at %lx",
		    inst_addr);
		/* Dump (unrecognized) instruction bytes in userspace */
		vie_fallback_exitinfo(vie, vme);
		return (-1);
	}
	if (vme->u.mmio_emul.gla != VIE_INVALID_GLA &&
	    vie_verify_gla(vie, vm, vcpuid, vme->u.mmio_emul.gla) != 0) {
		/* Decoded GLA does not match GLA from VM exit state */
		vie_fallback_exitinfo(vie, vme);
		return (-1);
	}

repeat:
	error = vie_emulate_mmio(vie, vm, vcpuid);
	if (error < 0) {
		/*
		 * MMIO not handled by any of the in-kernel-emulated devices, so
		 * make a trip out to userspace for it.
		 */
		vie_exitinfo(vie, vme);
	} else if (error == EAGAIN) {
		/*
		 * Continue emulating the rep-prefixed instruction, which has
		 * not completed its iterations.
		 *
		 * In case this can be emulated in-kernel and has a high
		 * repetition count (causing a tight spin), it should be
		 * deferential to yield conditions.
		 */
		if (!vcpu_should_yield(vm, vcpuid)) {
			goto repeat;
		} else {
			/*
			 * Defer to the contending load by making a trip to
			 * userspace with a no-op (BOGUS) exit reason.
			 */
			vie_reset(vie);
			vme->exitcode = VM_EXITCODE_BOGUS;
			return (-1);
		}
	} else if (error == 0) {
		/* Update %rip now that instruction has been emulated */
		vie_advance_pc(vie, &vcpu->nextrip);
	}
	return (error);
}

static int
vm_handle_inout(struct vm *vm, int vcpuid, struct vm_exit *vme)
{
	struct vcpu *vcpu;
	struct vie *vie;
	int err;

	vcpu = &vm->vcpu[vcpuid];
	vie = vcpu->vie_ctx;

repeat:
	err = vie_emulate_inout(vie, vm, vcpuid);

	if (err < 0) {
		/*
		 * In/out not handled by any of the in-kernel-emulated devices,
		 * so make a trip out to userspace for it.
		 */
		vie_exitinfo(vie, vme);
		return (err);
	} else if (err == EAGAIN) {
		/*
		 * Continue emulating the rep-prefixed ins/outs, which has not
		 * completed its iterations.
		 *
		 * In case this can be emulated in-kernel and has a high
		 * repetition count (causing a tight spin), it should be
		 * deferential to yield conditions.
		 */
		if (!vcpu_should_yield(vm, vcpuid)) {
			goto repeat;
		} else {
			/*
			 * Defer to the contending load by making a trip to
			 * userspace with a no-op (BOGUS) exit reason.
			 */
			vie_reset(vie);
			vme->exitcode = VM_EXITCODE_BOGUS;
			return (-1);
		}
	} else if (err != 0) {
		/* Emulation failure.  Bail all the way out to userspace. */
		vme->exitcode = VM_EXITCODE_INST_EMUL;
		bzero(&vme->u.inst_emul, sizeof (vme->u.inst_emul));
		return (-1);
	}

	vie_advance_pc(vie, &vcpu->nextrip);
	return (0);
}

static int
vm_handle_suspend(struct vm *vm, int vcpuid)
{
#ifdef __FreeBSD__
	int error, i;
	struct vcpu *vcpu;
	struct thread *td;

	error = 0;
	vcpu = &vm->vcpu[vcpuid];
	td = curthread;
#else
	int i;
	struct vcpu *vcpu;

	vcpu = &vm->vcpu[vcpuid];
#endif

	CPU_SET_ATOMIC(vcpuid, &vm->suspended_cpus);

#ifdef __FreeBSD__
	/*
	 * Wait until all 'active_cpus' have suspended themselves.
	 *
	 * Since a VM may be suspended at any time including when one or
	 * more vcpus are doing a rendezvous we need to call the rendezvous
	 * handler while we are waiting to prevent a deadlock.
	 */
	vcpu_lock(vcpu);
	while (error == 0) {
		if (CPU_CMP(&vm->suspended_cpus, &vm->active_cpus) == 0) {
			VCPU_CTR0(vm, vcpuid, "All vcpus suspended");
			break;
		}

		if (vm->rendezvous_func == NULL) {
			VCPU_CTR0(vm, vcpuid, "Sleeping during suspend");
			vcpu_require_state_locked(vm, vcpuid, VCPU_SLEEPING);
			msleep_spin(vcpu, &vcpu->mtx, "vmsusp", hz);
			vcpu_require_state_locked(vm, vcpuid, VCPU_FROZEN);
			if ((td->td_flags & TDF_NEEDSUSPCHK) != 0) {
				vcpu_unlock(vcpu);
				error = thread_check_susp(td, false);
				vcpu_lock(vcpu);
			}
		} else {
			VCPU_CTR0(vm, vcpuid, "Rendezvous during suspend");
			vcpu_unlock(vcpu);
			error = vm_handle_rendezvous(vm, vcpuid);
			vcpu_lock(vcpu);
		}
	}
	vcpu_unlock(vcpu);
#else
	vcpu_lock(vcpu);
	while (1) {
		int rc;

		if (CPU_CMP(&vm->suspended_cpus, &vm->active_cpus) == 0) {
			VCPU_CTR0(vm, vcpuid, "All vcpus suspended");
			break;
		}

		vcpu_require_state_locked(vm, vcpuid, VCPU_SLEEPING);
		rc = cv_reltimedwait_sig(&vcpu->vcpu_cv, &vcpu->mtx.m, hz,
		    TR_CLOCK_TICK);
		vcpu_require_state_locked(vm, vcpuid, VCPU_FROZEN);

		/*
		 * If the userspace process driving the instance is killed, any
		 * vCPUs yet to be marked suspended (because they are not
		 * VM_RUN-ing in the kernel presently) will never reach that
		 * state.
		 *
		 * To avoid vm_handle_suspend() getting stuck in the kernel
		 * waiting for those vCPUs, offer a bail-out even though it
		 * means returning without all vCPUs in a suspended state.
		 */
		if (rc <= 0) {
			if ((curproc->p_flag & SEXITING) != 0) {
				break;
			}
		}
	}
	vcpu_unlock(vcpu);

#endif

	/*
	 * Wakeup the other sleeping vcpus and return to userspace.
	 */
	for (i = 0; i < vm->maxcpus; i++) {
		if (CPU_ISSET(i, &vm->suspended_cpus)) {
			vcpu_notify_event(vm, i, false);
		}
	}

	return (-1);
}

static int
vm_handle_reqidle(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	KASSERT(vcpu->reqidle, ("invalid vcpu reqidle %d", vcpu->reqidle));
	vcpu->reqidle = 0;
	vcpu_unlock(vcpu);
	return (-1);
}

#ifndef __FreeBSD__
static int
vm_handle_wrmsr(struct vm *vm, int vcpuid, struct vm_exit *vme)
{
	struct vcpu *cpu = &vm->vcpu[vcpuid];
	const uint32_t code = vme->u.msr.code;
	const uint64_t val = vme->u.msr.wval;

	switch (code) {
	case MSR_TSC:
		cpu->tsc_offset = val - rdtsc();
		return (0);
	}

	return (-1);
}
#endif /* __FreeBSD__ */

void
vm_req_spinup_ap(struct vm *vm, int req_vcpuid, uint64_t req_rip)
{
	if (vm->sipi_req) {
		/* This should never occur if userspace is doing its job. */
		vm->stats.sipi_supersede++;
	}
	vm->sipi_req = true;
	vm->sipi_req_vcpu = req_vcpuid;
	vm->sipi_req_rip = req_rip;
}

int
vm_suspend(struct vm *vm, enum vm_suspend_how how)
{
	int i;

	if (how <= VM_SUSPEND_NONE || how >= VM_SUSPEND_LAST)
		return (EINVAL);

	if (atomic_cmpset_int((uint_t *)&vm->suspend, 0, how) == 0) {
		VM_CTR2(vm, "virtual machine already suspended %d/%d",
		    vm->suspend, how);
		return (EALREADY);
	}

	VM_CTR1(vm, "virtual machine successfully suspended %d", how);

	/*
	 * Notify all active vcpus that they are now suspended.
	 */
	for (i = 0; i < vm->maxcpus; i++) {
		if (CPU_ISSET(i, &vm->active_cpus))
			vcpu_notify_event(vm, i, false);
	}

	return (0);
}

void
vm_exit_suspended(struct vm *vm, int vcpuid, uint64_t rip)
{
	struct vm_exit *vmexit;

	KASSERT(vm->suspend > VM_SUSPEND_NONE && vm->suspend < VM_SUSPEND_LAST,
	    ("vm_exit_suspended: invalid suspend type %d", vm->suspend));

	vmexit = vm_exitinfo(vm, vcpuid);
	vmexit->rip = rip;
	vmexit->inst_length = 0;
	vmexit->exitcode = VM_EXITCODE_SUSPENDED;
	vmexit->u.suspended.how = vm->suspend;
}

void
vm_exit_debug(struct vm *vm, int vcpuid, uint64_t rip)
{
	struct vm_exit *vmexit;

	vmexit = vm_exitinfo(vm, vcpuid);
	vmexit->rip = rip;
	vmexit->inst_length = 0;
	vmexit->exitcode = VM_EXITCODE_DEBUG;
}

void
vm_exit_runblock(struct vm *vm, int vcpuid, uint64_t rip)
{
	struct vm_exit *vmexit;

	vmexit = vm_exitinfo(vm, vcpuid);
	vmexit->rip = rip;
	vmexit->inst_length = 0;
	vmexit->exitcode = VM_EXITCODE_RUNBLOCK;
	vmm_stat_incr(vm, vcpuid, VMEXIT_RUNBLOCK, 1);
}

void
vm_exit_reqidle(struct vm *vm, int vcpuid, uint64_t rip)
{
	struct vm_exit *vmexit;

	vmexit = vm_exitinfo(vm, vcpuid);
	vmexit->rip = rip;
	vmexit->inst_length = 0;
	vmexit->exitcode = VM_EXITCODE_REQIDLE;
	vmm_stat_incr(vm, vcpuid, VMEXIT_REQIDLE, 1);
}

void
vm_exit_astpending(struct vm *vm, int vcpuid, uint64_t rip)
{
	struct vm_exit *vmexit;

	vmexit = vm_exitinfo(vm, vcpuid);
	vmexit->rip = rip;
	vmexit->inst_length = 0;
	vmexit->exitcode = VM_EXITCODE_BOGUS;
	vmm_stat_incr(vm, vcpuid, VMEXIT_ASTPENDING, 1);
}

#ifndef __FreeBSD__
/*
 * Some vmm resources, such as the lapic, may have CPU-specific resources
 * allocated to them which would benefit from migration onto the host CPU which
 * is processing the vcpu state.
 */
static void
vm_localize_resources(struct vm *vm, struct vcpu *vcpu)
{
	/*
	 * Localizing cyclic resources requires acquisition of cpu_lock, and
	 * doing so with kpreempt disabled is a recipe for deadlock disaster.
	 */
	VERIFY(curthread->t_preempt == 0);

	/*
	 * Do not bother with localization if this vCPU is about to return to
	 * the host CPU it was last localized to.
	 */
	if (vcpu->lastloccpu == curcpu)
		return;

	/*
	 * Localize system-wide resources to the primary boot vCPU.  While any
	 * of the other vCPUs may access them, it keeps the potential interrupt
	 * footprint constrained to CPUs involved with this instance.
	 */
	if (vcpu == &vm->vcpu[0]) {
		vhpet_localize_resources(vm->vhpet);
		vrtc_localize_resources(vm->vrtc);
		vatpit_localize_resources(vm->vatpit);
	}

	vlapic_localize_resources(vcpu->vlapic);

	vcpu->lastloccpu = curcpu;
}

static void
vmm_savectx(void *arg)
{
	vm_thread_ctx_t *vtc = arg;
	struct vm *vm = vtc->vtc_vm;
	const int vcpuid = vtc->vtc_vcpuid;

	if (ops->vmsavectx != NULL) {
		ops->vmsavectx(vm->cookie, vcpuid);
	}

	/*
	 * If the CPU holds the restored guest FPU state, save it and restore
	 * the host FPU state before this thread goes off-cpu.
	 */
	if ((vtc->vtc_status & VTCS_FPU_RESTORED) != 0) {
		struct vcpu *vcpu = &vm->vcpu[vcpuid];

		save_guest_fpustate(vcpu);
		vtc->vtc_status &= ~VTCS_FPU_RESTORED;
	}
}

static void
vmm_restorectx(void *arg)
{
	vm_thread_ctx_t *vtc = arg;
	struct vm *vm = vtc->vtc_vm;
	const int vcpuid = vtc->vtc_vcpuid;

	/*
	 * When coming back on-cpu, only restore the guest FPU status if the
	 * thread is in a context marked as requiring it.  This should be rare,
	 * occurring only when a future logic error results in a voluntary
	 * sleep during the VMRUN critical section.
	 *
	 * The common case will result in elision of the guest FPU state
	 * restoration, deferring that action until it is clearly necessary
	 * during vm_run.
	 */
	VERIFY((vtc->vtc_status & VTCS_FPU_RESTORED) == 0);
	if ((vtc->vtc_status & VTCS_FPU_CTX_CRITICAL) != 0) {
		struct vcpu *vcpu = &vm->vcpu[vcpuid];

		restore_guest_fpustate(vcpu);
		vtc->vtc_status |= VTCS_FPU_RESTORED;
	}

	if (ops->vmrestorectx != NULL) {
		ops->vmrestorectx(vm->cookie, vcpuid);
	}

}

/*
 * If we're in removectx(), we might still have state to tidy up.
 */
static void
vmm_freectx(void *arg, int isexec)
{
	vmm_savectx(arg);
}

#endif /* __FreeBSD */

static int
vm_entry_actions(struct vm *vm, int vcpuid, const struct vm_entry *entry,
    struct vm_exit *vme)
{
	struct vcpu *vcpu;
	struct vie *vie;
	int err;

	vcpu = &vm->vcpu[vcpuid];
	vie = vcpu->vie_ctx;
	err = 0;

	switch (entry->cmd) {
	case VEC_DEFAULT:
		return (0);
	case VEC_DISCARD_INSTR:
		vie_reset(vie);
		return (0);
	case VEC_COMPLETE_MMIO:
		err = vie_fulfill_mmio(vie, &entry->u.mmio);
		if (err == 0) {
			err = vie_emulate_mmio(vie, vm, vcpuid);
			if (err == 0) {
				vie_advance_pc(vie, &vcpu->nextrip);
			} else if (err < 0) {
				vie_exitinfo(vie, vme);
			} else if (err == EAGAIN) {
				/*
				 * Clear the instruction emulation state in
				 * order to re-enter VM context and continue
				 * this 'rep <instruction>'
				 */
				vie_reset(vie);
				err = 0;
			}
		}
		break;
	case VEC_COMPLETE_INOUT:
		err = vie_fulfill_inout(vie, &entry->u.inout);
		if (err == 0) {
			err = vie_emulate_inout(vie, vm, vcpuid);
			if (err == 0) {
				vie_advance_pc(vie, &vcpu->nextrip);
			} else if (err < 0) {
				vie_exitinfo(vie, vme);
			} else if (err == EAGAIN) {
				/*
				 * Clear the instruction emulation state in
				 * order to re-enter VM context and continue
				 * this 'rep ins/outs'
				 */
				vie_reset(vie);
				err = 0;
			}
		}
		break;
	default:
		return (EINVAL);
	}
	return (err);
}

static int
vm_loop_checks(struct vm *vm, int vcpuid, struct vm_exit *vme)
{
	struct vie *vie;

	vie = vm->vcpu[vcpuid].vie_ctx;

	if (vie_pending(vie)) {
		/*
		 * Userspace has not fulfilled the pending needs of the
		 * instruction emulation, so bail back out.
		 */
		vie_exitinfo(vie, vme);
		return (-1);
	}

	if (vcpuid == 0 && vm->sipi_req) {
		/* The boot vCPU has sent a SIPI to one of the other CPUs */
		vme->exitcode = VM_EXITCODE_SPINUP_AP;
		vme->u.spinup_ap.vcpu = vm->sipi_req_vcpu;
		vme->u.spinup_ap.rip = vm->sipi_req_rip;

		vm->sipi_req = false;
		vm->sipi_req_vcpu = 0;
		vm->sipi_req_rip = 0;
		return (-1);
	}

	return (0);
}

int
vm_run(struct vm *vm, int vcpuid, const struct vm_entry *entry)
{
	struct vm_eventinfo evinfo;
	int error;
	struct vcpu *vcpu;
#ifdef	__FreeBSD__
	struct pcb *pcb;
#endif
	uint64_t tscval;
	struct vm_exit *vme;
	bool intr_disabled;
	pmap_t pmap;
#ifndef	__FreeBSD__
	vm_thread_ctx_t vtc;
	int affinity_type = CPU_CURRENT;
#endif

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (!CPU_ISSET(vcpuid, &vm->active_cpus))
		return (EINVAL);

	if (CPU_ISSET(vcpuid, &vm->suspended_cpus))
		return (EINVAL);

	pmap = vmspace_pmap(vm->vmspace);
	vcpu = &vm->vcpu[vcpuid];
	vme = &vcpu->exitinfo;
	evinfo.rptr = &vcpu->runblock;
	evinfo.sptr = &vm->suspend;
	evinfo.iptr = &vcpu->reqidle;

#ifndef	__FreeBSD__
	vtc.vtc_vm = vm;
	vtc.vtc_vcpuid = vcpuid;
	vtc.vtc_status = 0;

	installctx(curthread, &vtc, vmm_savectx, vmm_restorectx, NULL, NULL,
	    NULL, vmm_freectx);
#endif

	error = vm_entry_actions(vm, vcpuid, entry, vme);
	if (error != 0) {
		goto exit;
	}

restart:
	error = vm_loop_checks(vm, vcpuid, vme);
	if (error != 0) {
		goto exit;
	}

#ifndef	__FreeBSD__
	thread_affinity_set(curthread, affinity_type);
	/*
	 * Resource localization should happen after the CPU affinity for the
	 * thread has been set to ensure that access from restricted contexts,
	 * such as VMX-accelerated APIC operations, can occur without inducing
	 * cyclic cross-calls.
	 *
	 * This must be done prior to disabling kpreempt via critical_enter().
	 */
	vm_localize_resources(vm, vcpu);

	affinity_type = CPU_CURRENT;
#endif

	critical_enter();

	KASSERT(!CPU_ISSET(curcpu, &pmap->pm_active),
	    ("vm_run: absurd pm_active"));

	tscval = rdtsc();

#ifdef	__FreeBSD__
	pcb = PCPU_GET(curpcb);
	set_pcb_flags(pcb, PCB_FULL_IRET);
#else
	/* Force a trip through update_sregs to reload %fs/%gs and friends */
	PCB_SET_UPDATE_SEGS(&ttolwp(curthread)->lwp_pcb);
#endif

#ifdef	__FreeBSD__
	restore_guest_fpustate(vcpu);
#else
	if ((vtc.vtc_status & VTCS_FPU_RESTORED) == 0) {
		restore_guest_fpustate(vcpu);
		vtc.vtc_status |= VTCS_FPU_RESTORED;
	}
	vtc.vtc_status |= VTCS_FPU_CTX_CRITICAL;
#endif

	vcpu_require_state(vm, vcpuid, VCPU_RUNNING);
	error = VMRUN(vm->cookie, vcpuid, vcpu->nextrip, pmap, &evinfo);
	vcpu_require_state(vm, vcpuid, VCPU_FROZEN);

#ifdef	__FreeBSD__
	save_guest_fpustate(vcpu);
#else
	vtc.vtc_status &= ~VTCS_FPU_CTX_CRITICAL;
#endif

#ifndef	__FreeBSD__
	/*
	 * Once clear of the delicate contexts comprising the VM_RUN handler,
	 * thread CPU affinity can be loosened while other processing occurs.
	 */
	thread_affinity_clear(curthread);
#endif

	vmm_stat_incr(vm, vcpuid, VCPU_TOTAL_RUNTIME, rdtsc() - tscval);

	critical_exit();

	if (error != 0) {
		/* Communicate out any error from VMRUN() above */
		goto exit;
	}

	vcpu->nextrip = vme->rip + vme->inst_length;
	switch (vme->exitcode) {
	case VM_EXITCODE_REQIDLE:
		error = vm_handle_reqidle(vm, vcpuid);
		break;
	case VM_EXITCODE_SUSPENDED:
		error = vm_handle_suspend(vm, vcpuid);
		break;
	case VM_EXITCODE_IOAPIC_EOI:
		vioapic_process_eoi(vm, vcpuid,
		    vme->u.ioapic_eoi.vector);
		break;
	case VM_EXITCODE_RUNBLOCK:
		break;
	case VM_EXITCODE_HLT:
		intr_disabled = ((vme->u.hlt.rflags & PSL_I) == 0);
		error = vm_handle_hlt(vm, vcpuid, intr_disabled);
		break;
	case VM_EXITCODE_PAGING:
		error = vm_handle_paging(vm, vcpuid);
		break;
	case VM_EXITCODE_MMIO_EMUL:
		error = vm_handle_mmio_emul(vm, vcpuid);
		break;
	case VM_EXITCODE_INOUT:
		error = vm_handle_inout(vm, vcpuid, vme);
		break;
	case VM_EXITCODE_MONITOR:
	case VM_EXITCODE_MWAIT:
	case VM_EXITCODE_VMINSN:
		vm_inject_ud(vm, vcpuid);
		break;
#ifndef __FreeBSD__
	case VM_EXITCODE_WRMSR:
		if (vm_handle_wrmsr(vm, vcpuid, vme) != 0) {
			error = -1;
		}
		break;

	case VM_EXITCODE_HT: {
		affinity_type = CPU_BEST;
		break;
	}
#endif

	case VM_EXITCODE_MTRAP:
		vm_suspend_cpu(vm, vcpuid);
		error = -1;
		break;
	default:
		/* handled in userland */
		error = -1;
		break;
	}

	if (error == 0) {
		/* VM exit conditions handled in-kernel, continue running */
		goto restart;
	}

exit:
#ifndef	__FreeBSD__
	removectx(curthread, &vtc, vmm_savectx, vmm_restorectx, NULL, NULL,
	    NULL, vmm_freectx);
#endif

	VCPU_CTR2(vm, vcpuid, "retu %d/%d", error, vme->exitcode);

	return (error);
}

int
vm_restart_instruction(void *arg, int vcpuid)
{
	struct vm *vm;
	struct vcpu *vcpu;
	enum vcpu_state state;
	uint64_t rip;
	int error;

	vm = arg;
	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];
	state = vcpu_get_state(vm, vcpuid, NULL);
	if (state == VCPU_RUNNING) {
		/*
		 * When a vcpu is "running" the next instruction is determined
		 * by adding 'rip' and 'inst_length' in the vcpu's 'exitinfo'.
		 * Thus setting 'inst_length' to zero will cause the current
		 * instruction to be restarted.
		 */
		vcpu->exitinfo.inst_length = 0;
		VCPU_CTR1(vm, vcpuid, "restarting instruction at %lx by "
		    "setting inst_length to zero", vcpu->exitinfo.rip);
	} else if (state == VCPU_FROZEN) {
		/*
		 * When a vcpu is "frozen" it is outside the critical section
		 * around VMRUN() and 'nextrip' points to the next instruction.
		 * Thus instruction restart is achieved by setting 'nextrip'
		 * to the vcpu's %rip.
		 */
		error = vm_get_register(vm, vcpuid, VM_REG_GUEST_RIP, &rip);
		KASSERT(!error, ("%s: error %d getting rip", __func__, error));
		VCPU_CTR2(vm, vcpuid, "restarting instruction by updating "
		    "nextrip from %lx to %lx", vcpu->nextrip, rip);
		vcpu->nextrip = rip;
	} else {
		panic("%s: invalid state %d", __func__, state);
	}
	return (0);
}

int
vm_exit_intinfo(struct vm *vm, int vcpuid, uint64_t info)
{
	struct vcpu *vcpu;
	int type, vector;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];

	if (info & VM_INTINFO_VALID) {
		type = info & VM_INTINFO_TYPE;
		vector = info & 0xff;
		if (type == VM_INTINFO_NMI && vector != IDT_NMI)
			return (EINVAL);
		if (type == VM_INTINFO_HWEXCEPTION && vector >= 32)
			return (EINVAL);
		if (info & VM_INTINFO_RSVD)
			return (EINVAL);
	} else {
		info = 0;
	}
	VCPU_CTR2(vm, vcpuid, "%s: info1(%lx)", __func__, info);
	vcpu->exitintinfo = info;
	return (0);
}

enum exc_class {
	EXC_BENIGN,
	EXC_CONTRIBUTORY,
	EXC_PAGEFAULT
};

#define	IDT_VE	20	/* Virtualization Exception (Intel specific) */

static enum exc_class
exception_class(uint64_t info)
{
	int type, vector;

	KASSERT(info & VM_INTINFO_VALID, ("intinfo must be valid: %lx", info));
	type = info & VM_INTINFO_TYPE;
	vector = info & 0xff;

	/* Table 6-4, "Interrupt and Exception Classes", Intel SDM, Vol 3 */
	switch (type) {
	case VM_INTINFO_HWINTR:
	case VM_INTINFO_SWINTR:
	case VM_INTINFO_NMI:
		return (EXC_BENIGN);
	default:
		/*
		 * Hardware exception.
		 *
		 * SVM and VT-x use identical type values to represent NMI,
		 * hardware interrupt and software interrupt.
		 *
		 * SVM uses type '3' for all exceptions. VT-x uses type '3'
		 * for exceptions except #BP and #OF. #BP and #OF use a type
		 * value of '5' or '6'. Therefore we don't check for explicit
		 * values of 'type' to classify 'intinfo' into a hardware
		 * exception.
		 */
		break;
	}

	switch (vector) {
	case IDT_PF:
	case IDT_VE:
		return (EXC_PAGEFAULT);
	case IDT_DE:
	case IDT_TS:
	case IDT_NP:
	case IDT_SS:
	case IDT_GP:
		return (EXC_CONTRIBUTORY);
	default:
		return (EXC_BENIGN);
	}
}

static int
nested_fault(struct vm *vm, int vcpuid, uint64_t info1, uint64_t info2,
    uint64_t *retinfo)
{
	enum exc_class exc1, exc2;
	int type1, vector1;

	KASSERT(info1 & VM_INTINFO_VALID, ("info1 %lx is not valid", info1));
	KASSERT(info2 & VM_INTINFO_VALID, ("info2 %lx is not valid", info2));

	/*
	 * If an exception occurs while attempting to call the double-fault
	 * handler the processor enters shutdown mode (aka triple fault).
	 */
	type1 = info1 & VM_INTINFO_TYPE;
	vector1 = info1 & 0xff;
	if (type1 == VM_INTINFO_HWEXCEPTION && vector1 == IDT_DF) {
		VCPU_CTR2(vm, vcpuid, "triple fault: info1(%lx), info2(%lx)",
		    info1, info2);
		vm_suspend(vm, VM_SUSPEND_TRIPLEFAULT);
		*retinfo = 0;
		return (0);
	}

	/*
	 * Table 6-5 "Conditions for Generating a Double Fault", Intel SDM, Vol3
	 */
	exc1 = exception_class(info1);
	exc2 = exception_class(info2);
	if ((exc1 == EXC_CONTRIBUTORY && exc2 == EXC_CONTRIBUTORY) ||
	    (exc1 == EXC_PAGEFAULT && exc2 != EXC_BENIGN)) {
		/* Convert nested fault into a double fault. */
		*retinfo = IDT_DF;
		*retinfo |= VM_INTINFO_VALID | VM_INTINFO_HWEXCEPTION;
		*retinfo |= VM_INTINFO_DEL_ERRCODE;
	} else {
		/* Handle exceptions serially */
		*retinfo = info2;
	}
	return (1);
}

static uint64_t
vcpu_exception_intinfo(struct vcpu *vcpu)
{
	uint64_t info = 0;

	if (vcpu->exception_pending) {
		info = vcpu->exc_vector & 0xff;
		info |= VM_INTINFO_VALID | VM_INTINFO_HWEXCEPTION;
		if (vcpu->exc_errcode_valid) {
			info |= VM_INTINFO_DEL_ERRCODE;
			info |= (uint64_t)vcpu->exc_errcode << 32;
		}
	}
	return (info);
}

int
vm_entry_intinfo(struct vm *vm, int vcpuid, uint64_t *retinfo)
{
	struct vcpu *vcpu;
	uint64_t info1, info2;
	int valid;

	KASSERT(vcpuid >= 0 &&
	    vcpuid < vm->maxcpus, ("invalid vcpu %d", vcpuid));

	vcpu = &vm->vcpu[vcpuid];

	info1 = vcpu->exitintinfo;
	vcpu->exitintinfo = 0;

	info2 = 0;
	if (vcpu->exception_pending) {
		info2 = vcpu_exception_intinfo(vcpu);
		vcpu->exception_pending = 0;
		VCPU_CTR2(vm, vcpuid, "Exception %d delivered: %lx",
		    vcpu->exc_vector, info2);
	}

	if ((info1 & VM_INTINFO_VALID) && (info2 & VM_INTINFO_VALID)) {
		valid = nested_fault(vm, vcpuid, info1, info2, retinfo);
	} else if (info1 & VM_INTINFO_VALID) {
		*retinfo = info1;
		valid = 1;
	} else if (info2 & VM_INTINFO_VALID) {
		*retinfo = info2;
		valid = 1;
	} else {
		valid = 0;
	}

	if (valid) {
		VCPU_CTR4(vm, vcpuid, "%s: info1(%lx), info2(%lx), "
		    "retinfo(%lx)", __func__, info1, info2, *retinfo);
	}

	return (valid);
}

int
vm_get_intinfo(struct vm *vm, int vcpuid, uint64_t *info1, uint64_t *info2)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];
	*info1 = vcpu->exitintinfo;
	*info2 = vcpu_exception_intinfo(vcpu);
	return (0);
}

int
vm_inject_exception(struct vm *vm, int vcpuid, int vector, int errcode_valid,
    uint32_t errcode, int restart_instruction)
{
	struct vcpu *vcpu;
	uint64_t regval;
	int error;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (vector < 0 || vector >= 32)
		return (EINVAL);

	/*
	 * A double fault exception should never be injected directly into
	 * the guest. It is a derived exception that results from specific
	 * combinations of nested faults.
	 */
	if (vector == IDT_DF)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];

	if (vcpu->exception_pending) {
		VCPU_CTR2(vm, vcpuid, "Unable to inject exception %d due to "
		    "pending exception %d", vector, vcpu->exc_vector);
		return (EBUSY);
	}

	if (errcode_valid) {
		/*
		 * Exceptions don't deliver an error code in real mode.
		 */
		error = vm_get_register(vm, vcpuid, VM_REG_GUEST_CR0, &regval);
		KASSERT(!error, ("%s: error %d getting CR0", __func__, error));
		if (!(regval & CR0_PE))
			errcode_valid = 0;
	}

	/*
	 * From section 26.6.1 "Interruptibility State" in Intel SDM:
	 *
	 * Event blocking by "STI" or "MOV SS" is cleared after guest executes
	 * one instruction or incurs an exception.
	 */
	error = vm_set_register(vm, vcpuid, VM_REG_GUEST_INTR_SHADOW, 0);
	KASSERT(error == 0, ("%s: error %d clearing interrupt shadow",
	    __func__, error));

	if (restart_instruction)
		vm_restart_instruction(vm, vcpuid);

	vcpu->exception_pending = 1;
	vcpu->exc_vector = vector;
	vcpu->exc_errcode = errcode;
	vcpu->exc_errcode_valid = errcode_valid;
	VCPU_CTR1(vm, vcpuid, "Exception %d pending", vector);
	return (0);
}

void
vm_inject_fault(struct vm *vm, int vcpuid, int vector, int errcode_valid,
    int errcode)
{
	int error;

	error = vm_inject_exception(vm, vcpuid, vector, errcode_valid,
	    errcode, 1);
	KASSERT(error == 0, ("vm_inject_exception error %d", error));
}

void
vm_inject_ud(struct vm *vm, int vcpuid)
{
	vm_inject_fault(vm, vcpuid, IDT_UD, 0, 0);
}

void
vm_inject_gp(struct vm *vm, int vcpuid)
{
	vm_inject_fault(vm, vcpuid, IDT_GP, 1, 0);
}

void
vm_inject_ac(struct vm *vm, int vcpuid, int errcode)
{
	vm_inject_fault(vm, vcpuid, IDT_AC, 1, errcode);
}

void
vm_inject_ss(struct vm *vm, int vcpuid, int errcode)
{
	vm_inject_fault(vm, vcpuid, IDT_SS, 1, errcode);
}

void
vm_inject_pf(struct vm *vm, int vcpuid, int error_code, uint64_t cr2)
{
	int error;

	VCPU_CTR2(vm, vcpuid, "Injecting page fault: error_code %x, cr2 %lx",
	    error_code, cr2);

	error = vm_set_register(vm, vcpuid, VM_REG_GUEST_CR2, cr2);
	KASSERT(error == 0, ("vm_set_register(cr2) error %d", error));

	vm_inject_fault(vm, vcpuid, IDT_PF, 1, error_code);
}

static VMM_STAT(VCPU_NMI_COUNT, "number of NMIs delivered to vcpu");

int
vm_inject_nmi(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];

	vcpu->nmi_pending = 1;
	vcpu_notify_event(vm, vcpuid, false);
	return (0);
}

int
vm_nmi_pending(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_nmi_pending: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	return (vcpu->nmi_pending);
}

void
vm_nmi_clear(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_nmi_pending: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	if (vcpu->nmi_pending == 0)
		panic("vm_nmi_clear: inconsistent nmi_pending state");

	vcpu->nmi_pending = 0;
	vmm_stat_incr(vm, vcpuid, VCPU_NMI_COUNT, 1);
}

static VMM_STAT(VCPU_EXTINT_COUNT, "number of ExtINTs delivered to vcpu");

int
vm_inject_extint(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	vcpu = &vm->vcpu[vcpuid];

	vcpu->extint_pending = 1;
	vcpu_notify_event(vm, vcpuid, false);
	return (0);
}

int
vm_extint_pending(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_extint_pending: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	return (vcpu->extint_pending);
}

void
vm_extint_clear(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_extint_pending: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	if (vcpu->extint_pending == 0)
		panic("vm_extint_clear: inconsistent extint_pending state");

	vcpu->extint_pending = 0;
	vmm_stat_incr(vm, vcpuid, VCPU_EXTINT_COUNT, 1);
}

int
vm_get_capability(struct vm *vm, int vcpu, int type, int *retval)
{
	if (vcpu < 0 || vcpu >= vm->maxcpus)
		return (EINVAL);

	if (type < 0 || type >= VM_CAP_MAX)
		return (EINVAL);

	return (VMGETCAP(vm->cookie, vcpu, type, retval));
}

int
vm_set_capability(struct vm *vm, int vcpu, int type, int val)
{
	if (vcpu < 0 || vcpu >= vm->maxcpus)
		return (EINVAL);

	if (type < 0 || type >= VM_CAP_MAX)
		return (EINVAL);

	return (VMSETCAP(vm->cookie, vcpu, type, val));
}

struct vlapic *
vm_lapic(struct vm *vm, int cpu)
{
	return (vm->vcpu[cpu].vlapic);
}

struct vioapic *
vm_ioapic(struct vm *vm)
{

	return (vm->vioapic);
}

struct vhpet *
vm_hpet(struct vm *vm)
{

	return (vm->vhpet);
}

#ifdef	__FreeBSD__
bool
vmm_is_pptdev(int bus, int slot, int func)
{
	int b, f, i, n, s;
	char *val, *cp, *cp2;
	bool found;

	/*
	 * XXX
	 * The length of an environment variable is limited to 128 bytes which
	 * puts an upper limit on the number of passthru devices that may be
	 * specified using a single environment variable.
	 *
	 * Work around this by scanning multiple environment variable
	 * names instead of a single one - yuck!
	 */
	const char *names[] = { "pptdevs", "pptdevs2", "pptdevs3", NULL };

	/* set pptdevs="1/2/3 4/5/6 7/8/9 10/11/12" */
	found = false;
	for (i = 0; names[i] != NULL && !found; i++) {
		cp = val = kern_getenv(names[i]);
		while (cp != NULL && *cp != '\0') {
			if ((cp2 = strchr(cp, ' ')) != NULL)
				*cp2 = '\0';

			n = sscanf(cp, "%d/%d/%d", &b, &s, &f);
			if (n == 3 && bus == b && slot == s && func == f) {
				found = true;
				break;
			}

			if (cp2 != NULL)
				*cp2++ = ' ';

			cp = cp2;
		}
		freeenv(val);
	}
	return (found);
}
#endif

void *
vm_iommu_domain(struct vm *vm)
{

	return (vm->iommu);
}

int
vcpu_set_state(struct vm *vm, int vcpuid, enum vcpu_state newstate,
    bool from_idle)
{
	int error;
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_set_run_state: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	error = vcpu_set_state_locked(vm, vcpuid, newstate, from_idle);
	vcpu_unlock(vcpu);

	return (error);
}

enum vcpu_state
vcpu_get_state(struct vm *vm, int vcpuid, int *hostcpu)
{
	struct vcpu *vcpu;
	enum vcpu_state state;

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		panic("vm_get_run_state: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	state = vcpu->state;
	if (hostcpu != NULL)
		*hostcpu = vcpu->hostcpu;
	vcpu_unlock(vcpu);

	return (state);
}

void
vcpu_block_run(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= VM_MAXCPU)
		panic("vcpu_block_run: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	vcpu->runblock++;
	if (vcpu->runblock == 1 && vcpu->state == VCPU_RUNNING) {
		vcpu_notify_event_locked(vcpu, false);
	}
	while (vcpu->state == VCPU_RUNNING) {
#ifdef __FreeBSD__
		msleep_spin(&vcpu->state, &vcpu->mtx, "vcpublk", 0);
#else
		cv_wait(&vcpu->state_cv, &vcpu->mtx.m);
#endif
	}
	vcpu_unlock(vcpu);
}

void
vcpu_unblock_run(struct vm *vm, int vcpuid)
{
	struct vcpu *vcpu;

	if (vcpuid < 0 || vcpuid >= VM_MAXCPU)
		panic("vcpu_block_run: invalid vcpuid %d", vcpuid);

	vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	KASSERT(vcpu->runblock != 0, ("expected non-zero runblock"));
	vcpu->runblock--;
	if (vcpu->runblock == 0) {
#ifdef __FreeBSD__
		wakeup(&vcpu->state);
#else
		cv_broadcast(&vcpu->state_cv);
#endif
	}
	vcpu_unlock(vcpu);
}

#ifndef	__FreeBSD__
uint64_t
vcpu_tsc_offset(struct vm *vm, int vcpuid)
{
	return (vm->vcpu[vcpuid].tsc_offset);
}
#endif /* __FreeBSD__ */

int
vm_activate_cpu(struct vm *vm, int vcpuid)
{

	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (CPU_ISSET(vcpuid, &vm->active_cpus))
		return (EBUSY);

	VCPU_CTR0(vm, vcpuid, "activated");
	CPU_SET_ATOMIC(vcpuid, &vm->active_cpus);
	return (0);
}

int
vm_suspend_cpu(struct vm *vm, int vcpuid)
{
	int i;

	if (vcpuid < -1 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (vcpuid == -1) {
		vm->debug_cpus = vm->active_cpus;
		for (i = 0; i < vm->maxcpus; i++) {
			if (CPU_ISSET(i, &vm->active_cpus))
				vcpu_notify_event(vm, i, false);
		}
	} else {
		if (!CPU_ISSET(vcpuid, &vm->active_cpus))
			return (EINVAL);

		CPU_SET_ATOMIC(vcpuid, &vm->debug_cpus);
		vcpu_notify_event(vm, vcpuid, false);
	}
	return (0);
}

int
vm_resume_cpu(struct vm *vm, int vcpuid)
{

	if (vcpuid < -1 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (vcpuid == -1) {
		CPU_ZERO(&vm->debug_cpus);
	} else {
		if (!CPU_ISSET(vcpuid, &vm->debug_cpus))
			return (EINVAL);

		CPU_CLR_ATOMIC(vcpuid, &vm->debug_cpus);
	}
	return (0);
}

int
vcpu_debugged(struct vm *vm, int vcpuid)
{

	return (CPU_ISSET(vcpuid, &vm->debug_cpus));
}

cpuset_t
vm_active_cpus(struct vm *vm)
{

	return (vm->active_cpus);
}

cpuset_t
vm_debug_cpus(struct vm *vm)
{

	return (vm->debug_cpus);
}

cpuset_t
vm_suspended_cpus(struct vm *vm)
{

	return (vm->suspended_cpus);
}

void *
vcpu_stats(struct vm *vm, int vcpuid)
{

	return (vm->vcpu[vcpuid].stats);
}

int
vm_get_x2apic_state(struct vm *vm, int vcpuid, enum x2apic_state *state)
{
	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	*state = vm->vcpu[vcpuid].x2apic_state;

	return (0);
}

int
vm_set_x2apic_state(struct vm *vm, int vcpuid, enum x2apic_state state)
{
	if (vcpuid < 0 || vcpuid >= vm->maxcpus)
		return (EINVAL);

	if (state >= X2APIC_STATE_LAST)
		return (EINVAL);

	vm->vcpu[vcpuid].x2apic_state = state;

	vlapic_set_x2apic_state(vm, vcpuid, state);

	return (0);
}

/*
 * This function is called to ensure that a vcpu "sees" a pending event
 * as soon as possible:
 * - If the vcpu thread is sleeping then it is woken up.
 * - If the vcpu is running on a different host_cpu then an IPI will be directed
 *   to the host_cpu to cause the vcpu to trap into the hypervisor.
 */
static void
vcpu_notify_event_locked(struct vcpu *vcpu, bool lapic_intr)
{
	int hostcpu;

	hostcpu = vcpu->hostcpu;
	if (vcpu->state == VCPU_RUNNING) {
		KASSERT(hostcpu != NOCPU, ("vcpu running on invalid hostcpu"));
		if (hostcpu != curcpu) {
			if (lapic_intr) {
				vlapic_post_intr(vcpu->vlapic, hostcpu,
				    vmm_ipinum);
			} else {
				ipi_cpu(hostcpu, vmm_ipinum);
			}
		} else {
			/*
			 * If the 'vcpu' is running on 'curcpu' then it must
			 * be sending a notification to itself (e.g. SELF_IPI).
			 * The pending event will be picked up when the vcpu
			 * transitions back to guest context.
			 */
		}
	} else {
		KASSERT(hostcpu == NOCPU, ("vcpu state %d not consistent "
		    "with hostcpu %d", vcpu->state, hostcpu));
		if (vcpu->state == VCPU_SLEEPING) {
#ifdef __FreeBSD__
			wakeup_one(vcpu);
#else
			cv_signal(&vcpu->vcpu_cv);
#endif
		}
	}
}

void
vcpu_notify_event(struct vm *vm, int vcpuid, bool lapic_intr)
{
	struct vcpu *vcpu = &vm->vcpu[vcpuid];

	vcpu_lock(vcpu);
	vcpu_notify_event_locked(vcpu, lapic_intr);
	vcpu_unlock(vcpu);
}

struct vmspace *
vm_get_vmspace(struct vm *vm)
{

	return (vm->vmspace);
}

int
vm_apicid2vcpuid(struct vm *vm, int apicid)
{
	/*
	 * XXX apic id is assumed to be numerically identical to vcpu id
	 */
	return (apicid);
}

struct vatpic *
vm_atpic(struct vm *vm)
{
	return (vm->vatpic);
}

struct vatpit *
vm_atpit(struct vm *vm)
{
	return (vm->vatpit);
}

struct vpmtmr *
vm_pmtmr(struct vm *vm)
{

	return (vm->vpmtmr);
}

struct vrtc *
vm_rtc(struct vm *vm)
{

	return (vm->vrtc);
}

enum vm_reg_name
vm_segment_name(int seg)
{
	static enum vm_reg_name seg_names[] = {
		VM_REG_GUEST_ES,
		VM_REG_GUEST_CS,
		VM_REG_GUEST_SS,
		VM_REG_GUEST_DS,
		VM_REG_GUEST_FS,
		VM_REG_GUEST_GS
	};

	KASSERT(seg >= 0 && seg < nitems(seg_names),
	    ("%s: invalid segment encoding %d", __func__, seg));
	return (seg_names[seg]);
}

void
vm_copy_teardown(struct vm *vm, int vcpuid, struct vm_copyinfo *copyinfo,
    int num_copyinfo)
{
	int idx;

	for (idx = 0; idx < num_copyinfo; idx++) {
		if (copyinfo[idx].cookie != NULL)
			vm_gpa_release(copyinfo[idx].cookie);
	}
	bzero(copyinfo, num_copyinfo * sizeof(struct vm_copyinfo));
}

int
vm_copy_setup(struct vm *vm, int vcpuid, struct vm_guest_paging *paging,
    uint64_t gla, size_t len, int prot, struct vm_copyinfo *copyinfo,
    int num_copyinfo, int *fault)
{
	int error, idx, nused;
	size_t n, off, remaining;
	void *hva, *cookie;
	uint64_t gpa;

	bzero(copyinfo, sizeof(struct vm_copyinfo) * num_copyinfo);

	nused = 0;
	remaining = len;
	while (remaining > 0) {
		KASSERT(nused < num_copyinfo, ("insufficient vm_copyinfo"));
		error = vm_gla2gpa(vm, vcpuid, paging, gla, prot, &gpa, fault);
		if (error || *fault)
			return (error);
		off = gpa & PAGE_MASK;
		n = min(remaining, PAGE_SIZE - off);
		copyinfo[nused].gpa = gpa;
		copyinfo[nused].len = n;
		remaining -= n;
		gla += n;
		nused++;
	}

	for (idx = 0; idx < nused; idx++) {
		hva = vm_gpa_hold(vm, vcpuid, copyinfo[idx].gpa,
		    copyinfo[idx].len, prot, &cookie);
		if (hva == NULL)
			break;
		copyinfo[idx].hva = hva;
		copyinfo[idx].cookie = cookie;
	}

	if (idx != nused) {
		vm_copy_teardown(vm, vcpuid, copyinfo, num_copyinfo);
		return (EFAULT);
	} else {
		*fault = 0;
		return (0);
	}
}

void
vm_copyin(struct vm *vm, int vcpuid, struct vm_copyinfo *copyinfo, void *kaddr,
    size_t len)
{
	char *dst;
	int idx;

	dst = kaddr;
	idx = 0;
	while (len > 0) {
		bcopy(copyinfo[idx].hva, dst, copyinfo[idx].len);
		len -= copyinfo[idx].len;
		dst += copyinfo[idx].len;
		idx++;
	}
}

void
vm_copyout(struct vm *vm, int vcpuid, const void *kaddr,
    struct vm_copyinfo *copyinfo, size_t len)
{
	const char *src;
	int idx;

	src = kaddr;
	idx = 0;
	while (len > 0) {
		bcopy(src, copyinfo[idx].hva, copyinfo[idx].len);
		len -= copyinfo[idx].len;
		src += copyinfo[idx].len;
		idx++;
	}
}

/*
 * Return the amount of in-use and wired memory for the VM. Since
 * these are global stats, only return the values with for vCPU 0
 */
VMM_STAT_DECLARE(VMM_MEM_RESIDENT);
VMM_STAT_DECLARE(VMM_MEM_WIRED);

static void
vm_get_rescnt(struct vm *vm, int vcpu, struct vmm_stat_type *stat)
{

	if (vcpu == 0) {
		vmm_stat_set(vm, vcpu, VMM_MEM_RESIDENT,
		    PAGE_SIZE * vmspace_resident_count(vm->vmspace));
	}
}

static void
vm_get_wiredcnt(struct vm *vm, int vcpu, struct vmm_stat_type *stat)
{

	if (vcpu == 0) {
		vmm_stat_set(vm, vcpu, VMM_MEM_WIRED,
		    PAGE_SIZE * pmap_wired_count(vmspace_pmap(vm->vmspace)));
	}
}

VMM_STAT_FUNC(VMM_MEM_RESIDENT, "Resident memory", vm_get_rescnt);
VMM_STAT_FUNC(VMM_MEM_WIRED, "Wired memory", vm_get_wiredcnt);

#ifndef __FreeBSD__
int
vm_ioport_hook(struct vm *vm, uint_t ioport, vmm_rmem_cb_t rfunc,
    vmm_wmem_cb_t wfunc, void *arg, void **cookie)
{
	list_t *ih = &vm->ioport_hooks;
	vm_ioport_hook_t *hook, *node;

	if (ioport == 0) {
		return (EINVAL);
	}

	/*
	 * Find the node position in the list which this region should be
	 * inserted behind to maintain sorted order.
	 */
	for (node = list_tail(ih); node != NULL; node = list_prev(ih, node)) {
		if (ioport == node->vmih_ioport) {
			/* Reject duplicate port hook  */
			return (EEXIST);
		} else if (ioport > node->vmih_ioport) {
			break;
		}
	}

	hook = kmem_alloc(sizeof (*hook), KM_SLEEP);
	hook->vmih_ioport = ioport;
	hook->vmih_arg = arg;
	hook->vmih_rmem_cb = rfunc;
	hook->vmih_wmem_cb = wfunc;
	if (node == NULL) {
		list_insert_head(ih, hook);
	} else {
		list_insert_after(ih, node, hook);
	}

	*cookie = (void *)hook;
	return (0);
}

void
vm_ioport_unhook(struct vm *vm, void **cookie)
{
	vm_ioport_hook_t *hook;
	list_t *ih = &vm->ioport_hooks;

	hook = *cookie;
	list_remove(ih, hook);
	kmem_free(hook, sizeof (*hook));
	*cookie = NULL;
}

int
vm_ioport_handle_hook(struct vm *vm, int cpuid, bool in, int port, int bytes,
    uint32_t *val)
{
	vm_ioport_hook_t *hook;
	list_t *ih = &vm->ioport_hooks;
	int err = 0;

	for (hook = list_head(ih); hook != NULL; hook = list_next(ih, hook)) {
		if (hook->vmih_ioport == port) {
			break;
		}
	}
	if (hook == NULL) {
		return (ESRCH);
	}

	if (in) {
		uint64_t tval;

		if (hook->vmih_rmem_cb == NULL) {
			return (ESRCH);
		}
		err = hook->vmih_rmem_cb(hook->vmih_arg, (uintptr_t)port,
		    (uint_t)bytes, &tval);
		*val = (uint32_t)tval;
	} else {
		if (hook->vmih_wmem_cb == NULL) {
			return (ESRCH);
		}
		err = hook->vmih_wmem_cb(hook->vmih_arg, (uintptr_t)port,
		    (uint_t)bytes, (uint64_t)*val);
	}

	return (err);
}


#endif /* __FreeBSD__ */
