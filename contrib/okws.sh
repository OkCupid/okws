#!/bin/sh
#
# OKWS rc.d control script
#
# This is a standard FreeBSD 5.X rc.d script to control okws.  It 
# responds to all standard rc.d commands:
#
#   Usage: okld.sh [fast|force](start|stop|restart|rcvar|status|poll)
#
# In order for it to work, okws_enable="YES" needs to be set in 
# /etc/rc.conf.  A good place for this script would be 
# /usr/local/etc/rc.d.
#
# I don't recommend using 'fast'.
#
# Author:  Patrick Crosby <patrick@okcupid.com>
#
# $Id$
#

. /etc/rc.subr

name="okws"
rcvar=`set_rcvar`
command="/usr/local/sbin/okld"
okws_flags="-q"
pidfile="/var/run/okld.pid"
required_files="/usr/local/etc/okws/okws_config"
extra_command="reload"

load_rc_config $name
run_rc_command "$1"

