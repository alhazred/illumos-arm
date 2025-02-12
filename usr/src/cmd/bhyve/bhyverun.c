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

#include <sys/types.h>
#ifndef WITHOUT_CAPSICUM
#include <sys/capsicum.h>
#endif
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/cpuset.h>

#ifdef __FreeBSD__
#include <amd64/vmm/intel/vmcs.h>
#else
#include <intel/vmcs.h>
#endif

#include <machine/atomic.h>
#include <machine/segments.h>

#ifndef WITHOUT_CAPSICUM
#include <capsicum_helpers.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <pthread_np.h>
#include <sysexits.h>
#include <stdbool.h>
#include <stdint.h>

#include <machine/vmm.h>
#ifndef WITHOUT_CAPSICUM
#include <machine/vmm_dev.h>
#endif
#include <vmmapi.h>

#include "bhyverun.h"
#include "acpi.h"
#include "atkbdc.h"
#include "console.h"
#include "bootrom.h"
#include "inout.h"
#include "dbgport.h"
#include "debug.h"
#include "fwctl.h"
#include "gdb.h"
#include "ioapic.h"
#include "kernemu_dev.h"
#include "mem.h"
#include "mevent.h"
#include "mptbl.h"
#include "pci_emul.h"
#include "pci_irq.h"
#include "pci_lpc.h"
#include "smbiostbl.h"
#include "xmsr.h"
#include "spinup_ap.h"
#include "rfb.h"
#include "rtc.h"
#include "vga.h"
#include "vmgenc.h"

#define GUEST_NIO_PORT		0x488	/* guest upcalls via i/o port */

#define MB		(1024UL * 1024)
#define GB		(1024UL * MB)

static const char * const vmx_exit_reason_desc[] = {
	[EXIT_REASON_EXCEPTION] = "Exception or non-maskable interrupt (NMI)",
	[EXIT_REASON_EXT_INTR] = "External interrupt",
	[EXIT_REASON_TRIPLE_FAULT] = "Triple fault",
	[EXIT_REASON_INIT] = "INIT signal",
	[EXIT_REASON_SIPI] = "Start-up IPI (SIPI)",
	[EXIT_REASON_IO_SMI] = "I/O system-management interrupt (SMI)",
	[EXIT_REASON_SMI] = "Other SMI",
	[EXIT_REASON_INTR_WINDOW] = "Interrupt window",
	[EXIT_REASON_NMI_WINDOW] = "NMI window",
	[EXIT_REASON_TASK_SWITCH] = "Task switch",
	[EXIT_REASON_CPUID] = "CPUID",
	[EXIT_REASON_GETSEC] = "GETSEC",
	[EXIT_REASON_HLT] = "HLT",
	[EXIT_REASON_INVD] = "INVD",
	[EXIT_REASON_INVLPG] = "INVLPG",
	[EXIT_REASON_RDPMC] = "RDPMC",
	[EXIT_REASON_RDTSC] = "RDTSC",
	[EXIT_REASON_RSM] = "RSM",
	[EXIT_REASON_VMCALL] = "VMCALL",
	[EXIT_REASON_VMCLEAR] = "VMCLEAR",
	[EXIT_REASON_VMLAUNCH] = "VMLAUNCH",
	[EXIT_REASON_VMPTRLD] = "VMPTRLD",
	[EXIT_REASON_VMPTRST] = "VMPTRST",
	[EXIT_REASON_VMREAD] = "VMREAD",
	[EXIT_REASON_VMRESUME] = "VMRESUME",
	[EXIT_REASON_VMWRITE] = "VMWRITE",
	[EXIT_REASON_VMXOFF] = "VMXOFF",
	[EXIT_REASON_VMXON] = "VMXON",
	[EXIT_REASON_CR_ACCESS] = "Control-register accesses",
	[EXIT_REASON_DR_ACCESS] = "MOV DR",
	[EXIT_REASON_INOUT] = "I/O instruction",
	[EXIT_REASON_RDMSR] = "RDMSR",
	[EXIT_REASON_WRMSR] = "WRMSR",
	[EXIT_REASON_INVAL_VMCS] =
	    "VM-entry failure due to invalid guest state",
	[EXIT_REASON_INVAL_MSR] = "VM-entry failure due to MSR loading",
	[EXIT_REASON_MWAIT] = "MWAIT",
	[EXIT_REASON_MTF] = "Monitor trap flag",
	[EXIT_REASON_MONITOR] = "MONITOR",
	[EXIT_REASON_PAUSE] = "PAUSE",
	[EXIT_REASON_MCE_DURING_ENTRY] =
	    "VM-entry failure due to machine-check event",
	[EXIT_REASON_TPR] = "TPR below threshold",
	[EXIT_REASON_APIC_ACCESS] = "APIC access",
	[EXIT_REASON_VIRTUALIZED_EOI] = "Virtualized EOI",
	[EXIT_REASON_GDTR_IDTR] = "Access to GDTR or IDTR",
	[EXIT_REASON_LDTR_TR] = "Access to LDTR or TR",
	[EXIT_REASON_EPT_FAULT] = "EPT violation",
	[EXIT_REASON_EPT_MISCONFIG] = "EPT misconfiguration",
	[EXIT_REASON_INVEPT] = "INVEPT",
	[EXIT_REASON_RDTSCP] = "RDTSCP",
	[EXIT_REASON_VMX_PREEMPT] = "VMX-preemption timer expired",
	[EXIT_REASON_INVVPID] = "INVVPID",
	[EXIT_REASON_WBINVD] = "WBINVD",
	[EXIT_REASON_XSETBV] = "XSETBV",
	[EXIT_REASON_APIC_WRITE] = "APIC write",
	[EXIT_REASON_RDRAND] = "RDRAND",
	[EXIT_REASON_INVPCID] = "INVPCID",
	[EXIT_REASON_VMFUNC] = "VMFUNC",
	[EXIT_REASON_ENCLS] = "ENCLS",
	[EXIT_REASON_RDSEED] = "RDSEED",
	[EXIT_REASON_PM_LOG_FULL] = "Page-modification log full",
	[EXIT_REASON_XSAVES] = "XSAVES",
	[EXIT_REASON_XRSTORS] = "XRSTORS"
};

