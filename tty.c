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
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
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

static struct termios oldtio, newtio;

#ifdef TIOCSLTC
static struct ltchars oldchars, newchars;
#endif

#ifndef OXTABS
#ifdef TAB3
#define OXTABS TAB3
#endif
#endif

#define NOCHAR 0

#define GET(addr) tcgetattr(CHANNEL, addr)
#define SET(addr) tcsetattr(CHANNEL, TCSADRAIN, addr)

void TtySet()
{
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
    SET(&oldtio);

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
