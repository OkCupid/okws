dnl $Id$
dnl
dnl Find full path to program
dnl
dnl
dnl Set OKWS tag
dnl
AC_DEFUN([OKWS_TAG],
[AC_ARG_WITH(tag,
--with-tag=TAG	    	Specify a custom OKWS build tag)
AC_ARG_WITH(mode,
--with-mode=[(debug|optmz)]   Specify a build mode for OKWS)
if test "$with_tag" != "no"; then
	okwstag=$with_tag
fi
case $with_mode in

	"debug" )
		sfstag=$with_mode
		okwstag=$with_mode
		DEBUG=-g
		CXXDEBUG=-g
		with_dmalloc=yes
		;;

	"std" )
		sfstag=$with_mode
		okwstag=$with_mode
		DEBUG=-g
		CXXDEBUG=-g
		;;

	"shopt" )
		sfstag=$with_mode
		okwstag=$with_mode
		enable_shared=yes
		DEBUG='-g -O2'
		CXXDEBUG='-g -O2'
		;;

	"shared" )
		sfstag=$with_mode
		okwstag=$with_mode
		enable_shared=yes
		DEBUG=-g
		CXXDEBUG=-g
		;;

	"shdbg"  )
		sfstag=$with_mode
		okwstag=$with_mode
		enable_shared=yes
		DEBUG=-g
		CXXDEBUG=-g
		with_dmalloc=yes
		;;

	"optmz" )
		dnl default build mode; dump as usual
		dnl sfstag=lite
		DEBUG='-g -O2'
		CXXDEBUG='-g -O2'
		;;

	"hiperf" )
		 sfstag=$with_mode
		 okwstag=$with_mode
		 DEBUG='-g -O3'
		 CXXDEBUG='-g -O3'
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

	"profile" )
		sfstag=$with_mode
		okwstag=$with_mode
		DEBUG='-g -pg -O2'
		CXXDEBUG='-g -pg -O2'
		;;

	* )
		if test "${with_mode+set}" = "set" ; then
			AC_MSG_ERROR([Unrecognized build mode specified])
		fi
		dnl
		dnl Shared libraries are on by default...
		dnl but dump libraries into /usr/local/lib/okws/ 
		dnl without a TAG specification...
		dnl
		enable_shared=yes
		;;
esac

AC_ARG_WITH(sfstag,
--with-sfstag=TAG	Specify an SFS build tag other than default)
if test "${with_sfstag+set}" = "set" ; then
	sfstag=$with_sfstag
fi
if test "${okwstag+set}" = "set" ; then
	okwstagdir="/$okwstag"
fi	
if test "${sfstag+set}" = "set" ; then
	sfstagdir="/$sfstag"
fi
AC_ARG_ENABLE(systembin,
--enable-systembin	Install execs to systemwide bin despite tag)
if test "${enable_systembin+set}" = "set" ; then
	okws_systembin=yes