typedef int (*vmexit_handler_t)(struct vmctx *, struct vm_exit *, int *vcpu);
extern int vmexit_task_switch(struct vmctx *, struct vm_exit *, int *vcpu);

char *vmname;

int guest_ncpus;
uint16_t cores, maxcpus, sockets, threads;

char *guest_uuid_str;

int raw_stdio = 0;

static int gdb_port = 0;
static int guest_vmexit_on_hlt, guest_vmexit_on_pause;
static int virtio_msix = 1;
static int x2apic_mode = 0;	/* default is xAPIC */

static int strictio;
static int strictmsr = 1;

static int acpi;

static char *progname;
static const int BSP = 0;

static cpuset_t cpumask;

static void vm_loop(struct vmctx *ctx, int vcpu, uint64_t rip);

static struct vm_exit vmexit[VM_MAXCPU];
static struct vm_entry vmentry[VM_MAXCPU];

struct bhyvestats {
	uint64_t	vmexit_bogus;
	uint64_t	vmexit_reqidle;
	uint64_t	vmexit_hlt;
	uint64_t	vmexit_pause;
	uint64_t	vmexit_mtrap;
	uint64_t	vmexit_mmio;
	uint64_t	vmexit_inout;
	uint64_t	cpu_switch_rotate;
	uint64_t	cpu_switch_direct;
	uint64_t	mmio_unhandled;
} stats;

struct mt_vmm_info {
	pthread_t	mt_thr;
	struct vmctx	*mt_ctx;
	int		mt_vcpu;
	uint64_t	mt_startrip;
} mt_vmm_info[VM_MAXCPU];

#ifdef	__FreeBSD__
static cpuset_t *vcpumap[VM_MAXCPU] = { NULL };
#endif

static void
usage(int code)
{

        fprintf(stderr,
		"Usage: %s [-abehuwxACHPSWY]\n"
		"       %*s [-c [[cpus=]numcpus][,sockets=n][,cores=n][,threads=n]]\n"
		"       %*s [-g <gdb port>] [-l <lpc>]\n"
#ifdef	__FreeBSD__
		"       %*s [-m mem] [-p vcpu:hostcpu] [-s <pci>] [-U uuid] <vm>\n"
#else
		"       %*s [-m mem] [-s <pci>] [-U uuid] <vm>\n"
#endif
		"       -a: local apic is in xAPIC mode (deprecated)\n"
		"       -A: create ACPI tables\n"
		"       -c: number of cpus and/or topology specification\n"
		"       -C: include guest memory in core file\n"
#ifndef __FreeBSD__
	        "       -d: suspend cpu at boot\n"
#endif
		"       -e: exit on unhandled I/O access\n"
		"       -g: gdb port\n"
		"       -h: help\n"
		"       -H: vmexit from the guest on hlt\n"
		"       -l: LPC device configuration\n"
		"       -m: memory size\n"
#ifdef	__FreeBSD__
		"       -p: pin 'vcpu' to 'hostcpu'\n"
#endif
		"       -P: vmexit from the guest on pause\n"
		"       -s: <slot,driver,configinfo> PCI slot config\n"
		"       -S: guest memory cannot be swapped\n"
		"       -u: RTC keeps UTC time\n"
		"       -U: uuid\n"
		"       -w: ignore unimplemented MSRs\n"
		"       -W: force virtio to use single-vector MSI\n"
		"       -x: local apic is in x2APIC mode\n"
		"       -Y: disable MPtable generation\n",
		progname, (int)strlen(progname), "", (int)strlen(progname), "",
		(int)strlen(progname), "");

	exit(code);
}

/*
 * XXX This parser is known to have the following issues:
 * 1.  It accepts null key=value tokens ",,".
 * 2.  It accepts whitespace after = and before value.
 * 3.  Values out of range of INT are silently wrapped.
 * 4.  It doesn't check non-final values.
 * 5.  The apparently bogus limits of UINT16_MAX are for future expansion.
 *
 * The acceptance of a null specification ('-c ""') is by design to match the
 * manual page syntax specification, this results in a topology of 1 vCPU.
 */
static int
topology_parse(const char *opt)
{
	uint64_t ncpus;
	int c, chk, n, s, t, tmp;
	char *cp, *str;
	bool ns, scts;

	c = 1, n = 1, s = 1, t = 1;
	ns = false, scts = false;
	str = strdup(opt);
	if (str == NULL)
		goto out;

	while ((cp = strsep(&str, ",")) != NULL) {
		if (sscanf(cp, "%i%n", &tmp, &chk) == 1) {
			n = tmp;
			ns = true;
		} else if (sscanf(cp, "cpus=%i%n", &tmp, &chk) == 1) {
			n = tmp;
			ns = true;
		} else if (sscanf(cp, "sockets=%i%n", &tmp, &chk) == 1) {
			s = tmp;
			scts = true;
		} else if (sscanf(cp, "cores=%i%n", &tmp, &chk) == 1) {
			c = tmp;
			scts = true;
		} else if (sscanf(cp, "threads=%i%n", &tmp, &chk) == 1) {
			t = tmp;
			scts = true;
#ifdef notyet  /* Do not expose this until vmm.ko implements it */
		} else if (sscanf(cp, "maxcpus=%i%n", &tmp, &chk) == 1) {
			m = tmp;
#endif
		/* Skip the empty argument case from -c "" */
		} else if (cp[0] == '\0')
			continue;
		else
			goto out;
		/* Any trailing garbage causes an error */
		if (cp[chk] != '\0')
			goto out;
	}
	free(str);
	str = NULL;

	/*
	 * Range check 1 <= n <= UINT16_MAX all values
	 */
	if (n < 1 || s < 1 || c < 1 || t < 1 ||
	    n > UINT16_MAX || s > UINT16_MAX || c > UINT16_MAX  ||
	    t > UINT16_MAX)
		return (-1);

	/* If only the cpus was specified, use that as sockets */
	if (!scts)
		s = n;
	/*
	 * Compute sockets * cores * threads avoiding overflow
	 * The range check above insures these are 16 bit values
	 * If n was specified check it against computed ncpus
	 */
	ncpus = (uint64_t)s * c * t;
	if (ncpus > UINT16_MAX || (ns && n != ncpus))
		return (-1);

	guest_ncpus = ncpus;
	sockets = s;
	cores = c;
	threads = t;
	return(0);

out:
	free(str);
	return (-1);
}

