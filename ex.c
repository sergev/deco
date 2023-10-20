/*
 *      Syntax of file ~/.deco
 *
 *      # comment
 *      pattern1 pattern2 ... patternN
 *              command
 *      pattern1 pattern2 ... patternN
 *              command
 *      ...
 */

/*
 *      Syntax of file ./.menu
 *
 *      # comment
 *      key1    string1
 *              command1
 *      key1    string1
 *              command1
 *      ...
 *
 *      Keys are:
 *
 *              a-z, A-Z, 0-9   - simple characters
 *              F1-F10          - function keys
 */

/*
 *      Command may contain macro characters:
 *
 *              %f      - current file name
 *              %b      - base name
 *              %d      - directory name
 *              %c      - full directory name
 *              %h      - full home directory name
 *              %u      - user name
 *              %g      - group name
 *              %t      - list of tagged files or current file name
 *              %%      - symbol '%'
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "deco.h"
#include "dir.h"
#include "scr.h"

#define EXSZ         64
#define BUFSZ        2048
#define STRSZ        256
#define EXFILE       ".deco"
#define MENUSZ       16
#define UMFILE       ".menu"
#define INITFILE     ".decoini"
#define ULDINITFILE  "/usr/lib/deco/initfile"
#define ULLDINITFILE "/usr/local/lib/deco/initfile"
#define ULDUMFILE    "/usr/lib/deco/menu"
#define ULLDUMFILE   "/usr/local/lib/deco/menu"
#define ULDEXFILE    "/usr/lib/deco/profile"
#define ULLDEXFILE   "/usr/local/lib/deco/profile"

struct ex {
    char *pat;
    char *cmd;
};

struct umenu {
    char key;
    char hist;
    char wait;
    char menu;
    char *str;
    char *cmd;
    char *cex;
};

static struct umenu um[EXSZ];
static int nm, menuwid, menucol, menurow;
static struct ex ex[EXSZ];
static int nex = -1;
static char *bufp;

static void skip()
{
    while (*bufp == ' ' || *bufp == '\t')
        ++bufp;
}

static void skipln()
{
    do {
        while (bufp && *bufp != '\n')
            ++bufp;
        if (!bufp)
            return;
        ++bufp;
    } while (*bufp == '#');
}

static char *getex()
{
    char pat[PATSZ];
    char *p;

    while (*bufp == ' ' || *bufp == '\t' || *bufp == '\n')
        skipln();
    if (!*bufp)
        return (0);
    for (p = pat; *bufp != ' ' && *bufp != '\t' && *bufp != '\n'; ++p, ++bufp) {
        if (!*bufp)
            break;
        if (p >= pat + PATSZ - 1) {
            while (bufp && *bufp != ' ' && *bufp != '\t' && *bufp != '\n')
                ++bufp;
            break;
        }
        *p = *bufp;
    }
    skip();
    if (*bufp == '\n')
        skipln();
    *p = 0;
    p  = malloc(p - pat + 1);
    strcpy(p, pat);
    return (p);
}

static int getkey()
{
    int key;

    while (*bufp == ' ' || *bufp == '\t' || *bufp == '\n')
        skipln();
    if (!*bufp)
        return (0);
    if (*bufp == 'F' && bufp[1] >= '1' && bufp[1] <= '9')
        key = bufp[1] - '0';
    else if ((*bufp >= 'a' && *bufp <= 'z') || (*bufp >= 'A' && *bufp <= 'Z') ||
             (*bufp >= '0' && *bufp <= '9'))
        key = *bufp;
    else
        key = '?';
    while (bufp && *bufp != ' ' && *bufp != '\t' && *bufp != '\n')
        ++bufp;
    skip();
    return (key);
}

static char *getstr()
{
    char cmd[STRSZ];
    char *p;

    if (!*bufp)
        return (0);
    for (p = cmd; *bufp != '\n'; ++p, ++bufp) {
        if (!*bufp)
            break;
        if (p >= cmd + STRSZ - 1) {
            skipln();
            break;
        }
        *p = *bufp;
    }
    if (*bufp == '\n')
        skipln();
    *p = 0;
    p  = malloc(p - cmd + 1);
    strcpy(p, cmd);
    return (p);
}

static char *getcmd()
{
    if (*bufp != ' ' && *bufp != '\t')
        return (0);
    skip();
    return (getstr());
}

static void readex()
{
    register char *cp, *ep;

    for (nex = 0; (ep = getex()); ++nex) {
        if (nex >= EXSZ - 1)
            break;
        ex[nex].pat = ep;
        if ((cp = getcmd()))
            ex[nex].cmd = cp;
    }
    while (nex > 0 && !ex[nex - 1].cmd) {
        --nex;
        free(ex[nex].pat);
    }
    ex[nex].pat = 0;
}

static void initex()
{
    register int fd, n;
    char fname[80];
    char buf[BUFSZ];

    /* look for ~/.deco */
    strcpy(fname, home);
    strcat(fname, "/");
    strcat(fname, EXFILE);
    fd = open(fname, 0);
    if (fd < 0) {
        /* look for /usr/local/lib/deco/profile */
        fd = open(ULLDEXFILE, 0);
        if (fd < 0) {
            /* look for /usr/lib/deco/profile */
            fd = open(ULDEXFILE, 0);
            if (fd < 0)
                return;
        }
    }
    bufp = buf;
    n    = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return;
    buf[n] = 0;
    bufp   = buf;
    if (*bufp == '#')
        skipln();

    readex();

    for (n = nex - 2; n >= 0; --n)
        if (!ex[n].cmd)
            ex[n].cmd = ex[n + 1].cmd;
}

