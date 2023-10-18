/*
 *      Record-oriented file access package.
 *      -------------------------------------
 *
 *      REC *RecOpen (int fd, int mode)
 *              - initialize Rec package for file fd,
 *              for writing if mode!=0.
 *
 *      int RecSave (REC *r, char *filename)
 *              - and write out changes,
 *              returns 0 if ok or -1 if cannot write file.
 *
 *      int RecClose (REC *r)
 *              - close Rec discarding changes.
 *
 *      RecBroken (REC *r)
 *              - if Rec has broken lines, return 1, else 0.
 *
 *      LINE *RecGet (REC *r, int linenum)
 *              - get line.
 *
 *      RecPut (LINE *l, int newlen)
 *              - put line.
 *
 *      RecInsLine (REC *r, int linenum)
 *              - insert empty line before line #linenum
 *
 *      RecDelLine (REC *r, int linenum)
 *              - delete line #linenum
 *
 *      RecInsChar (REC *r, int line, int off, int sym)
 *              - insert char into line
 *
 *      RecDelChar (REC *r, int line, int off)
 *              - delete char from line
 */

#include <stdlib.h>
#include <string.h>
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif
#if HAVE_FCNTL_H
#   include <fcntl.h>
#endif
#include "rec.h"
#include "deco.h"

#define BUFSIZ          512
#define TEMPCELL        16              /* minimal size of temp space */
#define QUANT           512
#define MAXLINE         1024            /* maximum length of unbroken line */
#define NOINDEX         (POOLSZ+2)

#define BAKSUFFIX       ".b"

static char mask [8] = { 1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, };
static char tfilepattern [] = "/tmp/recXXXXXX";

static int lenbuf;
static char *scanbuf, *pbuf;
static char *tfilename;
static int eoln;
static int MAXLEN = ((unsigned int) (int) -1 >> 1) / sizeof (struct index);

static int ffcopy (int from, int to)
{
	char buf [512], *p;
	register n, k;

	for (;;) {
		n = read (from, buf, sizeof (buf));
		if (n < 0) {
			error ("Cannot read file");
			return (-1);
		}
		if (n == 0)
			return (0);
		p = buf;
		while (n > 0) {
			k = write (to, p, (unsigned) n);
			if (k <= 0) {
				error ("Cannot write file");
				return (-1);
			}
			n -= k;
			p += k;
		}
	}
}

static void tempfree (REC *r, long seek, int len)  /* mark temp space free */
{
	register i;

	len = (len + TEMPCELL - 1) / TEMPCELL;
	seek /= TEMPCELL;
	for (i=0; i<len; ++i, ++seek)
		r->tmap [seek>>3] &= ~mask [seek & 7];
}

static long tempbusy (REC *r, int len)   /* search & mark temp space busy */
{
	register i, n;
	register long seek;

	len = (len + TEMPCELL - 1) / TEMPCELL;
	for (seek=0; seek<MAXTEMP; ++seek) {
		for (n=0; n<len; ++n)
			if (r->tmap [seek>>3] & mask [seek & 7])
				break;
		if (n >= len)
			break;
	}
	if (seek >= MAXTEMP) {
		error ("No temp space");
		return (0);
	}
	for (n=seek, i=0; i<len; ++i, ++n)
		r->tmap [n>>3] |= mask [n & 7];
	return (seek * TEMPCELL);
}

static long tempsave (REC *r, char *str, int len) /* save string in temp file, return seek */
{
	register long seek;

	if (! len)
		return (0);
	seek = tempbusy (r, len);
	if (lseek (r->tfd, seek, 0) < 0)
		error ("Cannot lseek on writing temp file");
	if (write (r->tfd, str, (unsigned) len) != len)
		error ("Cannot write temporary file");
	return (seek);
}

static int scanline (int fd)
{
	register len;

	len = 0;
	eoln = 0;
	for (;;) {
		if (pbuf >= &scanbuf[lenbuf]) {
			lenbuf = read (fd, pbuf = scanbuf, (unsigned) BUFSIZ);
			if (lenbuf <= 0)
				return (len ? len : -1);
		}
		if (*pbuf++ == '\n') {
			eoln = 1;
			return (len);
		}
		if (++len >= MAXLINE)
			return (len);
	}
}

