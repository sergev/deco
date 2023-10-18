# New ports collection makefile for:    deco
# Version required:     3.8
# Date created:         12 Jul 1997
# Whom:                 vak@cronyx.ru
#
# $Id: Makefile,v 1.9 1996/11/18 10:25:14 asami Exp $
#

DISTNAME=       deco
PKGNAME=        deco-3.8
CATEGORIES=	misc
MASTER_SITES=   ftp://ftp.cronyx.ru/cronyx/
DISTFILES=      deco38.tgz

MAINTAINER=     vak@cronyx.ru

GNU_CONFIGURE=	YES
CONFIGURE_ARGS=
STRIP=
MAN1=           deco.1

post-install:
	@strip ${PREFIX}/bin/deco
	@gzip ${PREFIX}/man/man1/deco.1
	@gzip ${PREFIX}/man/man1/deco-r.1

.include <bsd.port.mk>
