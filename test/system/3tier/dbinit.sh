#!/bin/sh

MYSQL=mysql
ROOTUSER="root"
ROOTPW="asdfqwer"
DBNAME=okws_db_tst2
OKUSER=okws
OKPW="abc123"
SQLFILE=tst2.sql
WHERE="@'localhost'"
OKUSERFULL=${OKUSER}${WHERE}

PROG=$0

mywarn()
{
    echo $* 1>&2
}

usage()
{
    mywarn "usage: $PROG [-u <user>] [-p <passwd>] <start|stop>"
    exit 1
}

root_mysql()
{
	${MYSQL} -u ${ROOTUSER} -p${ROOTPW} $*
}

ok_mysql()
{
	${MYSQL} -u ${OKUSER} -p${OKPW} ${DBNAME}
}

test_connect()
{
    ok_mysql < /dev/null > /dev/null 2>&1
    return $?
}

do_creat()
{
    echo "create database ${DBNAME} " | root_mysql 
    echo "create user ${OKUSERFULL}" | root_mysql
    echo "set password for ${OKUSERFULL} = PASSWORD('${OKPW}') " | root_mysql
    echo "grant all on ${DBNAME}.* TO ${OKUSERFULL}"| root_mysql
    echo "flush privileges"| root_mysql
    ok_mysql < ${SQLFILE}
}

do_stop()
{
    echo "drop user ${OKUSERFULL}" | root_mysql
    echo "flush privileges" | root_mysql
    echo "drop database ${DBNAME}" | root_mysql
}

do_start()
{
    test_connect
    if [ $? -ne 0 ]
    then
	do_creat
    fi
}

args=`getopt p:u:h $*`
if [ $? -ne 0 ] 
then
    usage 
fi

for i
do
  case "$i"
      in
      -u)
	  ROOTUSER=$2
	  shift ; shift
	  ;;
      -p) 
	  ROOTPW=$2
	  shift
	  shift
	  ;;
      --)
	  shift
	  break
	  ;;
  esac
done

if [ $# -ne 1 ]
then
    mywarn "No <start|stop> argument found"
    usage 
fi
mode=$1

case "$mode"
    in
    start)
	do_start
	;;
    stop)
	do_stop
	;;
    *)
	mywarn "unknown command:" $mode "; expected <stop|start>"
	;;
esac

exit 0

