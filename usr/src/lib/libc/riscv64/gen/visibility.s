
//   protected:
	.protected arc4random
	.protected arc4random_buf
	.protected arc4random_uniform
	.protected explicit_bzero
	.protected getentropy

//   protected:
	.protected iswxdigit_l
	.protected isxdigit_l

//   protected:
	.protected wcsnrtombs
	.protected wcsnrtombs_l

//   protected:
	.protected __global_locale
	.protected __mb_cur_max
	.protected __mb_cur_max_l
	.protected btowc_l
	.protected duplocale
	.protected fgetwc_l
	.protected freelocale
	.protected getwc_l
	.protected isalnum_l
	.protected isalpha_l
	.protected isblank_l
	.protected iscntrl_l
	.protected isdigit_l
	.protected isgraph_l
	.protected islower_l
	.protected isprint_l
	.protected ispunct_l
	.protected isspace_l
	.protected isupper_l
	.protected iswideogram
	.protected iswideogram_l
	.protected iswnumber
	.protected iswnumber_l
	.protected iswhexnumber
	.protected iswhexnumber_l
	.protected iswphonogram
	.protected iswphonogram_l
	.protected iswspecial
	.protected iswspecial_l
	.protected iswalnum_l
	.protected iswalpha_l
	.protected iswblank_l
	.protected iswcntrl_l
	.protected iswctype_l
	.protected iswdigit_l
	.protected iswgraph_l
	.protected iswlower_l
	.protected iswprint_l
	.protected iswpunct_l
	.protected iswspace_l
	.protected iswupper_l
	.protected mblen_l
	.protected mbrlen_l
	.protected mbsinit_l
	.protected mbsnrtowcs
	.protected mbsnrtowcs_l
	.protected mbsrtowcs_l
	.protected mbstowcs_l
	.protected mbtowc_l
	.protected newlocale
	.protected nl_langinfo_l
	.protected strcasecmp_l
	.protected strcasestr_l
	.protected strcoll_l
	.protected strfmon_l
	.protected strftime_l
	.protected strncasecmp_l
	.protected strptime_l
	.protected strxfrm_l
	.protected tolower_l
	.protected toupper_l
	.protected towlower_l
	.protected towupper_l
	.protected towctrans_l
	.protected uselocale
	.protected wcrtomb_l
	.protected wcscasecmp_l
	.protected wcscoll_l
	.protected wcsncasecmp_l
	.protected wcsrtombs_l
	.protected wcstombs_l
	.protected wcswidth_l
	.protected wcsxfrm_l
	.protected wctob_l
	.protected wctomb_l
	.protected wctrans_l
	.protected wctype_l
	.protected wcwidth_l

//   protected:
	.protected _glob_ext
	.protected _globfree_ext

//   protected:
	.protected getloginx
	.protected getloginx_r
	.protected __posix_getloginx_r

//   protected:
	.protected __cxa_atexit
	.protected __cxa_finalize

//   protected:
	.protected pipe2
	.protected dup3
	.protected mkostemp
	.protected mkostemps

//   protected:
	.protected assfail3

//   protected:
	.protected posix_spawn_pipe_np

//   protected:
	.protected timegm

//   global:
	.global _nl_domain_bindings
	.global _nl_msg_cat_cntr

//	.global dl_iterate_phdr

//   protected:

//	.protected __align_cpy_1

	.protected addrtosymstr
	.protected aio_cancel
	.protected aiocancel
	.protected aio_error
	.protected aio_fsync
	.protected aio_read
	.protected aioread
	.protected aio_return
	.protected aio_suspend
	.protected aiowait
	.protected aio_waitn
	.protected aio_write
	.protected aiowrite
	.protected asprintf
	.protected assfail
	.protected backtrace
	.protected backtrace_symbols
	.protected backtrace_symbols_fd
	.protected canonicalize_file_name
	.protected clearenv
	.protected clock_getres
	.protected clock_gettime
	.protected clock_nanosleep
	.protected clock_settime
	.protected daemon
	.protected dirfd
	.protected door_bind
	.protected door_call
	.protected door_create
	.protected door_cred
	.protected door_getparam
	.protected door_info
	.protected door_return
	.protected door_revoke
	.protected door_server_create
	.protected door_setparam
	.protected door_ucred
	.protected door_unbind
	.protected door_xcreate
	.protected err
	.protected errx
	.protected faccessat
	.protected fchmodat
	.protected fcloseall
	.protected fdatasync
	.protected ffsl
	.protected ffsll
	.protected fgetattr
	.protected fls
	.protected flsl
	.protected flsll
	.protected forkallx
	.protected forkx
	.protected fsetattr
	.protected getattrat
	.protected getdelim
	.protected getline
	.protected get_nprocs
	.protected get_nprocs_conf
	.protected getprogname
	.protected htonl
	.protected htonll
	.protected htons
	.protected linkat
	.protected lio_listio
	.protected memmem
	.protected mkdirat
	.protected mkdtemp
	.protected mkfifoat
	.protected mknodat
	.protected mkstemps
	.protected mmapobj
	.protected mq_close
	.protected mq_getattr
	.protected mq_notify
	.protected mq_open
	.protected mq_receive
	.protected mq_reltimedreceive_np
	.protected mq_reltimedsend_np
	.protected mq_send
	.protected mq_setattr
	.protected mq_timedreceive
	.protected mq_timedsend
	.protected mq_unlink
	.protected nanosleep
	.protected ntohl
	.protected ntohll
	.protected ntohs
	.protected posix_fadvise
	.protected posix_fallocate
	.protected posix_madvise
	.protected posix_memalign
	.protected posix_spawn_file_actions_addclosefrom_np
	.protected posix_spawnattr_getsigignore_np
	.protected posix_spawnattr_setsigignore_np
	.protected ppoll
	.protected priv_basicset
	.protected pthread_key_create_once_np
	.protected pthread_mutexattr_getrobust
	.protected pthread_mutexattr_setrobust
	.protected pthread_mutex_consistent
	.protected readlinkat
	.protected sched_getparam
	.protected sched_get_priority_max
	.protected sched_get_priority_min
	.protected sched_getscheduler
	.protected sched_rr_get_interval
	.protected sched_setparam
	.protected sched_setscheduler
	.protected sched_yield
	.protected sem_close
	.protected sem_destroy
	.protected sem_getvalue
	.protected sem_init
	.protected sem_open
	.protected sem_post
	.protected sem_reltimedwait_np
	.protected sem_timedwait
	.protected sem_trywait
	.protected sem_unlink
	.protected sem_wait
	.protected setattrat
	.protected setprogname
	.protected _sharefs
	.protected shm_open
	.protected shm_unlink
	.protected sigqueue
	.protected sigtimedwait
	.protected sigwaitinfo
	.protected smt_pause
	.protected stpcpy
	.protected stpncpy
	.protected strcasestr
	.protected strchrnul
	.protected strndup
	.protected strnlen
	.protected strnstr
	.protected strsep
	.protected symlinkat
	.protected thr_keycreate_once
	.protected timer_create
	.protected timer_delete
	.protected timer_getoverrun
	.protected timer_gettime
	.protected timer_settime
	.protected u8_strcmp
	.protected u8_validate
	.protected uconv_u16tou32
	.protected uconv_u16tou8
	.protected uconv_u32tou16
	.protected uconv_u32tou8
	.protected uconv_u8tou16
	.protected uconv_u8tou32
	.protected vasprintf
	.protected verr
	.protected verrx
	.protected vforkx
	.protected vwarn
	.protected vwarnx
	.protected warn
	.protected warnx
	.protected wcpcpy
	.protected wcpncpy
	.protected wcscasecmp
	.protected wcsdup
	.protected wcsncasecmp
	.protected wcsnlen

//	.protected aio_cancel64
//	.protected aio_error64
//	.protected aio_fsync64
//	.protected aio_read64
//	.protected aioread64
//	.protected aio_return64
//	.protected aio_suspend64
//	.protected aio_waitn64
//	.protected aio_write64
//	.protected aiowrite64
//	.protected lio_listio64
//	.protected mkstemps64
//	.protected posix_fadvise64
//	.protected posix_fallocate64

//   protected:
	.protected futimens
	.protected utimensat

//   protected:
	.protected getpagesizes2

//   protected:

//   protected:
	.protected mutex_consistent
	.protected u8_textprep_str
	.protected uucopy
	.protected uucopystr

//   protected:
	.protected is_system_labeled
	.protected ucred_getlabel
	.protected _ucred_getlabel

