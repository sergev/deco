/*
 *      int KeyInit (map, flushproc)
 *      struct KeyMap *map;
 *      void (*flushproc) (void);
 *
 *              - set key map table.
 *              Flushproc is called before actual reading from terminal.
 *              Returns 1 if ok else 0.
 *
 *      int KeyGet ()
 *
 *              - reads key from terminal.
 *              Returns key (always >= 0).
 *
 *      int KeyGetChar ()
 *
 *              - reads char from terminal.
 *              Returns char (always >= 0).
 */

#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "scr.h"

static struct KeyMap *km;
static void (*flush)(void);

#include <setjmp.h>
#include <signal.h>

#define NKEY 100

static jmp_buf wakeup;
static int keyback;
static unsigned oldalarm;

static int cyrinput;
extern char VCyrInputTable[];

/*
 *      Compare keys. Used in call to qsort.
 *      Return -1, 0, 1 iff a is less, equal or greater than b.
 *      First, check if there is ->str.
 *      Then compare lengths, then strings.
 *      If equal, check ->tcap field.
 */
static int compkeys(const void *arg1, const void *arg2)
{
    const struct KeyMap *a = arg1;
    const struct KeyMap *b = arg2;
    register int cmp;

    if (!a->str) {
        if (!b->str)
            return (0);
        return (1);
    }
    if (!b->str)
        return (-1);
    cmp = strcmp(a->str, b->str);
    if (cmp)
        return (cmp);
    if (!a->tcap) {
        if (!b->tcap)
            return (0);
        return (1);
    }
    if (!b->tcap)
        return (-1);
    return (0);
}

void KeyInit(struct KeyMap *map, void (*flushproc)(void))
{
    struct CapTab tab[NKEY];
    register struct KeyMap *kp;
    register struct CapTab *t;

    km    = map;
    flush = flushproc;
    for (t = tab, kp = km; kp->val && t < tab + NKEY - 1; ++kp, ++t) {
        if (!kp->tcap)
            continue;
        memcpy(t->tname, kp->tcap, sizeof(t->tname));
        t->ttype = CAPSTR;
        t->tdef  = 0;
        t->tc    = 0;
        t->ti    = 0;
        t->ts    = &kp->str;
    }
    kp->val     = 0;
    t->tname[0] = 0;
    CapGet(tab);
    qsort((char *)km, (unsigned)(kp - km), sizeof(km[0]), compkeys);
#ifdef notdef
    {
        register struct KeyMap *p;
        for (p = km; p < kp; ++p)
            if (kp->str)
                printf("'\\%03o%s' -> %x\n", p->str[0], p->str + 1, p->val);
    }
#endif
}

int KeyGetChar()
{
    char c;

    while (read(0, &c, 1) != 1)
        if (!isatty(0))
            exit(0);
    return (c & 0377);
}

static RETSIGTYPE badkey()
{
    alarm(oldalarm);
    longjmp(wakeup, -1);
}

static int nmgetch(struct KeyMap *kp)
{
    register int match, c;
    register struct KeyMap *lp;

    for (match = 1;; ++match) {
        if (!kp->str[match])
            return (kp->val);
        c = KeyGetChar();
        if (kp->str[match] == c)
            continue;
        lp = kp;
        do {
            ++kp;
            if (!kp->str)
                goto unknown;
        } while (kp->str[match] != c);
        if (lp->str[match - 1] != kp->str[match - 1])
            goto unknown;
        if (match > 1 && strncmp(lp->str, kp->str, match - 1))
            goto unknown;
    }
unknown:
    alarm(oldalarm);
    longjmp(wakeup, 1);
    /* NOTREACHED */
}

int KeyGet()
{
    static struct KeyMap *kp;
    static int c, j;

    if (keyback) {
        c       = keyback;
        keyback = 0;
        return (c);
    }
    if (flush)
        (*flush)();
nextkey:
    c = KeyGetChar();
    for (kp = km; kp->str; ++kp)
        if ((char)c == kp->str[0])
            break;
    if (!kp->str) {
#ifdef DOC
        if (c == cntrl('_')) {
            _prscreen();
            goto nextkey;
        }
#endif
        /*
         * Handle cyrillic input.
         * ^N turns on cyrillic input mode,
         * after that all input characters
         * in range ' '...'~' must be recoded
         * by special cyrillic input table.
         * It is specified by Ct termcap descriptor.
         * ^O turns off cyrillic input mode.
         */
        if (c == cntrl('N')) {
            cyrinput = 1;
            goto nextkey;
        } else if (c == cntrl('O')) {
            cyrinput = 0;
            goto nextkey;
        } else if (cyrinput && c >= ' ' && c <= '~')
            c = VCyrInputTable[c - ' '] & 0xff;

        return (c);
    }
    if (!kp->str[1])
        return (kp->val);
    /* look for escape sequence */
    if ((j = setjmp(wakeup))) {
        /* time out or unknown escape sequence */
        TtyFlushInput();
        if (j < 0)
            return (c);
        goto nextkey;
    }
    signal(SIGALRM, badkey);
    oldalarm = alarm(2);
    c        = nmgetch(kp);
    alarm(oldalarm);
    return (c);
}

void KeyUnget(int key)
{
    keyback = key;
}
