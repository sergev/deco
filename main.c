#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include "scr.h"
#include "dir.h"
#include "deco.h"
#include "env.h"

struct dir left, right;                 /* left and right directories */
struct dir *cur;                        /* current directory */
int H;                                  /* file window height */
int uid;                                /* real user id */
int main_pid;                           /* our process id */
int ppid;                               /* parent process id */
int widewin;                            /* full window mode */
char *user;
char *group;
char *tty = 0;
char *machine;
int cpos;                               /* position of command line cursor */
int cmdreg;                             /* cursor in command line */
int visualwin = 1;                      /* cursor in command line */
int lang;

#ifndef GETGROUPS_T
#define GETGROUPS_T int
#endif

GETGROUPS_T gidlist [128];              /* groups list */
int gidnum;                             /* number of groups in list */

struct palette dflt_palette = { 7, 0, 7, 6, 15, 0, 15, 6, 3, 0, 0, 7, };
struct palette palette;

static struct KeyMap keymap [] = {
	{ "kl",         0,              meta ('l')      },
	{ "kr",         0,              meta ('r')      },
	{ "ku",         0,              meta ('u')      },
	{ "kd",         0,              meta ('d')      },
	{ "kN",         0,              meta ('n')      },
	{ "kP",         0,              meta ('p')      },
	{ "kh",         0,              meta ('h')      },
	{ "kH",         0,              meta ('e')      },
	{ "@7",         0,              meta ('e')      },
	{ "kI",         0,              cntrl ('T')     },
	{ "k.",         0,              cntrl ('G')     },
	{ "kD",         0,              cntrl ('G')     },
	{ "kE",         0,              meta ('h')      },
	{ "kS",         0,              meta ('e')      },
	{ "k1",         0,              meta ('A')      },
	{ "k2",         0,              meta ('B')      },
	{ "k3",         0,              meta ('C')      },
	{ "k4",         0,              meta ('D')      },
	{ "k5",         0,              meta ('E')      },
	{ "k6",         0,              meta ('F')      },
	{ "k7",         0,              meta ('G')      },
	{ "k8",         0,              meta ('H')      },
	{ "k9",         0,              meta ('I')      },
	{ "k0",         0,              meta ('J')      },
	{ "k;",         0,              meta ('J')      },
	{ "f1",         0,              meta ('A')      },
	{ "f2",         0,              meta ('B')      },
	{ "f3",         0,              meta ('C')      },
	{ "f4",         0,              meta ('D')      },
	{ "f5",         0,              meta ('E')      },
	{ "f6",         0,              meta ('F')      },
	{ "f7",         0,              meta ('G')      },
	{ "f8",         0,              meta ('H')      },
	{ "f9",         0,              meta ('I')      },
	{ "f0",         0,              meta ('J')      },
	{ "kb",         "\b",           meta ('b')      },
	{ 0,            "\0331",        meta ('A')      },
	{ 0,            "\0332",        meta ('B')      },
	{ 0,            "\0333",        meta ('C')      },
	{ 0,            "\0334",        meta ('D')      },
	{ 0,            "\0335",        meta ('E')      },
	{ 0,            "\0336",        meta ('F')      },
	{ 0,            "\0337",        meta ('G')      },
	{ 0,            "\0338",        meta ('H')      },
	{ 0,            "\0339",        meta ('I')      },
	{ 0,            "\0330",        meta ('J')      },

	{ 0,            "\033l",        meta ('l')      },
	{ 0,            "\033r",        meta ('r')      },
	{ 0,            "\033u",        meta ('u')      },
	{ 0,            "\033d",        meta ('d')      },
	{ 0,            "\033n",        meta ('n')      },
	{ 0,            "\033p",        meta ('p')      },
	{ 0,            "\033h",        meta ('h')      },
	{ 0,            "\033e",        meta ('e')      },

	{ 0,            "\033\033",     cntrl ('C')     },
	{ 0,            0,              0               },
};

