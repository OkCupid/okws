#!/bin/sh
#
#
# profile_syscalls
#
#   A small script that calls strace to figure out how long we spend in
#   each of the system calls.
#
# $Id$
#

usage()
{
    echo "usage: $0 [-f<filename>] [-d<duration>] <pid>"
    exit 2
}

args=`getopt f:d: $* `
if [ $? != 0 ]
then
    usage 
fi

set -- $args

DELETEFILE=1
DURATION=10

for i
do
  case "$i"
      in
      -f)
	  FILENAME="$2"; shift ; shift
	  DELETEFILE=0
	  ;;
      -d)
	  DURATION="$2"; shift; shift;
	  ;;
      --)
	  shift; break;;
  esac
done

if [ $# -ne  1 ]
then
    usage
fi

TRACEPID=$1

if [ ! "$FILENAME" ]
then
    FILENAME=`mktemp /var/tmp/tmp.XXXXXX`
fi

strace -T -p $TRACEPID >$FILENAME 2>&1 &
BG=`jobid`
sleep $DURATION
kill $BG

perl parse_strace.pl < $FILENAME

if [ $DELETEFILE -eq 1 ]
then
    rm -f $FILENAME
fi
