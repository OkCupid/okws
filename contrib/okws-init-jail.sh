#!/bin/sh
##
## okws-init-jail.sh
##
##   A shell script that given an OKWS module, and an OKWS config file,
##   will initialize the jail as required, setting up the appropriate
##   directories, twiddling permissions bits, and so on.
##
## Usage:
##
##    okws-init-jail.sh [-f <config-file>] <module-name>
##
##-----------------------------------------------------------------------
## $Id: okws-init-jail.sh,v 1.1 2006/06/19 17:20:20 max Exp $
##-----------------------------------------------------------------------

#
# initialize various tools, allowing the user to overload them if
# if necessary.
#
oij_init() {

    test -z "$LDD"     || LDD=ldd
    test -z "$PERL"    || PERL=perl
    test -z "$INSTALL" || INSTALL=install
    test -z "$CMP"     || CMP=cmp
    test -z "$MKDIR"   || MKDIR='mkdir -p'
    
    if test -z "$LINKER" ; then
	name=libexec/ld-elf.so.1
	linkers="/$name /usr/$name /lib/ld-linux.so.2"
	for LINKER in $linkers; do
	    if [ -f $LINKER ]; then
		break
	    fi
	done
	if [ ! -f $LINKER ]; then
	    echo "Cannot find linker: $name"
	    exit 128
	fi
    fi
}

#
# Do usage output.
#
usage() {
    echo "usage: $0 [-f <config-file>] <module-name>]" 1>&2
    exit 2
}

args=`getopt f: $* `
if [ $? -ne 0 ] ; then
    usage
fi

oij_init
usage
