#!/bin/sh
#
# A script to read OKWS distributions out of CVS, to push out the appropriate
# tarballs, to make the appropriate directories in the ports hierarchy, 
# and ulitimately to make a shell archive with the most up-to-date 
# OKWS ports nonsense.
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

ac_do_make_dist()
{
    cd $SRC/$1
    ./setup
    mkdir -p $BUILD/dist/$1
    cd $BUILD/dist/$1
    $SRC/$1/cfg
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
    sudo make makesum
}

py_do_make_dist()
{
    cd $SRC/$1
    rm dist/*.tar.gz
    python setup.py sdist
    scp -q dist/*.tar.gz $DIST
}


# SFSLITE
ac_do_make_dist sfslite1
for p in sfslite sfslite-dbg sfslite-noopt
do
  make_port sfslite1/dist/freebsd-port/$p devel/$p
done

# OKWS
ac_do_make_dist okws1
for p in okws okws-dbg okws-noopt
do
  make_port okws1/dist/freebsd-port/$p www/$p
done

# Py-SFS
py_do_make_dist pysfs1
make_port pysfs1/dist/freebsd-port devel/py-sfs

# Py-OKWS
py_do_make_dist pyokws1
make_port pyokws1/dist/freebsd-port www/py-okws
