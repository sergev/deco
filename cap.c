/*
 *      int CapInit (buf)
 *      char buf [2048];
 *
 *              - get termcap entry into buffer buf.
 *              Return 1 if ok, else 0.
 *
 *      CapGet (tab)
 *      struct CapTab *tab;
 *
 *              - read terminal properties, described in table tab.
 *              Buffer used as argument to CapInit must be available.
 *
 *      char *CapGoto (movestr, col, line)
 *      char *movestr;
 *
 *              - expand string movestr by column and line numbers.
 */
/* #define DEBUG */

#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_TERMCAP_H
#include <termcap.h>
#else
int tgetent(char *, const char *);
int tgetflag(const char *);
int tgetnum(const char *);
char *tgetstr(const char *, char **);
#endif
#include "deco.h"
#include "env.h"
#include "scr.h"

#define BUFSIZ        2048 /* max length of termcap entry */
#define MAXRETURNSIZE 64   /* max length of CapGoto string */
#define ERR(c)        (c)
#define TPARMERR(c)        \
    {                      \
        strcpy(outp, (c)); \
        return;            \
    }

static char *tname; /* terminal name */
static char *tbuf;  /* terminal entry buffer */

static void sysvgoto(char *outp, char *cp, int p1, int p2);

/*
 * Get an entry for terminal name in buffer bp,
 * from the termcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
int CapInit(char *bp)
{
    if (tbuf)
        return (1);
    if (!(tname = EnvGet("TERM")))
        tname = "unknown";
    if (tgetent(bp, tname) <= 0)
        return (0);
    tbuf = bp;
    return (1);
}

void CapGet(struct CapTab *t)
{
    char *area, *begarea, *bp, *val;
    struct CapTab *p;

    if (!tbuf)
        return;
    area = begarea = malloc(BUFSIZ);
    for (p = t; p->tname[0]; ++p)
        switch (p->ttype) {
        case CAPNUM:
            *(p->ti) = tgetnum(p->tname);
            if (*(p->ti) < 0)
                *(p->ti) = 0;
            break;
        case CAPFLG:
            *(p->tc) = tgetflag(p->tname);
            break;
        case CAPSTR:
            val = tgetstr(p->tname, &area);
            if (val) {
                *(p->ts) = val;
            }
            break;
        }
    bp = malloc(area - begarea);
    if (!bp) {
        free(begarea);
        return;
    }
    memcpy(bp, begarea, area - begarea);
    for (p = t; p->tname[0]; ++p)
        if (p->ttype == CAPSTR && *(p->ts) >= begarea && *(p->ts) < area)
            *(p->ts) += bp - begarea;
    free(begarea);
#ifdef DEBUG
    for (p = t; p->tname[0]; ++p) {
        printf("%c%c", p->tname[0], p->tname[1]);
        switch (p->ttype) {
        case CAPNUM:
            printf("#%d\n", *(p->ti));
            break;
        case CAPFLG:
            printf(" %s\n", *(p->tc) ? "on" : "off");
            break;
        case CAPSTR:
            if (*(p->ts))
                printf("='%s'\n", *(p->ts));
            else
                printf("=NULL\n");
            break;
        }
    }
#endif
}

/*
 * Routine to perform cursor addressing.
 * CM is a string containing printf type escapes to allow
 * cursor addressing.  We start out ready to print the destination
 * line, and switch each time we print row or column.
 * The following escapes are defined for substituting row/column:
 *
 *	%d	as in printf
 *	%2	like %2d
 *	%3	like %3d
 *	%.	gives %c hacking special case characters
 *	%+x	like %c but adding x first
 *
 *	The codes below affect the state but don't use up a value.
 *
 *	%>xy	if value > x add y
 *	%r	reverses row/column
 *	%i	increments row/column (for one origin indexing)
 *	%%	gives %
 *	%B	BCD (2 decimal digits encoded in one byte)
 *	%D	Delta Data (backwards bcd)
 *
 * all other characters are ``self-inserting''.
 */
