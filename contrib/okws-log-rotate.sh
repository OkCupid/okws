#!/bin/sh

# 
# okws-log-rotate.sh
#
#   Script to rotate access and error logs on a daily basis
#   for okws.
#
#   Might need some editting for your particular deployment.
#
# Author:
#   Max Krohn <max@okcupid.com>
#
#-----------------------------------------------------------------------
# $Id$
#-----------------------------------------------------------------------

LOGDIR=/var/okws/log
LOGS="access_log error_log"
LOGUSER=oklogd
LOGGROUP=oklogd
OKMGR=/usr/local/bin/okmgr

#
# Reset permissions and ownerships so there are no unexpected
# suprises
#
chown -R oklogd /var/okws/log
chgrp -R oklogd /var/okws/log
find /var/okws/log -type -f -exec chmod 0644 {} \;

#
# Timestamp the log with the rotation time, so that way nothing is
# overwritten.
#
DATE=`date '+%Y%m%d%H%M%S'`

for $l in $LOGS
do
    LOG=$LOGDIR/$l
    OUTLOG=$LOG-$DATE
    if [ -f $LOG ]
    then
	mv $LOG $OUTLOG
	OUTLOGS="$OUTLOGS $OUTLOG"
    fi
done

$OKMGR -t

for l in $OUTLOGS
do
    bzip2 $l
done
