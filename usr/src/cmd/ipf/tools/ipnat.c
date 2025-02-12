/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Added redirect stuff and a variety of bug fixes. (mcn@EnGarde.com)
 *
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 * Copyright (c) 2014, Joyent, Inc.  All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#if !defined(__SVR4) && !defined(__svr4__)
#include <strings.h>
#else
#include <sys/byteorder.h>
#endif
#include <sys/time.h>
#include <sys/param.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/file.h>
#define _KERNEL
#include <sys/uio.h>
#undef _KERNEL
#include <sys/socket.h>
#include <sys/ioctl.h>
#if defined(sun) && (defined(__svr4__) || defined(__SVR4))
# include <sys/ioccom.h>
# include <sys/sysmacros.h>
#endif
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/if.h>
#if __FreeBSD_version >= 300000
# include <net/if_var.h>
#endif
#include <netdb.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <ctype.h>
#if defined(linux)
# include <linux/a.out.h>
#else
# include <nlist.h>
#endif
#include "ipf.h"
#include "netinet/ipl.h"
#include "kmem.h"
#include "ipfzone.h"

#ifdef	__hpux
# define	nlist	nlist64
#endif

#if	defined(sun) && !SOLARIS2
# define	STRERROR(x)	sys_errlist[x]
extern	char	*sys_errlist[];
#else
# define	STRERROR(x)	strerror(x)
#endif

#if !defined(lint)
static const char sccsid[] ="@(#)ipnat.c	1.9 6/5/96 (C) 1993 Darren Reed";
static const char rcsid[] = "@(#)$Id: ipnat.c,v 1.24.2.2 2005/05/10 21:19:30 darrenr Exp $";
#endif


#ifdef	SOLARIS
#define	bzero(a,b)	memset(a,0,b)
#endif
int	use_inet6 = 0;

extern	char	*optarg;

void	dostats __P((int, natstat_t *, int, int));
void	flushtable __P((int, int));
void	usage __P((char *));
int	main __P((int, char*[]));
void	showhostmap __P((natstat_t *nsp));
void	natstat_dead __P((natstat_t *, char *));
void	dostats_live __P((int, natstat_t *, int));
void	showhostmap_live __P((int, natstat_t *));

int	opts;

void usage(name)
char *name;
{
	fprintf(stderr, "Usage: %s [-CdFhlnrRsv] [-f filename]", name);
	fprintf(stderr, " [-G|-z zonename]\n");
	exit(1);
}


int main(argc, argv)
int argc;
char *argv[];
{
	char *file, *core, *kernel;
	natstat_t ns, *nsp;
	int fd, c, mode;
	ipfobj_t obj;

	fd = -1;
	opts = 0;
	nsp = &ns;
	file = NULL;
	core = NULL;
	kernel = NULL;
	mode = O_RDWR;

	while ((c = getopt(argc, argv, "CdFf:G:hlM:N:nrRsvz:")) != -1)
		switch (c)
		{
		case 'C' :
			opts |= OPT_CLEAR;
			break;
		case 'd' :
			opts |= OPT_DEBUG;
			break;
		case 'f' :
			file = optarg;
			break;
		case 'F' :
			opts |= OPT_FLUSH;
			break;
		case 'G' :
			setzonename_global(optarg);
			break;
		case 'h' :
			opts |=OPT_HITS;
			break;
		case 'l' :
			opts |= OPT_LIST;
			mode = O_RDONLY;
			break;
		case 'M' :
			core = optarg;
			break;
		case 'N' :
			kernel = optarg;
			break;
		case 'n' :
			opts |= OPT_DONOTHING;
			mode = O_RDONLY;
			break;
		case 'R' :
			opts |= OPT_NORESOLVE;
			break;
		case 'r' :
			opts |= OPT_REMOVE;
			break;
		case 's' :
			opts |= OPT_STAT;
			mode = O_RDONLY;
			break;
		case 'v' :
			opts |= OPT_VERBOSE;
			break;
		case 'z' :
			setzonename(optarg);
			break;
		default :
			usage(argv[0]);
		}

	initparse();

	if ((kernel != NULL) || (core != NULL)) {
		(void) setgid(getgid());
		(void) setreuid(getuid(), getuid());
	}

	bzero((char *)&ns, sizeof(ns));

	if ((opts & OPT_DONOTHING) == 0) {
		if (checkrev(IPL_NAME) == -1) {
			fprintf(stderr, "User/kernel version check failed\n");
			exit(1);
		}
	}


	if (!(opts & OPT_DONOTHING) && (kernel == NULL) && (core == NULL)) {
#ifdef notdef
		if (openkmem(kernel, core) == -1)
			exit(1);
#endif
		if (((fd = open(IPNAT_NAME, mode)) == -1) &&
		    ((fd = open(IPNAT_NAME, O_RDONLY)) == -1)) {
			(void) fprintf(stderr, "%s: open: %s\n", IPNAT_NAME,
				STRERROR(errno));
			exit(1);
		}

		if (setzone(fd) != 0) {
			close(fd);
			exit(1);
		}

		bzero((char *)&obj, sizeof(obj));
		obj.ipfo_rev = IPFILTER_VERSION;
		obj.ipfo_size = sizeof(*nsp);
		obj.ipfo_type = IPFOBJ_NATSTAT;
		obj.ipfo_ptr = (void *)nsp;
		if (ioctl(fd, SIOCGNATS, &obj) == -1) {
			perror("ioctl(SIOCGNATS)");
			exit(1);
		}
		(void) setgid(getgid());
		(void) setreuid(getuid(), getuid());
	} else if ((kernel != NULL) || (core != NULL)) {
		if (openkmem(kernel, core) == -1)
			exit(1);

		natstat_dead(nsp, kernel);
		if (opts & (OPT_LIST|OPT_STAT))
			dostats(fd, nsp, opts, 0);
		exit(0);
	}

	if (opts & (OPT_FLUSH|OPT_CLEAR))
		flushtable(fd, opts);
	if (file) {
		ipnat_parsefile(fd, ipnat_addrule, ioctl, file);
	}
	if (opts & (OPT_LIST|OPT_STAT))
		dostats(fd, nsp, opts, 1);
	return 0;
}


