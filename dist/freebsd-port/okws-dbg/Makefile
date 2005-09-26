
#
# Compile without optimization and with dmalloc memory debugging
#
BUILD_MODE=	shdbg
SFSLITE_PORT=	sfslite-dbg
MASTERDIR=	${.CURDIR}/../okws

LIB_DEPENDS+=	dmalloc:${PORTSDIR}/devel/dmalloc
PKG_INSTALL=	${MASTERDIR}/pkg-install

.include "${MASTERDIR}/Makefile"