//   protected:
//	.protected atomic_add_8
//	.protected atomic_add_8_nv
//	.protected atomic_add_char
////	.protected atomic_add_char_nv
//	.protected atomic_add_int
//	.protected atomic_add_int_nv
//	.protected atomic_add_ptr
//	.protected atomic_add_ptr_nv
//	.protected atomic_add_short
////	.protected atomic_add_short_nv
//	.protected atomic_and_16
//	.protected atomic_and_16_nv
//	.protected atomic_and_32_nv
//	.protected atomic_and_64
//	.protected atomic_and_64_nv
//	.protected atomic_and_8
//	.protected atomic_and_8_nv
//	.protected atomic_and_uchar
//	.protected atomic_and_uchar_nv
//	.protected atomic_and_uint_nv
//	.protected atomic_and_ulong
//	.protected atomic_and_ulong_nv
//	.protected atomic_and_ushort
////	.protected atomic_and_ushort_nv
//	.protected atomic_cas_16
//	.protected atomic_cas_32
//	.protected atomic_cas_64
//	.protected atomic_cas_8
//	.protected atomic_cas_ptr
//	.protected atomic_cas_uchar
//	.protected atomic_cas_uint
//	.protected atomic_cas_ulong
//	.protected atomic_cas_ushort
//	.protected atomic_clear_long_excl
//	.protected atomic_dec_16
//	.protected atomic_dec_16_nv
//	.protected atomic_dec_32
////	.protected atomic_dec_32_nv
//	.protected atomic_dec_64
////	.protected atomic_dec_64_nv
//	.protected atomic_dec_8
////	.protected atomic_dec_8_nv
//	.protected atomic_dec_uchar
////	.protected atomic_dec_uchar_nv
//	.protected atomic_dec_uint
//	.protected atomic_dec_uint_nv
//	.protected atomic_dec_ulong
////	.protected atomic_dec_ulong_nv
//	.protected atomic_dec_ushort
//	.protected atomic_dec_ushort_nv
//	.protected atomic_inc_16
////	.protected atomic_inc_16_nv
//	.protected atomic_inc_32
////	.protected atomic_inc_32_nv
//	.protected atomic_inc_64
//	.protected atomic_inc_64_nv
//	.protected atomic_inc_8
//	.protected atomic_inc_8_nv
//	.protected atomic_inc_uchar
////	.protected atomic_inc_uchar_nv
//	.protected atomic_inc_uint
////	.protected atomic_inc_uint_nv
//	.protected atomic_inc_ulong
////	.protected atomic_inc_ulong_nv
//	.protected atomic_inc_ushort
////	.protected atomic_inc_ushort_nv
//	.protected atomic_or_16
////	.protected atomic_or_16_nv
////	.protected atomic_or_32_nv
//	.protected atomic_or_64
//	.protected atomic_or_64_nv
//	.protected atomic_or_8
//	.protected atomic_or_8_nv
//	.protected atomic_or_uchar
//	.protected atomic_or_uchar_nv
//	.protected atomic_or_uint_nv
//	.protected atomic_or_ulong
////	.protected atomic_or_ulong_nv
//	.protected atomic_or_ushort
//	.protected atomic_or_ushort_nv
//	.protected atomic_set_long_excl
//	.protected atomic_swap_16
//	.protected atomic_swap_32
//	.protected atomic_swap_64
//	.protected atomic_swap_8
//	.protected atomic_swap_ptr
//	.protected atomic_swap_uchar
//	.protected atomic_swap_uint
//	.protected atomic_swap_ulong
//	.protected atomic_swap_ushort
//	.protected membar_consumer
//	.protected membar_enter
//	.protected membar_exit
//	.protected membar_producer

//	.protected enable_extended_FILE_stdio

//	.protected atomic_and_64_nv
////	.protected atomic_dec_64_nv
//	.protected atomic_inc_64_nv
//	.protected atomic_or_64_nv
//	.protected atomic_add_8_nv
//	.protected atomic_and_8_nv
//	.protected atomic_and_16_nv
//	.protected atomic_and_32_nv
//	.protected atomic_and_64_nv
////	.protected atomic_dec_8_nv
//	.protected atomic_dec_16_nv
////	.protected atomic_dec_32_nv
////	.protected atomic_dec_64_nv
//	.protected atomic_inc_8_nv
////	.protected atomic_inc_16_nv
////	.protected atomic_inc_32_nv
//	.protected atomic_inc_64_nv
//	.protected atomic_or_8_nv
////	.protected atomic_or_16_nv
////	.protected atomic_or_32_nv
//	.protected atomic_or_64_nv

//   global:
	.global dladdr
	.global dladdr1
	.global dlclose
	.global dldump
	.global dlerror
	.global dlinfo
	.global dlmopen
	.global dlopen
	.global dlsym
	.global dladdr
	.global dladdr1
	.global dlclose
	.global dldump
	.global dlerror
	.global dlinfo
	.global dlmopen
	.global dlopen
	.global dlsym
	.global dladdr
	.global dladdr1
//	.global dlamd64getunwind
	.global dlclose
	.global dldump
	.global dlerror
	.global dlinfo
	.global dlmopen
	.global dlopen
	.global dlsym

//   protected:
	.protected alphasort
	.protected _alphasort
//	.protected atomic_add_16
////	.protected atomic_add_16_nv
//	.protected atomic_add_32
//	.protected atomic_add_32_nv
//	.protected atomic_add_64
//	.protected atomic_add_64_nv
//	.protected atomic_add_long
////	.protected atomic_add_long_nv
//	.protected atomic_and_32
//	.protected atomic_and_uint
//	.protected atomic_or_32
//	.protected atomic_or_uint
	.protected _Exit
	.protected getisax
	.protected _getisax
	.protected getopt_clip
	.protected _getopt_clip
	.protected getopt_long
	.protected _getopt_long
	.protected getopt_long_only
	.protected _getopt_long_only
	.protected getpeerucred
	.protected _getpeerucred
	.protected getpflags
	.protected _getpflags
	.protected getppriv
	.protected _getppriv
	.protected getprivimplinfo
	.protected _getprivimplinfo
	.protected getzoneid
	.protected getzoneidbyname
	.protected getzonenamebyid
	.protected imaxabs
	.protected imaxdiv
	.protected isblank
	.protected iswblank
	.protected port_alert
	.protected port_associate
	.protected port_create
	.protected port_dissociate
	.protected port_get
	.protected port_getn
	.protected port_send
	.protected port_sendn
	.protected posix_openpt
	.protected posix_spawn
	.protected posix_spawnattr_destroy
	.protected posix_spawnattr_getflags
	.protected posix_spawnattr_getpgroup
	.protected posix_spawnattr_getschedparam
	.protected posix_spawnattr_getschedpolicy
	.protected posix_spawnattr_getsigdefault
	.protected posix_spawnattr_getsigmask
	.protected posix_spawnattr_init
	.protected posix_spawnattr_setflags
	.protected posix_spawnattr_setpgroup
	.protected posix_spawnattr_setschedparam
	.protected posix_spawnattr_setschedpolicy
	.protected posix_spawnattr_setsigdefault
	.protected posix_spawnattr_setsigmask
	.protected posix_spawn_file_actions_addclose
	.protected posix_spawn_file_actions_adddup2
	.protected posix_spawn_file_actions_addopen
	.protected posix_spawn_file_actions_destroy
	.protected posix_spawn_file_actions_init
	.protected posix_spawnp
	.protected priv_addset
	.protected _priv_addset
	.protected priv_allocset
	.protected _priv_allocset
	.protected priv_copyset
	.protected _priv_copyset
	.protected priv_delset
	.protected _priv_delset
	.protected priv_emptyset
	.protected _priv_emptyset
	.protected priv_fillset
	.protected _priv_fillset
	.protected __priv_free_info
	.protected priv_freeset
	.protected _priv_freeset
	.protected priv_getbyname
	.protected _priv_getbyname
	.protected __priv_getbyname
	.protected priv_getbynum
	.protected _priv_getbynum
	.protected __priv_getbynum
	.protected __priv_getdata
	.protected priv_getsetbyname
	.protected _priv_getsetbyname
	.protected __priv_getsetbyname
	.protected priv_getsetbynum
	.protected _priv_getsetbynum
	.protected __priv_getsetbynum
	.protected priv_gettext
	.protected _priv_gettext
	.protected priv_ineffect
	.protected _priv_ineffect
	.protected priv_intersect
	.protected _priv_intersect
	.protected priv_inverse
	.protected _priv_inverse
	.protected priv_isemptyset
	.protected _priv_isemptyset
	.protected priv_isequalset
	.protected _priv_isequalset
	.protected priv_isfullset
	.protected _priv_isfullset
	.protected priv_ismember
	.protected _priv_ismember
	.protected priv_issubset
	.protected _priv_issubset
	.protected __priv_parse_info
	.protected priv_set
	.protected _priv_set
	.protected priv_set_to_str
	.protected _priv_set_to_str
	.protected __priv_set_to_str
	.protected priv_str_to_set
	.protected _priv_str_to_set
	.protected priv_union
	.protected _priv_union
	.protected pselect
	.protected pthread_attr_getstack
	.protected pthread_attr_setstack
	.protected pthread_barrierattr_destroy
	.protected pthread_barrierattr_getpshared
	.protected pthread_barrierattr_init
	.protected pthread_barrierattr_setpshared
	.protected pthread_barrier_destroy
	.protected pthread_barrier_init
	.protected pthread_barrier_wait
	.protected pthread_condattr_getclock
	.protected pthread_condattr_setclock
	.protected pthread_mutexattr_getrobust_np
	.protected pthread_mutexattr_setrobust_np
	.protected pthread_mutex_consistent_np
	.protected pthread_mutex_reltimedlock_np
	.protected pthread_mutex_timedlock
	.protected pthread_rwlock_reltimedrdlock_np
	.protected pthread_rwlock_reltimedwrlock_np
	.protected pthread_rwlock_timedrdlock
	.protected pthread_rwlock_timedwrlock
	.protected pthread_setschedprio
	.protected pthread_spin_destroy
	.protected pthread_spin_init
	.protected pthread_spin_lock
	.protected pthread_spin_trylock
	.protected pthread_spin_unlock
	.protected rctlblk_set_recipient_pid
	.protected scandir
	.protected _scandir
	.protected schedctl_exit
	.protected schedctl_init
	.protected schedctl_lookup
	.protected sema_reltimedwait
	.protected sema_timedwait
	.protected setenv
	.protected setpflags
	.protected _setpflags
	.protected setppriv
	.protected _setppriv
	.protected strerror_r
	.protected strtof
	.protected strtoimax
	.protected strtold
	.protected strtoumax
	.protected ucred_free
	.protected _ucred_free
	.protected ucred_get
	.protected _ucred_get
	.protected ucred_getegid
	.protected _ucred_getegid
	.protected ucred_geteuid
	.protected _ucred_geteuid
	.protected ucred_getgroups
	.protected _ucred_getgroups
	.protected ucred_getpflags
	.protected _ucred_getpflags
	.protected ucred_getpid
	.protected _ucred_getpid
	.protected ucred_getprivset
	.protected _ucred_getprivset
	.protected ucred_getprojid
	.protected _ucred_getprojid
	.protected ucred_getrgid
	.protected _ucred_getrgid
	.protected ucred_getruid
	.protected _ucred_getruid
	.protected ucred_getsgid
	.protected _ucred_getsgid
	.protected ucred_getsuid
	.protected _ucred_getsuid
	.protected ucred_getzoneid
	.protected _ucred_getzoneid
	.protected ucred_size
	.protected _ucred_size
	.protected unsetenv
	.protected wcstof
	.protected wcstoimax
	.protected wcstold
	.protected wcstoll
	.protected wcstoull
	.protected wcstoumax

