#include <stdio.h>
#include <string.h>
#include <time.h>
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include "dir.h"
#include "deco.h"
#include "scr.h"

#define DECOVERSION "Version 3.9"

static BOX *cbox;                       /* box under cursor */
static struct dir *cdir;                /* drawed directory */

static void draw1dir (struct dir *c);
static void drawframe (int c, int view, int stawin);
static void drawcwd (int col, char *wd);
static void drawpat (int col, char *pat, int width);
static void drawsdir (struct dir *c);
static void drawstatus (int col, struct file *f, int view);
static void drawpage (int col, struct file *c, int n, struct dir *d);
static void drawfull (struct file *f, int tail);
static void drawmode (char *m, int u, int g);
static void drawtime (struct file *f);
static void drawlong (struct file *f);
static char *filnam (const char *p, int tail);
static char *filnam_wide (const char *p);

void draw ()
{
	if (visualwin) {
		drawbanners ();
		draw1dir (&left);
		draw1dir (&right);
	} else {
		VClearBox (1, 0, LINES-2, 80);
		drawbanners ();
	}
}

void drawdir (struct dir *c, int statusflag)    /* redraw window */
{
	draw1dir (c);
	if (statusflag) {
		c = (c == &left ? &right : &left);
		if (c->status)
			draw1dir (c);
	}
}

void drawbanners ()
{
	register i;
	register char *curtime;

	if (visualwin)
		if (widewin)
			drawframe (BASECOL (cur), cur->view, cur->status);
		else {
			drawframe (left.basecol, left.view, left.status);
			drawframe (right.basecol, right.view, right.status);
		}
	VSetDim ();
	VStandOut ();
	VMove (0, 0);
	for (i=80; --i>=0; VPutChar (' '));
	VMove (0, 1);
	if (machine) {
		VPutString (machine);
		VPutChar (':');
	}
	if (tty)
		VPutString (tty);
	if (user)
		VMPutString (0, 39 - strlen (user), user);
	VPutChar ('.');
	if (group)
		VPutString (group);
	if ((curtime = timestr (time ((long *) 0))))
		VMPutString (0, 79 - strlen (curtime), curtime);
	VStandEnd ();
	VMPutString (LINES-1, 0, cmdreg ?
"1\16Help  \17 2\16User  \17 3\16      \17 4\16      \17 5\16    \
  \17 6\16      \17 7\16      \17 8\16      \17 9\16Menu  \017 10\16Quit \17" :
"1\16Help  \17 2\16User  \17 3\16View  \17 4\16Edit  \17 5\16Copy\
  \17 6\16RenMov\17 7\16Mkdir \17 8\16Delete\17 9\16Menu  \017 10\16Quit \17");
	VSetNormal ();
}