int expandtagged(char *cp)
{
    register int i, n;
    register char *s;

    for (i = n = 0; i < cur->num; ++i)
        if (cur->cat[i].tag) {
            s = cur->cat[i].name;
            while (*s)
                *cp++ = *s++;
            *cp++ = ' ';
            ++n;
        }
    *cp = 0;
    return (n);
}

static void expand(char *cp, char *cmd, char *fname)
{
    register char *s;

    for (; *cmd; ++cmd) {
        if (*cmd != '%') {
            *cp++ = *cmd;
            continue;
        }
        switch (*++cmd) {
        default:
            *cp++ = *cmd;
            continue;
        case 't': /* list of tagged files */
            if (expandtagged(cp)) {
                for (; *cp; ++cp)
                    ;
                continue;
            }
        case 'f': /* current file name */
            s = fname;
            break;
        case 'b': /* base name */
            s = basename(fname);
            break;
        case 'd': /* directory name */
            s = basename(cur->cwd);
            break;
        case 'c': /* full directory name */
            s = cur->cwd;
            break;
        case 'h': /* full home directory name */
            s = home;
            break;
        case 'u': /* user name */
            s = user;
            break;
        case 'g': /* group name */
            s = group;
            break;
        }
        if (!s)
            continue;
        while (*s)
            *cp++ = *s++;
    }
    *cp = 0;
}

void excommand(char *cp, char *fname)
{
    register struct ex *ep;

    if (nex == -1)
        initex();
    for (ep = ex; ep->pat; ++ep) {
        if (match(fname, ep->pat)) {
            expand(cp, ep->cmd, fname);
            return;
        }
    }
    *cp = 0;
}

static void freeudm()
{
    register int n;

    for (n = 0; n < nm; ++n) {
        if (um[n].str)
            free(um[n].str);
        if (um[n].cmd)
            free(um[n].cmd);
    }
}

static void printudm()
{
    register int n;

    for (n = 0; n < nm; ++n) {
        VMove(menurow + n, menucol + 1);
        switch (um[n].key) {
        case 1:
            VPutString("F1");
            break;
        case 2:
            VPutString("F2");
            break;
        case 3:
            VPutString("F3");
            break;
        case 4:
            VPutString("F4");
            break;
        case 5:
            VPutString("F5");
            break;
        case 6:
            VPutString("F6");
            break;
        case 7:
            VPutString("F7");
            break;
        case 8:
            VPutString("F8");
            break;
        case 9:
            VPutString("F9");
            break;
        default:
            VPutChar(um[n].key);
            break;
        }
        if (um[n].menu)
            VMPutString(menurow + n, menucol + 3, "->");
        if (um[n].str)
            VMPutString(menurow + n, menucol + 5, um[n].str);
    }
}