//	.protected alphasort64
//	.protected _alphasort64
//	.protected pselect_large_fdset
//	.protected scandir64
//	.protected _scandir64

	.protected walkcontext

////	.protected atomic_add_16_nv
//	.protected atomic_add_32_nv
//	.protected atomic_add_64_nv

//	.protected atomic_add_64_nv

//	.protected _SUNW_Unwind_DeleteException
////	.protected _SUNW_Unwind_ForcedUnwind
//	.protected _SUNW_Unwind_GetCFA
//	.protected _SUNW_Unwind_GetGR
////	.protected _SUNW_Unwind_GetIP
////	.protected _SUNW_Unwind_GetLanguageSpecificData
////	.protected _SUNW_Unwind_GetRegionStart
//	.protected _SUNW_Unwind_RaiseException
////	.protected _SUNW_Unwind_Resume
//	.protected _SUNW_Unwind_SetGR
////	.protected _SUNW_Unwind_SetIP
//	.protected _UA_CLEANUP_PHASE
//	.protected _UA_FORCE_UNWIND
//	.protected _UA_HANDLER_FRAME
//	.protected _UA_SEARCH_PHASE
//	.protected _Unwind_DeleteException
//	.protected _Unwind_ForcedUnwind
//	.protected _Unwind_GetCFA
//	.protected _Unwind_GetGR
//	.protected _Unwind_GetIP
//	.protected _Unwind_GetLanguageSpecificData
//	.protected _Unwind_GetRegionStart
//	.protected _Unwind_RaiseException
//	.protected _Unwind_Resume
//	.protected _Unwind_SetGR
//	.protected _Unwind_SetIP

//   protected:
	.protected forkall

//   protected:
	.protected getustack
	.protected _getustack
	.protected setustack
	.protected _setustack
	.protected stack_getbounds
	.protected _stack_getbounds
	.protected _stack_grow
	.protected stack_inbounds
	.protected _stack_inbounds
	.protected stack_setbounds
	.protected _stack_setbounds
	.protected stack_violation
	.protected _stack_violation

//	.protected __makecontext_v2
////	.protected ___makecontext_v2

//   protected:
	.protected crypt_gensalt

//   protected:
	.protected attropen
	.protected _attropen
	.protected bind_textdomain_codeset
	.protected closefrom
	.protected _closefrom
	.protected cond_reltimedwait
	.protected dcngettext
	.protected dngettext
	.protected fchownat
	.protected _fchownat
	.protected fdopendir
	.protected _fdopendir
	.protected fdwalk
	.protected _fdwalk
	.protected fstatat
	.protected _fstatat
	.protected futimesat
	.protected _futimesat
	.protected getcpuid
	.protected _getcpuid
	.protected gethomelgroup
	.protected _gethomelgroup
	.protected getpagesizes
	.protected getrctl
	.protected _getrctl
	.protected issetugid
	.protected _issetugid
	.protected _lwp_cond_reltimedwait
	.protected meminfo
	.protected _meminfo
	.protected ngettext
	.protected openat
	.protected _openat
	.protected printstack
	.protected priocntl
	.protected priocntlset
	.protected pset_getattr
	.protected pset_getloadavg
	.protected pset_list
	.protected pset_setattr
	.protected pthread_cond_reltimedwait_np
	.protected rctlblk_get_enforced_value
	.protected rctlblk_get_firing_time
	.protected rctlblk_get_global_action
	.protected rctlblk_get_global_flags
	.protected rctlblk_get_local_action
	.protected rctlblk_get_local_flags
	.protected rctlblk_get_privilege
	.protected rctlblk_get_recipient_pid
	.protected rctlblk_get_value
	.protected rctlblk_set_local_action
	.protected rctlblk_set_local_flags
	.protected rctlblk_set_privilege
	.protected rctlblk_set_value
	.protected rctlblk_size
	.protected rctl_walk
	.protected renameat
	.protected setrctl
	.protected _setrctl
	.protected unlinkat
	.protected _unlinkat
	.protected vfscanf
	.protected _vfscanf
	.protected vfwscanf
	.protected vscanf
	.protected _vscanf
	.protected vsscanf
	.protected _vsscanf
	.protected vswscanf
	.protected vwscanf

	.protected walkcontext

////	.protected attropen64
////	.protected _attropen64
//	.protected fstatat64
////	.protected _fstatat64
//	.protected openat64
//	.protected _openat64

//   protected:
	.protected semtimedop
	.protected _semtimedop

//   protected:
	.protected getacct
	.protected _getacct
	.protected getprojid
	.protected _getprojid
	.protected gettaskid
	.protected _gettaskid
	.protected msgids
	.protected _msgids
	.protected msgsnap
	.protected _msgsnap
	.protected putacct
	.protected _putacct
	.protected semids
	.protected _semids
	.protected settaskid
	.protected _settaskid
	.protected shmids
	.protected _shmids
	.protected wracct
	.protected _wracct

//   protected:
	.protected getextmntent
	.protected resetmnttab

//   protected:
	.protected strlcat
	.protected strlcpy
	.protected umount2
	.protected _umount2

//   protected:
	.protected __fsetlocking

//   protected:
	.protected btowc
	.protected __fbufsize
	.protected __flbf
	.protected _flushlbf
	.protected __fpending
	.protected __fpurge
	.protected __freadable
	.protected __freading
	.protected fwide
	.protected fwprintf
	.protected __fwritable
	.protected __fwriting
	.protected fwscanf
	.protected getloadavg
	.protected isaexec
	.protected mbrlen
	.protected mbrtowc
	.protected mbsinit
	.protected mbsrtowcs
	.protected pcsample
	.protected pthread_attr_getguardsize
	.protected pthread_attr_setguardsize
	.protected pthread_getconcurrency
	.protected pthread_mutexattr_gettype
	.protected pthread_mutexattr_settype
	.protected pthread_rwlockattr_destroy
	.protected pthread_rwlockattr_getpshared
	.protected pthread_rwlockattr_init
	.protected pthread_rwlockattr_setpshared
	.protected pthread_rwlock_destroy
	.protected pthread_rwlock_init
	.protected pthread_rwlock_rdlock
	.protected pthread_rwlock_tryrdlock
	.protected pthread_rwlock_trywrlock
	.protected pthread_rwlock_unlock
	.protected pthread_rwlock_wrlock
	.protected pthread_setconcurrency
	.protected swprintf
	.protected swscanf
	.protected __sysconf_xpg5
	.protected vfwprintf
	.protected vswprintf
	.protected vwprintf
	.protected wcrtomb
	.protected wcsrtombs
	.protected wcsstr
	.protected wctob
	.protected wmemchr
	.protected wmemcmp
	.protected wmemcpy
	.protected wmemmove
	.protected wmemset
	.protected wprintf
	.protected wscanf

//	.protected select_large_fdset


//   global:
	.global __loc1
