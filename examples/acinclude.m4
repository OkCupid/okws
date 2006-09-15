dnl $Id$
dnl
dnl Find full path to program
dnl
AC_DEFUN([SFS_PATH_PROG],
[AC_PATH_PROG(PATH_[]translit($1, [a-z], [A-Z]), $1,,
$2[]ifelse($2,,,:)/usr/bin:/bin:/sbin:/usr/sbin:/usr/etc:/usr/libexec:/usr/ucb:/usr/bsd:/usr/5bin:$PATH:/usr/local/bin:/usr/local/sbin:/usr/X11R6/bin)
if test "$PATH_[]translit($1, [a-z], [A-Z])"; then
    AC_DEFINE_UNQUOTED(PATH_[]translit($1, [a-z], [A-Z]),
		       "$PATH_[]translit($1, [a-z], [A-Z])",
			Full path of $1 command)
fi])

dnl
dnl majorly ghetto but it works (unlike the ohter PATH_PROGS)
dnl
AC_DEFUN([OKWS_PATH_PROG],
[ for dir in ${okwslibdir} ${with_okws}/bin /usr/local/bin /usr/bin /bin
    do
      if test -x $dir/$1 ; then
	 RES="$dir/$1"
	 break;
      fi
    done
])
dnl
dnl File path to cpp
dnl
AC_DEFUN([SFS_PATH_CPP],
[AC_PATH_PROG(_PATH_CPP, cpp,,
/usr/ccs/bin:/usr/bin:/bin:/sbin:/usr/sbin:/usr/etc:/usr/libexec:/lib:/usr/lib:/usr/ucb:/usr/bsd:/usr/5bin:$PATH:/usr/local/bin:/usr/local/sbin:/usr/X11R6/bin)
if test -z "$_PATH_CPP"; then
    if test "$GCC" = yes; then
	_PATH_CPP=`$CC -print-prog-name=cpp`
    else
	_PATH_CPP=`gcc -print-prog-name=cpp 2> /dev/null`
    fi
fi
test -x "$_PATH_CPP" || unset _PATH_CPP
if test -z "$_PATH_CPP"; then
    AC_MSG_ERROR(Cannot find path for cpp)
fi
AC_DEFINE_UNQUOTED(PATH_CPP, "$_PATH_CPP",
			Path for the C preprocessor command)
])
dnl
dnl How to get BSD-like df output
dnl
AC_DEFUN([SFS_PATH_DF],
[SFS_PATH_PROG(df, /usr/ucb:/usr/bsd:/usr/local/bin)
AC_CACHE_CHECK(if [$PATH_DF] needs -k for BSD-formatted output,
	sfs_cv_df_dash_k,
sfs_cv_df_dash_k=no
[test "`$PATH_DF . | sed -e '2,$d;/Mounted on/d'`" \
	&& test "`$PATH_DF -k . | sed -ne '2,$d;/Mounted on/p'`" \
	&& sfs_cv_df_dash_k=yes])
if test $sfs_cv_df_dash_k = yes; then
	AC_DEFINE(DF_NEEDS_DASH_K, 1,
	  Define if you must run \"df -k\" to get BSD-formatted output)
fi])
dnl
dnl Check for declarations
dnl SFS_CHECK_DECL(symbol, headers-to-try, headers-to-include)
dnl
AC_DEFUN([SFS_CHECK_DECL],
[AC_CACHE_CHECK(for a declaration of $1, sfs_cv_$1_decl,
dnl    for hdr in [patsubst(builtin(shift, $@), [,], [ ])]; do
    for hdr in $2; do
	if test -z "${sfs_cv_$1_decl}"; then
dnl	    AC_HEADER_EGREP($1, $hdr, sfs_cv_$1_decl=yes)
	    AC_TRY_COMPILE(
patsubst($3, [\([^ ]+\) *], [#include <\1>
])[#include <$hdr>], &$1;, sfs_cv_$1_decl=yes)
	fi
    done
    test -z "${sfs_cv_$1_decl+set}" && sfs_cv_$1_decl=no)
if test "$sfs_cv_$1_decl" = no; then
	AC_DEFINE_UNQUOTED(NEED_[]translit($1, [a-z], [A-Z])_DECL, 1,
		Define if system headers do not declare $1.)
fi])
dnl
dnl Check if lsof keeps a device cache
dnl
AC_DEFUN([SFS_LSOF_DEVCACHE],
[if test "$PATH_LSOF"; then
    AC_CACHE_CHECK(if lsof supports a device cache, sfs_cv_lsof_devcache,
    if $PATH_LSOF -h 2>&1 | fgrep -e -D > /dev/null; then
	sfs_cv_lsof_devcache=yes
    else
	sfs_cv_lsof_devcache=no
    fi)
    if test "$sfs_cv_lsof_devcache" = yes; then
	AC_DEFINE(LSOF_DEVCACHE, 1,
		Define is lsof supports the -D option)
    fi
fi])
dnl
dnl Posix time subroutine
dnl
AC_DEFUN([SFS_TIME_CHECK],
[AC_CACHE_CHECK($3, sfs_cv_time_check_$1,
AC_TRY_COMPILE([
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#elif defined (HAVE_SYS_TIME_H)
# include <sys/time.h>
#else /* !TIME_WITH_SYS_TIME && !HAVE_SYS_TIME_H */
# include <time.h>
#endif /* !TIME_WITH_SYS_TIME && !HAVE_SYS_TIME_H */
], $2, sfs_cv_time_check_$1=yes, sfs_cv_time_check_$1=no))
if test "$sfs_cv_time_check_$1" = yes; then
	AC_DEFINE($1, 1, $4)
fi])
dnl
dnl Posix time stuff
dnl
AC_DEFUN([SFS_TIMESPEC],
[AC_CHECK_HEADERS(sys/time.h)
AC_HEADER_TIME
AC_CHECK_FUNCS(clock_gettime)
SFS_TIME_CHECK(HAVE_CLOCK_GETTIME_DECL,
		int (*x) () = &clock_gettime;,
		for a declaration of clock_gettime,
		Define if system headers declare clock_gettime.)
SFS_TIME_CHECK(HAVE_TIMESPEC,
		int x = sizeof (struct timespec),
		for struct timespec,
		Define if sys/time.h defines a struct timespec.)
dnl AC_EGREP_HEADER(clock_gettime, sys/time.h,
dnl 	AC_DEFINE(HAVE_CLOCK_GETTIME_DECL, 1,
dnl 	Define if system header files declare clock_gettime.))
dnl AC_EGREP_HEADER(timespec, sys/time.h,
dnl 	AC_DEFINE(HAVE_TIMESPEC, 1,
dnl 		  Define if sys/time.h defines a struct timespec.))
])
dnl
dnl Find the crypt function
dnl
AC_DEFUN([SFS_FIND_CRYPT],
[AC_SUBST(LIBCRYPT)
AC_CHECK_FUNC(crypt)
if test $ac_cv_func_crypt = no; then
	AC_CHECK_LIB(crypt, crypt, LIBCRYPT="-lcrypt")
fi
])
dnl
dnl Find pty functions
dnl
AC_DEFUN([SFS_PTYLIB],
[AC_SUBST(PTYLIB)
AC_CHECK_FUNCS(_getpty)
AC_CHECK_FUNCS(openpty)
if test $ac_cv_func_openpty = no; then
	AC_CHECK_LIB(util, openpty, PTYLIB="-lutil"
		AC_DEFINE(HAVE_OPENPTY, 1,
			Define if you have the openpty function.))
fi
if test "$ac_cv_func_openpty" = yes -o "$ac_cv_lib_util_openpty" = yes; then
	AC_CHECK_HEADERS(util.h libutil.h)
fi
AC_CHECK_HEADERS(pty.h)

AC_CACHE_CHECK(for BSD-style utmp slots, ac_cv_have_ttyent,
	AC_EGREP_HEADER(getttyent, ttyent.h,
		ac_cv_have_ttyent=yes, ac_cv_have_ttyent=no))
if test "$ac_cv_have_ttyent" = yes; then
	AC_DEFINE(USE_TTYENT, 1,
	    Define if utmp must be managed with BSD-style ttyent functions)
fi

AC_MSG_CHECKING(for pseudo ttys)
if test -c /dev/ptmx && test -c /dev/pts/0
then
  AC_DEFINE(HAVE_DEV_PTMX, 1,
	    Define if you have SYSV-style /dev/ptmx and /dev/pts/.)
  AC_MSG_RESULT(streams ptys)
else
if test -c /dev/pts && test -c /dev/ptc
then
  AC_DEFINE(HAVE_DEV_PTS_AND_PTC, 1,
	    Define if you have /dev/pts and /dev/ptc devices (as in AIX).)
  AC_MSG_RESULT(/dev/pts and /dev/ptc)
else
  AC_MSG_RESULT(bsd-style ptys)
fi
fi])
dnl
dnl Use -lresolv only if we need it
dnl
AC_DEFUN([SFS_FIND_RESOLV],
[AC_CHECK_FUNC(res_mkquery)
if test "$ac_cv_func_res_mkquery" != yes; then
	AC_CHECK_LIB(resolv, res_mkquery)
fi
dnl See if the resolv functions are actually declared
SFS_CHECK_DECL(res_init, resolv.h,
	sys/types.h sys/socket.h netinet/in.h arpa/nameser.h)
SFS_CHECK_DECL(res_mkquery, resolv.h,
	sys/types.h sys/socket.h netinet/in.h arpa/nameser.h)
SFS_CHECK_DECL(dn_skipname, resolv.h,
	sys/types.h sys/socket.h netinet/in.h arpa/nameser.h)
SFS_CHECK_DECL(dn_expand, resolv.h,
	sys/types.h sys/socket.h netinet/in.h arpa/nameser.h)
])
dnl
dnl Check if first element in grouplist is egid
dnl
AC_DEFUN([SFS_CHECK_EGID_IN_GROUPLIST],
[AC_TYPE_GETGROUPS
AC_CACHE_CHECK(if egid is first element of grouplist, sfs_cv_egid_in_grouplist,
AC_TRY_RUN([changequote changequote([[,]])
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <rpc/rpc.h>

#include "confdefs.h"

static int
getint (void *_p)
{
  unsigned char *p = _p;
  return p[0]<<24 | p[1]<<16 | p[2]<<8 | p[3];
}

int
main (int argc, char **argv)
{
  AUTH *a;
  GETGROUPS_T gids[24];
  int n, xn;
  char buf[408];
  char *p;
  XDR x;

  /* Must hard-code OSes with egid in grouplist *and* broken RPC lib */
#if __FreeBSD__
  return 0;
#endif 

  n = getgroups (24, gids);
  if (n <= 0)
    return 1;

  a = authunix_create_default ();
  xdrmem_create (&x, buf, sizeof (buf), XDR_ENCODE);
  if (!auth_marshall (a, &x))
    return 1;

  if (getint (buf) != AUTH_UNIX)
    return 1;
  p = buf + 12;			/* Skip auth flavor, length, timestamp */
  p += getint (p) + 7 & ~3;	/* Skip machine name */
  p += 8;			/* Skip uid & gid */

  xn = getint (p);		/* Length of grouplist in auth_unix */

  return n != xn + 1;
}
changequote([,])],
	sfs_cv_egid_in_grouplist=yes, sfs_cv_egid_in_grouplist=no))
if test $sfs_cv_egid_in_grouplist = yes; then
	AC_DEFINE(HAVE_EGID_IN_GROUPLIST, 1,
	  Define if the first element of a grouplist is the effective gid)
fi])
dnl
dnl Check if putenv copies arguments
dnl
AC_DEFUN([SFS_PUTENV_COPY],
[AC_CACHE_CHECK(if putenv() copies its argument, sfs_cv_putenv_copy,
AC_TRY_RUN([
changequote`'changequote([[,]])
#include <stdlib.h>

char var[] = "V=0";

int
main (int argc, char **argv)
{
  char *v;

  putenv (var);
  var[2] = '1';
  v = getenv (var);
  return *v != '0';
}
changequote([,])],
	sfs_cv_putenv_copy=yes, sfs_cv_putenv_copy=no,
	sfs_cv_putenv_copy=no)
)
if test $sfs_cv_putenv_copy = yes; then
	AC_DEFINE(PUTENV_COPIES_ARGUMENT, 1,
		  Define if putenv makes a copy of its argument)
fi])
dnl
dnl Check for wide select
dnl
AC_DEFUN([SFS_CHECK_WIDE_SELECT],
[AC_CACHE_CHECK(for wide select, sfs_cv_wideselect,
fdlim_h=${srcdir}/fdlim.h
test -f ${srcdir}/async/fdlim.h && fdlim_h=${srcdir}/async/fdlim.h
test -f ${srcdir}/libasync/fdlim.h && fdlim_h=${srcdir}/libasync/fdlim.h
AC_TRY_RUN([changequote changequote([[,]])
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include "${fdlim_h}"

struct timeval ztv;

int
main ()
{
  int pfd[2];
  int rfd, wfd;
  int maxfd;
  int i;
  fd_set *rfdsp, *wfdsp;

  maxfd = fdlim_get (1);
  fdlim_set (maxfd, 1);
  maxfd = fdlim_get (0);
  if (maxfd <= FD_SETSIZE) {
    printf ("[small fd limit anyway] ");
    exit (1);
  }
  if (pipe (pfd) < 0)
    exit (1);

#ifdef F_DUPFD
  if ((rfd = fcntl (pfd[0], F_DUPFD, maxfd - 2)) < 0)
    exit (1);
  if ((wfd = fcntl (pfd[1], F_DUPFD, maxfd - 1)) < 0)
    exit (1);
#else /* !F_DUPFD */
  if ((rfd = dup2 (pfd[0], maxfd - 2)) < 0)
    exit (1);
  if ((wfd = dup2 (pfd[1], maxfd - 1)) < 0)
    exit (1);
#endif /* !F_DUPFD */

  rfdsp = malloc (1 + (maxfd/8));
  for (i = 0; i < 1 + (maxfd/8); i++)
    ((char *) rfdsp)[i] = '\0';
  wfdsp = malloc (1 + (maxfd/8));
  for (i = 0; i < 1 + (maxfd/8); i++)
    ((char *) wfdsp)[i] = '\0';

  FD_SET (rfd, rfdsp);
  FD_SET (wfd, wfdsp);
  if (select (maxfd, rfdsp, wfdsp, NULL, &ztv) < 0)
    exit (1);

  if (FD_ISSET (wfd, wfdsp) && !FD_ISSET (rfd, rfdsp))
    exit (0);
  else
    exit (1);
}
changequote([,])],
sfs_cv_wideselect=yes, sfs_cv_wideselect=no, sfs_cv_wideselect=no))
if test $sfs_cv_wideselect = yes; then
	AC_DEFINE(HAVE_WIDE_SELECT, 1,
		  Define if select can take file descriptors >= FD_SETSIZE)
fi])
dnl
dnl Check for 64-bit off_t
dnl
AC_DEFUN([SFS_CHECK_OFF_T_64],
[AC_CACHE_CHECK(for 64-bit off_t, sfs_cv_off_t_64,
AC_TRY_COMPILE([
#include <unistd.h>
#include <sys/types.h>
],[
switch (0) case 0: case (sizeof (off_t) <= 4):;
], sfs_cv_off_t_64=no, sfs_cv_off_t_64=yes))
if test $sfs_cv_off_t_64 = yes; then
	AC_DEFINE(HAVE_OFF_T_64, 1, Define if off_t is 64 bits wide.)
fi])
dnl
dnl Check for type
dnl
AC_DEFUN([SFS_CHECK_TYPE],
[AC_CACHE_CHECK(for $1, sfs_cv_type_$1,
AC_TRY_COMPILE([
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_RPC_RPC_H
#include <rpc/rpc.h>
#endif
$2
],[
sizeof($1);
], sfs_cv_type_$1=yes, sfs_cv_type_$1=no))
if test $sfs_cv_type_$1 = yes; then
        AC_DEFINE(HAVE_[]translit($1, [a-z], [A-Z]), 1,
		  Define if system headers declare a $1 type.)
fi])
dnl
dnl Check for struct cmsghdr (for passing file descriptors)
dnl
AC_DEFUN([SFS_CHECK_FDPASS],
[
AC_CACHE_CHECK(for fd passing with msg_accrights in msghdr,
		sfs_cv_accrights,
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct msghdr mh;
mh.msg_accrights = 0;
], sfs_cv_accrights=yes, sfs_cv_accrights=no))

AC_CACHE_CHECK(for fd passing with struct cmsghdr, sfs_cv_cmsghdr,
if test "$sfs_cv_accrights" != "yes"; then
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct msghdr mh;
struct cmsghdr cmh;
mh.msg_control = (void *) &cmh;
], sfs_cv_cmsghdr=yes, sfs_cv_cmsghdr=no)
else
	sfs_cv_cmsghdr=no
fi)

if test $sfs_cv_accrights = yes; then
	AC_DEFINE(HAVE_ACCRIGHTS, 1,
	Define if msghdr has msg_accrights field for passing file descriptors.)
fi
if test $sfs_cv_cmsghdr = yes; then
	AC_DEFINE(HAVE_CMSGHDR, 1,
	Define if system has cmsghdr structure for passing file descriptors.)
fi])
dnl
dnl Check for sa_len in struct sockaddrs
dnl
AC_DEFUN([SFS_CHECK_SA_LEN],
[AC_CACHE_CHECK(for sa_len in struct sockaddr, sfs_cv_sa_len,
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
],[
struct sockaddr sa;
sa.sa_len = 0;
], sfs_cv_sa_len=yes, sfs_cv_sa_len=no))
if test $sfs_cv_sa_len = yes; then
	AC_DEFINE(HAVE_SA_LEN, 1,
	Define if struct sockaddr has sa_len field.)
fi])
dnl
dnl Check something about the nfs_args field
dnl
AC_DEFUN([SFS_TRY_NFSARG_FIELD],
[AC_TRY_COMPILE([
#include "${srcdir}/nfsconf.h"
],[
struct nfs_args na;
$1;
], $2, $3)])
dnl
dnl Check a particular field in nfs_args
dnl
AC_DEFUN([SFS_CHECK_NFSMNT_FIELD],
[AC_CACHE_CHECK(for $1 in nfs_args structure, sfs_cv_nfsmnt_$1,
SFS_TRY_NFSARG_FIELD(na.$1, sfs_cv_nfsmnt_$1=yes, sfs_cv_nfsmnt_$1=no))
if test $sfs_cv_nfsmnt_$1 = yes; then
  AC_DEFINE(HAVE_NFSMNT_[]translit($1, [a-z], [A-Z]), 1,
	    Define if the nfs_args structure has a $1 field.)
fi])
dnl
dnl Check if nfs_args hostname field is an array
dnl
AC_DEFUN([SFS_CHECK_NFSARG_HOSTNAME_ARRAY],
[AC_CACHE_CHECK(if nfs_args hostname field is an array, sfs_cv_nfs_hostarray,
	SFS_TRY_NFSARG_FIELD(na.hostname = 0,
		sfs_cv_nfs_hostarray=no, sfs_cv_nfs_hostarray=yes))
if test $sfs_cv_nfs_hostarray = yes; then
  AC_DEFINE(HAVE_NFSARG_HOSTNAME_ARRAY, 1,
	[The hostname field of nfs_arg is an array])
fi])
dnl
dnl Check if addr field is a pointer or not
dnl
AC_DEFUN([SFS_CHECK_NFSARG_ADDR_PTR],
[AC_CHECK_HEADERS(tiuser.h)
AC_CACHE_CHECK(if nfs_args addr field is a pointer, sfs_cv_nfsmnt_addr_ptr,
	SFS_TRY_NFSARG_FIELD(na.addr = (void *) 0, sfs_cv_nfsmnt_addr_ptr=yes,
				sfs_cv_nfsmnt_addr_ptr=no))
if test $sfs_cv_nfsmnt_addr_ptr = yes; then
  AC_DEFINE(HAVE_NFSARG_ADDR_PTR, 1,
	[The addr field of nfs_arg is a pointer])
  AC_CACHE_CHECK(if nfs_args addr is a netbuf *, sfs_cv_nfsmnt_addr_netbuf,
	SFS_TRY_NFSARG_FIELD(struct netbuf nb; *na.addr = nb,
	  sfs_cv_nfsmnt_addr_netbuf=yes, sfs_cv_nfsmnt_addr_netbuf=no))
  if test $sfs_cv_nfsmnt_addr_netbuf = yes; then
    AC_DEFINE(HAVE_NFSARG_ADDR_NETBUF, 1,
	[If the nfs_arg addr field is a netbuf pointer])
  fi
fi])
dnl
dnl Check for SVR4-like nfs_fh3 structure
dnl
AC_DEFUN([SFS_CHECK_FH3_SVR4],
[if test "$sfs_cv_nfsmnt_fhsize" != yes; then
  AC_CACHE_CHECK(for SVR4-like struct nfs_fh3, sfs_cv_fh3_svr4,
  AC_TRY_COMPILE([#include "${srcdir}/nfsconf.h"],
                 [ struct nfs_fh3 fh;
                   switch (0) case 0: case sizeof (fh.fh3_u.data) == 64:; ],
                 sfs_cv_fh3_svr4=yes, sfs_cv_fh3_svr4=no))
  if test $sfs_cv_fh3_svr4 = yes; then
    AC_DEFINE(HAVE_SVR4_FH3, 1,
	[The the fh field of the nfs_arg structure points to an SVR4 nfs_fh3])
  fi
fi])
dnl
dnl Check for 2 argument unmount
dnl
AC_DEFUN([SFS_CHECK_UNMOUNT_FLAGS],
[AC_CACHE_CHECK(for a 2 argument unmount, sfs_cv_umount_flags,
AC_TRY_COMPILE([
#include <sys/param.h>
#include <sys/mount.h>
],[
#ifdef HAVE_UNMOUNT
unmount
#else /* !HAVE_UNMOUNT */
umount
#endif /* !HAVE_UNMOUNT */
	(0);
], sfs_cv_umount_flags=no, sfs_cv_umount_flags=yes))
if test $sfs_cv_umount_flags = yes; then
	AC_DEFINE(UNMOUNT_FLAGS, 1,
		  Define if the unmount system call has 2 arguments.)
else
	AC_CHECK_FUNCS(umount2)
fi])
dnl
dnl Check if we can find the nfs_args structure
dnl
AC_DEFUN([SFS_CHECK_NFSMNT],
[AC_CHECK_FUNCS(vfsmount unmount)
need_nfs_nfs_h=no
AC_EGREP_HEADER(nfs_args, sys/mount.h,,
	AC_EGREP_HEADER(nfs_args, nfs/mount.h,
		AC_DEFINE(NEED_NFS_MOUNT_H, 1,
			[The nfs_args structure is in <nfs/nfsmount.h>]))
	AC_EGREP_HEADER(nfs_args, nfs/nfsmount.h,
		AC_DEFINE(NEED_NFS_NFSMOUNT_H, 1,
			[The nfs_args structure is in <nfs/nfsmount.h]))
	AC_EGREP_HEADER(nfs_args, nfsclient/nfs.h,
		AC_DEFINE(NEED_NFSCLIENT_NFS_H, 1,
			[The nfs_args structure is in <nfsclient/nfs.h>]))
	AC_EGREP_HEADER(nfs_args, nfs/nfs.h,
		AC_DEFINE(NEED_NFS_NFS_H, 1,
			[The nfs_args structure is in <nfs/nfs.h>])
		need_nfs_nfs_h=yes))
AC_CACHE_CHECK(for nfs_args mount structure, sfs_cv_nfsmnt_ok,
	SFS_TRY_NFSARG_FIELD(, sfs_cv_nfsmnt_ok=yes, sfs_cv_nfsmnt_ok=no))
if test $sfs_cv_nfsmnt_ok = no; then
	AC_MSG_ERROR([Could not find NFS mount argument structure!])
fi
if test "$need_nfs_nfs_h" = no; then
	AC_EGREP_HEADER(nfs_fh3, nfs/nfs.h,
		AC_DEFINE(NEED_NFS_NFS_H, 1,
			[The nfs_args structure is in <nfs/nfs.h>])
			need_nfs_nfs_h=yes)
fi
AC_CHECK_HEADERS(linux/nfs2.h)
SFS_CHECK_NFSMNT_FIELD(addrlen)
SFS_CHECK_NFSMNT_FIELD(sotype)
SFS_CHECK_NFSMNT_FIELD(proto)
SFS_CHECK_NFSMNT_FIELD(fhsize)
SFS_CHECK_NFSMNT_FIELD(fd)

dnl Check whether we have Linux 2.2 NFS V3 mount structure
SFS_CHECK_NFSMNT_FIELD(old_root)

dnl Check whether file handle is named "root" or "fh"
SFS_CHECK_NFSMNT_FIELD(root)
SFS_CHECK_NFSMNT_FIELD(fh)
dnl ksh apparently cannot handle this as a compound test.
if test "$sfs_cv_nfsmnt_root" = "no"; then
  if test "$sfs_cv_nfsmnt_fh" = "no"; then
    AC_MSG_ERROR([Could not find the nfs_args file handle field!])
  fi
fi
AC_CHECK_HEADERS(sys/mntent.h)
SFS_CHECK_FH3_SVR4
if test "$sfs_cv_nfsmnt_fh" = yes; then
  if test "$sfs_cv_fh3_svr4" = yes -o "$sfs_cv_nfsmnt_fhsize" = yes; then
    AC_DEFINE(HAVE_NFS_V3, 1, [If the system supports NFS 3])
  fi
elif test "$sfs_cv_nfsmnt_old_root" = yes; then
  AC_DEFINE(HAVE_NFS_V3, 1, [If the system supports NFS 3])
fi

SFS_CHECK_NFSARG_HOSTNAME_ARRAY
SFS_CHECK_NFSARG_ADDR_PTR
SFS_CHECK_UNMOUNT_FLAGS])
dnl
dnl Use -ldb only if we need it.
dnl
AC_DEFUN([SFS_FIND_DB],
[AC_CHECK_FUNC(dbopen)
if test $ac_cv_func_dbopen = no; then
	AC_CHECK_LIB(db, dbopen)
	if test $ac_cv_lib_db_dbopen = no; then
	  AC_MSG_ERROR([Could not find library for dbopen!])
	fi
fi
])
dnl
dnl Check something about the stat structure
dnl
AC_DEFUN([SFS_TRY_STAT_FIELD],
[AC_TRY_COMPILE([
#include <sys/stat.h>
],[
struct stat s;
$1;
], $2, $3)])
dnl
dnl Check for a particular field in stat
dnl
AC_DEFUN([SFS_CHECK_STAT_FIELD],
[AC_CACHE_CHECK(for $1 in stat structure, sfs_cv_stat_$1,
SFS_TRY_STAT_FIELD(s.$1, sfs_cv_stat_$1=yes, sfs_cv_stat_$1=no))
if test $sfs_cv_stat_$1 = yes; then
  AC_DEFINE(SFS_HAVE_STAT_[]translit($1, [a-z], [A-Z]), 1,
	    Define if the stat structure has a $1 field.)
fi])

dnl
dnl  Check whether we can get away with large socket buffers.
dnl
AC_DEFUN([SFS_CHECK_SOCK_BUF],
[AC_CACHE_CHECK(whether socket buffers > 64k are allowed, 
 		sfs_cv_large_sock_buf, AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>

int
main() 
{
  int bigbuf = 0x11000;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&bigbuf, sizeof(bigbuf))<0)
    exit(1);
  exit(0);
}
], sfs_cv_large_sock_buf=yes, 
   sfs_cv_large_sock_buf=no, 
   sfs_cv_large_sock_buf=no))
if test $sfs_cv_large_sock_buf = yes; then
	AC_DEFINE(SFS_ALLOW_LARGE_BUFFER, 1,
		  Define if SO_SNDBUF/SO_RCVBUF can exceed 64K.)
fi])

dnl
dnl Find Mysql
dnl
AC_DEFUN([SFS_MYSQL],
[AC_ARG_WITH(mysql,
--with-mysql=DIR          Specific location of mysqlclient library)
if test "$with_mysql" != "no"; then
	ac_save_CFLAGS=$CFLAGS
	ac_save_LIBS=$LIBS
	cdirs="${with_mysql}/include ${with_mysql}/include/mysql \
	       ${prefix}/include ${prefix}/include/mysql"
	dirs="$cdirs /usr/local/include/mysql /usr/local/mysql/include \
               /usr/include/mysql "
	AC_CACHE_CHECK(for mysql.h, sfs_cv_mysql_h,
	[for dir in " " $dirs; do
		case $dir in
			" ") iflags="-I" ;;
			*) iflags="-I${dir}" ;;
		esac
		CFLAGS="${ac_save_CFLAGS} $iflags"
		AC_TRY_COMPILE([#include "mysql.h"], 0,
		 sfs_cv_mysql_h="${iflags}"; break)
	done
	if test "$sfs_cv_mysql_h" = " "; then
		sfs_cv_mysql_h="yes"
	fi
	])
	if test "$sfs_cv_mysql_h" = "yes"; then
		sfs_cv_mysql_h=" "
	fi
	if test "${sfs_cv_mysql_h+set}"; then
		cdirs="${with_mysql}/lib ${with_mysql}/lib/mysql \
		       ${prefix}/lib ${prefix}/lib/mysql"
		dirs="$cdirs /usr/local/lib/mysql /usr/local/mysql/lib \
                      /usr/lib/mysql "
		AC_CACHE_CHECK(for libmysqlclient, sfs_cv_libmysqlclient,
		[for dir in "" " " $dirs; do
			case $dir in
				"") lflags=" " ;;
				" ") lflags="-lmysqlclient -lm" ;;
				*) lflags="-L${dir} -lmysqlclient -lm" ;;
			esac
			LIBS="$ac_save_LIBS $lflags"
			AC_TRY_LINK([#include "mysql.h"],
				mysql_real_connect (0,0,0,0,0,0,0,0);, 
				sfs_cv_libmysqlclient=$lflags; break)
		done
		if test -z ${sfs_cv_libmysqlclient+set}; then
			sfs_cv_libmysqlclient="no"
		fi
		])
	fi
	if test "${sfs_cv_libmysqlclient+set}" && \
	   test "$sfs_cv_libmysqlclient" != "no"; then


		AC_CACHE_CHECK(for MYSQL bind types, sfs_cv_mysqlbind,
		[
		sfs_cv_mysqlbind=no
 		AC_TRY_COMPILE([#include "mysql.h"], 
			MYSQL_BIND bnd;,
			sfs_cv_mysqlbind=yes)
		])

		AC_CACHE_CHECK(for MYSQL bind functions, sfs_cv_mysqlbind_fn,
		[
		sfs_cv_mysqlbind_fn=no
		AC_TRY_LINK([#include "mysql.h"],
			mysql_stmt_bind_param (0,0);,	
			sfs_cv_mysqlbind_fn=yes)
		])

		if test "$sfs_cv_mysqlbind_fn" = "yes"; then
		    AC_DEFINE(HAVE_MYSQL_BINDFUNCS, 1, MySQL Bind Functions)
		fi

		if test "$sfs_cv_mysqlbind" = "yes"; then
			AC_DEFINE(HAVE_MYSQL_BIND, 1, MySQL Prepared Stuff)
		fi

		CPPFLAGS="$CPPFLAGS $sfs_cv_mysql_h"
		AC_DEFINE(HAVE_MYSQL, 1, Have the MySQL C client library )
		LDADD_MYSQL="$sfs_cv_libmysqlclient"
  	 	if test "$ac_cv_lib_z" != "yes"; then
			LDADD_MYSQL="$LDADD_MYSQL -lz"
		fi
	fi
	AC_SUBST(LDADD_MYSQL)
	LIBS=$ac_save_LIBS
	CFLAGS=$ac_save_CFLAGS
fi
])

dnl
dnl Version Hack
dnl
AC_DEFUN([OKWS_SET_VERSION],
[
AC_DEFINE_UNQUOTED(OKWS_VERSION, "$VERSION", OKWS Library Version)
])
dnl
dnl Find pth
dnl
AC_DEFUN([SFS_FIND_PTH],
[AC_ARG_WITH(pth,
--with-pth=DIR		  Specify location of GNU Pth library)
ac_save_CFLAGS=$CFLAGS
ac_save_LIBS=$LIBS
dirs0="${with_pth} ${with_pth}/include"
if test "${prefix}" != "NONE"; then
	dirs0="$dirs0 ${prefix} ${prefix}/pth"
fi

dirs1="$dirs0 /usr/local/include/pth /usr/local/include "
dirs2=""

dnl
dnl only consider those directories that actually have a pth.h
dnl in them; otherwise, we'll get false positives.
dnl
for dir in $dirs1
do
    if test -r ${dir}/pth.h ; then
	dirs2="$dirs2 $dir"
    fi
done

AC_CACHE_CHECK(for pth.h, sfs_cv_pth_h,
[for dir in $dirs2 " " ; do
	case $dir in 
		" ") iflags=" " ;;
		*) iflags="-I${dir}" ;;
	esac
	CFLAGS="${ac_save_CFLAGS} $iflags"
	AC_TRY_COMPILE([#include <pth.h>], [
#if !defined(PTH_SYSCALL_HARD) || PTH_SYSCALL_HARD == 0
#error "HARD SYSTEM CALLS ARE REQUIRED"
#endif
#if PTH_SYSCALL_SOFT
#error "SOFT SYSTEM CALLS WILL BREAK LIBASYNC"
#endif
	],
	 sfs_cv_pth_h="${iflags}"; break)
done
if test "$sfs_cv_pth_h" = " "; then
	sfs_cv_pth_h="yes"
fi
])
if test "$sfs_cv_pth_h" = "yes"; then
	sfs_cv_pth_h=" "
fi
if test "${sfs_cv_pth_h+set}"; then
	dnl
	dnl only check the include directory that corresponds
	dnl to the library directory;  there might be multiple
	dnl versions of the library around.
	dnl
	dirs=`echo $sfs_cv_pth_h | sed 's/include/lib/' `
	dirs=`echo $dirs | sed 's/^-I//' `
	AC_CACHE_CHECK(for libpth, sfs_cv_libpth,
	[for dir in " " $dirs; do
		case $dir in
			" ") lflags="-lpth" ;;
			*) lflags="-L${dir} -lpth" ;;
		esac
		LIBS="$ac_save_LIBS $lflags"
		AC_TRY_LINK([#include <pth.h>],
			pth_init ();, 
			sfs_cv_libpth=$lflags; break)

		dnl
		dnl Linux seems to require linking against -ldl in
		dnl certain cases.  May as well give it a try
		dnl
                lflags="$lflags -ldl";
		LIBS="$ac_save_LIBS $lflags"
		AC_TRY_LINK([#include <pth.h>],
			pth_init ();, 
			sfs_cv_libpth=$lflags; break)
                
	done
	if test -z ${sfs_cv_libpth+set}; then
		sfs_cv_libpth="no"
	fi
])
fi
if test "${sfs_cv_libpth+set}" && test "$sfs_cv_libpth" != "no"; then
	CPPFLAGS="$CPPFLAGS $sfs_cv_pth_h"
	AC_DEFINE(HAVE_PTH, 1, Allow libamt to use the GNU Pth library)
	sfs_have_threads=yes
	LDADD_THR=$sfs_cv_libpth
fi
LIBS=$ac_save_LIBS
CFLAGS=$ac_save_CFLAGS
])


dnl
dnl Find pthreads
dnl
AC_DEFUN([SFS_FIND_PTHREADS],
[AC_ARG_WITH(pthreads,
--with-pthreads=DIR       Specify location of pthreads)
ac_save_CFLAGS=$CFLAGS
ac_save_LIBS=$LIBS
dirs="$with_pthreads ${prefix} ${prefix}/pthreads"
dirs="$dirs /usr/local /usr/local/pthreads"
AC_CACHE_CHECK(for pthread.h, sfs_cv_pthread_h,
[for dir in " " $dirs; do
	case $dir in
		" ") iflags=" " ;;
		*)   iflags="-I${dir}/include" ;;
	esac
	CFLAGS="${ac_save_CFLAGS} $iflags"
	AC_TRY_COMPILE([#include <pthread.h>], 0,
	  sfs_cv_pthread_h="${iflags}"; break)
done
if test "$sfs_cv_pthread_h" = " "; then
 	sfs_cv_pthread_h="yes"
fi
])
if test "$sfs_cv_pthread_h" = "yes"; then
	sfs_cv_pthread_h=" "
fi
if test "${sfs_cv_pthread_h+set}"; then
	AC_CACHE_CHECK(for libpthread, sfs_cv_libpthread,
	[for dir in "" " " $dirs; do
		case $dir in
			"") lflags=" " ;;
			" ") lflags="-lpthread" ;;
			*) lflags="-L${dir}/lib -lpthread" ;;
		esac
		LIBS="$ac_save_LIBS $lflags"
		AC_TRY_LINK([#include <pthread.h>],
			pthread_create (0, 0, 0, 0);,
			sfs_cv_libpthread=$lflags; break)
	done
	if test -z ${sfs_cv_libpthread+set}; then
		sfs_cv_libpthread="no"
	fi])
fi
if test "$sfs_cv_libpthread" != "no" && test "${sfs_cv_libpthread+set}" ; then
	CFLAGS=$ac_save_CFLAGS
	CPPFLAGS="$CPPFLAGS $sfs_cv_pthread_h"
	AC_DEFINE(HAVE_PTHREADS, 1, Allow libamt to use PTHREADS)
	sfs_have_threads=yes
	LIBS="$ac_save_LIBS $sfs_cv_libpthread"
	LDADD_THR=$sfs_cv_libpthread
fi
LIBS=$ac_save_LIBS 
CFLAGS=$ac_save_CFLAGS
])
dnl
dnl Find GMP
dnl
AC_DEFUN([SFS_GMP],
[AC_ARG_WITH(gmp,
--with-gmp[[[=/usr/local]]]   specify path for gmp)
AC_SUBST(GMP_DIR)
AC_SUBST(LIBGMP)
AC_MSG_CHECKING([for GMP library])
if test "$with_gmp" != "no"; then
	ac_save_CPPFLAGS=$CPPFLAGS
	ac_save_LIBS=$LIBS
	cdirs="${with_gmp}/include ${prefix}/include"
	dirs="$cdirs /usr/local/include /usr/include"
	AC_CACHE_CHECK(for gmp.h, sfs_cv_gmp_h,
	[for dir in " " $dirs; do
		case $dir in
			" ") iflags=" " ;;
			*) iflags="-I${dir}" ;;
		esac
		CPPFLAGS="${ac_save_CPPFLAGS} $iflags"
		AC_TRY_COMPILE([#include "gmp.h"], 0,
		 sfs_cv_gmp_h="${iflags}"; break)
	done
	if test "$sfs_cv_gmp_h" = " "; then
		sfs_cv_gmp_h="yes"
	fi
	])
	if test "$sfs_cv_gmp_h" = "yes"; then
		sfs_cv_gmp_h=" "
	fi
	if test "${sfs_cv_gmp_h+set}"; then
		cdirs="${with_gmp}/lib ${prefix}/lib"
		dirs="$cdirs /usr/local/lib /usr/lib"
		AC_CACHE_CHECK(for libgmp, sfs_cv_libgmp,
		[for dir in "" " " $dirs; do
			case $dir in
				"") lflags=" "; Lflags="" ;;
				" ") lflags="-lgmp"; Lflags="" ;;
				*) Lflags="-L${dir}"; lflags="-lgmp" ;;
			esac
			LIBS="$ac_save_LIBS $Lflags $lflags"
			AC_TRY_LINK([#include "gmp.h"],
				MP_INT i; mpz_init (&i);,
				sfs_cv_libgmp=$lflags;  \
				LDFLAGS="$LDFLAGS $Lflags" ; \
				LIBGMP="$lflags" ; break)
		done
		if test -z ${sfs_cv_libgmp+set}; then
			AC_MSG_ERROR([Could not find gmp library])
			sfs_cv_libgmp="no"
		fi
		])
		LIBS="$ac_save_LIBS"
	else	
		AC_MSG_ERROR([Could not find gmp.h header])
	fi
fi
ac_save_CFLAGS="$CFLAGS"
AC_CACHE_CHECK(for overloaded C++ operators in gmp.h, sfs_cv_gmp_cxx_ops,
	AC_EGREP_CPP(operator<<,
[#define __cplusplus 1
#include <gmp.h>
],
	sfs_cv_gmp_cxx_ops=yes, sfs_cv_gmp_cxx_ops=no)
    )
test "$sfs_cv_gmp_cxx_ops" = "yes" && AC_DEFINE([HAVE_GMP_CXX_OPS], 1,
	[Define if gmp.h overloads C++ operators])

AC_CACHE_CHECK(for mpz_xor, sfs_cv_have_mpz_xor,
unset sfs_cv_have_mpz_xor
AC_EGREP_HEADER(mpz_xor, [gmp.h], sfs_cv_have_mpz_xor=yes)
)
test "$sfs_cv_have_mpz_xor" && AC_DEFINE([HAVE_MPZ_XOR], 1,
	[Define if you have mpz_xor in your GMP library.])

AC_CACHE_CHECK(size of GMP mp_limb_t, sfs_cv_mp_limb_t_size,
sfs_cv_mp_limb_t_size=no
for size in 2 4 8; do
    AC_TRY_COMPILE([#include <gmp.h>],
    [switch (0) case 0: case (sizeof (mp_limb_t) == $size):;],
    sfs_cv_mp_limb_t_size=$size; break)
done)

CFLAGS="$ac_save_CFLAGS"

test "$sfs_cv_mp_limb_t_size" = no \
    && AC_MSG_ERROR(Could not determine size of mp_limb_t.)
AC_DEFINE_UNQUOTED(GMP_LIMB_SIZE, $sfs_cv_mp_limb_t_size,
		   Define to be the size of GMP's mp_limb_t type.)])
dnl
dnl Find BekeleyDB 3
dnl
AC_DEFUN([SFS_DB3],
[AC_ARG_WITH(db3,
--with-db3[[=/usr/local]]   specify path for BerkeleyDB-3)
AC_SUBST(DB3_DIR)
AC_CONFIG_SUBDIRS($DB3_DIR)
AC_SUBST(DB3_LIB)
unset DB3_LIB

DB3_DIR=`cd $srcdir && echo db-3.*/dist/`
if test -d "$srcdir/$DB3_DIR"; then
    DB3_DIR=`echo $DB3_DIR | sed -e 's!/$!!'`
else
    unset DB3_DIR
fi

if test ! "${with_db3+set}"; then
    if test "$DB3_DIR"; then
	with_db3=yes
    else
	with_db3=no
    fi
fi

if test "$with_db3" != no; then
    AC_MSG_CHECKING([for DB3 library])
    if test "$DB3_DIR" -a "$with_db3" = yes; then
	CPPFLAGS="$CPPFLAGS "'-I$(top_builddir)/'"$DB3_DIR"
	DB3_LIB='-L$(top_builddir)/'"$DB3_DIR -ldb"
	AC_MSG_RESULT([using distribution in $DB3_DIR subdirectory])
    else
	libdbrx='^libdb-?([[3.-]].*)?.(la|so|a)$'
	libdbrxla='^libdb-?([[3.-]].*)?.la$'
	libdbrxso='^libdb-?([[3.-]].*)?.so$'
	libdbrxa='^libdb-?([[3.-]].*)?.a$'
	if test "$with_db3" = yes; then
	    for dir in "$prefix/BerkeleyDB.3.1" /usr/local/BerkeleyDB.3.1 \
		    "$prefix/BerkeleyDB.3.0" /usr/local/BerkeleyDB.3.0 \
		    /usr "$prefix" /usr/local; do
		test -f $dir/include/db.h -o -f $dir/include/db3.h \
			-o -f $dir/include/db3/db.h || continue
		if test -f $dir/lib/libdb.a \
			|| ls $dir/lib | egrep "$libdbrx" >/dev/null 2>&1; then
		    with_db3="$dir"
		    break
		fi
	    done
	fi

	if test -f $with_db3/include/db3.h; then
	    AC_DEFINE(HAVE_DB3_H, 1, [Define if BerkeleyDB header is db3.h.])
   	    if test "$with_db3" != /usr; then
	      CPPFLAGS="$CPPFLAGS -I${with_db3}/include"
	    fi
	elif test -f $with_db3/include/db3/db.h; then
   	    if test "$with_db3" != /usr; then
	      CPPFLAGS="$CPPFLAGS -I${with_db3}/include/db3"
	    fi
	elif test -f $with_db3/include/db.h; then
	    if test "$with_db3" != /usr; then
	      CPPFLAGS="$CPPFLAGS -I${with_db3}/include"
	    fi
	else
	    AC_MSG_ERROR([Could not find BerkeleyDB library version 3])
	fi

	DB3_LIB=`ls $with_db3/lib | egrep "$libdbrxla" | tail -1`
	test ! -f "$with_db3/lib/$DB3_LIB" \
	    && DB3_LIB=`ls $with_db3/lib | egrep "$libdbrxso" | tail -1`
	test ! -f "$with_db3/lib/$DB3_LIB" \
	    && DB3_LIB=`ls $with_db3/lib | egrep "$libdbrxa" | tail -1`
	if test -f "$with_db3/lib/$DB3_LIB"; then
	    DB3_LIB="$with_db3/lib/$DB3_LIB"
	elif test "$with_db3" = /usr; then
	    with_db3=yes
	    DB3_LIB="-ldb"
	else
	    DB3_LIB="-L${with_db3}/lib -ldb"
	fi
	AC_MSG_RESULT([$with_db3])
    fi
fi

AM_CONDITIONAL(USE_DB3, test "${with_db3}" != no)
])

dnl
dnl Set up Parse-Debug Flags`
dnl
AC_DEFUN([PUB_PDEBUG],
[AC_SUBST(YDEBUG)
AC_SUBST(LDEBUG)
if test $PDEBUG; then
    YDEBUG='-t'
    LDEBUG='-d'
    AC_DEFINE(PDEBUG, 1, Enable parser/lexer debugging messages)
fi])

dnl
dnl Check that kernel threads are supported on this platform
dnl
AC_DEFUN([SFS_FIND_KTHREADS],
[AC_CHECK_HEADERS(unistd.h)
AC_CHECK_FUNCS(rfork_thread rfork clone) 
if test $ac_cv_func_rfork = yes || test $ac_cv_func_clone = yes ||
	test $ac_cv_func_rfork_thread = yes ; then
    AC_DEFINE(HAVE_KTHREADS, 1, Kernel threads are available)
    sfs_have_threads=yes
fi])
dnl
dnl Check that some threading exists
dnl
AC_DEFUN([SFS_REQUIRE_THREADS],
[
AC_ARG_ENABLE(pth,
--disable-pth		  Disable GNU Pth library, [])
AC_ARG_ENABLE(pthreads,
--disable-pthreads	  Disable POSIX pthreads library, [])
if test "$enable_pth" != "no" || test -n "$with_pth"; then
	SFS_FIND_PTH	
fi
if test -z "$sfs_have_threads" &&
	(test "$enable_pthreads" != "no" || test -n $with_pthreads); 
then
	SFS_FIND_PTHREADS
fi
SFS_FIND_KTHREADS
if test -z "$sfs_have_threads"; then
	AC_MSG_ERROR(No threading packages available; cannot procede.)
fi
AC_SUBST(LDADD_THR)
])
dnl
dnl Find OpenSSL
dnl
AC_DEFUN([SFS_OPENSSL],
[AC_SUBST(OPENSSL_DIR)
AC_ARG_WITH(openssl,
--with-openssl[[=/usr/local/openssl]]   Find OpenSSL libraries)
AC_MSG_CHECKING([for OpenSSL])
test "$with_openssl" = "yes" && unset with_openssl
unset OPENSSL_DIR
if test -z "$with_openssl"; then
    with_openssl=no
    for dir in /usr/local/openssl/ /usr/local/ssl/ \
		`ls -1d /usr/local/openssl-*/ 2>/dev/null | tail -1`; do
	if test -f $dir/lib/libssl.a -a -f $dir/include/openssl/ssl.h; then
	    with_openssl=`echo $dir | sed -e 's/\/$//'`
	    break
	fi
    done
fi
OPENSSL_DIR="$with_openssl"
AC_MSG_RESULT([$with_openssl])
if test "$with_openssl" = no; then
dnl    if test -z "$with_openssl"; then
dnl	AC_MSG_ERROR([Could not find OpenSSL libraries])
dnl    fi 
    unset OPENSSL_DIR
fi])


dnl
dnl Use dmalloc if requested
dnl
AC_DEFUN([SFS_DMALLOC],
[
dnl AC_ARG_WITH(small-limits,
dnl --with-small-limits       Try to trigger memory allocation bugs,
dnl CPPFLAGS="$CPPFLAGS -DSMALL_LIMITS"
dnl test "${with_dmalloc+set}" = set || with_dmalloc=yes
dnl )
AC_CHECK_HEADERS(memory.h)
AC_ARG_WITH(dmalloc,
--with-dmalloc            use debugging malloc from www.dmalloc.com
			  (set MAX_FILE_LEN to 1024 when installing),
pref=$prefix
test "$pref" = NONE && pref=$ac_default_prefix
test "$withval" = yes && withval="${pref}"
test "$withval" || withval="${pref}"
using_dmalloc=no
if test "$withval" != no; then
	AC_DEFINE(DMALLOC, 1, Define if compiling with dmalloc. )
dnl	CPPFLAGS="$CPPFLAGS -DDMALLOC"
	CPPFLAGS="$CPPFLAGS -I${withval}/include"
	LIBS="$LIBS -L${withval}/lib -ldmalloc"
	using_dmalloc=yes
fi)
AM_CONDITIONAL(DMALLOC, test "$using_dmalloc" = yes)
])
dnl
dnl Find perl
dnl
AC_DEFUN([SFS_PERLINFO],
[AC_ARG_WITH(perl,
--with-perl=PATH          Specify perl executable to use,
[case "$withval" in
	yes|no|"") ;;
	*) PERL="$withval" ;;
esac])
if test -z "$PERL" || test ! -x "$PERL"; then
	AC_PATH_PROGS(PERL, perl5 perl)
fi
if test -x "$PERL" && $PERL -e 'require 5.004'; then :; else
	AC_MSG_ERROR("Can\'t find perl 5.004 or later")
fi
AC_CACHE_CHECK(for perl includes, sfs_cv_perl_ccopts,
	sfs_cv_perl_ccopts=`$PERL -MExtUtils::Embed -e ccopts`
	sfs_cv_perl_ccopts=`echo $sfs_cv_perl_ccopts`
)
AC_CACHE_CHECK(for perl libraries, sfs_cv_perl_ldopts,
	sfs_cv_perl_ldopts=`$PERL -MExtUtils::Embed -e ldopts -- -std`
	sfs_cv_perl_ldopts=`echo $sfs_cv_perl_ldopts`
)
AC_CACHE_CHECK(for perl xsubpp, sfs_cv_perl_xsubpp,
	sfs_cv_perl_xsubpp="$PERL "`$PERL -MConfig -e 'print qq(\
	-I$Config{"installarchlib"} -I$Config{"installprivlib"}\
	$Config{"installprivlib"}/ExtUtils/xsubpp\
	-typemap $Config{"installprivlib"}/ExtUtils/typemap)'`
	sfs_cv_perl_xsubpp=`echo $sfs_cv_perl_xsubpp`
)
XSUBPP="$sfs_cv_perl_xsubpp"
PERL_INC="$sfs_cv_perl_ccopts"
PERL_LIB="$sfs_cv_perl_ldopts"
PERL_XSI="$PERL -MExtUtils::Embed -e xsinit -- -std"
AC_SUBST(PERL)
AC_SUBST(PERL_INC)
AC_SUBST(PERL_LIB)
AC_SUBST(PERL_XSI)
AC_SUBST(XSUBPP)
])
dnl'
dnl Various warning flags for gcc.  This must go at the very top,
dnl right after AC_PROG_CC and AC_PROG_CXX.
dnl
AC_DEFUN([SFS_WFLAGS],
[AC_SUBST(NW)
AC_SUBST(WFLAGS)
AC_SUBST(CXXWFLAGS)
AC_SUBST(DEBUG)
AC_SUBST(CXXDEBUG)
AC_SUBST(ECFLAGS)
AC_SUBST(ECXXFLAGS)
AC_SUBST(CXXNOERR)
test -z "${CXXWFLAGS+set}" -a "${WFLAGS+set}" && CXXWFLAGS="$WFLAGS"
test -z "${CXXDEBUG+set}" -a "${DEBUG+set}" && CXXDEBUG="$DEBUG"
test "${DEBUG+set}" || DEBUG="$CFLAGS"
export DEBUG
test "${CXXDEBUG+set}" || CXXDEBUG="$CXXFLAGS"
export CXXDEBUG
case $host_os in
    openbsd*)
	sfs_gnu_WFLAGS="-ansi -Wall -Wsign-compare -Wchar-subscripts -Werror"
	sfs_gnu_CXXWFLAGS="$sfs_gnu_WFLAGS"
	;;
    linux*|freebsd*)
	sfs_gnu_WFLAGS="-Wall -Werror"
	sfs_gnu_CXXWFLAGS="$sfs_gnu_WFLAGS"
	;;
    *)
	sfs_gnu_WFLAGS="-Wall"
	sfs_gnu_CXXWFLAGS="$sfs_gnu_WFLAGS"
	;;
esac
expr "$DEBUG" : '.*-O' > /dev/null \
    || sfs_gnu_WFLAGS="$sfs_gnu_WFLAGS -Wno-unused"
expr "$CXXDEBUG" : '.*-O' > /dev/null \
    || sfs_gnu_CXXWFLAGS="$sfs_gnu_CXXWFLAGS -Wno-unused"
NW='-w'
test "$GCC" = yes -a -z "${WFLAGS+set}" && WFLAGS="$sfs_gnu_WFLAGS"
test "$GXX" = yes -a -z "${CXXWFLAGS+set}" && CXXWFLAGS="$sfs_gnu_CXXWFLAGS"
CXXNOERR=
test "$GXX" = yes && CXXNOERR='-Wno-error'
# Temporarily set CFLAGS to ansi so tests for things like __inline go correctly
if expr "$DEBUG $WFLAGS $ECFLAGS" : '.*-ansi' > /dev/null; then
	CFLAGS="$CFLAGS -ansi"
	ac_cpp="$ac_cpp -ansi"
fi
expr "$CXXDEBUG $CXXWFLAGS $ECXXFLAGS" : '.*-ansi' > /dev/null \
    && CXXFLAGS="$CXXFLAGS -ansi"
])
dnl
dnl SFS_CFLAGS puts the effects of SFS_WFLAGS into place.
dnl This must be called after all tests have been run.
dnl
AC_DEFUN([SFS_CFLAGS],
[unset CFLAGS
unset CXXFLAGS
CFLAGS='$(DEBUG) $(WFLAGS) $(ECFLAGS)'
CXXFLAGS='$(CXXDEBUG) $(CXXWFLAGS) $(ECXXFLAGS)'])
dnl
dnl Check for xdr_u_intNN_t, etc
dnl
AC_DEFUN([SFS_CHECK_XDR],
[
dnl AC_CACHE_CHECK([for a broken <rpc/xdr.h>], sfs_cv_xdr_broken,
dnl AC_EGREP_HEADER(xdr_u_int32_t, [rpc/xdr.h], 
dnl                 sfs_cv_xdr_broken=no, sfs_cv_xdr_broken=yes))
dnl if test "$sfs_cv_xdr_broken" = "yes"; then
dnl     AC_DEFINE(SFS_XDR_BROKEN)
dnl     dnl We need to know the following in order to fix rpc/xdr.h: 
dnl     AC_CHECK_SIZEOF(short)
dnl     AC_CHECK_SIZEOF(int)
dnl     AC_CHECK_SIZEOF(long)
dnl fi
SFS_CHECK_DECL(xdr_callmsg, rpc/rpc.h)
AC_CACHE_CHECK(what second xdr_getlong arg points to, sfs_cv_xdrlong_t,
AC_EGREP_HEADER(\*x_getlong.* long *\*, [rpc/rpc.h], 
                sfs_cv_xdrlong_t=long)
if test -z "$sfs_cv_xdrlong_t"; then
    AC_EGREP_HEADER(\*x_getlong.* int *\*, [rpc/rpc.h], 
                    sfs_cv_xdrlong_t=int)
fi
if test -z "$sfs_cv_xdrlong_t"; then
    sfs_cv_xdrlong_t=u_int32_t
fi)
AC_DEFINE_UNQUOTED(xdrlong_t, $sfs_cv_xdrlong_t,
		   What the second argument of xdr_getlong points to)
])
dnl
dnl Check for random device
dnl
AC_DEFUN([SFS_DEV_RANDOM],
[AC_CACHE_CHECK([for kernel random number generator], sfs_cv_dev_random,
for dev in /dev/urandom /dev/srandom /dev/random /dev/srnd /dev/rnd; do
    if test -c "$dev"; then
	sfs_cv_dev_random=$dev
	break
    fi
    test "$sfs_cv_dev_random" || sfs_cv_dev_random=no
done)
if test "$sfs_cv_dev_random" != no; then
pushdef([SFS_DEV_RANDOM], [[SFS_DEV_RANDOM]])
    AC_DEFINE_UNQUOTED([SFS_DEV_RANDOM], "$sfs_cv_dev_random",
		       [Path to the strongest random number device, if any.])
popdef([SFS_DEV_RANDOM])
fi
])
dnl
dnl Check for getgrouplist function
dnl
AC_DEFUN([SFS_GETGROUPLIST_TRYGID], [
if test "$sfs_cv_grouplist_t" != gid_t; then
    AC_TRY_COMPILE([
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
int getgrouplist ([$*]);
		    ], 0, sfs_cv_grouplist_t=gid_t)
fi
])
AC_DEFUN([SFS_GETGROUPLIST],
[AC_CHECK_FUNCS(getgrouplist)
AC_CACHE_CHECK([whether getgrouplist uses int or gid_t], sfs_cv_grouplist_t,
    if test "$ac_cv_func_getgrouplist" = yes; then
	sfs_cv_grouplist_t=int
	AC_EGREP_HEADER(getgrouplist.*gid_t *\*, unistd.h,
			sfs_cv_grouplist_t=gid_t)
	if test "$sfs_cv_grouplist_t" != gid_t; then
	    AC_EGREP_HEADER(getgrouplist.*gid_t *\*, grp.h,
			    sfs_cv_grouplist_t=gid_t)
	fi

	SFS_GETGROUPLIST_TRYGID(const char *, gid_t, gid_t *, int *)
	SFS_GETGROUPLIST_TRYGID(const char *, int , gid_t *, int *)
	SFS_GETGROUPLIST_TRYGID(char *, gid_t, gid_t *, int *)
	SFS_GETGROUPLIST_TRYGID(char *, int, gid_t *, int *)
    else
	sfs_cv_grouplist_t=gid_t
    fi)
AC_DEFINE_UNQUOTED([GROUPLIST_T], $sfs_cv_grouplist_t,
	[Type pointed to by 3rd argument of getgrouplist.])])
dnl
dnl Check if <grp.h> is needed for setgroups declaration (linux)
dnl
AC_DEFUN([SFS_SETGROUPS],
[AC_CACHE_CHECK([for setgroups declaration in grp.h],
	sfs_cv_setgroups_grp_h,
	AC_EGREP_HEADER(setgroups, grp.h,
		sfs_cv_setgroups_grp_h=yes, sfs_cv_setgroups_grp_h=no))
if test "$sfs_cv_setgroups_grp_h" = yes; then
AC_DEFINE([SETGROUPS_NEEDS_GRP_H], 1,
	[Define if setgroups is declared in <grp.h>.])
fi])
dnl
dnl Check if authunix_create is broken and takes a gid_t *
dnl
AC_DEFUN([SFS_AUTHUNIX_GROUP_T],
[AC_CACHE_CHECK([what last authunix_create arg points to],
	sfs_cv_authunix_group_t,
AC_EGREP_HEADER([(authunix|authsys)_create.*(uid_t|gid_t)], rpc/rpc.h,
	sfs_cv_authunix_group_t=gid_t, sfs_cv_authunix_group_t=int))
if test "$sfs_cv_authunix_group_t" = gid_t; then
    AC_DEFINE_UNQUOTED(AUTHUNIX_GID_T, 1,
	[Define if last argument of authunix_create is a gid_t *.])
fi])
dnl
dnl Check the type of the x_ops field in XDR
dnl
AC_DEFUN([SFS_XDR_OPS_T],
[AC_CACHE_CHECK([type of XDR::x_ops], sfs_cv_xdr_ops_t,
AC_EGREP_HEADER([xdr_ops *\* *x_ops;], rpc/xdr.h,
	sfs_cv_xdr_ops_t=xdr_ops, sfs_cv_xdr_ops_t=XDR::xdr_ops))
AC_DEFINE_UNQUOTED(xdr_ops_t, $sfs_cv_xdr_ops_t,
	[The C++ type name of the x_ops field in struct XDR.])])
dnl
dnl Find OKWS libraries (only need -lz for now)
dnl
AC_DEFUN([OKWS_LIBS],
[AC_CHECK_LIB(z, deflate)
if test $ac_cv_lib_z_deflate = no; then
  AC_MSG_ERROR([Could not find zlib!])
else
  ac_cv_lib_z=yes
fi
])
dnl
dnl Find installed SFS libraries
dnl This is not for SFS, but for other packages that use SFS.
dnl
AC_DEFUN([SFS_SFS],
[AC_ARG_WITH(sfs,
--with-sfs[[=PATH]]         specify location of SFS libraries)
if test "$with_sfs" = yes -o "$with_sfs" = ""; then
    for dir in "$prefix" /usr/local /usr; do
	
	dnl
	dnl sfs${sfstagdir} in there for bkwds comptability
	dnl
	sfsprefixes="sfslite${sfstagdir} sfs${sfstagdir} sfs"

	BREAKOUT=0
	for sfsprfx in $sfsprefixes
	do
	    if test -f $dir/lib/${sfsprfx}/libasync.la; then
		with_sfs=$dir
		BREAKOUT=1
		break
	    fi
	done

	if test $BREAKOUT -eq 1; then
	    break
	fi
    done
fi
case "$with_sfs" in
    /*) ;;
    *) with_sfs="$PWD/$with_sfs" ;;
esac

if test -f ${with_sfs}/Makefile -a -f ${with_sfs}/autoconf.h; then
    if egrep '#define DMALLOC' ${with_sfs}/autoconf.h > /dev/null 2>&1; then
	test -z "$with_dmalloc" -o "$with_dmalloc" = no && with_dmalloc=yes
    elif test "$with_dmalloc" -a "$with_dmalloc" != no; then
	AC_MSG_ERROR("SFS libraries not compiled with dmalloc")
    fi
    sfssrcdir=`sed -ne 's/^srcdir *= *//p' ${with_sfs}/Makefile`
    case "$sfssrcdir" in
	/*) ;;
	*) sfssrcdir="${with_sfs}/${sfssrcdir}" ;;
    esac

    CPPFLAGS="$CPPFLAGS -I${with_sfs}"
    for lib in async arpc crypt sfsmisc; do
	CPPFLAGS="$CPPFLAGS -I${sfssrcdir}/$lib"
    done
    CPPFLAGS="$CPPFLAGS -I${with_sfs}/svc"
    LIBASYNC=${with_sfs}/async/libasync.la
    LIBARPC=${with_sfs}/arpc/libarpc.la
    LIBSFSCRYPT=${with_sfs}/crypt/libsfscrypt.la
    LIBSFSMISC=${with_sfs}/sfsmisc/libsfsmisc.la
    MALLOCK=${with_sfs}/sfsmisc/mallock.o
    RPCC=${with_sfs}/rpcc/rpcc
    TAME=${with_sfs}/tame/tame
elif test -f ${with_sfs}/include/${sfsprfx}/autoconf.h \
	-a -f ${with_sfs}/lib/${sfsprfx}/libasync.la; then
    sfsincludedir="${with_sfs}/include/${sfsprfx}"
    sfslibdir=${with_sfs}/lib/${sfsprfx}
    if egrep '#define DMALLOC' ${sfsincludedir}/autoconf.h > /dev/null; then
	test -z "$with_dmalloc" -o "$with_dmalloc" = no && with_dmalloc=yes
    else
	with_dmalloc=no
    fi
    CPPFLAGS="$CPPFLAGS -I${sfsincludedir}"
    LIBASYNC=${sfslibdir}/libasync.la
    LIBARPC=${sfslibdir}/libarpc.la
    LIBSFSCRYPT=${sfslibdir}/libsfscrypt.la
    LIBSFSMISC=${sfslibdir}/libsfsmisc.la
    MALLOCK=${sfslibdir}/mallock.o

    SFS_PATH_PROG(rpcc, ${with_sfs}/lib/${sfsprfx}:${with_sfs}/bin)
    RPCC="$PATH_RPCC"
    SFS_PATH_PROG(tame, ${with_sfs}/lib/${sfsprfx}:${with_sfs}/bin)
    TAME="$PATH_TAME"
else
    AC_MSG_ERROR("Can\'t find SFS libraries")
fi


if test "$enable_static" = yes -a -z "${NOPAGING+set}"; then
    case "$host_os" in
	openbsd*)
	    test "$ac_cv_prog_gcc" = yes && NOPAGING="-Wl,-Bstatic,-N"
	    MALLOCK=		# mallock.o panics the OpenBSD kernel
	;;
	freebsd*)
	    test "$ac_cv_prog_gcc" = yes && NOPAGING="-Wl,-Bstatic"
	;;
    esac
fi

sfslibdir='$(libdir)/sfs'
sfsincludedir='$(libdir)/include'
AC_SUBST(sfslibdir)
AC_SUBST(sfsincludedir)

AC_SUBST(LIBASYNC)
AC_SUBST(LIBARPC)
AC_SUBST(LIBSFSCRYPT)
AC_SUBST(LIBSFSMISC)
AC_SUBST(RPCC)
AC_SUBST(TAME)
AC_SUBST(MALLOCK)
AC_SUBST(NOPAGING)

SFS_GMP
SFS_DMALLOC

LDEPS='$(LIBSFSMISC) $(LIBSFSCRYPT) $(LIBARPC) $(LIBASYNC)'
LDADD="$LDEPS "'$(LIBGMP)'
AC_SUBST(LDEPS)
AC_SUBST(LDADD)
])

dnl
dnl OKWS_MODULE
dnl
dnl  What to name this module; where to put it
dnl
AC_DEFUN([OKWS_MODULE],
[AC_ARG_WITH(module,
--with-module=MODULE		Specify a name for this OKWS module)
AC_ARG_WITH(module_prefix,
--with-module-prefix=PRFX	Specify an install prefix for this module)
dnl
dnl DEFAULTS:
dnl
dnl	prefix - /var/okws/modules
dnl	name   - $PACKAGE
dnl
okm_prefix=/var/okws/modules
okm_name=$PACKAGE
if test "${with_module+set}" = "set" ; then
	okm_name="$with_module"
fi
if test "${with_module_prefix+set}" = "set" ; then
	okm_prefix="$with_module_prefix"
fi
])

dnl
dnl OKWS_TAGS
dnl
dnl   Reads sfs and okws build tags, to link against the proper version
dnl   of the libraries, especially on hosts when multiple such libraries
dnl   exist
dnl
AC_DEFUN([OKWS_TAGS],
[AC_ARG_WITH(mode,
--with-mode=MODE 	Specify OKWS/SFS build mode (debug|shared|shdbg|optmz))
AC_ARG_WITH(okwstag,
--with-okwstag=TAG		Specify a custom OKWS build tag)
AC_ARG_WITH(sfstag,
--with-sfstag=TAG		Specify a custom SFS build tag)
AC_ARG_WITH(tag,
--with-tag=TAG			Specify a custom tag for this build)
case $with_mode in
	debug )
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		DEBUG=-g
		CXXDEBUG=-g
		with_dmalloc=yes
		;;
	std )
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		DEBUG=-g
		CXXDEBUG=-g
		;;
	shared )
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		enable_shared=yes
		DEBUG=-g
		CXXDEBUG=-g
		;;
	shopt)
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		enable_shared=yes
		;;
	shdbg )
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		enable_shared=yes
		DEBUG=-g
		CXXDEBUG=-g
		with_dmalloc=yes
		;;
	profile )
		sfstag=$with_mode
		okwstag=$with_mode
		okmtag=$with_mode
		DEBUG='-g -pg -O2'
		CXXDEBUG='-g -pg -O2'
		;;
	"pydbg" )
		sfstag=$with_mode
		okwstag=$with_mode
		DEBUG='-g'
		CXXDEBUG='-g'
		with_python=yes
		enable_shared=yes
		with_dmalloc=no
		SFS_PYTHON
		;;
	"python" )
		sfstag=$with_mode
		okwstag=$with_mode
		with_python=yes
		enable_shared=yes
		with_dmalloc=no
		DEBUG='-g -O'
		CXXDEBUG='-g -O'
		SFS_PYTHON
		;;
	optmz)
		## install in the parent dir if no tag
		## sfstag=lite
		okmtag=$with_mode
		;;
	* )
		## install in parent dir if no tag
		## sfstag=lite
		okmtag=optmz
		if test "${with_mode+set}" = "set"; then
			AC_MSG_ERROR([Unrecognized build mode specified])
		fi
		;;
esac
if test "${with_tag+set}" = "set" ; then
	okmtag=$with_tag
	okwstag=$with_tag
	sfstag=$with_tag
fi
if test "${with_okwstag+set}" = "set"; then
	okwstag=$with_okwstag
	sfstag=$with_okwstag
fi
if test "${with_sfstag+set}" = "set" ; then
	sfstag=$with_sfstag
fi
if test "${okwstag+set}" = "set" ; then
	okwstagdir="/$okwstag"
fi
if test "${sfstag+set}" = "set" ; then
	sfstagdir="/$sfstag"
fi
AC_SUBST(okwstagdir)
AC_SUBST(okwstag)
AC_SUBST(sfstagdir)
AC_SUBST(sfstag)
])

dnl
dnl Check to find Python Headers
dnl
dnl SFS_PYTHON(vers)
dnl
AC_DEFUN([SFS_PYTHON],
[
AC_ARG_WITH(python,
--with-python[=prog]        specify a Python interpreter )
if test "${with_python+set}"
then
	ac_save_CFLAGS=$CFLAGS
	ac_save_LIBS=$LIBS
	ac_save_CC=$CC
	ac_save_CXX=$CXX
	ac_save_LDFLAGS=$LDFLAGS

	AC_CACHE_CHECK(for python memory allocation, sfs_cv_pymalloc,
	[
	if test "$with_python" = yes; then
		py=python
	else
		py=$with_python
	fi

	pyfull=`which $py`
	if test $? -ne 0; then
		AC_MSG_ERROR(Cannot find path for Python interpreter)
	fi
	cfg="${srcdir}/py/configure.py"

	inc=-I`$pyfull $cfg -I`
	lib=`$pyfull $cfg -l`
	CC=`$pyfull $cfg -c`
	CXX=`$pyfull $cfg -x`
	LDFLAGS=`$pyfull $cfg -F`


	CFLAGS="${ac_save_CFLAGS} $inc"
	LIBS="$lib ${ac_save_LIBS}"

	sfs_cv_pymalloc="no"

	AC_TRY_LINK([#include <Python.h>
                    ],
                    [(void )PyMem_Malloc (0); 
		    PyMem_Free ((void *)0); 
		    PyMem_Realloc ((void *)0, 0);
                    ], sfs_cv_pymalloc="yes" )
	])
	if test "$sfs_cv_pymalloc" = "yes"
	then
		CPPFLAGS="$CPPFLAGS $inc"
		LIBPY="$lib"
		AC_DEFINE(PYMALLOC, 1, Use Python memory alloc funcs)

		dnl clear out configure's cache
		unset ac_cv_prog_CC
		unset ac_cv_prog_ac_ct_CC
		unset ac_cv_prog_LDFLAGS
		unset ac_cv_prog_at_ct_LDFLAGS
		AC_PROG_CC
		AC_PROG_CPP
		AC_PROG_CXX
	else
		CC=$ac_save_CC
		CXX=$ac_save_CXX
		LDFLAGS=$ac_save_LDFLAGS
	fi
	AC_SUBST(LIBPY)
	LIBS=$ac_save_LIBS
	CFLAGS=$ac_save_CFLAGS
fi
])
dnl
dnl Find installed OkCupid OKWS Libraries
dnl This is not for OKWS, but for other packages that use OKWS
dnl
AC_DEFUN([OKWS_OKWS],
[AC_ARG_WITH(okws,
--with-okws[[=PATH]]	     specify location of SFS libraries)
if test "$with_okws" = yes -o "$with_okws" = ""; then
    for dir in "$prefix" /usr/local /usr; do
	if test -f $dir/lib/okws${okwstagdir}/libpub.la; then
	    with_okws=$dir
	    break
	fi
    done
fi
case "$with_okws" in
    /*) ;;
    *) with_okws="$PWD/$with_okws" ;;
esac
if test -f ${with_okws}/Makefile -a -f ${with_okws}/okwsconf.h; then
    if egrep '#define DMALLOC' ${with_okws}/okwsconf.h > /dev/null 2>&1; then
	test -z "$with_dmalloc" -o "$with_dmalloc" = no && with_dmalloc=yes
    elif test "$with_dmalloc" -a "$with_dmalloc" != no; then
	AC_MSG_ERROR("OKWS libraries not compiled with dmalloc")
    fi
    okwssrcdir=`sed -ne 's/^srcdir *= *//p' ${with_okws}/Makefile`
    case "$okwssrcdir" in
	/*) ;;
	*) okwssrcdir="${with_okws}/${okwssrcdir}" ;;
    esac

    CPPFLAGS="$CPPFLAGS -I${with_okws}"
    for lib in libpub libahttp libaok libweb libamt libamysql; do
	CPPFLAGS="$CPPFLAGS -I${okwssrcdir}/$lib"
    done
    for lib in libpub libaok libweb libamysql; do
	CPPFLAGS="$CPPFLAGS -I${with_okws}/$lib"
    done
    LIBPUB=${with_okws}/libpub/libpub.la
    LIBAOK=${with_okws}/libaok/libaok.la
    LIBWEB=${with_okws}/libweb/libweb.la
    LIBAMT=${with_okws}/libamt/libamt.la
    LIBAHTTP=${with_okws}/libahttp/libahttp.la
    LIBAMYSQL=${with_okws}/libamysql/libamysql.la
    LIBOKXML=${with_okws}/libokxml/libokxml.la
    PUB=${with_okws}/pub/pub
elif test -f ${with_okws}/include/okws${okwstagdir}/okwsconf.h \
	-a -f ${with_okws}/lib/okws${okwstagdir}/libpub.la; then
    okwsincludedir="${with_okws}/include/okws${okwstagdir}"
    okwslibdir=${with_okws}/lib/okws${okwstagdir}
    if egrep '#define DMALLOC' ${okwsincludedir}/okwsconf.h > /dev/null; then
	test -z "$with_dmalloc" -o "$with_dmalloc" = no && with_dmalloc=yes
    else
	with_dmalloc=no
    fi
    CPPFLAGS="$CPPFLAGS -I${okwsincludedir}"
    LIBPUB=${okwslibdir}/libpub.la
    LIBAOK=${okwslibdir}/libaok.la
    LIBWEB=${okwslibdir}/libweb.la
    LIBAMT=${okwslibdir}/libamt.la
    LIBAHTTP=${okwslibdir}/libahttp.la
    LIBAMYSQL=${okwslibdir}/libamysql.la
    LIBOKXML=${okwslibdir}/libokxml.la

    dnl
    dnl hack because AC_PATH_PROG is rocked
    dnl
    OKWS_PATH_PROG(pub)
    PUB="$RES"
    if test -z $PUB; then
	PUB=pub
    fi
    RES=
    OKWS_PATH_PROG(txarpcc)
    RPCC="$RES"
    if test -z $RPCC; then
	RPCC=rpcc
    fi
    RES=
else
    AC_MSG_ERROR("Can\'t find OKWS libraries")
fi
okwslibdir='$(libdir)/okws'
okwsincludedir='$(libdir)/include'
AC_SUBST(okwslibdir)
AC_SUBST(okwsincludedir)

AC_SUBST(LIBPUB)
AC_SUBST(LIBAOK)
AC_SUBST(LIBAHTTP)
AC_SUBST(LIBAMT)
AC_SUBST(LIBWEB)
AC_SUBST(LIBAMYSQL)
AC_SUBST(LIBOKXML)
AC_SUBST(PUB)

LDEPS='$(LIBOKXML) $(LIBWEB) $(LIBAOK) $(LIBAHTTP) $(LIBPUB)'" $LDEPS"
LDEPS_DB='$(LIBAMYSQL) $(LIBAMT) '" $LDEPS"
LDADD='$(LIBOKXML) $(LIBWEB) $(LIBAOK) $(LIBAHTTP) $(LIBPUB)'" $LDADD"' $(LIBEXPAT)'
LDADD_DB='$(LIBAMYSQL) $(LIBAMT) '"$LDADD "'$(LDADD_THR) $(LDADD_MYSQL)'

AC_SUBST(LDEPS)
AC_SUBST(LDADD)
AC_SUBST(LDADD_DB)
AC_SUBST(LDEPS_DB)

dnl
dnl if not shared executables, then we'll need to compile
dnl services statically (due to jailing...)
dnl
shared_tmp=1
if test -z "$enable_shared" -o "$enable_shared" = "no"; then
	shared_tmp=0
	SVC_LDFLAGS=-all-static
fi
AC_SUBST(SVC_LDFLAGS)
AM_CONDITIONAL(DLINKED_SERVICES, test $shared_tmp -eq 1)
])


dnl
dnl SFS_SET_CLOCK
dnl
dnl   sfslite extensions for different types of SFS clocks.
dnl
AC_DEFUN([SFS_SET_CLOCK],
[AC_CACHE_CHECK(for sfs_set_clock, sfs_cv_set_clock,
[
CC_REAL=$CC
CC=$CXX
AC_TRY_COMPILE([#include "async.h"], sfs_set_clock (SFS_CLOCK_TIMER);,
	 	sfs_cv_set_clock=yes)
CC=$CC_REAL
])
if test "$sfs_cv_set_clock" = "yes"
then
	AC_DEFINE(HAVE_SFS_SET_CLOCK, 1, Toggle SFS core clock)
fi
])

dnl
dnl Find the expat libraries
dnl
AC_DEFUN([OKWS_EXPAT],
[AC_ARG_WITH(expat,
--with-expat=DIR      Specify location of expat library)
use_xml=no
if test "$with_expat" != "no"; then
	ac_save_CFLAGS=$CFLAGS
	ac_save_LIBS=$LIBS
	dirs=""
	if test ${with_expat+set} && "${with_expat}"; then
		dirs="$dirs ${with_expat} ${with_expat}/include"
	fi
	if test "${prefix}" != "NONE"; then
		dirs="$dirs ${prefix} ${prefix}/include"
	fi
	dirs="$dirs /usr/local/include /usr/include"
	AC_CACHE_CHECK(for expat.h, okws_cv_expat_h,
	[for dir in " " $dirs ; do
		case $dir in
			" ") iflags=" " ;;
			*)   iflags="-I${dir}" ;; 
		esac
		CLFAGS="${ac_save_CFLAGS} $iflags"
		AC_TRY_COMPILE([#include <expat.h>], [ XML_ParserCreate(0);],
				okws_cv_expat_h="${iflags}"; break)
	done
	if test "$okws_cv_expat_h" = " " ; then
		okws_cv_expat_h="yes"
	fi
	])
	if test "$okws_cv_expat_h" = "yes"; then
		okws_cv_expat_h=" "
	fi
	if test "${okws_cv_expat_h+set}"; then
		dirs=`echo $okws_cv_expat_h | sed 's/include/lib/' `
		dirs=`echo $dirs | sed 's/^-I//' `
		AC_CACHE_CHECK(for libexpat, okws_cv_libexpat,
		[for dir in " " $dirs; do
			case $dir in
				" ") lflags="-lexpat" ;;
				*)   lflags="-L${dir} -lexpat" ;;
			esac
			LIBS="$ac_save_LIBS $lflags"
			AC_TRY_LINK([#include <expat.h>],
				    XML_ParserCreate (0);,
				    okws_cv_libexpat=$lflags; break)
		done
		if test -z ${okws_cv_libexpat+set}; then
			okws_cv_libexpat="no"
		fi
		])
	fi
	if test "${okws_cv_expat_h+set}" && test "$okws_cv_libexpat" != "no"
	then
		CPPFLAGS="$CPPFLAGS $okws_cv_expat_h"
		AC_DEFINE(HAVE_EXPAT, 1, Enable XML support with Expat library)
		LIBEXPAT="$okws_cv_libexpat"
		use_xml=yes
	else
		AC_MSG_ERROR("No XML Support! To disable use --without-expat")
	fi
	LIBS=$ac_save_LIBS
	CFLAGS=$ac_save_CFLAGS
fi
AC_SUBST(LIBEXPAT)
AM_CONDITIONAL(USE_XML, test "${use_xml}" != "no")
])