static int openmenu(char *filename)
{
    register int fd;
    char fname[80];

    if (filename)
        return (open(filename, 0));
    /* look for ./.menu */
    fd = open(UMFILE, 0);
    if (fd < 0) {
        /* look for ~/.menu */
        strcpy(fname, home);
        strcat(fname, "/");
        strcat(fname, UMFILE);
        fd = open(fname, 0);
        if (fd < 0) {
            /* look for /usr/local/lib/deco/menu */
            fd = open(ULLDUMFILE, 0);
            if (fd < 0)
                /* look for /usr/lib/deco/menu */
                fd = open(ULDUMFILE, 0);
        }
    }
    return (fd);
}

static int initudm(char *filename, char *scale)
{
    register int n;
    register char *p;
    int fd;
    char buf[BUFSZ];

    fd = openmenu(filename);
    if (fd < 0) {
        error("Menu not found");
        return (0);
    }
    nm   = 0;
    bufp = buf;
    n    = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        error("Cannot read menu");
        return (0);
    }
    buf[n] = 0;
    bufp   = buf;
    if (*bufp == '#')
        skipln();

    for (n = 0; n < 128; ++n)
        scale[n] = MENUSZ + 1;

    menuwid = 1;
    for (nm = 0; nm < MENUSZ && (n = getkey()); ++nm) {
        scale[n]    = nm;
        um[nm].key  = n;
        um[nm].wait = um[nm].hist = 1;
        um[nm].menu               = 0;
        um[nm].str                = getstr();
        if (um[nm].str && (n = strlen(um[nm].str)) > menuwid)
            menuwid = n;
        um[nm].cmd = getcmd();
        if (um[nm].cmd) {
            p = um[nm].cmd;
            /* if ((n = strlen (um[nm].cmd)) > menuwid) */
            /* menuwid = n; */
            if (*p == '!') {
                um[nm].menu = 1;
                ++p;
            }
            if (*p == '-') {
                um[nm].wait = 0;
                ++p;
            }
            if (*p == '@') {
                um[nm].hist = 0;
                ++p;
            }
            um[nm].cex = p;
        }
    }
    if (!nm) {
        error("Menu is empty");
        return (0);
    }
    menuwid += 6;
    if (menuwid > 70)
        menuwid = 70;
    menurow = (LINES - nm) / 2;
    menucol = (79 - menuwid) / 2;
    return (1);
}

static char *runudm(char *scale)
{
    /* user defined menu */
    register int key, cm;
    BOX *box, *curbox;

    box = VGetBox(menurow - 2, menucol - 4, nm + 4, menuwid + 8);
    VSetDim();
    VStandOut();
    VFillBox(menurow - 2, menucol - 4, nm + 4, menuwid + 8, ' ');
    VDrawBox(menurow - 1, menucol - 1, nm + 2, menuwid + 2);
    mvcaddstr(menurow - 1, 40, " User Menu ");
    printudm();
    VSetNormal();
    VStandEnd();
    cm = 0;
    for (;;) {
        curbox = VGetBox(menurow + cm, menucol, 1, menuwid);
        VPrintBox(curbox);
        hidecursor();
        VSync();
        key = KeyGet();
        VUngetBox(curbox);
        VFreeBox(curbox);
        switch (key) {
        default:
        findmenu:
            if (key > 0 && key < 0200 && scale[key] < MENUSZ) {
                cm = scale[key];
                break;
            }
            VBeep();
            continue;
        case meta('A'):
            key = 1;
            goto findmenu;
        case meta('B'):
            key = 2;
            goto findmenu;
        case meta('C'):
            key = 3;
            goto findmenu;
        case meta('D'):
            key = 4;
            goto findmenu;
        case meta('E'):
            key = 5;
            goto findmenu;
        case meta('F'):
            key = 6;
            goto findmenu;
        case meta('G'):
            key = 7;
            goto findmenu;
        case meta('H'):
            key = 8;
            goto findmenu;
        case meta('I'):
            key = 9;
            goto findmenu;
        case cntrl(']'): /* redraw screen */
            VRedraw();
            continue;
        case cntrl('M'):
        case cntrl('J'):
            break;
        case cntrl('C'):
        case cntrl('['):
        case meta('J'): /* f0 */
            cm = -1;
            break;
        case meta('u'): /* up */
            if (--cm < 0)
                cm = nm - 1;
            continue;
        case meta('d'): /* down */
            if (++cm >= nm)
                cm = 0;
            continue;
        }
        break;
    }
    VUngetBox(box);
    VFreeBox(box);
    if (cm >= 0 && um[cm].cex) {
        if (um[cm].menu)
            return (um[cm].cex);
        /* execute command from menu */
        expand(command, um[cm].cex, cur->cat[cur->curfile].name);
        cpos = strlen(command);
        if (command[0])
            execmd(um[cm].wait, um[cm].hist);
    }
    return ((char *)0);
}

