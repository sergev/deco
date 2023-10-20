#include <string.h>
#include <unistd.h>
#include "deco.h"
#include "rec.h"
#include "scr.h"

#define GETHEX(x) ((x) >= 'A' ? ((x) & 15) + 9 : (x) & 15)

extern int viewfd;
extern int viewraw;             /* raw mode */
extern int viewhlen;            /* number of hex lines */
extern char viewsrch[SEARCHSZ]; /* search string */
extern char viewsbuf[SEARCHSZ]; /* search string buffer */
extern int viewsbsz;            /* search string size */
extern long viewseek;
extern REC *viewrec;

static void viewhhead(char *filename, int line);
static void viewhline(int line, int i);
static void viewhpage(int line);
static void prhline(char *p, int len);
static int hsearch(int l, int c, int *pline, int *pcol);
static int cvtsrch(char *from, char *to);

int hexview(char *filename) /* hexadecimal viewer */
{
    register int baseline;
    register char *p;
    int stepud = LINES - 3; /* step up-down */
    int sline, soff;        /* search position */
    int hexcursor;          /* hex cursor mode */

    baseline  = (viewseek + 15) / 16;
    hexcursor = 0;
    sline = soff = -1;
    VMPutString(LINES - 1, 0,
                "\0011\16Help  \17 2\16      \17 3\16Raw   \17 4\16Hex   \17 5\16Top   \17 "
                "6\16Bottom\17 7\16Search\17 8\16      \17 9\16      \01710\16Quit \17\2");
    for (;;) {
        viewhpage(baseline);
        for (;;) {
            viewhhead(filename, baseline);
            if (sline >= baseline && sline < baseline + LINES - 2)
                VMove(sline - baseline + 1, hexcursor ? soff * 3 : soff * 2 + 48);
            else {
                hidecursor();
                sline = -1;
            }
            VSync();
            switch (KeyGet()) {
            case meta('A'): /* f1 */
                runhelp("hexview");
                continue;
            default:
                VBeep();
                continue;
            case cntrl('I'): /* hex/text cursor */
                hexcursor ^= 1;
                continue;
            case cntrl(']'): /* redraw screen */
                VRedraw();
                continue;
            case meta('C'): /* f3 - raw */
                viewraw ^= 1;
                break;
            case meta('D'): /* f4 - hex */
                viewseek = baseline * 16L;
                return (1);
            case cntrl('C'):
            case cntrl('['):
            case meta('J'): /* f0 - quit */
                return (0);
            case meta('u'): /* up */
                if (baseline <= 0)
                    continue;
                --baseline;
                VDelLine(LINES - 2);
                VInsLine(1);
                viewhline(baseline, 1);
                continue;
            case cntrl('M'): /* down */
            case cntrl('J'): /* down */
            case meta('d'):  /* down */
                if (baseline >= viewhlen - (LINES - 2))
                    continue;
                VDelLine(1);
                VInsLine(LINES - 2);
                viewhline(++baseline + LINES - 3, LINES - 2);
                continue;
            case meta('n'): /* next page */
                if (baseline >= viewhlen - (LINES - 2))
                    continue;
                baseline += stepud;
                if (baseline > viewhlen - (LINES - 2))
                    baseline = viewhlen - (LINES - 2);
                break;
            case meta('p'): /* prev page */
                if (baseline <= 0)
                    continue;
                baseline -= stepud;
                if (baseline < 0)
                    baseline = 0;
                break;
            case meta('E'): /* top */
            case meta('h'): /* home */
                if (baseline == 0)
                    continue;
                baseline = 0;
                break;
            case meta('F'): /* bottom */
            case meta('e'): /* end */
                if (baseline >= viewhlen - (LINES - 2))
                    continue;
                baseline = viewhlen - (LINES - 2);
                break;
            case meta('G'): /* f7 - search */
                if (!(p = getstring(SEARCHSZ - 1, viewsrch, " Search ", "Search for the string")))
                    continue;
                strcpy(viewsrch, p);
                viewsbsz = cvtsrch(viewsrch, viewsbuf);
                if (sline < 0) {
                    sline = baseline;
                    soff  = -1;
                }
                if (hsearch(sline, soff + 1, &sline, &soff)) {
                    if (baseline > sline || baseline + LINES - 2 <= sline)
                        baseline = sline;
                    if (baseline > viewhlen - (LINES - 2))
                        baseline = viewhlen - (LINES - 2);
                    if (baseline < 0)
                        baseline = 0;
                    break;
                }
                sline = -1;
                continue;
            }
            break;
        }
    }
}

static void viewhhead(char *filename, int line)
{
    register int i;

    VSetDim();
    VStandOut();
    VMove(0, 0);
    for (i = 80; --i >= 0; VPutChar(' '))
        ;
    VMPutString(0, 1, "File ");
    VPutString(filename);
    VMove(0, 22);
    VPrint("Lines %-5d Bytes %ld", viewrec->len, viewrec->size);
    VMove(0, 68);
    VPrint("Byte %ld", line * 16L);
    VSetNormal();
    VStandEnd();
}

static void viewhline(int line, int i)
{
    char buf[16], *p;
    int n;

    lseek(viewfd, line * 16L, 0);
    n = read(viewfd, buf, sizeof(buf));
    p = buf;
    VMove(i, 0); /* move to line */
    if (n > 0)
        prhline(p, n);
    VClearLine(); /* clear it */
}

static void viewhpage(int line)
{
    char buf[1024], *p;
    register int i;
    int n;

    lseek(viewfd, line * 16L, 0);
    n = read(viewfd, buf, (unsigned)LINES * 16);
    p = buf;
    for (i = 1; i < LINES - 1; ++i, n -= 16, p += 16) {
        VMove(i, 0); /* move to line */
        if (n > 0)
            prhline(p, n);
        VClearLine(); /* clear rest of line */
    }
}

static void prhline(char *p, int len)
{
    register int i, c;

    if (len > 16)
        len = 16;
    for (i = 0; i < 16; ++i)
        if (i < len) {
            c = p[i];
            VPutChar("0123456789abcdef"[c >> 4 & 0xf]);
            VPutChar("0123456789abcdef"[c & 0xf]);
            VPutChar(' ');
        } else {
            VPutChar(' ');
            VPutChar(' ');
            VPutChar(' ');
        }
    for (i = 0; i < len; ++i) {
        viewchar(*p++);
        if (i < len - 1)
            VPutChar(' ');
    }
}

static int hsearch(int l, int c, int *pline, int *pcol)
{
    char buf[2 * SEARCHSZ], *p;
    register char *s, *e;
    int n;

    for (;;) {
        lseek(viewfd, l * 16L, 0);
        n = read(viewfd, buf, sizeof(buf));
        if (n <= 0)
            break;
        p = buf;
        e = buf + SEARCHSZ;
        for (s = p + c; s < e; ++s)
            if (!memcmp(s, viewsbuf, n < viewsbsz ? n : viewsbsz)) {
                *pline = l + (s - p) / 16;
                *pcol  = (s - p) % 16;
                return (1);
            }
        if (n < sizeof(buf))
            break;
        l += SEARCHSZ / 16;
        c = 0;
    }
    error("String not found");
    return (0);
}

static int cvtsrch(char *from, char *to)
{
    register int c, x, count;

    count = 0;
    for (count = 0; (c = *from++); *to++ = c, ++count)
        if (c == '\\' && (x = *from)) {
            ++from;
            if (x != '\\') {
                c = GETHEX(x);
                if ((x = *from) && x != '\\') {
                    ++from;
                    c = c << 4 | GETHEX(x);
                }
            }
        }
    return (count);
}