#ifndef WITHOUT_CAPSICUM
/*
 * 11-stable capsicum helpers
 */
static void
bhyve_caph_cache_catpages(void)
{

	(void)catopen("libc", NL_CAT_LOCALE);
}

static int
bhyve_caph_limit_stdoe(void)
{
	cap_rights_t rights;
	unsigned long cmds[] = { TIOCGETA, TIOCGWINSZ };
	int i, fds[] = { STDOUT_FILENO, STDERR_FILENO };

	cap_rights_init(&rights, CAP_FCNTL, CAP_FSTAT, CAP_IOCTL);
	cap_rights_set(&rights, CAP_WRITE);

	for (i = 0; i < nitems(fds); i++) {
		if (cap_rights_limit(fds[i], &rights) < 0 && errno != ENOSYS)
			return (-1);

		if (cap_ioctls_limit(fds[i], cmds, nitems(cmds)) < 0 && errno != ENOSYS)
			return (-1);

		if (cap_fcntls_limit(fds[i], CAP_FCNTL_GETFL) < 0 && errno != ENOSYS)
			return (-1);
	}

	return (0);
}

#endif

#ifdef	__FreeBSD__
static int
pincpu_parse(const char *opt)
{
	int vcpu, pcpu;

	if (sscanf(opt, "%d:%d", &vcpu, &pcpu) != 2) {
		fprintf(stderr, "invalid format: %s\n", opt);
		return (-1);
	}

	if (vcpu < 0 || vcpu >= VM_MAXCPU) {
		fprintf(stderr, "vcpu '%d' outside valid range from 0 to %d\n",
		    vcpu, VM_MAXCPU - 1);
		return (-1);
	}

	if (pcpu < 0 || pcpu >= CPU_SETSIZE) {
		fprintf(stderr, "hostcpu '%d' outside valid range from "
		    "0 to %d\n", pcpu, CPU_SETSIZE - 1);
		return (-1);
	}

	if (vcpumap[vcpu] == NULL) {
		if ((vcpumap[vcpu] = malloc(sizeof(cpuset_t))) == NULL) {
			perror("malloc");
			return (-1);
		}
		CPU_ZERO(vcpumap[vcpu]);
	}
	CPU_SET(pcpu, vcpumap[vcpu]);
	return (0);
}

void
vm_inject_fault(void *arg, int vcpu, int vector, int errcode_valid,
    int errcode)
{
	struct vmctx *ctx;
	int error, restart_instruction;

	ctx = arg;
	restart_instruction = 1;

	error = vm_inject_exception(ctx, vcpu, vector, errcode_valid, errcode,
	    restart_instruction);
	assert(error == 0);
}
#endif /* __FreeBSD__ */

void *
paddr_guest2host(struct vmctx *ctx, uintptr_t gaddr, size_t len)
{

	return (vm_map_gpa(ctx, gaddr, len));
}

int
fbsdrun_vmexit_on_pause(void)
{

	return (guest_vmexit_on_pause);
}

int
fbsdrun_vmexit_on_hlt(void)
{

	return (guest_vmexit_on_hlt);
}

int
fbsdrun_virtio_msix(void)
{

	return (virtio_msix);
}

static void *
fbsdrun_start_thread(void *param)
{
	char tname[MAXCOMLEN + 1];
	struct mt_vmm_info *mtp;
	int vcpu;

	mtp = param;
	vcpu = mtp->mt_vcpu;

	snprintf(tname, sizeof(tname), "vcpu %d", vcpu);
	pthread_set_name_np(mtp->mt_thr, tname);

	if (gdb_port != 0)
		gdb_cpu_add(vcpu);

	vm_loop(mtp->mt_ctx, vcpu, mtp->mt_startrip);

	/* not reached */
	exit(1);
	return (NULL);
}

#ifdef __FreeBSD__
void
fbsdrun_addcpu(struct vmctx *ctx, int fromcpu, int newcpu, uint64_t rip)
#else
void
fbsdrun_addcpu(struct vmctx *ctx, int fromcpu, int newcpu, uint64_t rip,
    bool suspend)
#endif
{
	int error;

	assert(fromcpu == BSP);

	/*
	 * The 'newcpu' must be activated in the context of 'fromcpu'. If
	 * vm_activate_cpu() is delayed until newcpu's pthread starts running
	 * then vmm.ko is out-of-sync with bhyve and this can create a race
	 * with vm_suspend().
	 */
	error = vm_activate_cpu(ctx, newcpu);
	if (error != 0)
		err(EX_OSERR, "could not activate CPU %d", newcpu);

	CPU_SET_ATOMIC(newcpu, &cpumask);

#ifndef __FreeBSD__
	if (suspend)
		(void) vm_suspend_cpu(ctx, newcpu);
#endif

	/*
	 * Set up the vmexit struct to allow execution to start
	 * at the given RIP
	 */
	mt_vmm_info[newcpu].mt_ctx = ctx;
	mt_vmm_info[newcpu].mt_vcpu = newcpu;
	mt_vmm_info[newcpu].mt_startrip = rip;

	error = pthread_create(&mt_vmm_info[newcpu].mt_thr, NULL,
	    fbsdrun_start_thread, &mt_vmm_info[newcpu]);
	assert(error == 0);
}