fi
AC_SUBST(okws_systembin)
AC_SUBST(okwstagdir)
AC_SUBST(okwstag)
])
dnl
dnl Find Mysql
dnl
AC_DEFUN([OKWS_MYSQL],
[AC_ARG_WITH(mysql,
--with-mysql=DIR          Specific location of mysqlclient library)
if test "$with_mysql" != "no"; then
   	libname="mysqlclient"

	dnl For true pthread multithreading, we need the threaded
	dnl reentrant version of mysqlclient
	if test "${ac_do_pthreads}" = "1"; then
	   libname="mysqlclient_r"
	fi

	ac_save_CFLAGS=$CFLAGS
	ac_save_LIBS=$LIBS
	cdirs="${with_mysql}/include ${with_mysql}/include/mysql \
	       ${prefix}/include ${prefix}/include/mysql"
	dirs="$cdirs /usr/local/include/mysql /usr/local/mysql/include \
               /usr/include/mysql /opt/local/include/mysql5/mysql "
	AC_CACHE_CHECK(for mysql.h, sfs_cv_mysql_h,
	[for dir in " " $dirs; do
		case $dir in
			" ") iflags=" " ;;
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
                      /usr/lib/mysql /opt/local/lib/mysql5/mysql "
		AC_CACHE_CHECK(for libmysqlclient, sfs_cv_libmysqlclient,
		[for dir in "" " " $dirs; do
			case $dir in
				"") lflags=" " ;;
				" ") lflags="-l$libname -lm -lssl -lpthread -lrt" ;;
				*) lflags="-L${dir} -l$libname -lm -lssl -lpthread -lrt" ;;
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
AC_DEFUN([OKWS_FIND_PTH],
[AC_ARG_WITH(pth,
--with-pth=DIR		  Specify location of GNU Pth library)
if test "$with_pth" != "no"
then
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
		LDADD_THR="$LDADD_THR $sfs_cv_libpth"
	else
		AC_MSG_ERROR("Pth failed. To disable Pth use --without-pth")
	fi
	LIBS=$ac_save_LIBS
	CFLAGS=$ac_save_CFLAGS
fi
AC_SUBST(LDADD_THR)
])

dnl
dnl Find pthreads
dnl
AC_DEFUN([OKWS_DO_PTHREADS],
[AC_ARG_ENABLE(pthreads,
--enable-pthreads     Allow OKWS to use standard pthreads)
if test `uname` = "Linux"; then 
   	ac_do_pthreads=1
	if test "${enable_pthreads}" = "no";  then
	   ac_do_pthreads=0
	fi	
else
	ac_do_pthreads=0
	if test "${enable_pthreads}" = "yes" ; then
	   ac_do_pthreads=1
 	fi
fi

if test $ac_do_pthreads -eq 1 ; then
   AC_DEFINE(HAVE_PTHREADS, 1, Use pthread support)
fi

AM_CONDITIONAL(USE_PTHREADS, test ${ac_do_pthreads} -eq 1)

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
dnl Check that some threading exists
dnl
AC_DEFUN([OKWS_DO_THREADS],
[
if test `uname` != "Linux"
then
	OKWS_FIND_PTH	
fi
OKWS_DO_PTHREADS
])

#include <unistd.h>
#include <grp.h>
int getgrouplist ([$*]);
		    ], 0, sfs_cv_grouplist_t=gid_t)
fi
])
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
dnl Figure out if struct tm has a tm_gmoff field or not.
dnl From what I can tell, FreeBSD does, glibc doesn't.
dnl
AC_DEFUN([OKWS_GMTOFF],
[AC_CACHE_CHECK(for tm_gmtoff in struct tm, okws_cv_gmtoff,
[AC_TRY_COMPILE([#include <time.h>
], [ struct tm t; t.tm_gmtoff = 1; ], okws_cv_gmtoff="tm_gmtoff" )
if test ! ${okws_cv_gmtoff+set} ; then
   AC_TRY_COMPILE([#include <time.h>
   ], [ struct tm t; t.__tm_gmtoff = 1; ], okws_cv_gmtoff="__tm_gmtoff" )
fi
])
if test ${okws_cv_gmtoff+set} ; then
   AC_DEFINE_UNQUOTED(STRUCT_TM_GMTOFF, $okws_cv_gmtoff, struct tm has a tm_gmtoff field)
fi
])
dnl
dnl Figure out if RPC_UNKNOWNADDR is defined or not
dnl
AC_DEFUN([OKWS_RPC_UNKNOWNADDR],
[AC_CACHE_CHECK(for RPC_UNKNOWNADDR in rpc/rpc.h, okws_cv_rpc_unknownaddr,
[AC_TRY_COMPILE([#include <rpc/rpc.h>
], [ int i = RPC_UNKNOWNADDR; i++; ], okws_cv_rpc_unknownaddr="yes" )
])
if ! test ${okws_cv_rpc_unknownaddr+set} ; then
    AC_DEFINE_UNQUOTED(RPC_UNKNOWNADDR, clnt_stat(19), hard-code this for Mac OS/X)
fi
])