//   protected:
	.protected basename
	.protected bindtextdomain
	.protected bsd_signal
	.protected dbm_clearerr
	.protected dbm_error
	.protected dcgettext
	.protected dgettext
	.protected directio
	.protected dirname
	.protected endusershell
	.protected _exithandle
	.protected fgetwc
	.protected fgetws
	.protected fpgetround
	.protected fpsetround
	.protected fputwc
	.protected fputws
	.protected fseeko
	.protected ftello
	.protected ftrylockfile
	.protected getexecname
	.protected _getexecname
	.protected getpassphrase
	.protected gettext
	.protected getusershell
	.protected getwc
	.protected getwchar
	.protected getws
	.protected isenglish
	.protected isideogram
	.protected isnumber
	.protected isphonogram
	.protected isspecial
	.protected iswalnum
	.protected iswalpha
	.protected iswcntrl
	.protected iswctype
	.protected iswdigit
	.protected iswgraph
	.protected iswlower
	.protected iswprint
	.protected iswpunct
	.protected iswspace
	.protected iswupper
	.protected iswxdigit
	.protected ____loc1
	.protected _longjmp
	.protected _lwp_sema_trywait
	.protected ntp_adjtime
	.protected _ntp_adjtime
	.protected ntp_gettime
	.protected _ntp_gettime
	.protected __posix_asctime_r
	.protected __posix_ctime_r
	.protected __posix_getgrgid_r
	.protected __posix_getgrnam_r
	.protected __posix_getlogin_r
	.protected __posix_getpwnam_r
	.protected __posix_getpwuid_r
	.protected __posix_sigwait
	.protected __posix_ttyname_r
	.protected pset_assign
	.protected pset_bind
	.protected pset_create
	.protected pset_destroy
	.protected pset_info
	.protected pthread_atfork
	.protected pthread_attr_destroy
	.protected pthread_attr_getdetachstate
	.protected pthread_attr_getinheritsched
	.protected pthread_attr_getschedparam
	.protected pthread_attr_getschedpolicy
	.protected pthread_attr_getscope
	.protected pthread_attr_getstackaddr
	.protected pthread_attr_getstacksize
	.protected pthread_attr_init
	.protected pthread_attr_setdetachstate
	.protected pthread_attr_setinheritsched
	.protected pthread_attr_setschedparam
	.protected pthread_attr_setschedpolicy
	.protected pthread_attr_setscope
	.protected pthread_attr_setstackaddr
	.protected pthread_attr_setstacksize
	.protected pthread_cancel
	.protected __pthread_cleanup_pop
	.protected __pthread_cleanup_push
	.protected pthread_create
	.protected pthread_detach
	.protected pthread_equal
	.protected pthread_exit
	.protected pthread_getschedparam
	.protected pthread_getspecific
	.protected pthread_join
	.protected pthread_key_create
	.protected pthread_key_delete
	.protected pthread_kill
	.protected pthread_once
	.protected pthread_self
	.protected pthread_setcancelstate
	.protected pthread_setcanceltype
	.protected pthread_setschedparam
	.protected pthread_setspecific
	.protected pthread_sigmask
	.protected pthread_testcancel
	.protected putwc
	.protected putwchar
	.protected putws
	.protected regcmp
	.protected regex
	.protected resolvepath
	.protected _resolvepath
	.protected rwlock_destroy
	.protected _rwlock_destroy
	.protected sema_destroy
	.protected _sema_destroy
	.protected _setjmp
	.protected setusershell
	.protected siginterrupt
	.protected sigstack
	.protected snprintf
	.protected strtows
	.protected sync_instruction_memory
	.protected textdomain
	.protected thr_main
	.protected towctrans
	.protected towlower
	.protected towupper
	.protected ungetwc
	.protected vsnprintf
	.protected watoll
	.protected wcscat
	.protected wcschr
	.protected wcscmp
	.protected wcscoll
	.protected wcscpy
	.protected wcscspn
	.protected wcsftime
	.protected wcslen
	.protected wcsncat
	.protected wcsncmp
	.protected wcsncpy
	.protected wcspbrk
	.protected wcsrchr
	.protected wcsspn
	.protected wcstod
	.protected wcstok
	.protected wcstol
	.protected wcstoul
	.protected wcswcs
	.protected wcswidth
	.protected wcsxfrm
	.protected wctrans
	.protected wctype
	.protected wcwidth
	.protected wscasecmp
	.protected wscat
	.protected wschr
	.protected wscmp
	.protected wscol
	.protected wscoll
	.protected wscpy
	.protected wscspn
	.protected wsdup
	.protected wslen
	.protected wsncasecmp
	.protected wsncat
	.protected wsncmp
	.protected wsncpy
	.protected wspbrk
	.protected wsprintf
	.protected wsrchr
	.protected wsscanf
	.protected wsspn
	.protected wstod
	.protected wstok
	.protected wstol
	.protected wstoll
	.protected wstostr
	.protected wsxfrm
	.protected __xpg4_putmsg
	.protected __xpg4_putpmsg

//	.protected creat64
////	.protected _creat64
//	.protected fgetpos64
//	.protected fopen64
////	.protected freopen64
//	.protected fseeko64
//	.protected fsetpos64
//	.protected fstat64
//	.protected _fstat64
////	.protected fstatvfs64
////	.protected _fstatvfs64
//	.protected ftello64
//	.protected ftruncate64
//	.protected _ftruncate64
//	.protected ftw64
//	.protected _ftw64
//	.protected getdents64
////	.protected _getdents64
//	.protected getrlimit64
//	.protected _getrlimit64
//	.protected lockf64
//	.protected _lockf64
//	.protected lseek64
////	.protected _lseek64
//	.protected lstat64
//	.protected _lstat64
//	.protected mkstemp64
//	.protected _mkstemp64
//	.protected mmap64
////	.protected _mmap64
//	.protected nftw64
////	.protected _nftw64
//	.protected open64
////	.protected _open64
//	.protected __posix_readdir_r
//	.protected pread64
////	.protected _pread64
//	.protected pwrite64
////	.protected _pwrite64
//	.protected readdir64
//	.protected _readdir64
//	.protected readdir64_r
////	.protected _readdir64_r
//	.protected setrlimit64
//	.protected _setrlimit64
//	.protected s_fcntl
//	.protected _s_fcntl
//	.protected s_ioctl
//	.protected stat64
//	.protected _stat64
//	.protected statvfs64
////	.protected _statvfs64
//	.protected tell64
////	.protected _tell64
//	.protected tmpfile64
//	.protected truncate64
//	.protected _truncate64
////	.protected _xftw64

	.protected __flt_rounds

//   protected:
	.protected acl
	.protected bcmp
	.protected bcopy
	.protected bzero
	.protected facl
	.protected ftime
	.protected getdtablesize
	.protected gethostid
	.protected gethostname
	.protected getpagesize
	.protected getpriority
	.protected getrusage
	.protected getwd
	.protected index
	.protected initstate
	.protected killpg
	.protected _nsc_trydoorcall
	.protected pthread_condattr_destroy
	.protected pthread_condattr_getpshared
	.protected pthread_condattr_init
	.protected pthread_condattr_setpshared
	.protected pthread_cond_broadcast
	.protected pthread_cond_destroy
	.protected pthread_cond_init
	.protected pthread_cond_signal
	.protected pthread_cond_timedwait
	.protected pthread_cond_wait
	.protected pthread_mutexattr_destroy
	.protected pthread_mutexattr_getprioceiling
	.protected pthread_mutexattr_getprotocol
	.protected pthread_mutexattr_getpshared
	.protected pthread_mutexattr_init
	.protected pthread_mutexattr_setprioceiling
	.protected pthread_mutexattr_setprotocol
	.protected pthread_mutexattr_setpshared
	.protected pthread_mutex_destroy
	.protected pthread_mutex_getprioceiling
	.protected pthread_mutex_init
	.protected pthread_mutex_lock
	.protected pthread_mutex_setprioceiling
	.protected pthread_mutex_trylock
	.protected pthread_mutex_unlock
	.protected random
	.protected reboot
	.protected re_comp
	.protected re_exec
	.protected rindex
	.protected setbuffer
	.protected sethostname
	.protected setlinebuf
	.protected setpriority
	.protected setregid
	.protected setreuid
	.protected setstate
	.protected srandom
	.protected thr_min_stack
	.protected thr_stksegment
	.protected ualarm
	.protected usleep
	.protected wait3
	.protected wait4

//   global:
	.global __xpg4
//   protected:
	.protected addsev
	.protected cond_broadcast
	.protected cond_destroy
	.protected cond_init
	.protected cond_signal
	.protected cond_timedwait
	.protected cond_wait
	.protected confstr
	.protected fnmatch
	.protected _getdate_err_addr
	.protected glob
	.protected globfree
	.protected iconv
	.protected iconv_close
	.protected iconv_open
	.protected lfmt
	.protected mutex_destroy
	.protected mutex_init
	.protected mutex_lock
	.protected mutex_trylock
	.protected mutex_unlock
	.protected pfmt
	.protected regcomp
	.protected regerror
	.protected regexec
	.protected regfree
	.protected rwlock_init
	.protected rw_rdlock
	.protected rw_read_held
	.protected rw_tryrdlock
	.protected rw_trywrlock
	.protected rw_unlock
	.protected rw_write_held
	.protected rw_wrlock
	.protected sema_held
	.protected sema_init
	.protected sema_post
	.protected sema_trywait
	.protected sema_wait
	.protected setcat
	.protected sigfpe
	.protected strfmon
	.protected strptime
	.protected thr_continue
	.protected thr_create
	.protected thr_exit
	.protected thr_getconcurrency
	.protected thr_getprio
	.protected thr_getspecific
	.protected thr_join
	.protected thr_keycreate
	.protected thr_kill
	.protected thr_self
	.protected thr_setconcurrency
	.protected thr_setprio
	.protected thr_setspecific
	.protected thr_sigsetmask
	.protected thr_suspend
	.protected thr_yield
	.protected vlfmt
	.protected vpfmt
	.protected wordexp
	.protected wordfree

//   global:
	.global altzone
	.global _ctype
//	.global isnanf
	.global lone
	.global lten
	.global lzero
	.global memalign
//	.global modff
	.global nss_default_finders
	.global _sibuf
	.global _sobuf
	.global _sys_buslist
	.global _sys_cldlist
	.global _sys_fpelist
	.global _sys_illlist
	.global _sys_segvlist
	.global _sys_siginfolistp
	.global _sys_siglist
	.global _sys_siglistn
	.global _sys_siglistp
	.global _sys_traplist
	.global valloc