REC *RecOpen (int fd, int wmode)
{
	register REC *r;
	register struct index *x;
	register i;

	r = (REC*) malloc (sizeof (REC));
	r->fd = fd;
	r->bakfd = -1;
	if (wmode) {
		if (! tfilename)
			tfilename = tfilepattern;
		r->tfd = creat (tfilename, 0600);
		if (r->tfd < 0) {
			error ("Cannot open temporary file");
			return (0);
		}
		close (r->tfd);
		r->tfd = open (tfilename, 2);
		if (r->tfd < 0) {
			error ("Cannot reopen temporary file");
			return (0);
		}
		unlink (tfilename);
	} else
		r->tfd = -1;

	for (i=0; i<TEMPSZ; ++i)
		r->tmap [i] = 0;

	for (i=0; i<POOLSZ; ++i)
		r->map[i].busy = 0;

	lseek (r->fd, 0L, 0);
	r->broken = 0;
	r->len = 0;
	r->lindex = 0;
	r->nindex = 0;
	r->size = 0;
	scanbuf = malloc (BUFSIZ);
	pbuf = scanbuf;
	lenbuf = 0;
	for (;;) {
		mcheck (r->lindex, struct index *, r->nindex, QUANT, r->len+1);
		x = &r->lindex[r->len];
		x->seek = r->size;              /* store seek of current line */
		x->poolindex = NOINDEX;         /* clear pool index of current line */
		x->flags = 0;                   /* clear flags */
		x->len = scanline (r->fd);      /* scan next line */
		if (x->len < 0)
			break;
		r->size += x->len;              /* increment seek by length of line */
		if (eoln)
			++r->size;              /* \n at end of line */
		else {
			x->flags |= XNOEOLN;    /* no end of line */
			++r->broken;
		}
		if (++r->len >= MAXLEN) {
			error ("file too long");
			return (0);
		}
	}
	free (scanbuf);
	return (r);
}

void RecClose (REC *r)
{
	register i;

	for (i=0; i<POOLSZ; ++i)
		if (r->map[i].busy) {
			if (r->pool[i].len)
				free (r->pool[i].ptr);
			r->lindex[r->map[i].index].poolindex = NOINDEX;
			r->map[i].busy = 0;
		}
	free (r->lindex);
	if (r->tfd >= 0)
		close (r->tfd);
	if (r->bakfd >= 0)
		close (r->bakfd);
}

int RecSave (REC *r, char *filename)
{
	register i, fd;
	register LINE *p;
	char bak [40];

	if (r->bakfd < 0) {
		strcpy (bak, filename);
		strcat (bak, BAKSUFFIX);
		r->bakfd = creat (bak, 0600);
		if (r->bakfd < 0) {
			error ("Cannot create %s", bak);
			return (-1);
		}
		lseek (r->fd, 0L, 0);
		if (ffcopy (r->fd, r->bakfd) < 0) {
			close (r->bakfd);
			r->bakfd = -1;
			unlink (bak);
			return (-1);
		}
		close (r->bakfd);
		r->bakfd = open (bak, 0);
		if (r->bakfd < 0) {
			error ("Cannot open %s", bak);
			unlink (bak);
			return (-1);
		}
		close (r->fd);
		r->fd = r->bakfd;
	}
	fd = creat (filename, 0664);
	if (fd < 0) {
		error ("Cannot create %s", filename);
		unlink (bak);
		return (-1);
	}
	for (i=0; i<r->len; ++i) {
		p = RecGet (r, i);
		if (p->len)
			write (fd, p->ptr, (unsigned) p->len);
		write (fd, "\n", 1);
	}
	close (fd);
	return (0);
}

void RecBreak (REC *r)
{
	register i;
	register struct index *x;

	x = r->lindex;
	for (i=0; i<r->len; ++i, ++x)
		x->flags &= ~XNOEOLN;
}

static void readline (int fd, long seek, int len, LINE *rez)
{
	register l, n;
	register char *s;

	rez->len = len;
	rez->mod = 0;
	if (! len) {
		rez->ptr = "";
		return;
	}
	rez->ptr = malloc (rez->len);
	if (lseek (fd, seek, 0) < 0)
		error ("Cannot lseek on reading fd=%d, seek=%ld", fd, seek);
	for (l=rez->len, s=rez->ptr; l>0; l-=n, s+=n) {
		n = read (fd, s, (unsigned) l);
		if (n <= 0) {
			error ("Cannot read line");
			if (rez->len)
				free (rez->ptr);
			rez->len = 0;
			rez->ptr = "";
			return;
		}
	}
}