static void draw1dir (struct dir *c)    /* redraw window */
{
	register len = c->num - c->topfile;

	/* check visibility in full window mode */
	if (! visualwin || (widewin && c != cur))
			return;
	cdir = c;

	/* if it is status window - draw status of another one */
	if (c->status) {
		drawsdir (c);
		return;
	}

	/* reprint dir name */
	VSetDim ();
	VHorLine (1, BASECOL (c) + 1, PAGEWID-2);
	VHorLine (H+2, BASECOL (c) + 1, PAGEWID-2);
	VHorLine (H+4, BASECOL (c) + 1, PAGEWID-2);
	if (widewin)
		switch (c->view) {
		case VIEW_BRIEF:
			VCorner (1, 19, 2);
			VCorner (1, 38, 2);
			VCorner (1, 57, 2);
			break;
		case VIEW_LONG:
		case VIEW_WIDE:
			VCorner (1, 38, 2);
			break;
		}
	else if (c->view == VIEW_BRIEF)
		VCorner (H+2, BASECOL(c) + 19, 8);
	drawcwd (c->basecol, c->shortcwd);
	if (c->pattern [0])
		drawpat (BASECOL (c), c->pattern, PAGEWID);
	else {
		char buf [80];

		sprintf (buf, " %s byte%s in %d file%s ", 
			ltoac (c->nbytes), c->nbytes==1 ? "" : "s",
			c->nfiles, c->nfiles==1 ? "" : "s");
		mvcaddstr (H+4, BASECOL (c) + PAGEWID/2 + 1, buf);
		if (c->tfiles) {
			sprintf (buf, " %s byte%s in %d file%s ",
				ltoac (c->tbytes), c->tbytes==1 ? "" : "s",
				c->tfiles, c->tfiles==1 ? "" : "s");
			VSetBold ();
			mvcaddstr (H+2, BASECOL (c) + PAGEWID/2 + 1, buf);
		}
	}
	VSetNormal ();

	switch (c->view) {
	case VIEW_INFO:
	case VIEW_FULL:
		if (widewin) {
			VClearBox (2, 1, H, 75);
			drawpage (1, &c->cat[c->topfile], len, c);
			break;
		}
		/* Fall through... */
	case VIEW_LONG:
	case VIEW_WIDE:
		VClearBox (2, BASECOL (c) + 1, H, 37);
		drawpage (BASECOL (c) + 1, &c->cat[c->topfile], len, c);
		if (widewin) {
			VClearBox (2, BASECOL (c) + 39, H, 37);
			if (len > H)
				drawpage (BASECOL (c) + 39,
					&c->cat[c->topfile+H], len-H, c);
		}
		break;
	case VIEW_BRIEF:
		VClearBox (2, BASECOL (c) + 1, H, 18);
		drawpage (BASECOL (c) + 1, &c->cat[c->topfile], len, c);

		VClearBox (2, BASECOL (c) + 20, H, 18);
		if (len > H)
			drawpage (BASECOL (c) + 20, &c->cat[c->topfile+H],
				len -= H, c);
		if (widewin) {
			VClearBox (2, BASECOL (c) + 39, H, 18);
			if (len > H)
				drawpage (BASECOL (c) + 39,
					&c->cat[c->topfile+H+H], len-=H, c);

			VClearBox (2, BASECOL (c) + 58, H, 18);
			if (len > H)
				drawpage (BASECOL (c) + 58,
					&c->cat[c->topfile+H+H+H], len-H, c);
		}
		break;
	}
	drawstatus (BASECOL (c), &c->cat[c->curfile], c->view);
}

void drawcursor ()
{
	register col, line, w;

	cdir = cur;
	if (cur->status) {
		VStandOut ();
		drawcwd (cur->basecol, " Status ");
		VStandEnd ();
		return;
	}
	switch (cur->view) {
	case VIEW_INFO:	case VIEW_FULL:	w = PAGEWID-2;	break;
	case VIEW_LONG:	case VIEW_WIDE:	w = 37;		break;
	default:			w = 18;		break;
	}
	line = cur->curfile - cur->topfile;
	if (line < H)
		col = BASECOL (cur) + 1;
	else {
		col = BASECOL (cur) + line/H * (w+1) + 1;
		line %= H;
	}
	line += 2;
	if (cbox)
		VFreeBox (cbox);
	cbox = VGetBox (line, col, 1, w);
	if (VStandOut ())
		if (cur->cat[cur->curfile].tag) {
			VSetBold ();
			VPrintBox (cbox);
			VSetNormal ();
		} else
			VPrintBox (cbox);
	else {
		VMPutChar (line, col, '[');
		VMPutChar (line, col+w-1, ']');
	}
	drawcwd (cur->basecol, cur->shortcwd);
	VStandEnd ();
	drawstatus (BASECOL (cur), &cur->cat[cur->curfile], cur->view);
}

void undrawcursor ()
{
	if (cbox) {
		VUngetBox (cbox);
		cbox = 0;
	}
	VSetDim ();
	drawcwd (cur->basecol, cur->status ? " Status " : cur->shortcwd);
	VSetNormal ();
}

/*
 * Draw frame of (one) window
 */
