#!/bin/sh
#
# $Id$

sysctl kern.sugid_coredump=1
sysctl kern.coredump=1
sysctl kern.corefile="/var/coredumps/%U/%N.core"


