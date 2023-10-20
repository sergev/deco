#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "deco.h"

#define SYSTEMID "/etc/systemid"

#define DELIM ','

#define STOREINT(p, nn)                        \
    {                                          \
        register char *cp = p;                 \
        register int n    = nn;                \
        *cp++             = n / 10 % 10 + '0'; \
        *cp               = n % 10 + '0';      \
    }

static char pattern[] = "00-Mon-1900 00:00:00";

char *timestr(long tim)
{
    struct tm *t     = localtime(&tim);
    register char *p = pattern;

    STOREINT(p, t->tm_mday);
    if (*p == '0')
        *p = ' ';

    strncpy(p + 3, &"JanFebMarAprMayJunJulAugSepOctNovDec"[t->tm_mon * 3], 3);
    STOREINT(p + 9, t->tm_year);
    if (t->tm_year < 100) {
        p[7] = '1';
        p[8] = '9';
    } else {
        p[7] = '2';
        p[8] = '0';
    }
    STOREINT(p + 12, t->tm_hour);
    STOREINT(p + 15, t->tm_min);
    STOREINT(p + 18, t->tm_sec);
    return (p);
}

char *getmachine()
{
    static char buf[64];
    register int f;
    register char *p;

    if (gethostname(buf, sizeof(buf) - 1) >= 0)
        return (buf);

    if ((f = open(SYSTEMID, 0)) >= 0) {
        read(f, buf, sizeof(buf) - 1);
        close(f);
        for (p = buf; *p; ++p)
            if (*p < ' ' || *p > '~') {
                *p = 0;
                break;
            }
        return (buf);
    }
    return (0);
}

const char *extension(const char *p)
{
    const char *s;

    if (p[0] == '.' && p[1] == '.' && p[2] == 0)
        return (p + 2);
    s = 0;
    while (*++p)
        if (*p == '.')
            s = p;
    return (s ? s : p);
}

char *ltoac(long l)
{
    char num[15];
    static char buf[15];
    register char *p, *q;
    register int n;

    sprintf(num, "%ld", l);
    for (p = num; *p; ++p)
        ;                         /* find end of num */
    q    = buf + sizeof(buf) - 1; /* set q to the end of buf */
    *q-- = 0;
    for (n = 0; p > num; ++n) {
        if (n && n % 3 == 0)
            *q-- = DELIM;
        *q-- = *--p;
    }
    return (q + 1);
}

char *basename(char *p)
{
    register char *s, *e;
    static char buf[32];

    /* convert "zzz/name" to "name" */
    for (s = p; *s; ++s)
        if (*s == '/')
            p = s + 1;
    if (!*p)
        return ("");

    /* convert "name.ext" to "name" */
    for (e = s, s = p + 1; *s; ++s)
        if (*s == '.')
            e = s;
    strncpy(buf, p, e - p);
    buf[e - p] = 0;
    return (buf);
}

char *strtail(char *p, int delim, int maxlen)
{
    register char *s;

    /* return tail of string */
    for (s = p; *s; ++s)
        ;
    s -= maxlen;
    if (s <= p)
        return (p);
    for (; *s; ++s)
        if (*s == delim)
            return (s + 1);
    return (s - maxlen);
}

int strbcmp(char *s, char *b)
{
    /* compare string s with string b */
    /* return 1 if s == b or s == b+' '+x */

    while (*b && *b == *s)
        ++b, ++s;
    if (*b || (*s && *s != ' '))
        return (0);
    return (1);
}

void outerr(char *s, ...)
{
    char buf[100];
    register char *p;
    va_list ap;

    va_start(ap, s);
    vsprintf(buf, s, ap);
    va_end(ap);
    for (p = buf; *p; ++p)
        if (*p == '\033')
            *p = '[';
    write(2, buf, (unsigned)(p - buf));
}

/*
 * int match (char *name, char *pattern)
 *
 * Match name with pattern. Returns 1 on success.
 * Pattern may contain wild symbols:
 *
 * ^       - at beginning of string - any string not matched
 * *       - any string
 * ?       - any symbol
 * [a-z]   - symbol in range
 * [^a-z]  - symbol out of range
 * [!a-z]  - symbol out of range
 */
int match(char *name, char *pat)
{
    int ok, negate_range, matched;

    if (*pat == '^') {
        matched = 0;
        ++pat;
    } else
        matched = 1;
    for (;;) {
        switch (*pat++) {
        case '*':
            if (!*pat)
                return (matched);
            for (; *name; ++name)
                if (match(name, pat))
                    return (matched);
            return (!matched);
        case '?':
            if (!*name++)
                return (!matched);
            break;
        case 0:
            if (*name)
                return (!matched);
            return (matched);
        default:
            if (*name++ != pat[-1])
                return (!matched);
            break;
        case '[':
            ok           = 0;
            negate_range = (*pat == '^' || *pat == '!');
            if (negate_range)
                ++pat;
            while (*pat++ != ']')
                if (*pat == '-') {
                    if (pat[-1] <= *name && *name <= pat[1])
                        ok = 1;
                    pat += 2;
                } else if (pat[-1] == *name)
                    ok = 1;
            if (ok == negate_range)
                return (!matched);
            ++name;
            break;
        }
    }
}

char *mdup(char *s)
{
    char *p;

    p = malloc(strlen(s) + 1);
    strcpy(p, s);
    return (p);
}
