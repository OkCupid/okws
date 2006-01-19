#!/bin/sh
#
# jail.sh
#
#   usage:
#       jail.sh <jaildir> <exes>
#
#   Scans all of the exes for dynamically linked libraries, and copies
#   those libraries, as necessary, into the specified jail directory.
#   Also copies the runtime linker, and the linker hints file, so
#   that runtime linking should work for jailed OKWS executables.
#
#-----------------------------------------------------------------------
# $Id$
#-----------------------------------------------------------------------
#

JAILDIR=$1
shift


if test -z "$LDD" ; then
    LDD=ldd
fi

if test -z "$PERL" ; then
    PERL=perl
fi

if test -z "$INSTALL" ; then
    INSTALL=install
fi

if test -z "$DIFF" ; then
    DIFF=diff
fi

if test -z "$LINKER" ; then
    name=libexec/ld-elf.so.1
    linkers="/$name /usr/$name /lib/ld-linux.so.2"
    for LINKER in $linkers
    do
	if [ -f $LINKER ]
	then
	    break
	fi
    done
    if [ ! -f $LINKER ]
    then
	echo "Cannot find linker: $name"
	exit 128
    fi
fi

if test -z "$MKDIR" ; then
    MKDIR='mkdir -p'
fi

if test -z "$LINKER_HINTS" ; then
    LINKER_HINTS='/var/run/ld-elf.so.hints /var/run/ld.so.hints /etc/ld.so.cache'
fi

# Only consider executable files and non-directories
for i in $*
do
    if test -x $i -a ! -d $i
    then
	FILES="$FILES $i"
    fi
done

LIBS=`$LDD $FILES | \
      $PERL -ne '{ print "$1\n" if /=>\s*(\S+)/; }' | \
      sort | uniq | xargs `

for lib in $LIBS $LINKER $LINKER_HINTS
do
    jlib="$JAILDIR$lib"
    $DIFF $jlib $lib > /dev/null 2>& 1
    diff=$?
    pdir=`echo $jlib | sed -ne 's/\/[^/]*$//p' `
    if test ! -d $pdir; then
	echo "making directory: $pdir"
	$MKDIR $pdir
    fi
    if test '(' ! -f $jlib -o ! $diff = 0 ')' -a -f $lib
    then
	echo $INSTALL $lib $jlib
	$INSTALL $lib $jlib
    fi
done
    
