#!/bin/sh

PROGS="okd/okd 
      logd/oklogd 
      pub/pubd 
      test/system/xmlex 
      test/system/static 
      test/system/configtest 
      test/system/simple 
      test/system/pub1tst 
      test/system/xmlxlate/xlatetst"

for p in $PROGS
do
  if [ -e $p ] ; then
      $p -? > /dev/null 2>&1
  fi
done