static void drawframe (int c, int view, int stawin)
{
	VSetDim ();
	if (stawin) {
		VHorLine (4, c+1, PAGEWID-2);
		VHorLine (11, c+1, PAGEWID-2);
		VVertLine (c, 2, H+2);
		VVertLine (c+PAGEWID-1, 2, H+2);
		VCorner (1, c, 1);
		VCorner (1, c+PAGEWID-1, 3);
		VCorner (4, c, 4);
		VCorner (4, c+PAGEWID-1, 6);
		VCorner (11, c, 4);
		VCorner (11, c+PAGEWID-1, 6);
		VCorner (H+4, c, 7);
		VCorner (H+4, c+PAGEWID-1, 9);
	} else if (widewin) switch (view) {
	case VIEW_INFO:
	case VIEW_FULL:
		VHorLine (H+2, 1, PAGEWID-2);
		VVertLine (0, 2, H+2);
		VVertLine (PAGEWID-1, 2, H+2);
		VCorner (1, 0, 1);
		VCorner (1, PAGEWID-1, 3);
		VCorner (H+2, 0, 4);
		VCorner (H+2, PAGEWID-1, 6);
		VCorner (H+4, 0, 7);
		VCorner (H+4, PAGEWID-1, 9);
		break;
	case VIEW_WIDE:
	case VIEW_LONG:
		VHorLine (H+2, 1, PAGEWID-2);
		VVertLine (0, 2, H+2);
		VVertLine (38, 2, H);
		VVertLine (PAGEWID-1, 2, H+2);
		VCorner (1, 0, 1);
		VCorner (1, PAGEWID-1, 3);
		VCorner (H+2, 0, 4);
		VCorner (H+2, 38, 8);
		VCorner (H+2, PAGEWID-1, 6);
		VCorner (H+4, 0, 7);
		VCorner (H+4, PAGEWID-1, 9);
		break;
	case VIEW_BRIEF:
		VHorLine (H+2, 1, PAGEWID-2);
		VVertLine (0, 2, H+2);
		VVertLine (19, 2, H);
		VVertLine (38, 2, H);
		VVertLine (57, 2, H);
		VVertLine (PAGEWID-1, 2, H+2);
		VCorner (1, 0, 1);
		VCorner (1, PAGEWID-1, 3);
		VCorner (H+2, 0, 4);
		VCorner (H+2, 19, 8);
		VCorner (H+2, 38, 8);
		VCorner (H+2, 57, 8);
		VCorner (H+2, PAGEWID-1, 6);
		VCorner (H+4, 0, 7);
		VCorner (H+4, PAGEWID-1, 9);
		break;
	} else {
		VHorLine (H+2, c+1, PAGEWID-2);
		VVertLine (c, 2, H+2);
		if (view == VIEW_BRIEF)
			VVertLine (c+19, 2, H);
		VVertLine (c+38, 2, H+2);
		VCorner (1, c, 1);
		VCorner (1, c+38, 3);
		VCorner (H+2, c, 4);
		VCorner (H+2, c+38, 6);
		VCorner (H+4, c, 7);
		if (view == VIEW_BRIEF)
			VCorner (H+2, c+19, 8);
		VCorner (H+4, c+38, 9);
	}
	VSetNormal ();
}

static void drawcwd (int col, char *wd)
{
	register len;

	if (! visualwin)
		return;
	len = strlen (wd);
	if (widewin && col)
		col = 38;
	VMPutString (1, col + (39-len) / 2, wd);
}

static void drawpat (int col, char *pat, int width)
{
	col += width - strlen (pat) - 1;
	VMove (H+4, col);
	VPutString (pat);
}