static int freeline (REC *r)
{
	register struct map *m;
	register struct index *x;
	register LINE *l;
	register mintime, minindex;

	/* find free place in pool */
	for (m=r->map; m<r->map+POOLSZ; ++m)
		if (! m->busy)
			return (m - r->map);
	/* pool is full; find the oldest line */
	mintime = r->map[0].time;
	minindex = 0;
	for (m=r->map; m<r->map+POOLSZ; ++m)
		if (m->time < mintime)
			minindex = m - r->map;
	m = &r->map[minindex];
	l = &r->pool[minindex];
	x = &r->lindex[m->index];
	/* remove line from pool */
	if (l->mod) {           /* line is modified, save it in temp file */
		if ((x->flags & XTEMP) && l->oldlen)
			tempfree (r, x->seek, l->oldlen);
		x->seek = tempsave (r, l->ptr, l->len);
		x->len = l->len;
		x->flags |= XTEMP;
	}
	if (l->len)
		free (l->ptr);
	x->poolindex = NOINDEX;
	m->busy = 0;
	return (minindex);
}

LINE *RecGet (REC *r, int n)
{
	register struct index *x;
	register struct map *m;
	register LINE *p;
	static long timecount = 1;              /* time stamp */

	if (n < 0 || n >= r->len)
		return (0);
	x = &r->lindex[n];
	if (x->poolindex != NOINDEX) {          /* line is in cache */
		r->map[x->poolindex].time = ++timecount;
		return (&r->pool[x->poolindex]);
	}
	x->poolindex = freeline (r);            /* get free pool index */
	p = &r->pool[x->poolindex];
	m = &r->map[x->poolindex];
	m->time = ++timecount;
	m->index = n;
	m->busy = 1;
	/* read line from file */
	readline ((x->flags & XTEMP) ? r->tfd : r->fd, x->seek, x->len, p);
	p->noeoln = (x->flags & XNOEOLN) != 0;
	return (p);
}

void RecPut (LINE *p, int newlen)
{
	if (! p->mod)
		p->oldlen = p->len;
	p->mod = 1;
	p->len = newlen;
}

void RecDelChar (REC *r, int line, int off)
{
	register LINE *p;
	char *s;

	if (! (p = RecGet (r, line)))
		return;
	if (p->len <= off)
		return;
	s = malloc (p->len-1);
	if (off)
		memcpy (s, p->ptr, off);
	if (off <= p->len-1)
		memcpy (s+off, p->ptr+off+1, p->len-off-1);
	free (p->ptr);
	p->ptr = s;
	RecPut (p, p->len - 1);
}

void RecInsChar (REC *r, int line, int off, int sym)
{
	register LINE *p;
	char *s;

	if (! (p = RecGet (r, line)))
		return;
	s = malloc (p->len+1);
	if (off)
		memcpy (s, p->ptr, off);
	s [off] = sym;
	if (off < p->len)
		memcpy (s+off+1, p->ptr+off, p->len-off);
	if (p->len)
		free (p->ptr);
	p->ptr = s;
	RecPut (p, p->len + 1);
}

void RecInsLine (REC *r, int n)
{
	register struct index *x, *i;
	register struct map *m;
	register k;

	if (n<0 || n>r->len)
		return;
	++r->len;
	mcheck (r->lindex, struct index *, r->nindex, QUANT, r->len+1);
	i = &r->lindex[n];
	for (x= &r->lindex[r->len-1]; x>i; --x)
		x[0] = x[-1];
	i->seek = 0;
	i->len = 0;
	i->poolindex = NOINDEX;
	i->flags = XTEMP;
	m = r->map;
	for (k=0; k<POOLSZ; ++k, ++m)
		if (m->index >= n)
			++m->index;
}

void RecDelLine (REC *r, int n)
{
	register struct index *x, *i;
	register struct map *m;
	register LINE *l;
	register k;

	if (n<0 || n>=r->len)
		return;
	x = &r->lindex[n];
	m = r->map;
	if (x->poolindex != NOINDEX) {          /* exclude line from pool */
		l = &r->pool[x->poolindex];
		if (x->flags & XTEMP)
			if (l->mod) {
				if (l->oldlen)
					tempfree (r, x->seek, (long) l->oldlen);
			} else if (l->len)
				tempfree (r, x->seek, l->len);
		if (l->len)
			free (l->ptr);
		m[x->poolindex].busy = 0;
	}
	for (k=0; k<POOLSZ; ++k, ++m)
		if (m->index > n)
			--m->index;
	i = &r->lindex[r->len-1];
	for (x= &r->lindex[n]; x<i; ++x)
		x[0] = x[1];
	--r->len;
}