static void init (void);
static void inithome (void);
static void initlang (void);
static void execute (void);
static void docmdreg (int c);
static void doscrreg (int c);

int main (int argc, char **argv, char **envp)
{
	register c;

	if (argc > 2) {
		outerr ("Usage: deco [dirname]\n");
		exit (1);
	}
#ifdef GC
	/*GC_set_warn_proc (outerr);*/
#endif
	outerr ("Demos Commander, Copyright (C) 1989-1997 Serge Vakulenko\n");
	palette = dflt_palette;
	if (argc > 1)
		chdir (argv [1]);
	EnvInit (envp);
	initlang ();
	uid = getuid ();
#ifdef HAVE_GETGROUPS
	gidnum = getgroups (sizeof(gidlist)/sizeof(gidlist[0]), gidlist);
#else
	gidlist[0] = getgid ();
	gidnum = 1;
#endif
	main_pid = getpid ();
	ppid = getppid ();
	user = username (uid);
	group = groupname (gidlist[0]);
	tty = ttyname (0);
	machine = getmachine ();
	sigign ();
	tcsetpgrp (2, main_pid);

	init ();
	inithome ();
	VClear ();
	right.sort = SORTEXT;
	left.sort = SORTEXT;
	right.typesort = 1;
	left.typesort = 1;
	right.alignext = 1;
	left.alignext = 1;
	readinitfile ();
	if (uid == 0)
		palette.dimfg = 6;
	VSetPalette (palette.fg, palette.bg, palette.revfg, palette.revbg,
		palette.boldfg, palette.boldbg, palette.boldrevfg, palette.boldrevbg,
		palette.dimfg, palette.dimbg, palette.dimrevfg, palette.dimrevbg);
	setdir (&left, ".");
	setdir (&right, home);
	chdir (left.cwd);
	cur = &left;
	draw ();
	for (;;) {
		if (! cmdreg)
			drawcursor ();
		drawcmd ();
		VSync ();
		c = KeyGet ();
		if (! cmdreg)
			undrawcursor ();
		switch (c) {
		case '+':               /* select */
		case '-':               /* unselect */
			if (! cpos && ! cmdreg && ! cur->status) {
				if (c == '+')
					tagall ();
				else
					untagall ();
				draw ();
				continue;
			}
		default:
			if ((c>=' ' && c<='~') || (c>=0300 && c<=0377)) {
				if (cpos || c!=' ')
					inscmd (c);
				continue;
			}
			VBeep ();
			continue;
		case cntrl ('V'):       /* quote next char */
			inscmd (quote ());
			continue;
		case cntrl ('J'):       /* insert file name */
			if (! cmdreg && ! cur->status)
				namecmd ();
			continue;
		case cntrl ('G'):
			delcmd ();
			continue;
		case meta ('b'):        /* backspace */
			if (cpos) {
				leftcmd ();
				delcmd ();
			}
			continue;
		case cntrl ('O'):       /* set/unset command mode */
		case cntrl ('P'):       /* set/unset command mode */
			switchcmdreg ();
			if (! cmdreg)
				visualwin = 1;
			draw ();
			continue;
		case cntrl ('M'):         /* return */
			if (command [0]) {
				execmd (1, 1);
				draw ();
				continue;
			}
			if (cmdreg) {
				cmdreg = 0;
				if (! visualwin) {
					visualwin = 1;
					setdir (cur==&left ? &right : &left, 0);
					setdir (cur, 0);
				}
				draw ();
				continue;
			}
			execute ();
			continue;
		case cntrl (']'):       /* redraw screen */
			VRedraw ();
			continue;
		case cntrl ('B'):        /* history */
			if (! visualwin)
				VClearBox (1, 0, LINES-2, 80);
			histmenu ();
			draw ();
			continue;
		case meta ('A'):        /* f1 */
			genhelp ();
			draw ();
			continue;
		case meta ('B'):          /* f2 */
			usermenu ();
			draw ();
			continue;
		case meta ('I'):        /* f9 */
			runmenu (cur==&left ? 'l' : 'r');
			draw ();
			continue;
		case meta ('J'):        /* f0 */
		case cntrl ('C'):       /* quit */
			quit ();
			continue;
		case cntrl ('U'):       /* swap panels */
			swappanels ();
			draw ();
			continue;
		case cntrl ('F'):       /* full screen */
			fullscreen (0, 0);
			draw ();
			continue;
		case cntrl ('^'):       /* cd / */
			directory (0, 'r');
			if (! cur->status)
				drawdir (cur, 1);
			continue;
		case cntrl ('\\'):      /* cd $HOME */
			directory (0, 'o');
			if (! cur->status)
				drawdir (cur, 1);
			continue;
		case cntrl ('Y'):       /* clear line */
			command [cpos = 0] = 0;
			continue;
		case cntrl ('X'):       /* next history */
			nextcmd ();
			continue;
		case cntrl ('E'):       /* prev history */
			prevcmd ();
			continue;
		case cntrl ('S'):       /* char left */
		case cntrl ('A'):       /* char left */
			leftcmd ();
			continue;
		case cntrl ('D'):       /* char right */
			rightcmd ();
			continue;
		case cntrl ('I'):       /* tab */
			if (cmdreg)
				if (command [cpos])
					endcmd ();
				else
					homecmd ();
			else {
				switchpanels ();
				if (widewin) {
					drawbanners ();
					drawdir (cur, 0);
					break;
				}
			}
			continue;
		case cntrl ('W'):       /* double width */
			if (! cmdreg) {
				setdwid ();
				draw ();
			}
			continue;
		case meta ('G'):        /* f7 */
			makedir ();
			draw ();
			continue;
		case meta ('h'):        /* home */
		case meta ('e'):        /* end */
		case meta ('u'):        /* up */
		case meta ('d'):        /* down */
		case meta ('l'):        /* left */
		case meta ('r'):        /* right */
		case meta ('n'):        /* next page */
		case meta ('p'):        /* prev page */
		case cntrl ('K'):       /* find file */
		case cntrl ('R'):       /* reread catalog */
		case cntrl ('T'):       /* tag file */
		case meta ('C'):        /* f3 */
		case meta ('D'):        /* f4 */
		case meta ('E'):        /* f5 */
		case meta ('F'):        /* f6 */
		case meta ('H'):        /* f8 */
		case cntrl ('L'):       /* status */
			if (cmdreg || cur->status)
				docmdreg (c);
			else
				doscrreg (c);
			continue;
		}
	}
}