static void drawsdir (struct dir *c)    /* draw status window */
{
	struct dir *d = c == &left ? &right : &left;
	char buf [80];

	/* print window name " Status " */
	VSetDim ();
	VHorLine (1, BASECOL (c) + 1, PAGEWID-2);
	drawcwd (c->basecol, " Status ");
	VHorLine (H+4, BASECOL (c) + 1, PAGEWID-2);
	VSetNormal ();
	VClearBox (2, BASECOL (c) + 1, 2, PAGEWID-2);
	VClearBox (5, BASECOL (c) + 1, 6, PAGEWID-2);
	VClearBox (12, BASECOL (c) + 1, H-8, PAGEWID-2);
	sprintf (buf, "The Demos Commander, %s", DECOVERSION);
	mvcaddstr (2, BASECOL (c) + PAGEWID/2 + 1, buf);
	mvcaddstr (3, BASECOL (c) + PAGEWID/2 + 1, "Copyright (C) 1989-2000 S.Vakulenko");
	mvcaddstr (5, BASECOL (c) + PAGEWID/2 + 1, "Directory");
	mvcaddstr (6, BASECOL (c) + PAGEWID/2 + 1, d->shortcwd);
	sprintf (buf, "\3%d\2 file%s", d->nfiles, d->nfiles==1 ? "" : "s");
	mvcaddstr (7, BASECOL (c) + PAGEWID/2 + 2, buf);
	sprintf (buf, "\3%s\2 byte%s", ltoac (d->nbytes), d->nbytes==1 ? "" : "s");
	mvcaddstr (8, BASECOL (c) + PAGEWID/2 + 2, buf);
	if (d->pattern [0]) {
		sprintf (buf, "\3%s\2 byte%s in \3%d\2 matched file%s",
			ltoac (d->mbytes), d->mbytes==1 ? "" : "s",
			d->mfiles, d->mfiles==1 ? "" : "s");
		mvcaddstr (9, BASECOL (c) + PAGEWID/2 + 3, buf);
	}
	if (d->tfiles) {
		sprintf (buf, "\3%s\2 byte%s in \3%d\2 selected file%s",
			ltoac (d->tbytes), d->tbytes==1 ? "" : "s",
			d->tfiles, d->tfiles==1 ? "" : "s");
		mvcaddstr (10, BASECOL (c) + PAGEWID/2 + 3, buf);
	}
#ifdef notyet
	mvcaddstr (13, BASECOL (c) + PAGEWID/2 + 1, "No .dirinfo file in this directory");
#endif
}

static void drawstatus (int col, struct file *f, int view)
{
	/* draw current file status box */
	VMove (H+3, col+2);
	VSetDim ();
	VPutChar (filetype (f));
	VSetNormal ();
	VPutChar (' ');
	if (widewin) switch (view) {
	case VIEW_FULL:
		VPutString (filnam (f->name, 1));
		drawtime (f);
		break;
	case VIEW_INFO:
	case VIEW_LONG:
	case VIEW_WIDE:
	case VIEW_BRIEF:
		drawfull (f, 1);
		drawlong (f);
		break;
	} else
		drawfull (f, 1);
}

/*
 * Redraw file window
 */
static void drawpage (int col, struct file *c, int n, struct dir *d)
{
	register i;
	char *info;

	if (n > H)
		n = H;
	++col;
	for (i=0; i<n; ++i, ++c) {
		VSetDim ();
		VMPutChar (i+2, col, filetype (c));
		if (c->tag) {
			VPutChar ('>');
			VSetBold ();
		} else if (c->link) {
			VPutChar ('@');
			VSetNormal ();
		} else {
			VPutChar (' ');
			VSetNormal ();
		}
		switch (d->view) {
		case VIEW_INFO:
			if (! widewin)
				goto draw_long;
			VPutString (filnam (c->name, 0));	/* 14 */
			VPutChar (' ');                         /* 1 */
			VPutChar (' ');                         /* 1 */
			if (! c->infoflag)
				setinfo (c, d);
			if (! c->dateflag)
				VSetDim ();
			VPutString (filedate (c->infodate));    /* 8 */
			if (! c->dateflag)
				VSetPrev ();
			if (c->rinfo && (lang==RUS || lang==UKR))
				info = c->rinfo;
			else
				info = c->info;
			if (info) {
				VPutChar (' ');                 /* 1 */
				VPutChar (' ');                 /* 1 */
				VPutString (info);              /* 48 */
			}
			break;
		case VIEW_FULL:
			drawfull (c, 0);
			if (widewin)
				drawlong (c);
			break;
		case VIEW_LONG:
draw_long:		drawfull (c, 0);
			break;
		case VIEW_WIDE:
			VPutString (filnam_wide (c->name));
			break;
		case VIEW_BRIEF:
			VPutString (filnam (c->name, 0));
			break;
		}
		if (c->tag)
			VSetNormal ();
	}
}

