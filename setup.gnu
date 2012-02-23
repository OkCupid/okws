#!/bin/sh

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
if test ! -f $ACSFS
then
    for d in /usr/local/lib/sfslite-1.2  \
	/usr/local/lib/sfslite-1.1 \
	/usr/local/lib/sfslite \
	/usr/local/share/aclocal \
	/usr/local/gnu-autotools/shared/aclocal-1.9  \
	/usr/local/gnu-autotools/shared/aclocal-1.10
    do
      if test -f $d/$ACSFS; then
	  ln -s $d/$ACSFS $ACSFS
	  break
      fi
    done
fi

autoreconf -f -i -s