static void doscrreg (int c)
{
	switch (c) {
	case meta ('h'):          /* home */
		cur->curfile = cur->topfile = 0;
		break;
	case meta ('e'):          /* end */
		cur->curfile = cur->num - 1;
		cur->topfile = cur->num - PAGELEN (cur);
		if (cur->topfile < 0)
			cur->topfile = 0;
		break;
	case meta ('u'):          /* up */
		if (cur->curfile <= 0)
			return;
		if (cur->curfile > cur->topfile) {
			--cur->curfile;
			return;
		}
		cur->topfile = --cur->curfile;
		break;
	case meta ('d'):          /* down */
		if (cur->curfile >= cur->num-1)
			return;
		if (cur->topfile + PAGELEN (cur) - 1 > cur->curfile) {
			++cur->curfile;
			return;
		}
		cur->topfile = ++cur->curfile - PAGELEN (cur) + 1;
		break;
	case meta ('l'):          /* left */
		if (cur->curfile < H) {
			if (cur->topfile <= 0)
				return;
			cur->curfile -= cur->topfile;
			cur->topfile = 0;
			break;
		}
		cur->curfile -= H;
		if (cur->topfile <= cur->curfile)
			return;
		cur->topfile -= H;
		if (cur->topfile <= 0) {
			cur->curfile -= cur->topfile;
			cur->topfile = 0;
		}
		break;
	case meta ('r'):          /* right */
		if (cur->curfile + H < cur->num) {
			cur->curfile += H;
			if (cur->topfile + PAGELEN (cur) > cur->curfile)
				return;
			cur->topfile += H;
			break;
		}
		if (cur->topfile + PAGELEN (cur) < cur->num) {
			cur->curfile = cur->num-1;
			cur->topfile += H;
			break;
		}
		if ((cur->curfile - cur->topfile) / H <
		    (cur->num - cur->topfile - 1) / H)
			cur->curfile = cur->num-1;
		return;
	case meta ('n'):          /* next page */
		if (cur->topfile + PAGELEN (cur) >= cur->num) {
			cur->curfile = cur->num-1;
		} else if (cur->topfile + 2 * PAGELEN (cur) >= cur->num) {
			cur->curfile = cur->num-1;
			cur->topfile = cur->num - PAGELEN (cur);
		} else {
			cur->curfile += PAGELEN (cur);
			cur->topfile += PAGELEN (cur);
		}
		break;
	case meta ('p'):          /* prev page */
		if (cur->topfile == 0) {
			cur->curfile = 0;
		} else {
			cur->curfile -= PAGELEN (cur);
			if (cur->topfile > cur->curfile)
				cur->topfile -= PAGELEN (cur);
			if (cur->topfile < 0) {
				cur->curfile -= cur->topfile;
				cur->topfile = 0;
			}
		}
		break;
	case cntrl ('K'):         /* find file */
		findname ();
		break;
	case cntrl ('R'):         /* reread catalog */
		reread (cur==&left ? 'l' : 'r', 0);
		break;
	case cntrl ('T'):         /* tag file */
		if ((cur->cat[cur->curfile].mode & S_IFMT) == (unsigned) S_IFREG) {
			cur->cat[cur->curfile].tag ^= 1;
			counttag (cur);
		}
		if (cur->curfile < cur->num-1) {
			/* move down */
			++cur->curfile;
			if (cur->topfile + PAGELEN (cur) - 1 < cur->curfile)
				cur->topfile = cur->curfile - PAGELEN (cur) + 1;
		}
		break;
	case meta ('C'):          /* f3 */
		if ((cur->cat[cur->curfile].mode & S_IFMT) != (unsigned) S_IFREG)
			return;
		view ();
		draw ();
		return;
	case meta ('D'):          /* f4 */
		edit ();
		setdir (cur==&left ? &right : &left, 0);
		setdir (cur, 0);
		draw ();
		return;
	case meta ('E'):          /* f5 */
		copy ();
		draw ();
		return;
	case meta ('F'):          /* f6 */
		renmove ();
		draw ();
		return;
	case meta ('H'):          /* f8 */
		delete ();
		draw ();
		return;
	case cntrl ('L'):         /* status */
		setstatus ();
		draw ();
		return;
	}
	drawdir (cur, 1);
}

