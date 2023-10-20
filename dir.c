#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define direct dirent
#else
#include <sys/dir.h>
#if !HAVE_OPENDIR
static int dirfile = -1;
static struct direct direntry;
#define DIR           int
#define opendir(name) ((dirfile = open(name, 0)) >= 0 ? &dirfile : 0)
#define closedir(f)   (close(dirfile), dirfile = -1)
#define readdir(f)    (read(dirfile, &direntry, sizeof(direntry)) == sizeof(direntry) ? &direntry : 0)
#undef dirfd
#define dirfd(f) dirfile
#endif
#endif

#if !HAVE_DIRFD && !defined(dirfd)
#define dirfd(f) ((f)->dd_fd)
#endif

#include "deco.h"
#include "dir.h"

#define QUANT 128

#ifndef MAXNAMLEN
#define MAXNAMLEN 255
#endif

#ifndef S_IFLNK
#define lstat stat
#endif

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')

static struct dir *compdir;

static int comptype(const struct file *f);
static void chdirup(char *name);
static char *prepend(char *p, char *str);
static char *getcwdname(char *pathname);
static void retag(struct dir *d, struct dir *od);
static int fgetinfo(char *info, int maxlen, char *rinfo, int rmaxlen, unsigned long *date, FILE *fd);

/*
 *      Compare files. Used in call to qsort.
 *      Return -1, 0, 1 iff a is less, equal or greater than b.
 */
int compfile(const void *arg1, const void *arg2)
{
    const struct file *a = arg1;
    const struct file *b = arg2;
    register int atype, btype;
    int rez;

    if (compdir->revsort) {
        const struct file *t;

        t = a;
        a = b;
        b = t;
    }
    if (compdir->typesort) {
        atype = comptype(a);
        btype = comptype(b);
        if (atype != btype)
            return (atype < btype ? -1 : 1);
    }
    switch (compdir->sort) {
    case SORTEXT:
        atype = a->name[0] != '.';
        btype = b->name[0] != '.';
        if (atype != btype)
            return (atype < btype ? -1 : 1);
        if ((rez = strcmp(extension(a->name), extension(b->name))))
            return (rez);
    case SORTNAME:
        return (strcmp(a->name, b->name));
    case SORTTIME:
        return (a->mtime < b->mtime ? -1 : a->mtime > b->mtime ? 1 : 0);
    case SORTSIZE:
        return (a->size < b->size ? -1 : a->size > b->size ? 1 : 0);
    }
    return (0);
}

/*
 *      Return integer number, used for comparing file types.
 */
static int comptype(const struct file *f)
{
    switch (f->mode & S_IFMT) {
    case S_IFDIR:
        if (f->name[0] == '.') {
            if (!f->name[1])
                return (-1);
            if (f->name[1] == '.' && !f->name[2])
                return (-2);
        }
        if (!f->execable)
            return (1);
        return (0);
    case S_IFCHR:
        return (100);
    case S_IFBLK:
        return (110);
#ifdef S_IFIFO
    case S_IFIFO:
        return (120);
#endif
#ifdef S_IFNAM
    case S_IFNAM:
        return (130);
#endif
#ifdef S_IFSOCK
    case S_IFSOCK:
        return (140);
#endif
#ifdef S_IFLNK
    case S_IFLNK:
        return (150);
#endif
    case S_IFREG:
        return (500);
    default:
        return (900);
    }
}

static void chdirup(char *name)
{
    register char *s, *p;

    for (;;) {
        if (chdir(name) < 0)
            error("Cannot chdir to %s", name);
        else if (access(".", 4) < 0)
            error("Cannot read %s", name);
        else
            break;
        if (name[0] == '/' && name[1] == 0) { /* root directory */
            error("DECO cannot work on this system -- goodbye!");
            quitdeco();
        }
        /* cut last directory name */
        for (p = s = name; *s; ++s)
            if (*s == '/')
                p = s;
        *p = 0;
        if (!*name) {
            name[0] = '/';
            name[1] = 0;
        }
    }
}

static char *prepend(char *p, char *str)
{
    int len = strlen(str);
    memcpy(p - len, str, len);
    return (p - len);
}

/*
 * Get current directory name.  Pathname should be at least
 * MAXPATHLEN long.  Returns 0 on error, and puts message into pathname.
 * Compatible with BSD getwd().
 */