//	.global _bufendtab
//	.global _lastbuf
//	.global sys_errlist
//	.global sys_nerr
	.global _sys_nsig

//   protected:
	.protected a64l
	.protected adjtime
	.protected ascftime
	.protected _assert
	.protected atoll
	.protected brk
//	.protected __builtin_alloca
	.protected cftime
	.protected closelog
	.protected csetcol
	.protected csetlen
	.protected ctermid_r
	.protected dbm_close
	.protected dbm_delete
	.protected dbm_fetch
	.protected dbm_firstkey
	.protected dbm_nextkey
	.protected dbm_open
	.protected dbm_store
	.protected decimal_to_double
	.protected decimal_to_extended
	.protected decimal_to_quadruple
	.protected decimal_to_single
	.protected double_to_decimal
	.protected drand48
	.protected econvert
	.protected ecvt
	.protected endnetgrent
	.protected endspent
	.protected endutent
	.protected endutxent
	.protected erand48
	.protected euccol
	.protected euclen
	.protected eucscol
	.protected extended_to_decimal
	.protected fchroot
	.protected fconvert
	.protected fcvt
	.protected ffs
	.protected fgetspent
	.protected fgetspent_r
	.protected _filbuf
	.protected file_to_decimal
	.protected finite
	.protected _flsbuf
	.protected fork1
	.protected fpclass
	.protected fpgetmask
	.protected fpgetsticky
	.protected fpsetmask
	.protected fpsetsticky
	.protected fstatfs
	.protected ftruncate
	.protected ftw
	.protected func_to_decimal
	.protected gconvert
	.protected gcvt
	.protected getdents
	.protected gethrtime
	.protected gethrvtime
	.protected getmntany
	.protected getmntent
	.protected getnetgrent
	.protected getnetgrent_r
	.protected getpw
	.protected getspent
	.protected getspent_r
	.protected getspnam
	.protected getspnam_r
	.protected getutent
	.protected getutid
	.protected getutline
	.protected getutmp
	.protected getutmpx
	.protected getutxent
	.protected getutxid
	.protected getutxline
	.protected getvfsany
	.protected getvfsent
	.protected getvfsfile
	.protected getvfsspec
	.protected getwidth
	.protected gsignal
	.protected hasmntopt
	.protected innetgr
	.protected insque
	.protected _insque
	.protected jrand48
	.protected l64a
	.protected ladd
	.protected lckpwdf
	.protected lcong48
	.protected ldivide
	.protected lexp10
	.protected llabs
	.protected lldiv
	.protected llog10
	.protected llseek
	.protected lltostr
	.protected lmul
	.protected lrand48
	.protected lshiftl
	.protected lsub
	.protected _lwp_cond_broadcast
	.protected _lwp_cond_signal
	.protected _lwp_cond_timedwait
	.protected _lwp_cond_wait
	.protected _lwp_continue
	.protected _lwp_info
	.protected _lwp_kill
	.protected _lwp_mutex_lock
	.protected _lwp_mutex_trylock
	.protected _lwp_mutex_unlock
	.protected _lwp_self
	.protected _lwp_sema_init
	.protected _lwp_sema_post
	.protected _lwp_sema_wait
	.protected _lwp_suspend
	.protected madvise
	.protected __major
	.protected __makedev
	.protected mincore
	.protected __minor
	.protected mkstemp
	.protected _mkstemp
	.protected mlockall
	.protected mrand48
	.protected munlockall
	.protected _mutex_held
	.protected _mutex_lock
	.protected nrand48
	.protected _nss_netdb_aliases
	.protected _nss_XbyY_buf_alloc
	.protected _nss_XbyY_buf_free
	.protected __nsw_extended_action
	.protected __nsw_freeconfig
	.protected __nsw_getconfig
	.protected openlog
	.protected plock
	.protected p_online
	.protected pread
	.protected __priocntl
	.protected __priocntlset
	.protected processor_bind
	.protected processor_info
	.protected psiginfo
	.protected psignal
	.protected putpwent
	.protected putspent
	.protected pututline
	.protected pututxline
	.protected pwrite
	.protected qeconvert
	.protected qecvt
	.protected qfconvert
	.protected qfcvt
	.protected qgconvert
	.protected qgcvt
	.protected quadruple_to_decimal
	.protected realpath
	.protected remque
	.protected _remque
	.protected _rw_read_held
	.protected _rw_write_held
	.protected seconvert
	.protected seed48
	.protected select
	.protected _sema_held
	.protected setegid
	.protected seteuid
	.protected setlogmask
	.protected setnetgrent
	.protected setspent
	.protected settimeofday
	.protected setutent
	.protected setutxent
	.protected sfconvert
	.protected sgconvert
	.protected sig2str
	.protected sigwait
	.protected single_to_decimal
	.protected srand48
	.protected ssignal
	.protected statfs
	.protected str2sig
	.protected strcasecmp
	.protected string_to_decimal
	.protected strncasecmp
	.protected strsignal
	.protected strtoll
	.protected strtoull
	.protected swapctl
	.protected _syscall
	.protected sysfs
	.protected syslog
	.protected _syslog
	.protected tmpnam_r
	.protected truncate
	.protected ttyslot
	.protected uadmin
	.protected ulckpwdf
	.protected ulltostr
	.protected unordered
	.protected updwtmp
	.protected updwtmpx
	.protected ustat
	.protected utimes
	.protected utmpname
	.protected utmpxname
	.protected vfork
	.protected vhangup
	.protected vsyslog
	.protected yield

	.protected _syscall



//   global:
	.global errno
	.global _iob

//   protected:
	.protected addseverity
	.protected _addseverity
	.protected asctime_r
	.protected crypt
	.protected _crypt
	.protected ctime_r
	.protected encrypt
	.protected _encrypt
	.protected endgrent
	.protected endpwent
	.protected ___errno
	.protected fgetgrent
	.protected fgetgrent_r
	.protected fgetpwent
	.protected fgetpwent_r
	.protected flockfile
	.protected funlockfile
	.protected getchar_unlocked
	.protected getc_unlocked
	.protected getgrent
	.protected getgrent_r
	.protected getgrgid_r
	.protected getgrnam_r
	.protected getitimer
	.protected _getitimer
	.protected getlogin_r
	.protected getpwent
	.protected getpwent_r
	.protected getpwnam_r
	.protected getpwuid_r
	.protected gettimeofday
	.protected _gettimeofday
	.protected gmtime_r
	.protected localtime_r
	.protected putchar_unlocked
	.protected putc_unlocked
	.protected rand_r
	.protected readdir_r
	.protected setgrent
	.protected setitimer
	.protected _setitimer
	.protected setkey
	.protected _setkey
	.protected setpwent
	.protected strtok_r
	.protected sysinfo
	.protected _sysinfo
	.protected ttyname_r

//	.protected __div64
//	.protected __mul64
//	.protected __rem64
//	.protected __udiv64
//	.protected __urem64

//	.protected __dtoll
////	.protected __dtoull
//	.protected __ftoll
////	.protected __ftoull
//	.protected _Q_lltoq
//	.protected _Q_qtoll
//	.protected _Q_qtoull
//	.protected _Q_ulltoq
	.protected sbrk
	.protected _sbrk
//	.protected __umul64

//   global:
	.global _altzone
	.global calloc
	.global __ctype
	.global daylight
	.global _daylight
	.global environ
	.global _environ
	.global free
//	.global frexp
	.global getdate_err
	.global _getdate_err
	.global getenv
//	.global __huge_val
	.global __iob
//	.global isnan
//	.global _isnan
//	.global isnand
//	.global _isnand
//	.global ldexp
//	.global logb
	.global malloc
	.global memcmp
	.global memcpy
	.global memmove
	.global memset
//	.global modf
//	.global _modf
//	.global nextafter
//	.global _nextafter
	.global _numeric
	.global optarg
	.global opterr
	.global optind
	.global optopt
	.global realloc
//	.global scalb
//	.global _scalb
	.global timezone
	.global _timezone
	.global tzname
	.global _tzname
//	.global _fp_hw