static void docmdreg (int c)
{
	switch (c) {
	case meta ('h'):          /* home */
		homecmd ();
		return;
	case meta ('e'):          /* end */
		endcmd ();
		return;
	case meta ('l'):          /* left */
		leftcmd ();
		return;
	case meta ('r'):          /* right */
		rightcmd ();
		return;
	case meta ('u'):          /* up */
		upcmd ();
		return;
	case meta ('d'):          /* down */
		downcmd ();
		return;
	case meta ('n'):          /* next page */
		nextcmd ();
		return;
	case meta ('p'):          /* prev page */
		prevcmd ();
		return;
	}
}

static void execute ()
{
	register struct file *file = &cur->cat[cur->curfile];
	register char *name = file->name;
	int updir, dev = 0, ino = 0;

	switch (file->mode & S_IFMT) {
	case S_IFDIR:
		if (! strcmp (name, ".")) {
			setdir (cur, 0);
			break;
		}
		if ((updir = !strcmp (name, ".."))) {
			dev = cur->dev;
			ino = cur->ino;
		}
		if (chdir (name) < 0)
			break;
		setdir (cur, ".");
		if (updir) {
			for (cur->curfile=0; cur->curfile<cur->num; ++cur->curfile) {
				if (cur->cat[cur->curfile].dev == dev &&
				    cur->cat[cur->curfile].ino == ino)
					break;
			}
			if (cur->curfile >= cur->num)
				cur->curfile = 0;
			if (cur->topfile + PAGELEN (cur) <= cur->curfile)
				cur->topfile = cur->curfile - PAGELEN (cur) + 1;
		}
		if (visualwin) {
			drawdir (&left, 0);
			drawdir (&right, 0);
		}
		break;
	case S_IFREG:
		if (file->execable)
			strcpy (command, file->name);
		else
			excommand (command, file->name);
		cpos = strlen (command);
		if (! command [0])
			break;
		execmd (file->execable ? 1 : 0, 1);
		draw ();
		break;
	}
}

