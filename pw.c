#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include "deco.h"

#define NAMLEN 9
#define QUANT  20

struct table {
	int key;
	char name [NAMLEN];
};

static struct table *uname;
static struct table *grname;
static nuname, ngrname;

static char root[] = "/";
char *home = root;

static int compare (const void *arg1, const void *arg2)
{
	const struct table *a = arg1;
	const struct table *b = arg2;
	register ak = a->key, bk = b->key;

	return (ak<bk ? -1 : ak>bk ? 1 : 0);
}

static void initpw ()
{
	register t, tt;
	register struct passwd *pw;
	register struct group *gr;

	setpwent ();
	for (t=0; (pw=getpwent()); ) {
		mcheck (uname, struct table *, nuname, QUANT, t);
		if (pw->pw_uid == uid) {
			if (home != root)
				free (home);
			home = malloc (strlen (pw->pw_dir) + 1);
			strcpy (home, pw->pw_dir);
		}
		for (tt=0; tt<t; ++tt)
			if (uname[tt].key == pw->pw_uid)
				break;
		if (tt < t)
			continue;	/* ignore entry with same uid */
		uname[t].key = pw->pw_uid;
		strncpy (uname[t].name, pw->pw_name, NAMLEN-1);
		uname[t].name[NAMLEN-1] = 0;
		++t;
	}
	endpwent ();
	nuname = t;
	if (nuname)
		qsort ((char *) uname, (unsigned) nuname, sizeof (struct table), compare);
	setgrent ();
	for (t=0; (gr=getgrent()); ) {
		mcheck (grname, struct table *, ngrname, QUANT, t);
		for (tt=0; tt<t; ++tt)
			if (grname[tt].key == gr->gr_gid)
				break;
		if (tt < t)
			continue;       /* ignore entry with same gid */
		grname[t].key = gr->gr_gid;
		strncpy (grname[t].name, gr->gr_name, NAMLEN-1);
		grname[t].name[NAMLEN-1] = 0;
		++t;
	}
	endgrent ();
	ngrname = t;
	if (ngrname) {
		qsort ((char *) grname, (unsigned) ngrname, sizeof (struct table), compare);
	}
}

static struct table *tabsearch (int key, struct table *base, int nel)
{
	struct table *last;     /* Last element in table */
	register struct table *p;

	if (! nel)
		return ((struct table *) 0);
	last = base + (nel - 1);
	while (last >= base) {
		p = base + ((last - base)/2);
		if (p->key == key)
			return (p);	/* Key found */
		if (p->key > key)
			last = p - 1;
		else
			base = p + 1;
	}
	return ((struct table *) 0);    /* Key not found */
}

char *username (int u)
{
	register struct table *t;
	static char buf [NAMLEN];

	if (! nuname)
		initpw ();
	if ((t = tabsearch (u, uname, nuname)))
		return (t->name);
	sprintf (buf, "%d", u);
	return (buf);
}

char *groupname (int g)
{
	register struct table *t;
	static char buf [NAMLEN];

	if (! ngrname)
		initpw ();
	if ((t = tabsearch (g, grname, ngrname)))
		return (t->name);
	sprintf (buf, "%d", g);
	return (buf);
}
