#!/bin/sh

if test -z $1 ; then
    DIR=`pwd`
else 
    DIR=$1
fi

TARFILE=`echo $0 | sed 's/\.sh$/\.tar/'`
mkdir $DIR
cp $TARFILE $DIR
cd $DIR
tar -xvf bld.tar
rm bld.tar
