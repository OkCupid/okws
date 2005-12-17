#!/bin/sh
#
# A script to read OKWS distributions out of CVS, to push out the appropriate
# tarballs, to make the appropriate directories in the ports hierarchy, 
# and ulitimately to make a shell archive with the most up-to-date 
# OKWS ports nonsense.
#
# Assumes devel/sfslite and devel/py-sfs have already been installed!
#
# Author: Maxwell Krohn
#   ports@okws.org
#
# $Id$
#

SRC=~
BUILD=~/build
DIST=rael.lcs.mit.edu:okdist/dist
PORTS=/usr/ports
PATH=/usr/local/gnu-autotools/bin:$PATH

ac_do_make_dist()
{
    cd $SRC/$1
    if [ ! -f configure ]; then
	autoreconf -f -i -s
    fi
    mkdir -p $BUILD/dist/$1
    cd $BUILD/dist/$1
    if [ ! -f Makefile ] ; then
	$SRC/$1/cfg
    fi
    rm -f *.tar.gz
    gmake
    gmake dist
    scp -q *.tar.gz $DIST
}

make_port()
{
    cd $SRC
    sudo mkdir -p $PORTS/$2
    for f in Makefile pkg-descr pkg-install pkg-plist
    do
      if [ -f $1/$f ]; then
	  sudo cp $1/$f $PORTS/$2
      fi
    done
    cd $PORTS/$2
    sudo rm -f distinfo
    sudo make distclean
    sudo make makesum
}

py_do_make_dist()
{
    cd $SRC/$1
    rm -f dist/*.tar.gz

    # Need to regenerate local file
    python setup.py -Tshopt -G sdist
    scp -q dist/*.tar.gz $DIST
}

if [ ! -f /usr/local/bin/rpcc -o ! -f /usr/local/lib/sfslite/shopt/env.mk ]
    then
    echo "Must have devel/sfslite and devel/py-sfs already installed!"
    exit 1
fi

LIST=""

# SFSLITE
ac_do_make_dist sfslite1
for p in sfslite sfslite-dbg sfslite-noopt
do
  make_port sfslite1/dist/freebsd-port/$p devel/$p
  LIST="$LIST devel/$p"
done

# OKWS
ac_do_make_dist okws1
for p in okws okws-dbg okws-noopt
do
  make_port okws1/dist/freebsd-port/$p www/$p
  LIST="$LIST www/$p"
done

# Py-SFS
py_do_make_dist pysfs1
make_port pysfs1/dist/freebsd-port devel/py-sfs
LIST="$LIST devel/py-sfs"

# Py-OKWS
py_do_make_dist pyokws1
make_port pyokws1/dist/freebsd-port www/py-okws
LIST="$LIST www/py-okws"

for p in $LIST
do
  LIST2="$LIST2 $PORTS/$p"
done

SHAR_OUT=$BUILD/okws-ports.shar

shar `find $LIST2` > $SHAR_OUT
scp -q $SHAR_OUT $DIST/freebsd/

echo "Wrote shell-archive: $SHAR_OUT"
