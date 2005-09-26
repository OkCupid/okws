# New ports collection makefile for: okws
# Date created:        15 September 2005
# Whom:                max
#
# $FreeBSD$
#
PORTNAME=	okws
PORTVERSION=	0.2
CATEGORIES=	www
MASTER_SITES=	http://dist.okws.org/dist/

MAINTAINER=	ports@okws.org
COMMENT= 	Fast, lightweight, secure async WS for dynamic content

GNU_CONFIGURE=	yes

#
# if being called from one of the slave distributions (like
# okws-dbg), then the build-mode will be set, and we should
# rename the port accordingly so that more than one of them can
# be built.
#
.if defined(BUILD_MODE)
PKGNAMESUFFIX+= -${BUILD_MODE}
EXE_BIN?=	lib/${PORTNAME}-${PORTVERSION}/${BUILD_MODE}
.else
CONFIGURE_ARGS+= --enable-systembin
BUILD_MODE?=	shopt
EXE_BIN?=	bin
.endif # define(BUILD_MODE)

#
# rpcc does not work with byacc, so just require BISON
#
USE_BISON=	yes

#
# Pull in the Port Tree's default Mysqlclient version...
#
USE_MYSQL=	yes

#
# sfslite port to build depends on which version of OKWS we're
# pulling in (i.e., which build tag).
#
SFSLITE_PORT?=	sfslite

LIB_DEPENDS+=   async:${PORTSDIR}/devel/${SFSLITE_PORT}
LIB_DEPENDS+=   pth:${PORTSDIR}/devel/pth

#
# RPCC for compiling .x files...
#
BUILD_DEPENDS+=	${LOCALBASE}/${EXE_BIN}/rpcc:${PORTSDIR}/devel/${SFSLITE_PORT}

#
# Do not add LDCONFIG dirs, since we want SFS to link to its own
# libraries, and not our (lite) ones...
#
INSTALLS_SHLIB= yes
LDCONFIG_DIRS=  %%PREFIX%%/lib/${PORTNAME}/${BUILD_MODE}

CONFIGURE_TARGET=--build=${MACHINE_ARCH}-portbld-freebsd${OSREL}
CONFIGURE_ARGS+=  --with-mode=${BUILD_MODE}

PLIST_SUB+=	VERSION=${PORTVERSION}
PLIST_SUB+=	TAG=${BUILD_MODE}
PLIST_SUB+=	EXE_BIN=${EXE_BIN}

PKG_INSTALL?=	pkg-install


OKWS_CONFIG_DIR?=	${PREFIX}/etc/okws
OKWS_LOG_DIR?=		/var/log/okws
OKWS_DATA_DIR?=		${PREFIX}/okws
OKWS_HTDOCS_DIR?=	${OKWS_DATA_DIR}/htdocs
OKWS_MODULE_DIR?=	${OKWS_DATA_DIR}/modules

post-extract:
	mkdir -p ${FILESDIR}
	cat ${WRKSRC}/etc/freebsd/okld.sh.in			|\
		${SED} -e "s=%%EXE_BIN%%=${EXE_BIN}=g"  	|\
		${SED} -e "s=%%PREFIX%%=${PREFIX}=g"    	>\
		${FILESDIR}/okld.sh
pre-install:
	PKG_PREFIX=${PREFIX} \
	OKWS_CONFIG_DIR=${OKWS_CONFIG_DIR} \
	OKWS_LOG_DIR=${OKWS_LOG_DIR} \
	OKWS_DATA_DIR=${OKWS_DATA_DIR} \
	OKWS_HTDOCS_DIR=${OKWS_HTDOCS_DIR} \
	OKWS_MODULE_DIR=${OKWS_MODULE_DIR} \
		${SH} ${PKG_INSTALL} ${PKGNAME} PRE-INSTALL

post-install:
	PKG_PREFIX=${PREFIX} \
	OKWS_CONFIG_DIR=${OKWS_CONFIG_DIR} \
	OKWS_LOG_DIR=${OKWS_LOG_DIR} \
	OKWS_DATA_DIR=${OKWS_DATA_DIR} \
	OKWS_HTDOCS_DIR=${OKWS_HTDOCS_DIR} \
	OKWS_MODULE_DIR=${OKWS_MODULE_DIR} \
		${SH} ${PKG_INSTALL} ${PKGNAME} POST-INSTALL

	${INSTALL_DATA} ${WRKSRC}/etc/okws_config.dist \
		${OKWS_CONFIG_DIR}/okws_config.dist
	${INSTALL_DATA} ${WRKSRC}/etc/pub_config.dist \
		${OKWS_CONFIG_DIR}/pub_config.dist

	${INSTALL_SCRIPT} ${FILESDIR}/okld.sh ${PREFIX}/etc/rc.d/okld.sh

.include <bsd.port.mk>