dnl
dnl Find the SSL libraries
dnl
AC_DEFUN([OKWS_SSL],
[AC_ARG_WITH(ssl,
--with-ssl=DIR      Specify location of expat library)
use_ssl=no
LIBSSL=""
sslbaselib="-lssl -lcrypto"
if test "$with_ssl" != "no"; then
	ac_save_CFLAGS=$CFLAGS
	ac_save_LIBS=$LIBS
	dirs=""
	if test ${with_ssl+set} -a "${with_ssl}"; then
		dirs="$dirs ${with_ssl} ${with_ssl}/include"
	fi
	if test "${prefix}" != "NONE"; then
		dirs="$dirs ${prefix} ${prefix}/include"
	fi
	dirs="$dirs /usr/local/include /usr/include"
	AC_CACHE_CHECK(for ssl.h, okws_cv_ssl_h,
	[for dir in " " $dirs ; do
		case $dir in
			" ") iflags=" " ;;
			*)   iflags="-I${dir}" ;; 
		esac
		CFLAGS="${ac_save_CFLAGS} $iflags"
		AC_TRY_COMPILE([#include <openssl/ssl.h>], 
                               [ (void)SSL_new((SSL_CTX *)NULL);],
				okws_cv_ssl_h="${iflags}"; break)
	done
	if test "$okws_cv_ssl_h" = " " ; then
		okws_cv_ssl_h="yes"
	fi
	])

dnl Check for no compression support from SSL
    AC_CACHE_CHECK(for ssl with compression disable, okws_cv_sslnc,
    [for dir in " " $dirs ; do
		case $dir in
			" ") iflags=" " ;;
			*)   iflags="-I${dir}" ;; 
		esac
		CFLAGS="${ac_save_CFLAGS} $iflags"
		AC_TRY_COMPILE([#include <openssl/ssl.h>], 
                               [ (void)SSL_new((SSL_CTX *)NULL);
                                 SSL_CTX_set_options(NULL, SSL_OP_NO_COMPRESSION);],
				okws_cv_sslnc="${iflags}"; break)
	done
	if test "$okws_cv_sslnc" = " " ; then
		okws_cv_sslnc="yes"
	fi
    if test -z $okws_cv_sslnc; then
        okws_cv_sslnc="no"
    fi
	])

	if test "$okws_cv_ssl_h" = "yes"; then
		okws_cv_ssl_h=" "
	fi

    if test "$okws_cv_sslnc" != "yes" && test "$okws_cv_sslnc" != "no"; then
        okws_cv_ssl_h=$okws_cv_sslnc
    fi

	if test "${okws_cv_ssl_h+set}"; then
		dirs=`echo $okws_cv_ssl_h | sed 's/include/lib/' `
		dirs=`echo $dirs | sed 's/^-I//' `
		dirs="$dirs /usr/lib /usr/local/lib"
		AC_CACHE_CHECK(for libssl, okws_cv_libssl,
		[for dir in " " $dirs; do
			case $dir in
				" ") lflags="${sslbaselib}" ;;
				*)   lflags="-L${dir} ${sslbaselib}" ;;
			esac
			LIBS="$ac_save_LIBS $lflags"
			AC_TRY_LINK([#include <openssl/ssl.h>],
				    [(void)SSL_new((SSL_CTX *)NULL);],
				    okws_cv_libssl=$lflags; break)
		done
		if test -z ${okws_cv_libssl+set}; then
			okws_cv_libssl="no"
        elif test "$okws_cv_sslnc" != "yes" -a "$okws_cv_sslnc" != "no"; then
            ldir=`echo $okws_cv_ssl_h | sed 's/include/lib/' `
            ldir=`echo $ldir | sed 's/^-I//' `
            okws_cv_libssl="-L${ldir} -Wl,-rpath ${ldir} $okws_cv_libssl"
        fi
		])
	fi

	if test "${okws_cv_ssl_h+set}" && test "$okws_cv_libssl" != "no"
	then
		CPPFLAGS="$CPPFLAGS $okws_cv_ssl_h"
		AC_DEFINE(HAVE_SSL, 1, Enable OpenSSL support)
        if test "$okws_cv_sslnc" != "no"; then
            AC_DEFINE(HAVE_SSL_NOCOMP, 1, Enable OpenSSL compression disable support)
        fi
		LIBSSL="$okws_cv_libssl"
		use_ssl=yes
	else
		AC_MSG_ERROR("No OpenSSL Support! To disable use --without-ssl")
	fi
	LIBS=$ac_save_LIBS
	CFLAGS=$ac_save_CFLAGS