/*
 * Read NAT statistic information in using a symbol table and memory file
 * rather than doing ioctl's.
 */
void natstat_dead(nsp, kernel)
natstat_t *nsp;
char *kernel;
{
	struct nlist nat_nlist[10] = {
		{ "nat_table" },		/* 0 */
		{ "nat_list" },
		{ "maptable" },
		{ "ipf_nattable_sz" },
		{ "ipf_natrules_sz" },
		{ "ipf_rdrrules_sz" },		/* 5 */
		{ "ipf_hostmap_sz" },
		{ "nat_instances" },
		{ "ap_sess_list" },
		{ NULL }
	};
	void *tables[2];

	if (nlist(kernel, nat_nlist) == -1) {
		fprintf(stderr, "nlist error\n");
		return;
	}

	/*
	 * Normally the ioctl copies all of these values into the structure
	 * for us, before returning it to userland, so here we must copy each
	 * one in individually.
	 */
	kmemcpy((char *)&tables, nat_nlist[0].n_value, sizeof(tables));
	nsp->ns_table[0] = tables[0];
	nsp->ns_table[1] = tables[1];

	kmemcpy((char *)&nsp->ns_list, nat_nlist[1].n_value,
		sizeof(nsp->ns_list));
	kmemcpy((char *)&nsp->ns_maptable, nat_nlist[2].n_value,
		sizeof(nsp->ns_maptable));
	kmemcpy((char *)&nsp->ns_nattab_sz, nat_nlist[3].n_value,
		sizeof(nsp->ns_nattab_sz));
	kmemcpy((char *)&nsp->ns_rultab_sz, nat_nlist[4].n_value,
		sizeof(nsp->ns_rultab_sz));
	kmemcpy((char *)&nsp->ns_rdrtab_sz, nat_nlist[5].n_value,
		sizeof(nsp->ns_rdrtab_sz));
	kmemcpy((char *)&nsp->ns_hostmap_sz, nat_nlist[6].n_value,
		sizeof(nsp->ns_hostmap_sz));
	kmemcpy((char *)&nsp->ns_instances, nat_nlist[7].n_value,
		sizeof(nsp->ns_instances));
	kmemcpy((char *)&nsp->ns_apslist, nat_nlist[8].n_value,
		sizeof(nsp->ns_apslist));
}


/*
 * Display NAT statistics.
 */
void dostats(fd, nsp, opts, alive)
natstat_t *nsp;
int fd, opts, alive;
{
	nat_t *np, nat;
	ipnat_t	ipn;

	/*
	 * Show statistics ?
	 */
	if (opts & OPT_STAT) {
		printf("mapped\tin\t%lu\tout\t%lu\n",
			nsp->ns_mapped[0], nsp->ns_mapped[1]);
		printf("added\t%lu\texpired\t%lu\n",
			nsp->ns_added, nsp->ns_expire);
		printf("no memory\t%lu\tbad nat\t%lu\n",
			nsp->ns_memfail, nsp->ns_badnat);
		printf("inuse\t%lu\norphans\t%u\nrules\t%lu\n",
			nsp->ns_inuse, nsp->ns_orphans, nsp->ns_rules);
		printf("wilds\t%u\n", nsp->ns_wilds);
		if (opts & OPT_VERBOSE)
			printf("table %p list %p\n",
				nsp->ns_table, nsp->ns_list);
	}

	/*
	 * Show list of NAT rules and NAT sessions ?
	 */
	if (opts & OPT_LIST) {
		if (alive) {
			dostats_live(fd, nsp, opts);
			return;
		}
		printf("List of active MAP/Redirect filters:\n");
		while (nsp->ns_list) {
			if (kmemcpy((char *)&ipn, (long)nsp->ns_list,
				    sizeof(ipn))) {
				perror("kmemcpy");
				break;
			}
			if (opts & OPT_HITS)
				printf("%lu ", ipn.in_hits);
			printnat(&ipn, opts & (OPT_DEBUG|OPT_VERBOSE));
			nsp->ns_list = ipn.in_next;
		}

		printf("\nList of active sessions:\n");

		for (np = nsp->ns_instances; np; np = nat.nat_next) {
			if (kmemcpy((char *)&nat, (long)np, sizeof(nat)))
				break;
			printactivenat(&nat, opts, 0);
			if (nat.nat_aps)
				printaps(nat.nat_aps, opts);
		}

		if (opts & OPT_VERBOSE)
			showhostmap(nsp);
	}
}


