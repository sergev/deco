# deco Makefile
V= 3.9
PORT= deco
PROG= ${PORT}
SRCS= cap.c choice.c cmd.c com.c dir.c draw.c edit.c env.c ex.c help.c \
      hexview.c key.c main.c menu.c mh.c pw.c rec.c run.c scr.c tty.c ut.c \
      view.c

CFLAGS+= -ffunction-sections -fdata-sections -DHAVE_DIRENT_H=1 -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_FCNTL_H=1 -DHAVE_SYS_IOCTL_H=1 -DHAVE_TERMIO_H=1 -DHAVE_TERMIOS_H=1 -DHAVE_UNISTD_H=1 -DHAVE_TERMCAP_H=1 -DHAVE_ST_RDEV=1 -DSTDC_HEADERS=1 -DGETGROUPS_T=gid_t -DRETSIGTYPE=void -DHAVE_VPRINTF=1 -DHAVE_GETHOSTNAME=1 -DHAVE_GETGROUPS=1 -DHAVE_OPENDIR=1 -DHAVE_MEMCPY=1 -DHAVE_STRTOL=1 -DHAVE_DIRFD=1 -DHAVE_TCGETATTR=1 -DHAVE_TCSETPGRP=1 -DHAVE_KILLPG=1 -DHAVE_LIBNCURSES=1 
LDFLAGS+= -Wl,--gc-sections -s
LDADD+= -lncurses

beforeinstall:
	install -d ${DESTDIR}${PREFIX}/lib/deco/help
	install -c -m 444 menu ${DESTDIR}${PREFIX}/lib/deco/menu
	install -c -m 444 profile ${DESTDIR}${PREFIX}/lib/deco/profile
	for f in help/*.general help/*.edit help/*.view help/*.hexview; do \
	    install -c -m 444 $$f ${DESTDIR}${PREFIX}/lib/deco/$$f; done

.include <bsd.port.mk>