char *CapGoto(char *CM, int destcol, int destline)
{
    register char *cp, *dp;
    register int c, which, oncol;
    static char result[MAXRETURNSIZE];

#ifdef DEBUG
    printf("CM='%s'\n", CM);
#endif
    cp = CM;
    if (!cp)
        return "";
    dp    = result;
    oncol = 0;
    which = destline;
    while ((c = *cp++)) {
        if (c != '%') {
            *dp++ = c;
            continue;
        }
        switch (c = *cp++) {
        case 'n':
            destcol ^= 0140;
            destline ^= 0140;
            goto setwhich;
        case 'd':
            if (which < 10)
                goto one;
            if (which < 100)
                goto two;
            /* fall into... */
        case '3':
            *dp++ = (which / 100) | '0';
            which %= 100;
            /* fall into... */
        case '2':
        two:
            *dp++ = which / 10 | '0';
        one:
            *dp++ = which % 10 | '0';
        swap:
            oncol = 1 - oncol;
        setwhich:
            which = oncol ? destcol : destline;
            continue;
        case '>':
            if (which > *cp++)
                which += *cp++;
            else
                cp++;
            continue;
        case '+':
            which += *cp++;
            /* fall into... */
        case '.':
            *dp++ = which;
            goto swap;
        case 'r':
            oncol = 1;
            goto setwhich;
        case 'i':
            destcol++;
            destline++;
            which++;
            continue;
        case '%':
            *dp++ = c;
            continue;
        case '/':
            c     = *cp;
            *dp++ = which / c | '0';
            which %= *cp++;
            continue;
        case 'B':
            which = (which / 10 << 4) + which % 10;
            continue;
        case 'D':
            which = which - 2 * (which % 16);
            continue;
        case 'p':
        case 'P':
        case '\'':
        case '{':
        case '?':
            /* string is in tparm format... */
            sysvgoto(dp, cp - 2, destline, destcol);
            return (result);
        default:
            return "bad capgoto";
        }
    }
    *dp = 0;
    return (result);
}

static char *branchto(register char *cp, int to)
{
    register int c, level;

    level = 0;
    while ((c = *cp++)) {
        if (c == '%') {
            if ((c = *cp++) == to || c == ';') {
                if (level == 0) {
                    return cp;
                }
            }
            if (c == '?')
                level++;
            if (c == ';')
                level--;
        }
    }
    return ERR("no matching ENDIF");
}

/*
 * Routine to perform parameter substitution.
 * instring is a string containing printf type escapes.
 * The whole thing uses a stack, much like an HP 35.
 * The following escapes are defined for substituting row/column:
 *
 *	%d	print pop() as in printf
 *      %[0]2d  print pop() like %2d
 *      %[0]3d  print pop() like %3d
 *	%c	print pop() like %c
 *
 *	%p[1-0]	push ith parm
 *	%P[a-z] set variable
 *	%g[a-z] get variable
 *	%'c'	char constant c
 *	%{nn}	integer constant nn
 *
 *	%+ %- %* %/ %m		arithmetic (%m is mod): push(pop() op pop())
 *	%& %| %^		bit operations:		push(pop() op pop())
 *	%= %> %<		logical operations:	push(pop() op pop())
 *	%! %~			unary operations	push(op pop())
 *	%b			unary BCD conversion
 *	%d			unary Delta Data conversion
 *
 *	%? expr %t thenpart %e elsepart %;
 *				if-then-else, %e elsepart is optional.
 *				else-if's are possible ala Algol 68:
 *				%? c1 %t %e c2 %t %e c3 %t %e c4 %t %e %;
 *
 * all other characters are ``self-inserting''.  %% gets % output.
 */