static void drawfull (struct file *f, int tail)
{
	VPutString (filnam (f->name, tail));			/* 14 */
	VPutString (filesize (f));                              /* 8 */
	VPutChar (' ');                                         /* 1 */
	VPutChar (' ');                                         /* 1 */
	drawmode (filemode ((int) f->mode), (int) f->uid, (int) f->gid);
}

static void drawmode (char *m, int file_uid, int file_gid)
{
	int u, g;

	u = (file_uid == uid);
	g = mygroup (file_gid);
	if (! u)
		VSetDim ();
	VPutChar (*m++);
	VPutChar (*m++);
	VPutChar (*m++);
	if (u)
		VSetDim ();
	else if (g)
		VSetPrev ();
	VPutChar (*m++);
	VPutChar (*m++);
	VPutChar (*m++);
	if (! u)
		if (g)
			VSetDim ();
		else
			VSetPrev ();
	VPutChar (*m++);
	VPutChar (*m++);
	VPutChar (*m++);
	if (u || g)
		VSetPrev ();
}

static void drawtime (struct file *f)
{
	VPrint ("%4d/%-5d", f->dev, f->ino);
	if (f->nlink > 1)
		VPrint (" %-3d ", f->nlink);
	else
		VPutString ("     ");
	VPutString (timestr (f->atime));
	VPutChar (' ');
	VPutChar (' ');
	VPutString (timestr (f->ctime));
}

static void drawlong (struct file *f)
{
	VPrint (" %8.8s.%-8.8s", username ((int) f->uid), groupname ((int) f->gid));
	VPutString (timestr (f->mtime));
}

static char *filnam (const char *p, int tail)
{
	static char buf [20];
	register char *s;
	const char *e, *q;

	if (strlen (p) > 14) {
		if (tail) {
			buf [0] = '~';
			strcpy (buf+1, p + strlen(p) - 13);
		} else {
			strncpy (buf, p, 13);
			buf [13] = '~';
		}
		return (buf);
	}
	for (s=buf; s<buf+14; *s++=' ');		/* clear buf */
	if (cdir->alignext) {
		e = extension (p);			/* make ptr to extension */
		for (s=buf; p<e; *s++= *p++);		/* copy file name */
		for (q=e; *q; ++q);			/* search extension end */
		for (s=buf+13, --q; q>=e; *s--= *q--);	/* copy extension */
	} else {
		e = p + 14;				/* make ptr to end */
		for (s=buf; *p && p<e; *s++= *p++);	/* copy file name */
	}
	buf [14] = 0;
	return (buf);
}

static char *filnam_wide (const char *p)
{
	static char buf [40];
	register char *s;
	const char *e, *q;

#define N 33
	if (strlen (p) > N) {
		strncpy (buf, p, N-1);
		buf [N-1] = '~';
		return (buf);
	}
	for (s=buf; s<buf+N; *s++=' ');			/* clear buf */
	if (cdir->alignext) {
		e = extension (p);			/* make ptr to extension */
		for (s=buf; p<e; *s++= *p++);		/* copy file name */
		for (q=e; *q; ++q);			/* search extension end */
		for (s=buf+N-1, --q; q>=e; *s--= *q--);	/* copy extension */
	} else {
		e = p + N;				/* make ptr to end */
		for (s=buf; *p && p<e; *s++= *p++);	/* copy file name */
	}
	buf [N] = 0;
	return (buf);
}