void usermenu() /* user defined menu */
{
    char scale[128], filename[80];
    register char *p;

    p = 0;
    while (initudm(p, scale)) {
        p = runudm(scale);
        freeudm();
        if (!p)
            break;
        expand(filename, p, cur->cat[cur->curfile].name);
        p = filename;
    }
}

static char *getlin()
{
    register char *p, *s;

    if (!*bufp)
        return (0);
    for (s = p = bufp; *p && *p != '\n'; ++p)
        ;
    if (*p == '\n')
        *p++ = 0;
    bufp = p;
    return (s);
}

void readinitfile()
{
    register int fd, n;
    register char *line;
    struct dir *d;
    char fname[80];
    char buf[BUFSZ];

    /* look for ~/.decoini */
    strcpy(fname, home);
    strcat(fname, "/");
    strcat(fname, INITFILE);
    fd = open(fname, 0);
    if (fd < 0) {
        /* look for /usr/local/lib/deco/initfile */
        fd = open(ULLDINITFILE, 0);
        if (fd < 0) {
            /* look for /usr/lib/deco/initfile */
            fd = open(ULDINITFILE, 0);
            if (fd < 0)
                return;
        }
    }
    bufp = buf;
    n    = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0)
        return;
    buf[n] = 0;
    bufp   = buf;

    while ((line = getlin())) {
        switch (line[0]) {
        case 'r': /* right panel */
            d = &right;
            goto initdir;
        case 'l': /* left panel */
            d = &left;
        initdir:
            switch (line[1]) {
            case 'o': /* Sort */
                switch (line[2]) {
                case 'n':
                    d->sort = SORTNAME;
                    break;
                case 'x':
                    d->sort = SORTEXT;
                    break;
                case 't':
                    d->sort = SORTTIME;
                    break;
                case 'z':
                    d->sort = SORTSIZE;
                    break;
                case 'u':
                    d->sort = SORTSKIP;
                    break;
                }
                break;
            case 'l': /* Long */
                d->view = VIEW_LONG;
                break;
            case 'w': /* Wide */
                d->view = VIEW_WIDE;
                break;
            case 'f': /* Full */
                d->view = VIEW_FULL;
                break;
            case 'i': /* Info */
                d->view = VIEW_INFO;
                break;
            case 's': /* Status */
                d->status = 1;
                break;
            case 'p': /* pattern */
                strcpy(d->pattern, line + 2);
                break;
            case 'a': /* not align extensions */
                d->alignext = 0;
                break;
            case 't': /* no type sorting */
                d->typesort = 0;
                break;
            case 'r': /* reverse sorting */
                d->revsort = 1;
                break;
            }
            break;
        case 'c': /* use cshell */
            usecshell = 1;
            break;
        case 'f': /* full screen */
            H = LINES - 7;
            break;
        case 'w': /* double width */
            widewin = 1;
            break;
        case 'h': /* show hidden files */
            showhidden ^= 1;
            break;
        case 'v': /* viewer */
            switch (line[1]) {
            case 'b': /* built-in */
                userview = 1;
                break;
            case 'n': /* name */
                strcpy(viewname, line + 2);
                break;
            }
            break;
        case 'e': /* editor */
            switch (line[1]) {
            case 'b': /* built-in */
                useredit = 1;
                break;
            case 'n': /* name */
                strcpy(editname, line + 2);
                break;
            case 'r': /* raw */
                viewraw = 1;
                break;
            case 'h': /* hex */
                viewhex = 1;
                break;
            case 't': /* tabs */
                viewtabs = 1;
                break;
            }
            break;
        case 'p': /* palette */
            switch (line[1]) {
            case 'n': /* normal */
                switch (line[2]) {
                case 'f': /* foreground */
                    palette.fg      = strtol(line + 3, 0, 0);
                    dflt_palette.fg = -1;
                    break;
                case 'b': /* background */
                    palette.bg      = strtol(line + 3, 0, 0);
                    dflt_palette.bg = -1;
                    break;
                case 'r': /* reverse */
                    switch (line[3]) {
                    case 'f': /* foreground */
                        palette.revfg      = strtol(line + 4, 0, 0);
                        dflt_palette.revfg = -1;
                        break;
                    case 'b': /* background */
                        palette.revbg      = strtol(line + 4, 0, 0);
                        dflt_palette.revbg = -1;
                        break;
                    }
                    break;
                }
                break;
            case 'b': /* bold */
                switch (line[2]) {
                case 'f': /* foreground */
                    palette.boldfg      = strtol(line + 3, 0, 0);
                    dflt_palette.boldfg = -1;
                    break;
                case 'b': /* background */
                    palette.boldbg      = strtol(line + 3, 0, 0);
                    dflt_palette.boldbg = -1;
                    break;
                case 'r': /* reverse */
                    switch (line[3]) {
                    case 'f': /* foreground */
                        palette.boldrevfg      = strtol(line + 4, 0, 0);
                        dflt_palette.boldrevfg = -1;
                        break;
                    case 'b': /* background */
                        palette.boldrevbg      = strtol(line + 4, 0, 0);
                        dflt_palette.boldrevbg = -1;
                        break;
                    }
                    break;
                }
                break;
            case 'd': /* dim */
                switch (line[2]) {
                case 'f': /* foreground */
                    palette.dimfg      = strtol(line + 3, 0, 0);
                    dflt_palette.dimfg = -1;
                    break;
                case 'b': /* background */
                    palette.dimbg      = strtol(line + 3, 0, 0);
                    dflt_palette.dimbg = -1;
                    break;
                case 'r': /* reverse */
                    switch (line[3]) {
                    case 'f': /* foreground */
                        palette.dimrevfg      = strtol(line + 4, 0, 0);
                        dflt_palette.dimrevfg = -1;
                        break;
                    case 'b': /* background */
                        palette.dimrevbg      = strtol(line + 4, 0, 0);
                        dflt_palette.dimrevbg = -1;
                        break;
                    }
                    break;
                }
                break;
            }
            break;
        }
    }
}

