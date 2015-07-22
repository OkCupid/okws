#!/bin/bash

#
# Recommended args:
#
#	 -f -i -s


if [ ! -d m4 ]
then
   mkdir m4
fi

for f in README NEWS
do
  if [ ! -f $f ]
  then
    touch $f
  fi
done

ACSFS=acsfs.m4

if [[ "${SFS_ACLOCAL_PATH:+set}" == "set" ]]; then
    IFS=':' read -ra M4_PATH <<< "${SFS_ACLOCAL_PATH}"
else
    declare -a M4_PATH
fi

for d in "${M4_PATH[@]}" \
	/usr/local/lib/sfslite-1.2  \
	/usr/local/lib/sfslite-1.1 \
	/usr/local/lib/sfslite \
	/usr/local/share/aclocal \
	/usr/local/gnu-autotools/shared/aclocal-1.9  \
	/usr/local/gnu-autotools/shared/aclocal-1.10
    do
      if test -f $d/$ACSFS; then
	  ln -sf $d/$ACSFS $ACSFS
	  break
      fi
done

autoreconf -f -i -s