static int
fbsdrun_deletecpu(struct vmctx *ctx, int vcpu)
{

	if (!CPU_ISSET(vcpu, &cpumask)) {
		fprintf(stderr, "Attempting to delete unknown cpu %d\n", vcpu);
		exit(4);
	}

	CPU_CLR_ATOMIC(vcpu, &cpumask);
	return (CPU_EMPTY(&cpumask));
}

static void
vmentry_mmio_read(int vcpu, uint64_t gpa, uint8_t bytes, uint64_t data)
{
	struct vm_entry *entry = &vmentry[vcpu];
	struct vm_mmio *mmio = &entry->u.mmio;

	assert(entry->cmd == VEC_DEFAULT);

	entry->cmd = VEC_COMPLETE_MMIO;
	mmio->bytes = bytes;
	mmio->read = 1;
	mmio->gpa = gpa;
	mmio->data = data;
}

static void
vmentry_mmio_write(int vcpu, uint64_t gpa, uint8_t bytes)
{
	struct vm_entry *entry = &vmentry[vcpu];
	struct vm_mmio *mmio = &entry->u.mmio;

	assert(entry->cmd == VEC_DEFAULT);

	entry->cmd = VEC_COMPLETE_MMIO;
	mmio->bytes = bytes;
	mmio->read = 0;
	mmio->gpa = gpa;
	mmio->data = 0;
}

static void
vmentry_inout_read(int vcpu, uint16_t port, uint8_t bytes, uint32_t data)
{
	struct vm_entry *entry = &vmentry[vcpu];
	struct vm_inout *inout = &entry->u.inout;

	assert(entry->cmd == VEC_DEFAULT);

	entry->cmd = VEC_COMPLETE_INOUT;
	inout->bytes = bytes;
	inout->flags = INOUT_IN;
	inout->port = port;
	inout->eax = data;
}

static void
vmentry_inout_write(int vcpu, uint16_t port, uint8_t bytes)
{
	struct vm_entry *entry = &vmentry[vcpu];
	struct vm_inout *inout = &entry->u.inout;

	assert(entry->cmd == VEC_DEFAULT);

	entry->cmd = VEC_COMPLETE_INOUT;
	inout->bytes = bytes;
	inout->flags = 0;
	inout->port = port;
	inout->eax = 0;
}

static int
vmexit_handle_notify(struct vmctx *ctx, struct vm_exit *vme, int *pvcpu,
		     uint32_t eax)
{
#if BHYVE_DEBUG
	/*
	 * put guest-driven debug here
	 */
#endif
	return (VMEXIT_CONTINUE);
}

static int
vmexit_inout(struct vmctx *ctx, struct vm_exit *vme, int *pvcpu)
{
	int error;
	int vcpu;
	struct vm_inout inout;
	bool in;
	uint8_t bytes;

	stats.vmexit_inout++;

	vcpu = *pvcpu;
	inout = vme->u.inout;
	in = (inout.flags & INOUT_IN) != 0;
	bytes = inout.bytes;

        /* Extra-special case of host notifications */
        if (!in && inout.port == GUEST_NIO_PORT) {
                error = vmexit_handle_notify(ctx, vme, pvcpu, inout.eax);
		vmentry_inout_write(vcpu, inout.port, bytes);
		return (error);
	}

	error = emulate_inout(ctx, vcpu, &inout, strictio != 0);
	if (error) {
		fprintf(stderr, "Unhandled %s%c 0x%04x at 0x%lx\n",
		    in ? "in" : "out",
		    bytes == 1 ? 'b' : (bytes == 2 ? 'w' : 'l'),
		    inout.port, vmexit->rip);
		return (VMEXIT_ABORT);
	} else {
		/*
		 * Communicate the status of the inout operation back to the
		 * in-kernel instruction emulation.
		 */
		if (in) {
			vmentry_inout_read(vcpu, inout.port, bytes, inout.eax);
		} else {
			vmentry_inout_write(vcpu, inout.port, bytes);
		}
		return (VMEXIT_CONTINUE);
	}
}

static int
vmexit_rdmsr(struct vmctx *ctx, struct vm_exit *vme, int *pvcpu)
{
	uint64_t val;
	uint32_t eax, edx;
	int error;

	val = 0;
	error = emulate_rdmsr(ctx, *pvcpu, vme->u.msr.code, &val);
	if (error != 0) {
		fprintf(stderr, "rdmsr to register %#x on vcpu %d\n",
		    vme->u.msr.code, *pvcpu);
		if (strictmsr) {
			vm_inject_gp(ctx, *pvcpu);
			return (VMEXIT_CONTINUE);
		}
	}

	eax = val;
	error = vm_set_register(ctx, *pvcpu, VM_REG_GUEST_RAX, eax);
	assert(error == 0);

	edx = val >> 32;
	error = vm_set_register(ctx, *pvcpu, VM_REG_GUEST_RDX, edx);
	assert(error == 0);

	return (VMEXIT_CONTINUE);
}

static int
vmexit_wrmsr(struct vmctx *ctx, struct vm_exit *vme, int *pvcpu)
{
	int error;

	error = emulate_wrmsr(ctx, *pvcpu, vme->u.msr.code, vme->u.msr.wval);
	if (error != 0) {
		fprintf(stderr, "wrmsr to register %#x(%#lx) on vcpu %d\n",
		    vme->u.msr.code, vme->u.msr.wval, *pvcpu);
		if (strictmsr) {
			vm_inject_gp(ctx, *pvcpu);
			return (VMEXIT_CONTINUE);
		}
	}
	return (VMEXIT_CONTINUE);
}

static int
vmexit_spinup_ap(struct vmctx *ctx, struct vm_exit *vme, int *pvcpu)
{

	(void)spinup_ap(ctx, *pvcpu,
		    vme->u.spinup_ap.vcpu, vme->u.spinup_ap.rip);

	return (VMEXIT_CONTINUE);
}