//   protected:
	.protected abort
	.protected abs
	.protected access
	.protected _access
	.protected acct
	.protected _acct
	.protected alarm
	.protected _alarm
	.protected asctime
	.protected __assert
	.protected atexit
	.protected atof
	.protected atoi
	.protected atol
	.protected bsearch
	.protected catclose
	.protected _catclose
	.protected catgets
	.protected _catgets
	.protected catopen
	.protected _catopen
	.protected cfgetispeed
	.protected _cfgetispeed
	.protected cfgetospeed
	.protected _cfgetospeed
	.protected cfsetispeed
	.protected _cfsetispeed
	.protected cfsetospeed
	.protected _cfsetospeed
	.protected chdir
	.protected _chdir
	.protected chmod
	.protected _chmod
	.protected chown
	.protected _chown
	.protected chroot
	.protected _chroot
	.protected _cleanup
	.protected clearerr
	.protected clock
	.protected _close
	.protected close
	.protected closedir
	.protected _closedir
	.protected creat
	.protected _creat
	.protected ctermid
	.protected ctime
	.protected cuserid
	.protected _cuserid
	.protected difftime
	.protected div
	.protected dup
	.protected _dup
	.protected dup2
	.protected _dup2
	.protected execl
	.protected _execl
	.protected execle
	.protected _execle
	.protected execlp
	.protected _execlp
	.protected execv
	.protected _execv
	.protected execve
	.protected _execve
	.protected execvp
	.protected _execvp
	.protected exit
	.protected _exit
	.protected fattach
	.protected _fattach
	.protected fchdir
	.protected _fchdir
	.protected fchmod
	.protected _fchmod
	.protected fchown
	.protected _fchown
	.protected fclose
	.protected fcntl
	.protected _fcntl
	.protected fdetach
	.protected _fdetach
	.protected fdopen
	.protected _fdopen
	.protected feof
	.protected ferror
	.protected fflush
	.protected fgetc
	.protected fgetpos
	.protected fgets
	.protected __filbuf
	.protected fileno
	.protected _fileno
	.protected __flsbuf
	.protected fmtmsg
	.protected _fmtmsg
	.protected fopen
	.protected _fork
	.protected fork
	.protected fpathconf
	.protected _fpathconf
	.protected fprintf
	.protected fputc
	.protected fputs
	.protected fread
	.protected freopen
	.protected fscanf
	.protected fseek
	.protected fsetpos
	.protected fstat
	.protected _fstat
	.protected fstatvfs
	.protected _fstatvfs
	.protected fsync
	.protected _fsync
	.protected ftell
	.protected ftok
	.protected _ftok
	.protected fwrite
	.protected getc
	.protected getchar
	.protected getcontext
//	.protected _getcontext
	.protected getcwd
	.protected _getcwd
	.protected getdate
	.protected _getdate
	.protected getegid
	.protected _getegid
	.protected geteuid
	.protected _geteuid
	.protected getgid
	.protected _getgid
	.protected getgrgid
	.protected getgrnam
	.protected getgroups
	.protected _getgroups
	.protected getlogin
	.protected getmsg
	.protected _getmsg
	.protected getopt
	.protected _getopt
	.protected getpass
	.protected _getpass
	.protected getpgid
	.protected _getpgid
	.protected getpgrp
	.protected _getpgrp
	.protected getpid
	.protected _getpid
	.protected getpmsg
	.protected _getpmsg
	.protected getppid
	.protected _getppid
	.protected getpwnam
	.protected getpwuid
	.protected getrlimit
	.protected _getrlimit
	.protected gets
	.protected getsid
	.protected _getsid
	.protected getsubopt
	.protected _getsubopt
	.protected gettxt
	.protected _gettxt
	.protected getuid
	.protected _getuid
	.protected getw
	.protected _getw
	.protected gmtime
	.protected grantpt
	.protected _grantpt
	.protected hcreate
	.protected _hcreate
	.protected hdestroy
	.protected _hdestroy
	.protected hsearch
	.protected _hsearch
	.protected initgroups
	.protected _initgroups
	.protected ioctl
	.protected _ioctl
	.protected isalnum
	.protected isalpha
	.protected isascii
	.protected _isascii
	.protected isastream
	.protected _isastream
	.protected isatty
	.protected _isatty
	.protected iscntrl
	.protected isdigit
	.protected isgraph
	.protected islower
	.protected isprint
	.protected ispunct
	.protected isspace
	.protected isupper
	.protected isxdigit
	.protected kill
	.protected _kill
	.protected labs
	.protected lchown
	.protected _lchown
	.protected ldiv
	.protected lfind
	.protected _lfind
	.protected link
	.protected _link
	.protected localeconv
	.protected localtime
	.protected lockf
	.protected _lockf
	.protected longjmp
	.protected lsearch
	.protected _lsearch
	.protected lseek
	.protected _lseek
	.protected lstat
	.protected _lstat
	.protected makecontext
	.protected _makecontext
	.protected mblen
	.protected mbstowcs
	.protected mbtowc
	.protected memccpy
	.protected _memccpy
	.protected memchr
	.protected memcntl
	.protected _memcntl
	.protected mkdir
	.protected _mkdir
	.protected mkfifo
	.protected _mkfifo
	.protected mknod
	.protected _mknod
	.protected mktemp
	.protected _mktemp
	.protected mktime
	.protected mlock
	.protected _mlock
	.protected mmap
	.protected _mmap
	.protected monitor
	.protected _monitor
	.protected mount
	.protected _mount
	.protected mprotect
	.protected _mprotect
	.protected msgctl
	.protected _msgctl
	.protected msgget
	.protected _msgget
	.protected msgrcv
	.protected _msgrcv
	.protected msgsnd
	.protected _msgsnd
	.protected msync
	.protected _msync
	.protected munlock
	.protected _munlock
	.protected munmap
	.protected _munmap
	.protected nftw
	.protected _nftw
	.protected nice
	.protected _nice
	.protected nl_langinfo
	.protected _nl_langinfo
	.protected open
	.protected _open
	.protected opendir
	.protected _opendir
	.protected pathconf
	.protected _pathconf
	.protected pause
	.protected _pause
	.protected pclose
	.protected _pclose
	.protected perror
	.protected pipe
	.protected _pipe
	.protected poll
	.protected _poll
	.protected popen
	.protected _popen
	.protected printf
	.protected profil
	.protected _profil
	.protected ptsname
	.protected _ptsname
	.protected putc
	.protected putchar
	.protected putenv
	.protected _putenv
	.protected putmsg
	.protected _putmsg
	.protected putpmsg
	.protected _putpmsg
	.protected puts
	.protected putw
	.protected _putw
	.protected qsort
	.protected raise
	.protected rand
	.protected read
	.protected _read
	.protected readdir
	.protected _readdir
	.protected readlink
	.protected _readlink
	.protected readv
	.protected _readv
	.protected remove
	.protected rename
	.protected _rename
	.protected rewind
	.protected rewinddir
	.protected _rewinddir
	.protected rmdir
	.protected _rmdir
	.protected scanf
	.protected seekdir
	.protected _seekdir
	.protected semctl
	.protected _semctl
	.protected semget
	.protected _semget
	.protected semop
	.protected _semop
	.protected setbuf
	.protected setcontext
	.protected _setcontext
	.protected setgid
	.protected _setgid
	.protected setgroups
	.protected _setgroups
	.protected setjmp
	.protected setlabel
	.protected setlocale
	.protected setpgid
	.protected _setpgid
	.protected setpgrp
	.protected _setpgrp
	.protected setrlimit
	.protected _setrlimit
	.protected setsid
	.protected _setsid
	.protected setuid
	.protected _setuid
	.protected setvbuf
	.protected shmat
	.protected _shmat
	.protected shmctl
	.protected _shmctl
	.protected shmdt
	.protected _shmdt
	.protected shmget
	.protected _shmget
	.protected sigaction
	.protected _sigaction
	.protected sigaddset
	.protected _sigaddset
	.protected sigaltstack
	.protected _sigaltstack
	.protected sigdelset
	.protected _sigdelset
	.protected sigemptyset
	.protected _sigemptyset
	.protected sigfillset
	.protected _sigfillset
	.protected sighold
	.protected _sighold
	.protected sigignore
	.protected _sigignore
	.protected sigismember
	.protected _sigismember
	.protected siglongjmp
	.protected _siglongjmp
	.protected signal
	.protected sigpause
	.protected _sigpause
	.protected sigpending
	.protected _sigpending
	.protected sigprocmask
	.protected _sigprocmask
	.protected sigrelse
	.protected _sigrelse
	.protected sigsend
	.protected _sigsend
	.protected sigsendset
	.protected _sigsendset
	.protected sigset
	.protected _sigset
	.protected sigsetjmp
	.protected _sigsetjmp
	.protected sigsuspend
	.protected _sigsuspend
	.protected sleep
	.protected _sleep
	.protected sprintf
	.protected srand
	.protected sscanf
	.protected stat
	.protected _stat
	.protected statvfs
	.protected _statvfs
	.protected stime
	.protected _stime
	.protected strcat
	.protected strchr
	.protected strcmp
	.protected strcoll
	.protected strcpy
	.protected strcspn
	.protected strdup
	.protected _strdup
	.protected strerror
	.protected strftime
	.protected strlen
	.protected strncat
	.protected strncmp
	.protected strncpy
	.protected strpbrk
	.protected strrchr
	.protected strspn
	.protected strstr
	.protected strtod
	.protected strtok
	.protected strtol
	.protected strtoul
	.protected strxfrm
	.protected swab
	.protected _swab
	.protected swapcontext