/*
 * Display the active host mapping table.
 */
void showhostmap(nsp)
natstat_t *nsp;
{
	hostmap_t hm, *hmp, **maptable;
	u_int hv;

	printf("\nList of active host mappings:\n");

	maptable = (hostmap_t **)malloc(sizeof(hostmap_t *) *
					nsp->ns_hostmap_sz);
	if (maptable == NULL) {
		perror("malloc");
		exit(1);
	}
	if (kmemcpy((char *)maptable, (u_long)nsp->ns_maptable,
		    sizeof(hostmap_t *) * nsp->ns_hostmap_sz)) {
		perror("kmemcpy (maptable)");
		return;
	}

	for (hv = 0; hv < nsp->ns_hostmap_sz; hv++) {
		hmp = maptable[hv];

		while (hmp) {
			if (kmemcpy((char *)&hm, (u_long)hmp, sizeof(hm))) {
				perror("kmemcpy (hostmap)");
				return;
			}

			printhostmap(&hm, hv);
			hmp = hm.hm_next;
		}
	}
	free(maptable);
}


/*
 * Issue an ioctl to flush either the NAT rules table or the active mapping
 * table or both.
 */
void flushtable(fd, opts)
int fd, opts;
{
	int n = 0;

	if (opts & OPT_FLUSH) {
		n = FLUSH_TABLE_ALL;
		if (!(opts & OPT_DONOTHING) && ioctl(fd, SIOCIPFFL, &n) == -1)
			perror("ioctl(SIOCFLNAT)");
		else
			printf("%d entries flushed from NAT table\n", n);
	}

	if (opts & OPT_CLEAR) {
		n = FLUSH_LIST;
		if (!(opts & OPT_DONOTHING) && ioctl(fd, SIOCIPFFL, &n) == -1)
			perror("ioctl(SIOCCNATL)");
		else
			printf("%d entries flushed from NAT list\n", n);
	}
}

/*
 * Display NAT statistics.
 */
void dostats_live(fd, nsp, opts)
natstat_t *nsp;
int fd, opts;
{
	ipfgeniter_t iter;
	ipfobj_t obj;
	ipnat_t	ipn;
	nat_t nat;

	bzero((char *)&obj, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_GENITER;
	obj.ipfo_size = sizeof(iter);
	obj.ipfo_ptr = &iter;

	iter.igi_type = IPFGENITER_IPNAT;
	iter.igi_nitems = 1;
	iter.igi_data = &ipn;

	/*
	 * Show list of NAT rules and NAT sessions ?
	 */
	printf("List of active MAP/Redirect filters:\n");
	while (nsp->ns_list) {
		if (ioctl(fd, SIOCGENITER, &obj) == -1)
			break;
		if (opts & OPT_HITS)
			printf("%lu ", ipn.in_hits);
		printnat(&ipn, opts & (OPT_DEBUG|OPT_VERBOSE));
		nsp->ns_list = ipn.in_next;
	}

	printf("\nList of active sessions:\n");

	iter.igi_type = IPFGENITER_NAT;
	iter.igi_nitems = 1;
	iter.igi_data = &nat;

	while (nsp->ns_instances != NULL) {
		if (ioctl(fd, SIOCGENITER, &obj) == -1)
			break;
		printactivenat(&nat, opts, 1);
		if (nat.nat_aps)
			printaps(nat.nat_aps, opts);
		nsp->ns_instances = nat.nat_next;
	}

	if (opts & OPT_VERBOSE)
		showhostmap_live(fd, nsp);
}

/*
 * Display the active host mapping table.
 */
void showhostmap_live(fd, nsp)
int fd;
natstat_t *nsp;
{
	hostmap_t hm, *hmp;
	ipfgeniter_t iter;
	ipfobj_t obj;

	bzero((char *)&obj, sizeof(obj));
	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_GENITER;
	obj.ipfo_size = sizeof(iter);
	obj.ipfo_ptr = &iter;

	iter.igi_type = IPFGENITER_HOSTMAP;
	iter.igi_nitems = 1;
	iter.igi_data = &hm;

	printf("\nList of active host mappings:\n");

	while (nsp->ns_maplist != NULL) {
		if (ioctl(fd, SIOCGENITER, &obj) == -1)
			break;
		printhostmap(&hm, 0);
		nsp->ns_maplist = hm.hm_next;
	}
}