#ifdef __FreeBSD__
#define	DEBUG_EPT_MISCONFIG
#else
/* EPT misconfig debugging not possible now that raw VMCS access is gone */
#endif

#ifdef DEBUG_EPT_MISCONFIG
#define	VMCS_GUEST_PHYSICAL_ADDRESS	0x00002400

static uint64_t ept_misconfig_gpa, ept_misconfig_pte[4];
static int ept_misconfig_ptenum;
#endif

static const char *
vmexit_vmx_desc(uint32_t exit_reason)
{

	if (exit_reason >= nitems(vmx_exit_reason_desc) ||
	    vmx_exit_reason_desc[exit_reason] == NULL)
		return ("Unknown");
	return (vmx_exit_reason_desc[exit_reason]);
}

static int
vmexit_vmx(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	fprintf(stderr, "vm exit[%d]\n", *pvcpu);
	fprintf(stderr, "\treason\t\tVMX\n");
	fprintf(stderr, "\trip\t\t0x%016lx\n", vmexit->rip);
	fprintf(stderr, "\tinst_length\t%d\n", vmexit->inst_length);
	fprintf(stderr, "\tstatus\t\t%d\n", vmexit->u.vmx.status);
	fprintf(stderr, "\texit_reason\t%u (%s)\n", vmexit->u.vmx.exit_reason,
	    vmexit_vmx_desc(vmexit->u.vmx.exit_reason));
	fprintf(stderr, "\tqualification\t0x%016lx\n",
	    vmexit->u.vmx.exit_qualification);
	fprintf(stderr, "\tinst_type\t\t%d\n", vmexit->u.vmx.inst_type);
	fprintf(stderr, "\tinst_error\t\t%d\n", vmexit->u.vmx.inst_error);
#ifdef DEBUG_EPT_MISCONFIG
	if (vmexit->u.vmx.exit_reason == EXIT_REASON_EPT_MISCONFIG) {
		vm_get_register(ctx, *pvcpu,
		    VMCS_IDENT(VMCS_GUEST_PHYSICAL_ADDRESS),
		    &ept_misconfig_gpa);
		vm_get_gpa_pmap(ctx, ept_misconfig_gpa, ept_misconfig_pte,
		    &ept_misconfig_ptenum);
		fprintf(stderr, "\tEPT misconfiguration:\n");
		fprintf(stderr, "\t\tGPA: %#lx\n", ept_misconfig_gpa);
		fprintf(stderr, "\t\tPTE(%d): %#lx %#lx %#lx %#lx\n",
		    ept_misconfig_ptenum, ept_misconfig_pte[0],
		    ept_misconfig_pte[1], ept_misconfig_pte[2],
		    ept_misconfig_pte[3]);
	}
#endif	/* DEBUG_EPT_MISCONFIG */
	return (VMEXIT_ABORT);
}

static int
vmexit_svm(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	fprintf(stderr, "vm exit[%d]\n", *pvcpu);
	fprintf(stderr, "\treason\t\tSVM\n");
	fprintf(stderr, "\trip\t\t0x%016lx\n", vmexit->rip);
	fprintf(stderr, "\tinst_length\t%d\n", vmexit->inst_length);
	fprintf(stderr, "\texitcode\t%#lx\n", vmexit->u.svm.exitcode);
	fprintf(stderr, "\texitinfo1\t%#lx\n", vmexit->u.svm.exitinfo1);
	fprintf(stderr, "\texitinfo2\t%#lx\n", vmexit->u.svm.exitinfo2);
	return (VMEXIT_ABORT);
}

static int
vmexit_bogus(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	assert(vmexit->inst_length == 0);

	stats.vmexit_bogus++;

	return (VMEXIT_CONTINUE);
}

static int
vmexit_reqidle(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	assert(vmexit->inst_length == 0);

	stats.vmexit_reqidle++;

	return (VMEXIT_CONTINUE);
}

static int
vmexit_hlt(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	stats.vmexit_hlt++;

	/*
	 * Just continue execution with the next instruction. We use
	 * the HLT VM exit as a way to be friendly with the host
	 * scheduler.
	 */
	return (VMEXIT_CONTINUE);
}

static int
vmexit_pause(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	stats.vmexit_pause++;

	return (VMEXIT_CONTINUE);
}

static int
vmexit_mtrap(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	assert(vmexit->inst_length == 0);

	stats.vmexit_mtrap++;

	if (gdb_port == 0) {
		fprintf(stderr, "vm_loop: unexpected VMEXIT_MTRAP\n");
		exit(4);
	}
	gdb_cpu_mtrap(*pvcpu);
	return (VMEXIT_CONTINUE);
}