//	.protected _swapcontext
	.protected symlink
	.protected _symlink
	.protected sync
	.protected _sync
	.protected sysconf
	.protected _sysconf
	.protected system
	.protected tcdrain
	.protected _tcdrain
	.protected tcflow
	.protected _tcflow
	.protected tcflush
	.protected _tcflush
	.protected tcgetattr
	.protected _tcgetattr
	.protected tcgetpgrp
	.protected _tcgetpgrp
	.protected tcgetsid
	.protected _tcgetsid
	.protected tcsendbreak
	.protected _tcsendbreak
	.protected tcsetattr
	.protected _tcsetattr
	.protected tcsetpgrp
	.protected _tcsetpgrp
	.protected tdelete
	.protected _tdelete
	.protected tell
	.protected _tell
	.protected telldir
	.protected _telldir
	.protected tempnam
	.protected _tempnam
	.protected tfind
	.protected _tfind
	.protected time
	.protected _time
	.protected times
	.protected _times
	.protected tmpfile
	.protected tmpnam
	.protected toascii
	.protected _toascii
	.protected tolower
	.protected _tolower
	.protected toupper
	.protected _toupper
	.protected tsearch
	.protected _tsearch
	.protected ttyname
	.protected twalk
	.protected _twalk
	.protected tzset
	.protected _tzset
	.protected ulimit
	.protected _ulimit
	.protected umask
	.protected _umask
	.protected umount
	.protected _umount
	.protected uname
	.protected _uname
	.protected ungetc
	.protected unlink
	.protected _unlink
	.protected unlockpt
	.protected _unlockpt
	.protected utime
	.protected _utime
	.protected vfprintf
	.protected vprintf
	.protected vsprintf
	.protected wait
	.protected _wait
	.protected waitid
	.protected _waitid
	.protected waitpid
	.protected _waitpid
	.protected wcstombs
	.protected wctomb
	.protected write
	.protected _write
	.protected writev
	.protected _writev
	.protected _xftw

	.protected ptrace
	.protected _ptrace

//	.protected _fxstat
//	.protected _lxstat
//	.protected nuname
//	.protected _nuname
//	.protected _xmknod
//	.protected _xstat

	.protected sbrk

//	.protected __dtou
//	.protected __ftou

//	.protected _Q_add
//	.protected _Q_cmp
//	.protected _Q_cmpe
//	.protected _Q_div
//	.protected _Q_dtoq
//	.protected _Q_feq
//	.protected _Q_fge
//	.protected _Q_fgt
//	.protected _Q_fle
//	.protected _Q_flt
//	.protected _Q_fne
//	.protected _Q_itoq
//	.protected _Q_mul
//	.protected _Q_neg
//	.protected _Q_qtod
//	.protected _Q_qtoi
//	.protected _Q_qtos
//	.protected _Q_qtou
//	.protected _Q_sqrt
//	.protected _Q_stoq
//	.protected _Q_sub
//	.protected _Q_utoq

//	.protected __align_cpy_1
//	.protected __align_cpy_16
//	.protected __align_cpy_2
//	.protected __align_cpy_4
//	.protected __align_cpy_8
//	.protected __dtoul
////	.protected __ftoul
//	.protected _Qp_add
//	.protected _Qp_cmp
////	.protected _Qp_cmpe
//	.protected _Qp_div
//	.protected _Qp_dtoq
//	.protected _Qp_feq
//	.protected _Qp_fge
//	.protected _Qp_fgt
//	.protected _Qp_fle
//	.protected _Qp_flt
//	.protected _Qp_fne
//	.protected _Qp_itoq
//	.protected _Qp_mul
//	.protected _Qp_neg
//	.protected _Qp_qtod
//	.protected _Qp_qtoi
//	.protected _Qp_qtos
//	.protected _Qp_qtoui
//	.protected _Qp_qtoux
//	.protected _Qp_qtox
//	.protected _Qp_sqrt
//	.protected _Qp_stoq
//	.protected _Qp_sub
//	.protected _Qp_uitoq
//	.protected _Qp_uxtoq
//	.protected _Qp_xtoq
//	.protected __sparc_utrap_install


//   global:
	.global __flt_rounds

//   protected:
	.protected _ctermid
	.protected _getgrgid
	.protected _getgrnam
	.protected _getlogin
	.protected _getpwnam
	.protected _getpwuid
	.protected _ttyname

	.protected _sbrk

//	.protected _fpstart
//	.protected __fpstart




//   global:
	.global ___Argv
	.global cfree
	.global _cswidth
	.global __ctype_mask
	.global __environ_lock
	.global __inf_read
	.global __inf_written
	.global __i_size
//	.global _isnanf
	.global __iswrune
	.global __libc_threaded
	.global _lib_version
//	.global _logb
	.global _lone
	.global _lten
	.global _lzero
	.global __malloc_lock
	.global _memcmp
	.global _memcpy
	.global _memmove
	.global _memset
//	.global _modff
	.global __nan_read
	.global __nan_written
	.global __nextwctype
	.global __nis_debug_bind
	.global __nis_debug_calls
	.global __nis_debug_file
	.global __nis_debug_rpc
	.global __nis_prefsrv
	.global __nis_preftype
	.global __nis_server
	.global _nss_default_finders
	.global __progname
	.global _smbuf
	.global _sp
//	.global __strdupa_str
//	.global __strdupa_len
	.global _tdb_bootstrap
	.global __threaded
	.global thr_probe_getfunc_addr
	.global __trans_lower
	.global __trans_upper
	.global _uberdata
	.global __xpg6

	.global _dladdr
	.global _dladdr1
	.global _dlclose
	.global _dldump
	.global _dlerror
	.global _dlinfo
	.global _dlmopen
	.global _dlopen
	.global _dlsym
	.global _ld_libc
//	.global _sys_errlist
	.global _sys_errs
	.global _sys_index
//	.global _sys_nerr
	.global _sys_num_err
	.global _dladdr
	.global _dladdr1
	.global _dlclose
	.global _dldump
	.global _dlerror
	.global _dlinfo
	.global _dlmopen
	.global _dlopen
	.global _dlsym
	.global _ld_libc
	.global _dladdr
	.global _dladdr1
//	.global _dlamd64getunwind
	.global _dlclose
	.global _dldump
	.global _dlerror
	.global _dlinfo
	.global _dlmopen
	.global _dlopen
	.global _dlsym
	.global _ld_libc

	.global __lyday_to_month
	.global __mon_lengths
	.global __yday_to_month
//	.global _sse_hw

//   protected:
	.protected acctctl
	.protected allocids
	.protected _assert_c99
	.protected __assert_c99
	.protected _assfail
	.protected attr_count
	.protected attr_to_data_type
	.protected attr_to_name
	.protected attr_to_option
	.protected attr_to_xattr_view
	.protected _autofssys
	.protected _bufsync
	.protected _cladm
	.protected __class_quadruple
	.protected core_get_default_content
	.protected core_get_default_path
	.protected core_get_global_content
	.protected core_get_global_path
	.protected core_get_options
	.protected core_get_process_content
	.protected core_get_process_path
	.protected core_set_default_content
	.protected core_set_default_path
	.protected core_set_global_content
	.protected core_set_global_path
	.protected core_set_options
	.protected core_set_process_content
	.protected core_set_process_path
	.protected dbm_close_status
	.protected dbm_do_nextkey
	.protected dbm_setdefwrite
//	.protected _D_cplx_div
////	.protected _D_cplx_div_ix
//	.protected _D_cplx_div_rx
//	.protected _D_cplx_mul
	.protected defclose_r
	.protected defcntl
	.protected defcntl_r
	.protected defopen
	.protected defopen_r
	.protected defread
	.protected defread_r
	.protected _delete
	.protected _dgettext
	.protected _doprnt
	.protected _doscan
	.protected _errfp
	.protected _errxfp
	.protected exportfs
//	.protected _F_cplx_div
//	.protected _F_cplx_div_ix
//	.protected _F_cplx_div_rx
//	.protected _F_cplx_mul
	.protected __fgetwc_xpg5
	.protected __fgetws_xpg5
	.protected _findbuf
	.protected _findiop
	.protected __fini_daemon_priv
	.protected _finite
//	.protected _fork1
//	.protected _forkall
	.protected _fpclass
//	.protected _fpgetmask
//	.protected _fpgetround
//	.protected _fpgetsticky
	.protected _fprintf
//	.protected _fpsetmask
//	.protected _fpsetround
//	.protected _fpsetsticky
	.protected __fputwc_xpg5
	.protected __fputws_xpg5
	.protected _ftw
	.protected _gcvt
	.protected _getarg