static char *getcwdname(char *pathname)
{
    char upname[MAXPATHLEN + MAXNAMLEN];
    char myname[MAXPATHLEN];
    char *upptr, *myptr;
    struct stat root, up, this, f;
    register struct direct *rec;
    DIR *dir;

    myptr  = &myname[MAXPATHLEN - 1];
    upptr  = &upname[MAXPATHLEN - 1];
    *myptr = *upptr = 0;
    upptr           = prepend(upptr, "./");
    stat("/", &root);
    stat(upptr, &this);
    while (this.st_dev != root.st_dev || this.st_ino != root.st_ino) {
        upptr = prepend(upptr, "../");
        dir   = opendir(upptr);
        if (!dir) {
            strcpy(pathname, "Cannot read `..'");
            return (0);
        }
        fstat(dirfd(dir), &up);
        for (;;) {
            if (!(rec = readdir(dir))) {
                strcpy(pathname, "Cannot find path");
                return (0);
            }
            if (this.st_dev == up.st_dev) {
                if (this.st_ino == rec->d_ino)
                    break;
                continue;
            }
            if (rec->d_name[0] == '.' &&
                (!rec->d_name[1] || (rec->d_name[1] == '.' && rec->d_name[2] == 0)))
                continue;
            strcpy(&upname[MAXPATHLEN - 1], rec->d_name);
            lstat(upptr, &f);
            upname[MAXPATHLEN - 1] = 0;
            if (this.st_ino == f.st_ino && this.st_dev == f.st_dev)
                break;
        }
        myptr    = prepend(myptr, rec->d_name);
        *--myptr = '/';
        closedir(dir);
        this = up;
    }
    strcpy(pathname, *myptr ? myptr : "/");
    return (pathname);
}

/*
 *      Set up new directory.
 *      struct dir *d   - directory structure
 *      char *dirname   - name of directory.
 *                      If 0 then use d->cwd.
 */