void writeinitfile()
{
    register int fd;
    char fname[80];
    char buf[BUFSZ];

    /* look for ~/.decoini */
    strcpy(fname, home);
    strcat(fname, "/");
    strcat(fname, INITFILE);
    fd = creat(fname, 0600);
    if (fd < 0) {
        error("Cannot create %s", fname);
        return;
    }
    bufp = buf;

    if (left.view == VIEW_LONG)
        *bufp++ = 'l', *bufp++ = 'l', *bufp++ = '\n';
    else if (left.view == VIEW_WIDE)
        *bufp++ = 'l', *bufp++ = 'w', *bufp++ = '\n';
    else if (left.view == VIEW_FULL)
        *bufp++ = 'l', *bufp++ = 'f', *bufp++ = '\n';
    else if (left.view == VIEW_INFO)
        *bufp++ = 'l', *bufp++ = 'i', *bufp++ = '\n';
    if (right.view == VIEW_LONG)
        *bufp++ = 'r', *bufp++ = 'l', *bufp++ = '\n';
    else if (right.view == VIEW_WIDE)
        *bufp++ = 'r', *bufp++ = 'w', *bufp++ = '\n';
    else if (right.view == VIEW_FULL)
        *bufp++ = 'r', *bufp++ = 'f', *bufp++ = '\n';
    else if (right.view == VIEW_INFO)
        *bufp++ = 'r', *bufp++ = 'i', *bufp++ = '\n';
    if (left.status)
        *bufp++ = 'l', *bufp++ = 's', *bufp++ = '\n';
    if (right.status)
        *bufp++ = 'r', *bufp++ = 's', *bufp++ = '\n';
    switch (left.sort) {
    case SORTEXT:
        break;
    case SORTNAME:
        *bufp++ = 'l';
        *bufp++ = 'o';
        *bufp++ = 'n';
        *bufp++ = '\n';
        break;
    case SORTTIME:
        *bufp++ = 'l';
        *bufp++ = 'o';
        *bufp++ = 't';
        *bufp++ = '\n';
        break;
    case SORTSIZE:
        *bufp++ = 'l';
        *bufp++ = 'o';
        *bufp++ = 'z';
        *bufp++ = '\n';
        break;
    default:
        *bufp++ = 'l';
        *bufp++ = 'o';
        *bufp++ = 'u';
        *bufp++ = '\n';
        break;
    }
    switch (right.sort) {
    case SORTEXT:
        break;
    case SORTNAME:
        *bufp++ = 'r';
        *bufp++ = 'o';
        *bufp++ = 'n';
        *bufp++ = '\n';
        break;
    case SORTTIME:
        *bufp++ = 'r';
        *bufp++ = 'o';
        *bufp++ = 't';
        *bufp++ = '\n';
        break;
    case SORTSIZE:
        *bufp++ = 'r';
        *bufp++ = 'o';
        *bufp++ = 'z';
        *bufp++ = '\n';
        break;
    default:
        *bufp++ = 'r';
        *bufp++ = 'o';
        *bufp++ = 'u';
        *bufp++ = '\n';
        break;
    }
    if (left.pattern[0]) {
        *bufp++ = 'l';
        *bufp++ = 'p';
        strcpy(bufp, left.pattern);
        while (*bufp++)
            ;
        bufp[-1] = '\n';
    }
    if (right.pattern[0]) {
        *bufp++ = 'r';
        *bufp++ = 'p';
        strcpy(bufp, right.pattern);
        while (*bufp++)
            ;
        bufp[-1] = '\n';
    }
    if (usecshell)
        *bufp++ = 'c', *bufp++ = '\n';
    if (H == LINES - 7)
        *bufp++ = 'f', *bufp++ = '\n';
    if (widewin)
        *bufp++ = 'w', *bufp++ = '\n';
    if (!showhidden)
        *bufp++ = 'h', *bufp++ = '\n';
    if (viewraw)
        *bufp++ = 'e', *bufp++ = 'r', *bufp++ = '\n';
    if (viewhex)
        *bufp++ = 'e', *bufp++ = 'h', *bufp++ = '\n';
    if (viewtabs)
        *bufp++ = 'e', *bufp++ = 't', *bufp++ = '\n';
    if (userview)
        *bufp++ = 'v', *bufp++ = 'b', *bufp++ = '\n';
    if (useredit)
        *bufp++ = 'e', *bufp++ = 'b', *bufp++ = '\n';
    if (!left.alignext)
        *bufp++ = 'l', *bufp++ = 'a', *bufp++ = '\n';
    if (!right.alignext)
        *bufp++ = 'r', *bufp++ = 'a', *bufp++ = '\n';
    if (!left.typesort)
        *bufp++ = 'l', *bufp++ = 't', *bufp++ = '\n';
    if (!right.typesort)
        *bufp++ = 'r', *bufp++ = 't', *bufp++ = '\n';
    if (left.revsort)
        *bufp++ = 'l', *bufp++ = 'r', *bufp++ = '\n';
    if (right.revsort)
        *bufp++ = 'r', *bufp++ = 'r', *bufp++ = '\n';
    if (viewname[0]) {
        *bufp++ = 'v';
        *bufp++ = 'n';
        strcpy(bufp, viewname);
        while (*bufp++)
            ;
        bufp[-1] = '\n';
    }
    if (editname[0]) {
        *bufp++ = 'e';
        *bufp++ = 'n';
        strcpy(bufp, editname);
        while (*bufp++)
            ;
        bufp[-1] = '\n';
    }

    if (palette.fg != dflt_palette.fg) {
        *bufp++ = 'p';
        *bufp++ = 'n';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.fg / 10;
        *bufp++ = '0' + palette.fg % 10;
        *bufp++ = '\n';
    }
    if (palette.bg != dflt_palette.bg) {
        *bufp++ = 'p';
        *bufp++ = 'n';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.bg / 10;
        *bufp++ = '0' + palette.bg % 10;
        *bufp++ = '\n';
    }
    if (palette.revfg != dflt_palette.revfg) {
        *bufp++ = 'p';
        *bufp++ = 'n';
        *bufp++ = 'r';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.revfg / 10;
        *bufp++ = '0' + palette.revfg % 10;
        *bufp++ = '\n';
    }
    if (palette.revbg != dflt_palette.revbg) {
        *bufp++ = 'p';
        *bufp++ = 'n';
        *bufp++ = 'r';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.revbg / 10;
        *bufp++ = '0' + palette.revbg % 10;
        *bufp++ = '\n';
    }

    if (palette.boldfg != dflt_palette.boldfg) {
        *bufp++ = 'p';
        *bufp++ = 'b';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.boldfg / 10;
        *bufp++ = '0' + palette.boldfg % 10;
        *bufp++ = '\n';
    }
    if (palette.boldbg != dflt_palette.boldbg) {
        *bufp++ = 'p';
        *bufp++ = 'b';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.boldbg / 10;
        *bufp++ = '0' + palette.boldbg % 10;
        *bufp++ = '\n';
    }
    if (palette.boldrevfg != dflt_palette.boldrevfg) {
        *bufp++ = 'p';
        *bufp++ = 'b';
        *bufp++ = 'r';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.boldrevfg / 10;
        *bufp++ = '0' + palette.boldrevfg % 10;
        *bufp++ = '\n';
    }
    if (palette.boldrevbg != dflt_palette.boldrevbg) {
        *bufp++ = 'p';
        *bufp++ = 'b';
        *bufp++ = 'r';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.boldrevbg / 10;
        *bufp++ = '0' + palette.boldrevbg % 10;
        *bufp++ = '\n';
    }

    if (palette.dimfg != dflt_palette.dimfg) {
        *bufp++ = 'p';
        *bufp++ = 'd';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.dimfg / 10;
        *bufp++ = '0' + palette.dimfg % 10;
        *bufp++ = '\n';
    }
    if (palette.dimbg != dflt_palette.dimbg) {
        *bufp++ = 'p';
        *bufp++ = 'd';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.dimbg / 10;
        *bufp++ = '0' + palette.dimbg % 10;
        *bufp++ = '\n';
    }
    if (palette.dimrevfg != dflt_palette.dimrevfg) {
        *bufp++ = 'p';
        *bufp++ = 'd';
        *bufp++ = 'r';
        *bufp++ = 'f';
        *bufp++ = '0' + palette.dimrevfg / 10;
        *bufp++ = '0' + palette.dimrevfg % 10;
        *bufp++ = '\n';
    }
    if (palette.dimrevbg != dflt_palette.dimrevbg) {
        *bufp++ = 'p';
        *bufp++ = 'd';
        *bufp++ = 'r';
        *bufp++ = 'b';
        *bufp++ = '0' + palette.dimrevbg / 10;
        *bufp++ = '0' + palette.dimrevbg % 10;
        *bufp++ = '\n';
    }

    message(" Setup ", "Saving setup in %s ...", fname);
    write(fd, buf, (unsigned)(bufp - buf));
    close(fd);
    endmesg();
}