static int
vmexit_inst_emul(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{
	uint8_t i, valid;

	fprintf(stderr, "Failed to emulate instruction sequence ");

	valid = vmexit->u.inst_emul.num_valid;
	if (valid != 0) {
		assert(valid <= sizeof (vmexit->u.inst_emul.inst));
		fprintf(stderr, "[");
		for (i = 0; i < valid; i++) {
			if (i == 0) {
				fprintf(stderr, "%02x",
				    vmexit->u.inst_emul.inst[i]);
			} else {
				fprintf(stderr, ", %02x",
				    vmexit->u.inst_emul.inst[i]);
			}
		}
		fprintf(stderr, "] ");
	}
	fprintf(stderr, "@ %rip = %x\n", vmexit->rip);

	return (VMEXIT_ABORT);
}

static int
vmexit_mmio(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{
	int vcpu, err;
	struct vm_mmio mmio;
	bool is_read;

	stats.vmexit_mmio++;

	vcpu = *pvcpu;
	mmio = vmexit->u.mmio;
	is_read = (mmio.read != 0);

	err = emulate_mem(ctx, vcpu, &mmio);

	if (err == ESRCH) {
		fprintf(stderr, "Unhandled memory access to 0x%lx\n", mmio.gpa);
		stats.mmio_unhandled++;

		/*
		 * Access to non-existent physical addresses is not likely to
		 * result in fatal errors on hardware machines, but rather reads
		 * of all-ones or discarded-but-acknowledged writes.
		 */
		mmio.data = ~0UL;
		err = 0;
	}

	if (err == 0) {
		if (is_read) {
			vmentry_mmio_read(vcpu, mmio.gpa, mmio.bytes,
			    mmio.data);
		} else {
			vmentry_mmio_write(vcpu, mmio.gpa, mmio.bytes);
		}
		return (VMEXIT_CONTINUE);
	}

	fprintf(stderr, "Unhandled mmio error to 0x%lx: %d\n", mmio.gpa, err);
	return (VMEXIT_ABORT);
}

static pthread_mutex_t resetcpu_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t resetcpu_cond = PTHREAD_COND_INITIALIZER;

static int
vmexit_suspend(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{
	enum vm_suspend_how how;

	how = vmexit->u.suspended.how;

	fbsdrun_deletecpu(ctx, *pvcpu);

	if (*pvcpu != BSP) {
		pthread_mutex_lock(&resetcpu_mtx);
		pthread_cond_signal(&resetcpu_cond);
		pthread_mutex_unlock(&resetcpu_mtx);
		pthread_exit(NULL);
	}

	pthread_mutex_lock(&resetcpu_mtx);
	while (!CPU_EMPTY(&cpumask)) {
		pthread_cond_wait(&resetcpu_cond, &resetcpu_mtx);
	}
	pthread_mutex_unlock(&resetcpu_mtx);

	switch (how) {
	case VM_SUSPEND_RESET:
		exit(0);
	case VM_SUSPEND_POWEROFF:
		exit(1);
	case VM_SUSPEND_HALT:
		exit(2);
	case VM_SUSPEND_TRIPLEFAULT:
		exit(3);
	default:
		fprintf(stderr, "vmexit_suspend: invalid reason %d\n", how);
		exit(100);
	}
	return (0);	/* NOTREACHED */
}

static int
vmexit_debug(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	if (gdb_port == 0) {
		fprintf(stderr, "vm_loop: unexpected VMEXIT_DEBUG\n");
		exit(4);
	}
	gdb_cpu_suspend(*pvcpu);
	return (VMEXIT_CONTINUE);
}

static int
vmexit_breakpoint(struct vmctx *ctx, struct vm_exit *vmexit, int *pvcpu)
{

	if (gdb_port == 0) {
		fprintf(stderr, "vm_loop: unexpected VMEXIT_DEBUG\n");
		exit(4);
	}
	gdb_cpu_breakpoint(*pvcpu, vmexit);
	return (VMEXIT_CONTINUE);
}

static vmexit_handler_t handler[VM_EXITCODE_MAX] = {
	[VM_EXITCODE_INOUT]  = vmexit_inout,
	[VM_EXITCODE_MMIO]  = vmexit_mmio,
	[VM_EXITCODE_VMX]    = vmexit_vmx,
	[VM_EXITCODE_SVM]    = vmexit_svm,
	[VM_EXITCODE_BOGUS]  = vmexit_bogus,
	[VM_EXITCODE_REQIDLE] = vmexit_reqidle,
	[VM_EXITCODE_RDMSR]  = vmexit_rdmsr,
	[VM_EXITCODE_WRMSR]  = vmexit_wrmsr,
	[VM_EXITCODE_MTRAP]  = vmexit_mtrap,
	[VM_EXITCODE_INST_EMUL] = vmexit_inst_emul,
	[VM_EXITCODE_SPINUP_AP] = vmexit_spinup_ap,
	[VM_EXITCODE_SUSPENDED] = vmexit_suspend,
	[VM_EXITCODE_TASK_SWITCH] = vmexit_task_switch,
	[VM_EXITCODE_DEBUG] = vmexit_debug,
	[VM_EXITCODE_BPT] = vmexit_breakpoint,
};

static void
vm_loop(struct vmctx *ctx, int vcpu, uint64_t startrip)
{
	int error, rc;
	enum vm_exitcode exitcode;
	cpuset_t active_cpus;
	struct vm_exit *vexit;
	struct vm_entry *ventry;

#ifdef	__FreeBSD__
	if (vcpumap[vcpu] != NULL) {
		error = pthread_setaffinity_np(pthread_self(),
		    sizeof(cpuset_t), vcpumap[vcpu]);
		assert(error == 0);
	}
#endif
	error = vm_active_cpus(ctx, &active_cpus);
	assert(CPU_ISSET(vcpu, &active_cpus));

	error = vm_set_register(ctx, vcpu, VM_REG_GUEST_RIP, startrip);
	assert(error == 0);

	ventry = &vmentry[vcpu];
	vexit = &vmexit[vcpu];

	while (1) {
		error = vm_run(ctx, vcpu, ventry, vexit);
		if (error != 0)
			break;

		if (ventry->cmd != VEC_DEFAULT) {
			/*
			 * Discard any lingering entry state after it has been
			 * submitted via vm_run().
			 */
			bzero(ventry, sizeof (*ventry));
		}

		exitcode = vexit->exitcode;
		if (exitcode >= VM_EXITCODE_MAX || handler[exitcode] == NULL) {
			fprintf(stderr, "vm_loop: unexpected exitcode 0x%x\n",
			    exitcode);
			exit(4);
		}

		rc = (*handler[exitcode])(ctx, vexit, &vcpu);

		switch (rc) {
		case VMEXIT_CONTINUE:
			break;
		case VMEXIT_ABORT:
			abort();
		default:
			exit(4);
		}
	}
	fprintf(stderr, "vm_run error %d, errno %d\n", error, errno);
}

static int
num_vcpus_allowed(struct vmctx *ctx)
{
#ifdef __FreeBSD__
	int tmp, error;

	error = vm_get_capability(ctx, BSP, VM_CAP_UNRESTRICTED_GUEST, &tmp);

	/*
	 * The guest is allowed to spinup more than one processor only if the
	 * UNRESTRICTED_GUEST capability is available.
	 */
	if (error == 0)
		return (VM_MAXCPU);
	else
		return (1);
#else
	/* Unrestricted Guest is always enabled on illumos */
	return (VM_MAXCPU);
#endif /* __FreeBSD__ */
}

void
fbsdrun_set_capabilities(struct vmctx *ctx, int cpu)
{
	int err, tmp;

	if (fbsdrun_vmexit_on_hlt()) {
		err = vm_get_capability(ctx, cpu, VM_CAP_HALT_EXIT, &tmp);
		if (err < 0) {
			fprintf(stderr, "VM exit on HLT not supported\n");
			exit(4);
		}
		vm_set_capability(ctx, cpu, VM_CAP_HALT_EXIT, 1);
		if (cpu == BSP)
			handler[VM_EXITCODE_HLT] = vmexit_hlt;
	}

        if (fbsdrun_vmexit_on_pause()) {
		/*
		 * pause exit support required for this mode
		 */
		err = vm_get_capability(ctx, cpu, VM_CAP_PAUSE_EXIT, &tmp);
		if (err < 0) {
			fprintf(stderr,
			    "SMP mux requested, no pause support\n");
			exit(4);
		}
		vm_set_capability(ctx, cpu, VM_CAP_PAUSE_EXIT, 1);
		if (cpu == BSP)
			handler[VM_EXITCODE_PAUSE] = vmexit_pause;
        }

	if (x2apic_mode)
		err = vm_set_x2apic_state(ctx, cpu, X2APIC_ENABLED);
	else
		err = vm_set_x2apic_state(ctx, cpu, X2APIC_DISABLED);

	if (err) {
		fprintf(stderr, "Unable to set x2apic state (%d)\n", err);
		exit(4);
	}

#ifdef	__FreeBSD__
	vm_set_capability(ctx, cpu, VM_CAP_ENABLE_INVPCID, 1);
#endif
}

static struct vmctx *
do_open(const char *vmname)
{
	struct vmctx *ctx;
	int error;
	bool reinit, romboot;
#ifndef WITHOUT_CAPSICUM
	cap_rights_t rights;
	const cap_ioctl_t *cmds;	
	size_t ncmds;
#endif

	reinit = romboot = false;

	if (lpc_bootrom())
		romboot = true;

	error = vm_create(vmname);
	if (error) {
		if (errno == EEXIST) {
			if (romboot) {
				reinit = true;
			} else {
				/*
				 * The virtual machine has been setup by the
				 * userspace bootloader.
				 */
			}
		} else {
			perror("vm_create");
			exit(4);
		}
	} else {
		if (!romboot) {
			/*
			 * If the virtual machine was just created then a
			 * bootrom must be configured to boot it.
			 */
			fprintf(stderr, "virtual machine cannot be booted\n");
			exit(4);
		}
	}

	ctx = vm_open(vmname);
	if (ctx == NULL) {
		perror("vm_open");
		exit(4);
	}

#ifndef WITHOUT_CAPSICUM
	cap_rights_init(&rights, CAP_IOCTL, CAP_MMAP_RW);
	if (caph_rights_limit(vm_get_device_fd(ctx), &rights) == -1) 
		errx(EX_OSERR, "Unable to apply rights for sandbox");
	vm_get_ioctls(&ncmds);
	cmds = vm_get_ioctls(NULL);
	if (cmds == NULL)
		errx(EX_OSERR, "out of memory");
	if (caph_ioctls_limit(vm_get_device_fd(ctx), cmds, ncmds) == -1)
		errx(EX_OSERR, "Unable to apply rights for sandbox");
	free((cap_ioctl_t *)cmds);
#endif
 
	if (reinit) {
		error = vm_reinit(ctx);
		if (error) {
			perror("vm_reinit");
			exit(4);
		}
	}
	error = vm_set_topology(ctx, sockets, cores, threads, maxcpus);
	if (error)
		errx(EX_OSERR, "vm_set_topology");
	return (ctx);
}

int
main(int argc, char *argv[])
{
	int c, error, dbg_port, err, bvmcons;
	int max_vcpus, mptgen, memflags;
	int rtc_localtime;
	bool gdb_stop;
#ifndef __FreeBSD__
	bool suspend = false;
#endif
	struct vmctx *ctx;
	uint64_t rip;
	size_t memsize;
	char *optstr;

	bvmcons = 0;
	progname = basename(argv[0]);
	dbg_port = 0;
	gdb_stop = false;
	guest_ncpus = 1;
	sockets = cores = threads = 1;
	maxcpus = 0;
	memsize = 256 * MB;
	mptgen = 1;
	rtc_localtime = 1;
	memflags = 0;

#ifdef	__FreeBSD__
	optstr = "abehuwxACHIPSWYp:g:G:c:s:m:l:B:U:";
#else
	optstr = "abdehuwxACHIPSWYg:G:c:s:m:l:B:U:";
#endif
	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 'a':
			x2apic_mode = 0;
			break;
		case 'A':
			acpi = 1;
			break;
		case 'b':
			bvmcons = 1;
			break;
		case 'B':
			if (smbios_parse(optarg) != 0) {
				errx(EX_USAGE, "invalid SMBIOS "
				    "configuration '%s'", optarg);
			}
			break;
#ifndef	__FreeBSD__
		case 'd':
			suspend = true;
			break;
#else
		case 'p':
			if (pincpu_parse(optarg) != 0) {
				errx(EX_USAGE, "invalid vcpu pinning "
				    "configuration '%s'", optarg);
			}
			break;
#endif
                case 'c':
			if (topology_parse(optarg) != 0) {
			    errx(EX_USAGE, "invalid cpu topology "
				"'%s'", optarg);
			}
			break;
		case 'C':
			memflags |= VM_MEM_F_INCORE;
			break;
		case 'g':
			dbg_port = atoi(optarg);
			break;
		case 'G':
			if (optarg[0] == 'w') {
				gdb_stop = true;
				optarg++;
			}
			gdb_port = atoi(optarg);
			break;
		case 'l':
			if (strncmp(optarg, "help", strlen(optarg)) == 0) {
				lpc_print_supported_devices();
				exit(0);
			} else if (lpc_device_parse(optarg) != 0) {
				errx(EX_USAGE, "invalid lpc device "
				    "configuration '%s'", optarg);
			}
			break;
		case 's':
			if (strncmp(optarg, "help", strlen(optarg)) == 0) {
				pci_print_supported_devices();
				exit(0);
			} else if (pci_parse_slot(optarg) != 0)
				exit(4);
			else
				break;
		case 'S':
			memflags |= VM_MEM_F_WIRED;
			break;
                case 'm':
			error = vm_parse_memsize(optarg, &memsize);
			if (error)
				errx(EX_USAGE, "invalid memsize '%s'", optarg);
			break;
		case 'H':
			guest_vmexit_on_hlt = 1;
			break;
		case 'I':
			/*
			 * The "-I" option was used to add an ioapic to the
			 * virtual machine.
			 *
			 * An ioapic is now provided unconditionally for each
			 * virtual machine and this option is now deprecated.
			 */
			break;
		case 'P':
			guest_vmexit_on_pause = 1;
			break;
		case 'e':
			strictio = 1;
			break;
		case 'u':
			rtc_localtime = 0;
			break;
		case 'U':
			guest_uuid_str = optarg;
			break;
		case 'w':
			strictmsr = 0;
			break;
		case 'W':
			virtio_msix = 0;
			break;
		case 'x':
			x2apic_mode = 1;
			break;
		case 'Y':
			mptgen = 0;
			break;
		case 'h':
			usage(0);			
		default:
			usage(1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage(1);

	vmname = argv[0];
	ctx = do_open(vmname);

	max_vcpus = num_vcpus_allowed(ctx);
	if (guest_ncpus > max_vcpus) {
		fprintf(stderr, "%d vCPUs requested but only %d available\n",
			guest_ncpus, max_vcpus);
		exit(4);
	}

	fbsdrun_set_capabilities(ctx, BSP);

	vm_set_memflags(ctx, memflags);
#ifdef	__FreeBSD__
	err = vm_setup_memory(ctx, memsize, VM_MMAP_ALL);
#else
	do {
		errno = 0;
		err = vm_setup_memory(ctx, memsize, VM_MMAP_ALL);
		error = errno;
		if (err != 0 && error == ENOMEM) {
			(void) fprintf(stderr, "Unable to allocate memory "
			    "(%llu), retrying in 1 second\n", memsize);
			sleep(1);
		}
	} while (error == ENOMEM);
#endif
	if (err) {
		fprintf(stderr, "Unable to setup memory (%d)\n", errno);
		exit(4);
	}

	error = init_msr();
	if (error) {
		fprintf(stderr, "init_msr error %d", error);
		exit(4);
	}

	init_mem();
	init_inout();
#ifdef	__FreeBSD__
	kernemu_dev_init();
#endif
	init_bootrom(ctx);
	atkbdc_init(ctx);
	pci_irq_init(ctx);
	ioapic_init(ctx);

	rtc_init(ctx, rtc_localtime);
	sci_init(ctx);

	/*
	 * Exit if a device emulation finds an error in its initilization
	 */
	if (init_pci(ctx) != 0) {
		perror("device emulation initialization error");
		exit(4);
	}

	/*
	 * Initialize after PCI, to allow a bootrom file to reserve the high
	 * region.
	 */
	if (acpi)
		vmgenc_init(ctx);

	if (dbg_port != 0)
		init_dbgport(dbg_port);

#ifdef __FreeBSD__
	if (gdb_port != 0)
		init_gdb(ctx, gdb_port, gdb_stop);
#else
	if (gdb_port < 0) {
		/*
		 * Set up the internal gdb state needed for basic debugging, but
		 * skip the step of listening on a port for the GDB server.
		 */
		init_mdb(ctx, gdb_stop);
	} else if (gdb_port != 0) {
		init_gdb(ctx, gdb_port, gdb_stop);
	}
#endif

	if (bvmcons)
		init_bvmcons();

	vga_init(1);

	if (lpc_bootrom()) {
#ifdef __FreeBSD__
		if (vm_set_capability(ctx, BSP, VM_CAP_UNRESTRICTED_GUEST, 1)) {
			fprintf(stderr, "ROM boot failed: unrestricted guest "
			    "capability not available\n");
			exit(4);
		}
#else
		/* Unrestricted Guest is always enabled on illumos */
#endif
		error = vcpu_reset(ctx, BSP);
		assert(error == 0);
	}

	error = vm_get_register(ctx, BSP, VM_REG_GUEST_RIP, &rip);
	assert(error == 0);

	/*
 	 * build the guest tables, MP etc.
	 */
	if (mptgen) {
		error = mptable_build(ctx, guest_ncpus);
		if (error) {
			perror("error to build the guest tables");
			exit(4);
		}
	}

	error = smbios_build(ctx);
	assert(error == 0);

	if (acpi) {
		error = acpi_build(ctx, guest_ncpus);
		assert(error == 0);
	}

	if (lpc_bootrom())
		fwctl_init();

	/*
	 * Change the proc title to include the VM name.
	 */
	setproctitle("%s", vmname);

#ifndef WITHOUT_CAPSICUM
	caph_cache_catpages();

	if (caph_limit_stdout() == -1 || caph_limit_stderr() == -1)
		errx(EX_OSERR, "Unable to apply rights for sandbox");

	if (caph_enter() == -1)
		errx(EX_OSERR, "cap_enter() failed");
#endif

	/*
	 * Add CPU 0
	 */
#ifdef __FreeBSD__
	fbsdrun_addcpu(ctx, BSP, BSP, rip);
#else
	fbsdrun_addcpu(ctx, BSP, BSP, rip, suspend);
#endif
	/*
	 * Head off to the main event dispatch loop
	 */
	mevent_dispatch();

	exit(4);
}