int setdir(struct dir *d, char *dirname)
{
    DIR *f;
    struct direct *recp;
    register struct file *p;
    struct dir olddir;
    struct stat st;

    /* change directory */
    if (dirname) {
        if (chdir(dirname) < 0) {
            error("Cannot chdir to %s", dirname);
        } else {
            getcwdname(d->cwd);
        }
    }
    chdirup(d->cwd);
    d->shortcwd = strtail(d->cwd, '/', 35);
    stat(d->cwd, &st);
    d->dev = st.st_dev;
    d->ino = st.st_ino;

    f = opendir(".");
    if (!f) {
        error("Cannot read '.' -- good bye !");
        quitdeco();
    }

    if (!dirname) {
        /* save directory structure, then use it for retagging */
        olddir   = *d;
        d->catsz = 0;
        d->cat   = 0;
    }

    if (!d->cat) {
        d->catsz = QUANT;
        d->cat   = (struct file *)calloc(d->catsz, sizeof(struct file));
    } else
        for (p = d->cat; p < d->cat + d->num; ++p) {
            if (p->info)
                free(p->info);
            if (p->rinfo)
                free(p->rinfo);
            p->info = p->rinfo = 0;
        }

    d->num    = 0;
    d->nfiles = 0;
    d->tfiles = 0;
    d->nbytes = 0;
    d->mfiles = 0;
    d->mbytes = 0;
    while ((recp = readdir(f))) {
        if (!recp->d_ino)
            continue;
        mcheck(d->cat, struct file *, d->catsz, QUANT, d->num);
        p = &d->cat[d->num];
        lstat(recp->d_name, &st);
#ifdef S_IFLNK
        p->link = ((st.st_mode & S_IFMT) == (unsigned)S_IFLNK);
        if (p->link)
            stat(recp->d_name, &st);
#else
        p->link = 0;
#endif
        /* skip .. in root directory */
        if (recp->d_name[0] == '.' && recp->d_name[1] == '.' && recp->d_name[2] == 0 &&
            st.st_ino == d->ino && st.st_dev == d->dev)
            continue;

        /* skip all hidden files except parent dir entry */
        if (!showhidden && recp->d_name[0] == '.' && (recp->d_name[1] != '.' || recp->d_name[2]))
            continue;

        /* skip names not matched with pattern */
        if ((st.st_mode & S_IFMT) == (unsigned)S_IFREG) {
            if (!showhidden && recp->d_name[0] == '.')
                continue;
            ++d->nfiles;
            d->nbytes += st.st_size;
            if (d->pattern[0] && !match(recp->d_name, d->pattern))
                continue;
            ++d->mfiles;
            d->mbytes += st.st_size;
        }
        p->mode  = st.st_mode;
        p->mtime = st.st_mtime;
        p->atime = st.st_atime;
        p->ctime = st.st_ctime;
        p->size  = st.st_size;
        p->nlink = st.st_nlink;
        p->dev   = st.st_dev;
#if HAVE_ST_RDEV
        p->rdev = st.st_rdev;
#else
        p->rdev = 0;
#endif
        p->ino = st.st_ino;
        p->uid = st.st_uid;
        p->gid = st.st_gid;
        strncpy(p->name, recp->d_name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = 0;
        if (uid == 0)
            p->execable = ((p->mode & 0111) != 0);
        else if (uid == p->uid)
            p->execable = p->mode >> 6 & 1;
        else if (mygroup(p->gid))
            p->execable = p->mode >> 3 & 1;
        else
            p->execable = p->mode & 1;
        p->tag      = 0;
        p->info     = 0;
        p->rinfo    = 0;
        p->infodate = 0;
        p->infoflag = 0;
        p->dateflag = 0;
        ++d->num;
    }

    d->topfile = d->curfile = 0;

    compdir = d;
    qsort((char *)d->cat, (unsigned)d->num, sizeof(d->cat[0]), compfile);

    if (!dirname) {
        /* retag files, restore curfile */
        retag(d, &olddir);
        free(olddir.cat);
        counttag(d);
    }
    closedir(f);
    return (1);
}

char *filesize(struct file *f)
{
    static char buf[9];

    switch (f->mode & S_IFMT) {
    case S_IFDIR:
        return ("   <DIR>");
    case S_IFCHR:
        sprintf(buf, " %3d,%3d", f->rdev >> 8 & 0377, f->rdev & 0377);
        return (buf);
    case S_IFBLK:
        sprintf(buf, " %3d,%3d", f->rdev >> 8 & 0377, f->rdev & 0377);
        return (buf);
#ifdef S_IFIFO
    case S_IFIFO:
        return ("  <FIFO>");
#endif
#ifdef S_IFNAM
    case S_IFNAM:
        return ("  <NAME>");
#endif
#ifdef S_IFSOCK
    case S_IFSOCK:
        return ("  <SOCK>");
#endif
#ifdef S_IFLNK
    case S_IFLNK:
        return ("  <LINK>");
#endif
    case S_IFREG:
        sprintf(buf, "%8ld", (long)f->size);
        return (buf);
    default:
        return ("     ???");
    }
}

char *filemode(int m)
{
    static char buf[10];
    register char *p = buf;

    *p++ = m & S_IREAD ? 'r' : '-';
    *p++ = m & S_IWRITE ? 'w' : '-';
    *p++ = m & S_IEXEC ? (m & S_ISUID ? 's' : 'x') : (m & S_ISUID ? 'S' : '-');
    *p++ = m & S_IREAD >> 3 ? 'r' : '-';
    *p++ = m & S_IWRITE >> 3 ? 'w' : '-';
    *p++ = m & S_IEXEC >> 3 ? (m & S_ISGID ? 's' : 'x') : (m & S_ISGID ? 'S' : '-');
    *p++ = m & S_IREAD >> 6 ? 'r' : '-';
    *p++ = m & S_IWRITE >> 6 ? 'w' : '-';
    *p++ = m & S_IEXEC >> 6 ? (m & S_ISVTX ? 't' : 'x') : (m & S_ISVTX ? 'T' : '-');
    *p   = 0;
    return (buf);
}

char *filedate(long d)
{
    static char buf[10];
    register char *p = buf;

    *p++ = d / 100000 + '0';
    *p++ = d / 10000 % 10 + '0';
    *p++ = '.';
    *p++ = d / 1000 % 10 + '0';
    *p++ = d / 100 % 10 + '0';
    *p++ = '.';
    *p++ = d / 10 % 10 + '0';
    *p++ = d % 10 + '0';
    return (buf);
}

int filetype(struct file *f)
{
    switch (f->mode & S_IFMT) {
    case S_IFDIR:
        if (!f->execable)
            return ('&');
        return ('/');
    case S_IFCHR:
        return ('$');
    case S_IFBLK:
        return ('#');
#ifdef S_IFIFO
    case S_IFIFO:
        return ('=');
#endif
#ifdef S_IFNAM
    case S_IFNAM:
        return ('@');
#endif
#ifdef S_IFSOCK
    case S_IFSOCK:
        return ('!');
#endif
#ifdef S_IFLNK
    case S_IFLNK:
        return ('~');
#endif
    case S_IFREG:
        if (f->execable)
            return ('*');
        return (' ');
    default:
        return ('?');
    }
}

int exist(char *name)
{
    struct stat st;

    if (stat(name, &st) < 0)
        return (0);
    switch (st.st_mode & S_IFMT) {
    case S_IFDIR:
        return ('d');
    case S_IFREG:
        return ('f');
    default:
        return ('?');
    }
}

long modtime(char *name)
{
    struct stat st;

    if (stat(name, &st) < 0)
        return (0);
    return (st.st_mtime);
}

static void retag(struct dir *d, struct dir *od)
{
    register struct file *f, *of;
    struct file *end, *oend, *ocur, *otop;
    int cmp;

    end        = d->cat + d->num;
    oend       = od->cat + od->num;
    ocur       = od->cat + od->curfile;
    otop       = od->cat + od->topfile;
    d->curfile = -1;
    compdir    = d;
    for (f = d->cat, of = od->cat; of < oend; ++of) {
        while ((cmp = compfile(f, of)) < 0)
            if (++f >= end)
                goto breakloop;
        if (of == ocur)
            d->curfile = f - d->cat;
        if (of == otop)
            d->topfile = f - d->cat;
        if (cmp == 0)
            f->tag = of->tag;
    }
breakloop:
    if (d->curfile < 0) {
        d->curfile = d->num - 1;
        d->topfile = d->curfile - PAGELEN(d) + 1;
    } else if (d->topfile > d->curfile || d->topfile + PAGELEN(d) <= d->curfile)
        d->topfile = d->curfile - PAGELEN(d) + 1;
    while (d->topfile + PAGELEN(d) - H > d->curfile)
        d->topfile -= H;
    if (d->topfile < 0)
        d->topfile = 0;
}

void counttag(struct dir *d)
{
    register struct file *f;

    /* compute number of files and bytes */
    d->tfiles = 0;
    d->tbytes = 0;
    for (f = d->cat; f < d->cat + d->num; ++f)
        if ((f->mode & S_IFMT) == (unsigned)S_IFREG && f->tag) {
            ++d->tfiles;
            d->tbytes += f->size;
        }
}

void findfile(struct dir *d, char *name)
{
    /* set current file by name */
    register struct file *f;

    for (f = d->cat; f < d->cat + d->num; ++f)
        if (!strncmp(f->name, name, NAMESZ - 1)) {
            d->curfile = f - d->cat;
            break;
        }
    while (d->topfile > d->curfile)
        d->topfile -= H;
    if (d->topfile < 0)
        d->topfile = 0;
    while (d->topfile + PAGELEN(d) <= d->curfile)
        cur->topfile += H;
}

void setinfo(struct file *f, struct dir *d)
{
    char info[512], rinfo[512];
    FILE *fd;

    f->infoflag = 1;
    f->infodate = 0;

    /* No info for . and .. */
    if (f->name[0] == '.' && (f->name[1] == 0 || (f->name[1] == '.' && f->name[2] == 0)))
        return;

    sprintf(info, "%s/.info/%s", d->cwd, f->name);
    fd = fopen(info, "r");
    if (fd) {
        fgetinfo(info, sizeof(info), rinfo, sizeof(rinfo), &f->infodate, fd);
        if (*info) {
            if (strlen(info) > 45)
                info[45] = 0;
            f->info = mdup(info);
        }
        if (*rinfo) {
            if (strlen(rinfo) > 45)
                rinfo[45] = 0;
            f->rinfo = mdup(rinfo);
        }
        fclose(fd);
    }
    if (f->infodate)
        f->dateflag = 1;
    else {
        struct tm *t = localtime(&f->mtime);

        f->infodate = t->tm_year % 100 * 10000 + (t->tm_mon + 1) * 100 + t->tm_mday;
        f->dateflag = 0;
    }
}

/*
 * Get multistring from the file.
 * The continuation lines are indented by spaces.
 */
static int fgetinfo(char *info, int maxlen, char *rinfo, int rmaxlen, unsigned long *date, FILE *fd)
{
    char *p;
    int n;

    *info = *rinfo = 0;
    *date          = 0;
again:
    do
        n = getc(fd);
    while (n == ' ' || n == '\t');
    if (n < 0)
        return (0);
    ungetc(n, fd);

    fgets(info, maxlen, fd);
    if (ISDIGIT(info[0]) && ISDIGIT(info[1]) && ISDIGIT(info[2]) && ISDIGIT(info[3]) &&
        ISDIGIT(info[4]) && ISDIGIT(info[5])) {
        char *q = info;

        *date = strtoul(info, 0, 0);
        for (p = info + 6; *p == ' ' || *p == '\t'; ++p)
            continue;
        while (*p)
            *q++ = *p++;
        *q = 0;
    }
    p = info;
    while (maxlen > 0) {
        n = strlen(p);
        maxlen -= n;
        p += n;

        while (--n >= 0 && (p[-1] == '\n' || p[-1] == '\15')) {
            *--p = 0;
            ++maxlen;
        }

        n = getc(fd);
        if (n < 0)
            break;
        if (n != ' ' && n != '\t') {
            char buf[9];

            ungetc(n, fd);
            if (n != 'R' || !fgets(buf, 9, fd) || strncmp(buf, "Russian:", 8) != 0)
                break;
            info   = rinfo;
            maxlen = rmaxlen;
            goto again;
        }

        do
            n = getc(fd);
        while (n == ' ' || n == '\t');
        if (n < 0)
            break;
        ungetc(n, fd);
        *p++ = ' ';
        --maxlen;

        fgets(p, maxlen, fd);
    }
    return (1);
}
