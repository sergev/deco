/*
 *      TtySet ()
 *
 *              - set terminal CBREAK mode.
 *
 *      TtyReset ()
 *
 *              - restore terminal mode.
 *
 *      TtyFlushInput ()
 *              - flush input queue.
 */

#if HAVE_TERMIOS_H
#include <sys/termios.h>
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#define HAVE_TERMIO_H 1
#define termio        termios
#ifdef sun
#undef TCGETA
#undef TCSETA
#undef TCSETAW
#undef TIOCSLTC
#define TCGETA  TCGETS
#define TCSETA  TCSETS
#define TCSETAW TCSETSW
#endif
#else
#if HAVE_TERMIO_H
#include <termio.h>
#else
#include <sgtty.h>
#endif
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "scr.h"

#ifndef TCGETA
#define TCGETA TIOCGETA
#endif
#ifndef TCSETA
#define TCSETA TIOCSETA
#endif
#ifndef TCSETAW
#define TCSETAW TIOCSETAW
#endif
#ifndef OLCUC
#define OLCUC 0
#endif
#ifndef IUCLC
#define IUCLC 0
#endif
#ifndef OCRNL
#define OCRNL 0
#endif
#ifndef XCASE
#define XCASE 0
#endif

#define CHANNEL 2 /* output file descriptor */

#if HAVE_TERMIO_H
static struct termio oldtio, newtio;
#else
static struct sgttyb tty;
static ttyflags;
#ifdef TIOCSETC
static struct tchars oldtchars, newtchars;
#endif
#endif

#ifdef TIOCSLTC
static struct ltchars oldchars, newchars;
#endif

#ifndef OXTABS
#ifdef TAB3
#define OXTABS TAB3
#endif
#endif

int TtyUpperCase;

#define NOCHAR 0

#if HAVE_TCGETATTR
#define GET(addr) tcgetattr(CHANNEL, addr)
#define SET(addr) tcsetattr(CHANNEL, TCSADRAIN, addr)
#else
#define GET(addr) ioctl(CHANNEL, TCGETA, addr)
#define SET(addr) ioctl(CHANNEL, TCSETAW, addr)
#endif

void TtySet()
{
#if HAVE_TERMIO_H
    if (GET(&oldtio) < 0)
        return;
    if (oldtio.c_oflag & OLCUC)
        TtyUpperCase = 1; /* uppercase on output */
    newtio = oldtio;
    newtio.c_iflag &= ~(INLCR | ICRNL | IGNCR | ISTRIP | IUCLC);
    newtio.c_oflag &= ~(OLCUC | OCRNL | OXTABS);
    newtio.c_lflag &= ~(ECHO | ICANON | XCASE);
    newtio.c_cc[VMIN]  = 1; /* break input after each character */
    newtio.c_cc[VTIME] = 1; /* timeout is 100 msecs */
    newtio.c_cc[VINTR] = NOCHAR;
    newtio.c_cc[VQUIT] = NOCHAR;
#ifdef VSWTCH
    newtio.c_cc[VSWTCH] = NOCHAR;
#endif
#ifdef VDSUSP
    newtio.c_cc[VDSUSP] = NOCHAR;
#endif
#ifdef VLNEXT
    newtio.c_cc[VLNEXT] = NOCHAR;
#endif
#ifdef VDISCARD
    newtio.c_cc[VDISCARD] = NOCHAR;
#endif
    SET(&newtio);
#else
    if (gtty(CHANNEL, &tty) < 0)
        return;
    if (tty.sg_flags & LCASE)
        TtyUpperCase = 1; /* uppercase on output */
    ttyflags = tty.sg_flags;
    tty.sg_flags &= ~(XTABS | ECHO | CRMOD | LCASE);
#ifdef CBREAK
    tty.sg_flags |= CBREAK;
#endif
    stty(CHANNEL, &tty);
#ifdef TIOCSETC
    ioctl(CHANNEL, TIOCGETC, (char *)&oldtchars);
    newtchars         = oldtchars;
    newtchars.t_intrc = NOCHAR;
    newtchars.t_quitc = NOCHAR;
    newtchars.t_eofc  = NOCHAR;
    newtchars.t_brkc  = NOCHAR;
    ioctl(CHANNEL, TIOCSETC, (char *)&newtchars);
#endif
#endif /* HAVE_TERMIO_H */
#ifdef TIOCSLTC
    ioctl(CHANNEL, TIOCGLTC, (char *)&oldchars);
    newchars          = oldchars;
    newchars.t_lnextc = NOCHAR;
    newchars.t_rprntc = NOCHAR;
    newchars.t_dsuspc = NOCHAR;
    newchars.t_flushc = NOCHAR;
    ioctl(CHANNEL, TIOCSLTC, (char *)&newchars);
#endif
}

void TtyReset()
{
#if HAVE_TERMIO_H
    SET(&oldtio);
#else
    tty.sg_flags = ttyflags;
    stty(CHANNEL, &tty);
#ifdef TIOCSETC
    ioctl(CHANNEL, TIOCSETC, (char *)&oldtchars);
#endif
#endif
#ifdef TIOCSLTC
    ioctl(CHANNEL, TIOCSLTC, (char *)&oldchars);
#endif
}

void TtyFlushInput()
{
#ifdef TCFLSH
    ioctl(CHANNEL, TCFLSH, 0);
#else
#ifdef TIOCFLUSH
    int p = 1;

    ioctl(CHANNEL, TIOCFLUSH, (char *)&p);
#endif
#endif
}