static void sysvgoto(register char *outp, register char *cp, int p1, int p2)
{
    register int c, op;
    int vars[26], stack[10], top, sign;

#define PUSH(i) (stack[++top] = (i))
#define POP()   (stack[top--])

    if (!cp)
        TPARMERR("null arg");
    top = 0;
    while ((c = *cp++)) {
        if (c != '%') {
            *outp++ = c;
            continue;
        }
        op = stack[top];
        if (*cp == '0')
            ++cp;
        switch (c = *cp++) {
        case 'd': /* PRINTING CASES */
            if (op < 10)
                goto one;
            if (op < 100)
                goto two;
            /* fall into... */
        case '3':
        three:
            if (c == '3' && *cp++ != 'd')
                TPARMERR("bad char after %3");
            *outp++ = (op / 100) | '0';
            op %= 100;
            /* fall into... */
        case '2':
            if (op >= 100)
                goto three;
            if (c == '2' && *cp++ != 'd')
                TPARMERR("bad char after %2");
        two:
            *outp++ = op / 10 | '0';
        one:
            *outp++ = op % 10 | '0';
            (void)POP();
            continue;
        case 'c':
            *outp++ = op;
            (void)POP();
            break;
        case '%':
            *outp++ = c;
            break;
        /*
         * %i: shorthand for increment first two parms.
         * Useful for terminals that start numbering from
         * one instead of zero (like ANSI terminals).
         */
        case 'i':
            p1++;
            p2++;
            break;
        case 'p': /* %pi: push the ith parameter */
            switch (c = *cp++) {
            case '1':
                PUSH(p1);
                break;
            case '2':
                PUSH(p2);
                break;
            default:
                TPARMERR("bad parm number");
            }
            break;
        case 'P': /* %Pi: pop from stack into variable i (a-z) */
            vars[*cp++ - 'a'] = POP();
            break;
        case 'g': /* %gi: push variable i (a-z) */
            PUSH(vars[*cp++ - 'a']);
            break;
        case '\'': /* %'c' : character constant */
            PUSH(*cp++);
            if (*cp++ != '\'')
                TPARMERR("missing closing quote");
            break;
        case '{': /* %{nn} : integer constant.  */
            op   = 0;
            sign = 1;
            if (*cp == '-') {
                sign = -1;
                cp++;
            } else if (*cp == '+')
                cp++;
            while ((c = *cp++) >= '0' && c <= '9') {
                op = 10 * op + c - '0';
            }
            if (c != '}')
                TPARMERR("missing closing brace");
            PUSH(sign * op);
            break;
        /* binary operators */
        case '+':
            c  = POP();
            op = POP();
            PUSH(op + c);
            break;
        case '-':
            c  = POP();
            op = POP();
            PUSH(op - c);
            break;
        case '*':
            c  = POP();
            op = POP();
            PUSH(op * c);
            break;
        case '/':
            c  = POP();
            op = POP();
            PUSH(op / c);
            break;
        case 'm':
            c  = POP();
            op = POP();
            PUSH(op % c);
            break;
        case '&':
            c  = POP();
            op = POP();
            PUSH(op & c);
            break;
        case '|':
            c  = POP();
            op = POP();
            PUSH(op | c);
            break;
        case '^':
            c  = POP();
            op = POP();
            PUSH(op ^ c);
            break;
        case '=':
            c  = POP();
            op = POP();
            PUSH(op = c);
            break;
        case '>':
            c  = POP();
            op = POP();
            PUSH(op > c);
            break;
        case '<':
            c  = POP();
            op = POP();
            PUSH(op < c);
            break;
        /* Unary operators. */
        case '!':
            stack[top] = !stack[top];
            break;
        case '~':
            stack[top] = ~stack[top];
            break;
        /* Sorry, no unary minus, because minus is binary. */
        /*
         * If-then-else.  Implemented by a low level hack of
         * skipping forward until the match is found, counting
         * nested if-then-elses.
         */
        case '?': /* IF - just a marker */
            break;
        case 't': /* THEN - branch if false */
            if (!POP())
                cp = branchto(cp, 'e');
            break;
        case 'e': /* ELSE - branch to ENDIF */
            cp = branchto(cp, ';');
            break;
        case ';': /* ENDIF - just a marker */
            break;
        default:
            TPARMERR("bad % sequence");
        }
    }
    *outp = 0;
}