fi
AC_SUBST(LIBSSL)
AM_CONDITIONAL(USE_SSL, test "${use_ssl}" != "no")
])
dnl
dnl Find the expat libraries
dnl
AC_DEFUN([OKWS_EXPAT],
[AC_ARG_WITH(expat,
--with-expat=DIR      Specify location of expat library)
use_xml=no
LIBEXPAT=""
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
		CFLAGS="${ac_save_CFLAGS} $iflags"
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

dnl
dnl Find Snappy
dnl
AC_DEFUN([OKWS_SNAPPY],
[AC_ARG_WITH(snappy,
--with-snappy=DIR       Specify location of Snappy)
ac_save_CFLAGS=$CFLAGS
ac_save_LIBS=$LIBS
CC_REAL=$CC
CC=$CXX
dirs="$with_snappy ${prefix} ${prefix}/snappy"
dirs="$dirs /usr/local /usr/local/snappy"
AC_CACHE_CHECK(for snappy.h, okws_cv_snappy_h,
[for dir in " " $dirs; do
    iflags="-I${dir}/include"
    CFLAGS="${ac_save_CFLAGS} $iflags"
    AC_TRY_COMPILE([#include <snappy.h>], 0,
	okws_cv_snappy_h="${iflags}"; break)
done])
if test -z "${okws_cv_snappy_h+set}"; then
    AC_MSG_ERROR("Can\'t find snappy.h anywhere")
fi
AC_CACHE_CHECK(for libsnappy, okws_cv_libsnappy,
[for dir in "" " " $dirs; do
    case $dir in
	"") lflags=" " ;;
	" ") lflags="-lsnappy" ;;
	*) lflags="-L${dir}/lib -lsnappy" ;;
    esac
    LIBS="$ac_save_LIBS $lflags"
    AC_TRY_LINK([#include <snappy.h>],
	snappy::Compress (NULL, NULL);,
	okws_cv_libsnappy=$lflags; break)
done])
if test -z ${okws_cv_libsnappy+set}; then
    AC_MSG_ERROR("Can\'t find libsnappy anywhere")
fi
CC=$CC_REAL
CFLAGS=$ac_save_CFLAGS
CPPFLAGS="$CPPFLAGS $okws_cv_snappy_h"
LIBS="$ac_save_LIBS $okws_cv_libsnappy"])

dnl
dnl OKWS_PREFIX
dnl
dnl Make a prefix for OKWS to put all of its crap, like config files,
dnl pub templates, logs and modules.  Note that lib/ and include/ will
dnl be under the systemwide lib/ and include/, though maybe in the 
dnl future it would make sense to move them under the prefix.
dnl
AC_DEFUN([OKWS_PREFIX],
[AC_ARG_WITH(okws_prefix,
--with-okws-prefix=PATH         Specify an OKWS prefix (PREFIX/okws by default)
)
AC_ARG_WITH(okws_modules_dir,
--with-okws-modules-dir=PATH   module prefix (okws_prefix/modules by default)
)
AC_ARG_WITH(okws_config_dir,
--with-okws-config-dir=PATH    path to default configuration dir)

okws_prefix='${prefix}/okws'
if test "${with_okws_prefix+set}" = "set" ; then
	okws_prefix="$with_okws_prefix"
fi
okws_modules_dir='${okws_prefix}/modules'
if test "${with_okws_modules_dir+set}" = "set"; then
	okws_modules_dir="$with_okws_modules_dir"
fi
okwsconfdir='/usr/local/etc/okws'
if test "${with_okws_config_dir+set}" = "set"; then
	okwsconfdir="$with_okws_config_dir"
fi
AC_DEFINE_UNQUOTED(OKWS_CONFIG_DIR, "$okwsconfdir", 
	First in the search path for OKWS configuration)

okwsbuildtoolsdir='${okws_prefix}/buildtools'
okwshtdocsdir='${okws_prefix}/htdocs'
okwshtdocsconfdir='${okwshtdocsdir}/conf'
okwshtdocserrdir='${okwshtdocsdir}/err'
okwshtdocsimgdir='${okwshtdocsdir}/img'
okwshtdocslangdir='${okwshtdocsdir}/lang'
okwshtdocsdocsdir='${okwshtdocsdir}/docs'
okwsutildir='${okws_prefix}/bin'

AC_SUBST(okws_prefix)
AC_SUBST(okws_modules_dir)
AC_SUBST(okwsbuildtoolsdir)
AC_SUBST(okwsconfdir)
AC_SUBST(okwshtdocsdir)
AC_SUBST(okwshtdocserrdir)
AC_SUBST(okwshtdocslangdir)
AC_SUBST(okwshtdocsconfdir)
AC_SUBST(okwshtdocsimgdir)
AC_SUBST(okwshtdocsdocsdir)
AC_SUBST(okwsutildir)
])

dnl
dnl OKWS_MODULE
dnl
dnl A module is a bundle of services, libraries, and proxies that make
dnl a Web site running on this machine
dnl
AC_DEFUN([OKWS_MODULE],
[AC_ARG_WITH(module_name,
--with-module-name=NAME  module name ('PACKAGE' by default')
)
module_name='${PACKAGE}'
if test "${with_module_name+set}" = "set"; then
	module_name="$with_module_name"
fi

module_dir='${okws_modules_dir}/${okwstag}/${module_name}'
okwssvcdir='${module_dir}/svc'
okwsprxdir='${module_dir}/dbprox'

AC_SUBST(module_dir)
AC_SUBST(module_name)
AC_SUBST(okwssvcdir)
AC_SUBST(okwsprxdir)
])

dnl
dnl majorly ghetto but have had trouble with SFS_PATH_PROG in the 
dnl past.
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
dnl Find installed OkCupid OKWS Libraries
dnl This is not for OKWS, but for other packages that use OKWS
dnl
AC_DEFUN([OKWS_OKWS],
[AC_ARG_WITH(okws,
--with-okws[[=PATH]]	     specify location of SFS libraries)
AC_ARG_WITH(okws-version,
--with-okws-version=[[VERSION]] Specify a Major.Minor OKWS version)

dnl
dnl Need to set the OKWS prefixes for OKWS standard install scripts,
dnl contrib, etc.
dnl
OKWS_PREFIX

if test "$with_okws" = yes -o "$with_okws" = ""; then

    if test "$with_okws_version"
    then
	okwsvers="-${with_okws_version}"
    else
	okwsvers=""
    fi

    okwsstem="okws${okwsvers}${okwstagdir}"

    for dir in "$prefix" /usr/local /usr; do
	if test -f $dir/lib/${okwsstem}/libpub.la; then
	    with_okws=$dir
	    break
	fi
    done
fi

case "$with_okws" in
    /*) ;;
    *) with_okws="$PWD/$with_okws" ;;
esac

echo "1) with_okws = ${with_okws}" >&5

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
    LIBOKXML=${with_okws}/libokxml/libokxml.la
    LIBWEB=${with_okws}/libweb/libweb.la
    LIBAMT=${with_okws}/libamt/libamt.la
    if test "${ac_do_pthreads}" = "1"; then
       LIBAMT_PTHREAD=${with_okws}/libamt_pthread/libamt_pthread.la
    fi
    LIBAHTTP=${with_okws}/libahttp/libahttp.la
    LIBAMYSQL=${with_okws}/libamysql/libamysql.la
    LIBOKSSL=${with_okws}/libokssl/libokssl.la
    LIBRFN=${with_okws}/librfn/librfn.la
    PUB=${with_okws}/pub/pub
    XMLRPCC=${with_okws}/xmlrpcc/xmlrpcc

    XMLRPCC_COLLECT=${with_okws}/contrib/xmlrpcc-x-collect.pl
elif test -f ${with_okws}/include/${okwsstem}/okwsconf.h \
	-a -f ${with_okws}/lib/${okwsstem}/libpub.la; then
    okwsincludedir="${with_okws}/include/${okwsstem}"
    okwslibdir=${with_okws}/lib/${okwsstem}
    if egrep '#define DMALLOC' ${okwsincludedir}/okwsconf.h > /dev/null; then
	test -z "$with_dmalloc" -o "$with_dmalloc" = no && with_dmalloc=yes
    else
	with_dmalloc=no
    fi
    CPPFLAGS="$CPPFLAGS -I${okwsincludedir}"
    LIBPUB=${okwslibdir}/libpub.la
    LIBAOK=${okwslibdir}/libaok.la
    LIBOKXML=${okwslibdir}/libokxml.la
    LIBWEB=${okwslibdir}/libweb.la
    LIBAMT=${okwslibdir}/libamt.la
    if test "${ac_do_pthreads}" = "1"; then
       LIBAMT_PTHREAD=${okwslibdir}/libamt_pthread.la
    fi
    LIBAHTTP=${okwslibdir}/libahttp.la
    LIBAMYSQL=${okwslibdir}/libamysql.la
    LIBOKSSL=${okwslibdir}/libokssl.la
    LIBRFN=${okwslibdir}/librfn.la
    OKWS_LIB_MK=${okwslibdir}/env.mk

    XMLRPCC_COLLECT=${okwsutildir}/xmlrpcc-x-collect.pl

    dnl
    dnl hack because AC_PATH_PROG is rocked
    dnl
    OKWS_PATH_PROG(pub)
    PUB="$RES"
    if test -z "$PUB"; then
	PUB=pub
    fi
    RES=
    OKWS_PATH_PROG(xmlrpcc)
    XMLRPCC="$RES"
    if test -z "$XMLRPCC"; then
	XMLRPCC=xmlrpcc
    fi
    RES=
else
    AC_MSG_ERROR("Can't find OKWS libraries")
fi
okwslibdir='$(libdir)/okws'
okwsincludedir='$(libdir)/include'
AC_SUBST(okwslibdir)
AC_SUBST(okwsincludedir)

AC_SUBST(LIBPUB)
AC_SUBST(LIBAOK)
AC_SUBST(LIBOKXML)
AC_SUBST(LIBAHTTP)
AC_SUBST(LIBRFN)
AC_SUBST(LIBAMT)
AC_SUBST(LIBAMT_PTHREAD)
AC_SUBST(LIBWEB)
AC_SUBST(LIBOKSSL)
AC_SUBST(LIBAMYSQL)
AC_SUBST(PUB)
AC_SUBST(XMLRPCC)
AC_SUBST(XMLRPCC_COLLECT)
AC_SUBST(OKWS_LIB_MK)

LIBS='$(LIBEXPAT) $(LIBSSL)'"$LIBS"

LDEPS='$(LIBRFN) $(LIBWEB) $(LIBOKSSL) $(LIBAOK) $(LIBOKXML) $(LIBAHTTP) $(LIBPUB)'" $LDEPS"
LDEPS_DB='$(LIBAMYSQL) $(LIBAMT) $(LIBAMT_PTHREAD) '" $LDEPS"
LDADD='$(LIBRFN) $(LIBWEB) $(LIBOKSSL) $(LIBAOK) $(LIBAHTTP) $(LIBOKXML) $(LIBPUB)'" $LDADD"
LDADD_DB='$(LIBAMYSQL) $(LIBAMT) $(LIBAMT_PTHREAD)'"$LDADD "'$(LDADD_THR) $(LDADD_MYSQL)'

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
dnl
dnl
AC_DEFUN([OKWS_FLEX_VERSION],
[
AC_ARG_ENABLE(leaky_flex,
--enable-leaky-flex	Allow OKWS to compile with old (leaky) flex)
AM_PROG_LEX
dnl
dnl Wanted version is flex 2.5.33 or greater
dnl
actual=`$LEX --version`
current_flex=0
perl ${srcdir}/build/flex_vers.pl flex 2.5.33 "$actual"
if test $? -eq  0; then
	current_flex=1
	AC_DEFINE(HAVE_RECENT_FLEX, 1, Define if Flex version is >= 2.5.33)
else 
	if test "${enable_leaky_flex+set}" != "set" ; then
		AC_MSG_ERROR([Old flex version found; either upgrade or override with --enable-leaky-flex])
	fi
	
fi
AM_CONDITIONAL(MAKE_OLD_FLEX, test ${current_flex} -eq 0)
])

 
dnl
dnl Check for Linux prctl() to get coredumps after setuid/setgid
dnl
AC_DEFUN([OKWS_LINUX_PRCTL_DUMP],
[
AC_MSG_CHECKING(for Linux prctl(PR_SET_DUMPABLE))
AC_TRY_COMPILE(
[
#include <sys/prctl.h>
],
[prctl(PR_SET_DUMPABLE, 1);],
linux_prctl=yes, linux_prctl=no)
if test "$linux_prctl" = "yes"
then
    AC_DEFINE([HAVE_LINUX_PRCTL_DUMP], [1], [prctl(PR_SET_DUMPABLE, 1) can be used])
fi
echo $linux_prctl
])

dnl
dnl Find BISON in particular, not YACC
dnl
AC_DEFUN([OKWS_BISON],
[AC_PROG_YACC
echo "$YACC" | grep "bison" > /dev/null
if test $? -ne 0
then
   AC_MSG_ERROR("Cannot find a working implementation of `bison'")
fi
BISON="$YACC"
AC_SUBST(BISON)
])
