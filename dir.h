#include <sys/param.h>
#include <time.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024 /* max length of directory name */
#endif
#define PATSZ 20 /* max length of pattern */

#define SORTNAME 1 /* sort by name */
#define SORTEXT  2 /* sort by extension */
#define SORTTIME 3 /* sort by time */
#define SORTSIZE 4 /* sort by size */
#define SORTSKIP 5 /* unsorted */

#ifdef SHORTNAMES
#define NAMESZ 15
#else
#define NAMESZ 63
#endif

struct file {
    char name[NAMESZ];      /* file name */
    char *info;             /* file info */
    char *rinfo;            /* file info in russian */
    unsigned execable : 1;  /* file is executable for me */
    unsigned tag : 1;       /* file tagged */
    unsigned link : 1;      /* symbolic link */
    unsigned infoflag : 1;  /* info is read */
    unsigned dateflag : 1;  /* date is read from info file */
    unsigned long size;     /* file size */
    unsigned short mode;    /* access mode */
    unsigned short uid;     /* user ident */
    unsigned short gid;     /* group ident */
    unsigned short nlink;   /* number of file links */
    short dev;              /* device */
    short rdev;             /* special file parameters */
    unsigned long ino;      /* i-node */
    time_t mtime;           /* modification time */
    time_t atime;           /* access time */
    time_t ctime;           /* status change time */
    unsigned long infodate; /* date from info file */
};

struct dir {
    char cwd[MAXPATHLEN]; /* directory full name */
    char *shortcwd;       /* short printable directory name */
    short dev;            /* device */
    unsigned long ino;    /* i-node */
    struct file *cat;     /* catalog */
    int catsz;            /* length of catalog */
    int num;              /* number of files in cat */
    int topfile;          /* top file in the window */
    int curfile;          /* current file */
    char view;            /* view format */
    char status;          /* is it status window */
    char sort;            /* sort kind */
    char typesort;        /* sort by file types */
    char revsort;         /* reverse sorting */
    char alignext;        /* align extensions */
    int basecol;          /* base of window */
    int nfiles;           /* number of regular files */
    long nbytes;          /* number of bytes used */
    int mfiles;           /* number of matched files */
    long mbytes;          /* number of bytes matched */
    int tfiles;           /* number of tagged files */
    long tbytes;          /* number of bytes tagged */
    char pattern[PATSZ];  /* pattern for file names */
};
