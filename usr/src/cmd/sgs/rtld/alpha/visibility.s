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

	.protected dladdr
	.protected dladdr1
	.protected dldump
	.protected dlclose
	.protected dlerror
	.protected dlinfo
	.protected dlopen
	.protected dlmopen
	.protected dlsym

	.protected _dladdr
	.protected _dladdr1
	.protected _dldump
	.protected _dlclose
	.protected _dlerror
	.protected _dlinfo
	.protected _dlopen
	.protected _dlmopen
	.protected _dlsym

	.protected _ld_libc

	.protected elf_rtbndr
	.protected _rt_boot

	.protected rtld_db_dlactivity
	.protected rtld_db_preinit
	.protected rtld_db_postinit
	.protected r_debug

	.protected elf_plt_write
	.protected is_so_loaded
	.protected lml_main
	.protected lookup_sym

	.protected alist_append
	.protected ld_entry_cnt
	.protected dbg_desc
	.protected dbg_print
	.protected eprintf
	.protected veprintf

	.protected dgettext
	.protected strerror

	.protected calloc
	.protected free
	.protected malloc
	.protected realloc

	.protected _environ
	.protected environ

	.protected memcpy
	.protected snprintf
	.protected sprintf
	.protected strcat
	.protected strcmp
	.protected strcpy
	.protected strlen
	.protected strrchr
	.protected strtok_r
	.protected ___errno
	.protected qsort
	.protected dl_iterate_phdr

	.protected do64_reloc_rtld
	.protected reloc64_table
