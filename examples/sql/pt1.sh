#!/bin/sh

#
# Script to load up the database for the pt1.g/pt1d.C test suite
#
# $Id$

TAB=pt1
CLIHOST="127.0.0.1"
SQLOPTS="-u root"
MYSQL="mysql $SQLOPTS"


echo "GRANT ALL ON *.* to 'root'@'$CLIHOST' ; FLUSH PRIVILEGES ; " |  $MYSQL 

mysqladmin $SQLOPTS create $TAB
$MYSQL $TAB < pt1.sql
perl pt1-load.pl $CLIHOST