static void init ()
{
	char *buf;

	buf = malloc (2048);
	if (! CapInit (buf)) {
		outerr ("Cannot read termcap\n");
		exit (1);
	}
	if (! VInit ()) {
		outerr ("Cannot initialize video device\n");
		exit (1);
	}
	if (COLS < 80) {
		outerr ("The screen must have at least 80 columns\n");
		exit (1);
	}
	KeyInit (keymap, VFlush);
	VOpen ();
	free (buf);

	/* H is the number of lines at file window */

	H = LINES/2-1;

	left.basecol = 0;
	right.basecol = 40;
}

int pagelen (int view)
{
	register h = H;

	if (! widewin)
		return (view == VIEW_BRIEF ? h+h : h);
	switch (view) {
	case VIEW_BRIEF:
		h += h;
	case VIEW_LONG:
	case VIEW_WIDE:
		return (h+h);
	default:
		return (h);
	}
}

static void inithome ()
{
	register char *s;

	if ((s = EnvGet ("HOME")))
		home = s;
}

static void initlang ()
{
	register char *s = EnvGet ("MSG");

	if (! s)
		s = "e";
	switch (*s) {
	default:  lang = ENG; break;
	case 'r': lang = RUS; break;
	case 'u': lang = UKR; break;
	case 'd': lang = DEU; break;
	case 'f': lang = FRA; break;
	}
}

void setlang ()
{
	char *s, *altlang;

	s = EnvGet ("MSG");
	if (! s)
		s = "l";
	switch (*s) {
	default:  altlang = " Russian ";   break;
	case 'u': altlang = " Ukrainian "; break;
	case 'd': altlang = " Deutch ";    break;
	case 'f': altlang = " Francais ";  break;
	}
	switch (getchoice (0, " Language ", "Select language to use:",
	    0, " English ", altlang, 0)) {
	case 0:
		lang = ENG;
		break;
	case 1:
		switch (*s) {
		default:  lang = RUS; break;
		case 'u': lang = UKR; break;
		case 'd': lang = DEU; break;
		case 'f': lang = FRA; break;
		}
		break;
	}
}

void hidecursor ()
{
	VMove (LINES-1, COLS-2);
}

void mvcaddstr (int r, int c, char *s)
{
	VMPutString (r, c - (strlen (s) + 1) / 2, s);
}

int quote ()
{
	register c, i;

	c = KeyGet ();
	if (c<'0' || c>'3')
		return (c);
	i = c & 3;
	c = KeyGet ();
	if (c<'0' || c>'7') {
		KeyUnget (c);
		return (i);
	}
	i = i << 3 | (c & 7);
	c = KeyGet ();
	if (c<'0' || c>'7') {
		KeyUnget (c);
		return (i);
	}
	return (i << 3 | (c & 7));
}

int mygroup (int gid)
{
	int i;

	for (i=0; i<gidnum; ++i)
		if (gid == gidlist[i]) 
			return 1;
	return 0;
}