//	.protected __getcontext
	.protected _getdents
	.protected _get_exit_frame_monitor
	.protected _getfp
	.protected _getgroupsbymember
	.protected _getlogin_r
	.protected _getsp
	.protected __gettsp
	.protected getvmusage
	.protected __getwchar_xpg5
	.protected __getwc_xpg5
	.protected gtty
	.protected __idmap_flush_kcache
	.protected __idmap_reg
	.protected __idmap_unreg
	.protected __init_daemon_priv
	.protected __init_suid_priv
	.protected _insert
	.protected inst_sync
	.protected _iswctype
	.protected klpd_create
	.protected klpd_getpath
	.protected klpd_getport
	.protected klpd_getucred
	.protected klpd_register
	.protected klpd_register_id
	.protected klpd_unregister
	.protected klpd_unregister_id
	.protected _lgrp_home_fast
	.protected _lgrpsys
	.protected _lltostr
	.protected _lock_clear
	.protected _lock_try
	.protected _ltzset
	.protected lwp_self
	.protected makeut
	.protected makeutx
	.protected _mbftowc
	.protected mcfiller
	.protected mntopt
	.protected modctl
	.protected modutx
	.protected msgctl64
	.protected __multi_innetgr
	.protected _mutex_destroy
	.protected mutex_held
	.protected _mutex_init
	.protected _mutex_unlock
	.protected name_to_attr
	.protected nfs_getfh
	.protected nfssvc
	.protected _nfssys
	.protected __nis_get_environment
	.protected _nss_db_state_destr
	.protected nss_default_key2str
	.protected nss_delete
	.protected nss_endent
	.protected nss_getent
	.protected _nss_initf_group
	.protected _nss_initf_netgroup
	.protected _nss_initf_passwd
	.protected _nss_initf_shadow
	.protected nss_packed_arg_init
	.protected nss_packed_context_init
	.protected nss_packed_getkey
	.protected nss_packed_set_status
	.protected nss_search
	.protected nss_setent
	.protected _nss_XbyY_fgets
	.protected __nsw_extended_action_v1
	.protected __nsw_freeconfig_v1
	.protected __nsw_getconfig_v1
	.protected __nthreads
	.protected __openattrdirat
	.protected option_to_attr
	.protected __priv_bracket
	.protected __priv_relinquish
	.protected pset_assign_forced
	.protected pset_bind_lwp
	.protected _psignal
	.protected _pthread_setcleanupinit
	.protected __putwchar_xpg5
	.protected __putwc_xpg5
	.protected rctlctl
	.protected rctllist
	.protected _realbufend
	.protected _resume
	.protected _resume_ret
	.protected _rpcsys
	.protected _sbrk_grow_aligned
	.protected scrwidth
	.protected semctl64
	.protected _semctl64
	.protected set_setcontext_enforcement
	.protected _setbufend
	.protected __set_errno
	.protected setprojrctl
	.protected _setregid
	.protected _setreuid
	.protected setsigacthandler
	.protected shmctl64
	.protected _shmctl64
	.protected sigflag
	.protected _signal
	.protected _sigoff
	.protected _sigon
	.protected _so_accept
	.protected _so_bind
	.protected _sockconfig
	.protected _so_connect
	.protected _so_getpeername
	.protected _so_getsockname
	.protected _so_getsockopt
	.protected _so_listen
	.protected _so_recv
	.protected _so_recvfrom
	.protected _so_recvmsg
	.protected _so_send
	.protected _so_sendmsg
	.protected _so_sendto
	.protected _so_setsockopt
	.protected _so_shutdown
	.protected _so_socket
	.protected _so_socketpair
	.protected str2group
	.protected str2passwd
	.protected str2spwd
	.protected __strptime_dontzero
	.protected stty
	.protected syscall
	.protected _sysconfig
	.protected __systemcall
	.protected thr_continue_allmutators
	.protected _thr_continue_allmutators
	.protected thr_continue_mutator
	.protected _thr_continue_mutator
	.protected thr_getstate
	.protected _thr_getstate
	.protected thr_mutators_barrier
	.protected _thr_mutators_barrier
	.protected thr_probe_setup
	.protected _thr_schedctl
	.protected thr_setmutator
	.protected _thr_setmutator
	.protected thr_setstate
	.protected _thr_setstate
	.protected thr_sighndlrinfo
	.protected _thr_sighndlrinfo
	.protected _thr_slot_offset
	.protected thr_suspend_allmutators
	.protected _thr_suspend_allmutators
	.protected thr_suspend_mutator
	.protected _thr_suspend_mutator
	.protected thr_wait_mutator
	.protected _thr_wait_mutator
	.protected __tls_get_addr
	.protected tpool_create
	.protected tpool_dispatch
	.protected tpool_destroy
	.protected tpool_wait
	.protected tpool_suspend
	.protected tpool_suspended
	.protected tpool_resume
	.protected tpool_member
	.protected _ttyname_dev
	.protected _ucred_alloc
	.protected ucred_getamask
	.protected _ucred_getamask
	.protected ucred_getasid
	.protected _ucred_getasid
	.protected ucred_getatid
	.protected _ucred_getatid
	.protected ucred_getauid
	.protected _ucred_getauid
	.protected _ulltostr
	.protected _uncached_getgrgid_r
	.protected _uncached_getgrnam_r
	.protected _uncached_getpwnam_r
	.protected _uncached_getpwuid_r
	.protected __ungetwc_xpg5
	.protected _unordered
	.protected utssys
	.protected _verrfp
	.protected _verrxfp
	.protected _vwarnfp
	.protected _vwarnxfp
	.protected _warnfp
	.protected _warnxfp
	.protected __wcsftime_xpg5
	.protected __wcstok_xpg5
	.protected wdbindf
	.protected wdchkind
	.protected wddelim
	.protected _wrtchk
	.protected _xflsbuf
	.protected _xgetwidth
	.protected zone_add_datalink
	.protected zone_boot
	.protected zone_check_datalink
	.protected zone_create
	.protected zone_destroy
	.protected zone_enter
	.protected zone_getattr
	.protected zone_get_id
	.protected zone_list
	.protected zone_list_datalink
	.protected zonept
	.protected zone_remove_datalink
	.protected zone_setattr
	.protected zone_shutdown
	.protected zone_version

//	.protected __divdi3
//	.protected _file_set
//	.protected _fprintf_c89
//	.protected _fscanf_c89
//	.protected _fwprintf_c89
//	.protected _fwscanf_c89
//	.protected _imaxabs_c89
//	.protected _imaxdiv_c89
//	.protected __moddi3
//	.protected _printf_c89
//	.protected _scanf_c89
//	.protected _snprintf_c89
//	.protected _sprintf_c89
//	.protected _sscanf_c89
//	.protected _strtoimax_c89
//	.protected _strtoumax_c89
//	.protected _swprintf_c89
//	.protected _swscanf_c89
//	.protected __udivdi3
//	.protected __umoddi3
//	.protected _vfprintf_c89
//	.protected _vfscanf_c89
//	.protected _vfwprintf_c89
//	.protected _vfwscanf_c89
//	.protected _vprintf_c89
//	.protected _vscanf_c89
//	.protected _vsnprintf_c89
//	.protected _vsprintf_c89
//	.protected _vsscanf_c89
//	.protected _vswprintf_c89
//	.protected _vswscanf_c89
//	.protected _vwprintf_c89
//	.protected _vwscanf_c89
//	.protected _wcstoimax_c89
//	.protected _wcstoumax_c89
//	.protected _wprintf_c89
//	.protected _wscanf_c89

//	.protected _cerror
	.protected install_utrap
	.protected _install_utrap
//	.protected nop
//	.protected _Q_cplx_div
////	.protected _Q_cplx_div_ix
////	.protected _Q_cplx_div_rx
//	.protected _Q_cplx_lr_div
//	.protected _Q_cplx_lr_div_ix
//	.protected _Q_cplx_lr_div_rx
//	.protected _Q_cplx_lr_mul
//	.protected _Q_cplx_mul
	.protected _QgetRD
//	.protected _xregs_clrptr

//	.protected __ashldi3
//	.protected __ashrdi3
//	.protected _cerror64
//	.protected __cmpdi2
//	.protected __floatdidf
//	.protected __floatdisf
//	.protected __floatundidf
//	.protected __floatundisf
//	.protected __lshrdi3
//	.protected __muldi3
//	.protected __ucmpdi2

//	.protected _D_cplx_lr_div
//	.protected _D_cplx_lr_div_ix
//	.protected _D_cplx_lr_div_rx
//	.protected _F_cplx_lr_div
//	.protected _F_cplx_lr_div_ix
//	.protected _F_cplx_lr_div_rx
//	.protected __fltrounds
//	.protected sysi86
////	.protected _sysi86
//	.protected _X_cplx_div
//	.protected _X_cplx_div_ix
//	.protected _X_cplx_div_rx
//	.protected _X_cplx_lr_div
//	.protected _X_cplx_lr_div_ix
////	.protected _X_cplx_lr_div_rx
//	.protected _X_cplx_mul
//	.protected __xgetRD
//	.protected __xtol
//	.protected __xtoll
//	.protected __xtoul
////	.protected __xtoull

//	.protected __divrem64
//	.protected ___tls_get_addr
//	.protected __udivrem64

	.protected _brk
	.protected _cond_broadcast
	.protected _cond_init
	.protected _cond_signal
	.protected _cond_wait
	.protected _ecvt
	.protected _fcvt
	.protected _getc_unlocked
	.protected _llseek
	.protected _pthread_attr_getdetachstate
	.protected _pthread_attr_getinheritsched
	.protected _pthread_attr_getschedparam
	.protected _pthread_attr_getschedpolicy
	.protected _pthread_attr_getscope
	.protected _pthread_attr_getstackaddr
	.protected _pthread_attr_getstacksize
	.protected _pthread_attr_init
	.protected _pthread_condattr_getpshared
	.protected _pthread_condattr_init
	.protected _pthread_cond_init
	.protected _pthread_create
	.protected _pthread_getschedparam
	.protected _pthread_join
	.protected _pthread_key_create
	.protected _pthread_mutexattr_getprioceiling
	.protected _pthread_mutexattr_getprotocol
	.protected _pthread_mutexattr_getpshared
	.protected _pthread_mutexattr_init
	.protected _pthread_mutex_getprioceiling
	.protected _pthread_mutex_init
	.protected _pthread_sigmask
	.protected _rwlock_init
	.protected _rw_rdlock
	.protected _rw_unlock
	.protected _rw_wrlock
	.protected _sbrk_unlocked
	.protected _select
	.protected _sema_init
	.protected _sema_post
	.protected _sema_trywait
	.protected _sema_wait
	.protected _sysfs
	.protected _thr_create
	.protected _thr_exit
	.protected _thr_getprio
	.protected _thr_getspecific
	.protected _thr_join
	.protected _thr_keycreate
	.protected _thr_kill
	.protected _thr_main
	.protected _thr_self
	.protected _thr_setspecific
	.protected _thr_sigsetmask
	.protected _thr_stksegment
	.protected _ungetc_unlocked

