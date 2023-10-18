#define SEARCHSZ 41                  /* max size of search string */
#define MAXTEMP  2048                /* max of temp file size (in cells) */
#define TEMPSZ   ((MAXTEMP + 7) / 8) /* temp table size (in bytes) */
#define POOLSZ   50                  /* number of lines in cache */

#define XTEMP   1 /* line is in tmp file */
#define XNOEOLN 2 /* no end of line */

struct index {              /* out of core line descriptor */
    long seek;              /* seek in file */
    short len;              /* length of line */
    unsigned poolindex : 8; /* index in pool or NOINDEX */
    unsigned flags : 8;     /* is in tmp file */
};

struct map {     /* pool cell descriptor */
    short busy;  /* cell busy */
    short index; /* index in lindex */
    long time;   /* time of last access */
};

typedef struct {         /* in core line descriptor */
    char *ptr;           /* pointer to string */
    short len;           /* length of string */
    short oldlen;        /* length before mod */
    unsigned mod : 8;    /* line is modified */
    unsigned noeoln : 8; /* no end of line */
} LINE;

typedef struct {
    struct index *lindex;   /* out of core line descriptors */
    struct map map[POOLSZ]; /* line pool */
    LINE pool[POOLSZ];      /* in core line descriptors */
    char tmap[TEMPSZ];      /* temp file map */
    long nindex;            /* number of indexes malloc'ed */
    long size;              /* length of file in bytes */
    short fd;               /* file descriptor */
    short bakfd;            /* bak file descriptor */
    short tfd;              /* temp file descriptor */
    short len;              /* length of file in lines */
    short broken;           /* there are broken lines */
} REC;

REC *RecOpen(int fd, int wmode);
void RecClose(REC *r);
int RecSave(REC *r, char *filename);
void RecBreak(REC *r);
LINE *RecGet(REC *r, int n);
void RecPut(LINE *p, int newlen);
void RecDelChar(REC *r, int line, int off);
void RecInsChar(REC *r, int line, int off, int sym);
void RecInsLine(REC *r, int n);
void RecDelLine(REC *r, int n);
