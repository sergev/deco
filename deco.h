#undef GC /* use Boehm memory allocator */

#if !HAVE_TCSETPGRP
#define tcsetpgrp(fd, pgrp) ioctl(fd, TIOCSPGRP, &pgrp)
#endif

#if !HAVE_KILLPG
#define killpg(pid, sig) kill(-pid, sig)
#endif

/* width of directory window */

#define PAGEWID (widewin ? 77 : 39)

/* number of displayable files in directory window */

#define PAGELEN(c) pagelen((c)->view)

#define BASECOL(c) (widewin ? 0 : (c)->basecol)

#define CMDLEN 512 /* length of command string */

struct dir;
struct file;
struct menu;

extern struct dir left, right; /* left and right directories */
extern struct dir *cur;        /* current directory */
extern widewin;                /* wide window mode */
extern H;                      /* file window height */
extern cmdreg;                 /* command line mode */
extern char command[];         /* command line */
extern cpos;                   /* command line cursor */
extern char *user;             /* user name */
extern char *group;            /* user group name */
extern char *tty;              /* tty name */
extern char *machine;          /* machine name */
extern visualwin;              /* cursor in command line */
extern showhidden;             /* show hidden files ".*" */
extern uid;                    /* real user id */
extern main_pid;               /* our process id */
extern ppid;                   /* parent id */
extern lang;                   /* language */
extern char editname[40], viewname[40];
extern useredit, userview, usecshell;
extern int viewraw, viewhex, viewtabs;
extern char *home;
extern int errno;
extern struct palette palette, dflt_palette;

#define ENG 0 /* English */
#define RUS 1 /* Russian */
#define UKR 2 /* Ukranian */
#define DEU 3 /* German */
#define FRA 4 /* France */

#define VIEW_BRIEF 0
#define VIEW_LONG  1
#define VIEW_WIDE  2
#define VIEW_FULL  3
#define VIEW_INFO  4

struct palette {
    int fg, bg;
    int revfg, revbg;
    int boldfg, boldbg;
    int boldrevfg, boldrevbg;
    int dimfg, dimbg;
    int dimrevfg, dimrevbg;
};

/*
 * mcheck (type& array, type, int& bound, int quant, int index)
 *      Check index of dynamic array.
 *      Enlarge array if needed.
 */
#define mcheck(array, type, bound, quant, index) \
    if ((index) >= (bound))                      \
    (array) = (type)realloc(array, (int)((bound) += (quant)) * (int)sizeof(*(array)))

#if !HAVE_MEMCPY
#define memcpy(t, f, n) bcopy(f, t, n)
#define memcmp(a, b, n) bcmp(a, b, n)
#endif

#ifdef GC
#include <gc.h>
#define malloc(n)     GC_malloc(n)
#define calloc(n, m)  GC_malloc((n) * (m))
#define realloc(a, n) ((a) ? GC_realloc(a, n) : GC_malloc(n))
#define free(a) \
    if (a)      \
    GC_free(a)
#endif

void error(char *s, ...);
void message(char *name, char *s, ...);
int setdir(struct dir *d, char *dirname);
void hidecursor(void);
int getchoice(int bold, char *head, char *msg, char *mesg2, char *c1, char *c2, char *c3);
char *username(int u);
char *groupname(int g);
void sigign(void);
void sigdfl(void);
void outerr(char *msg, ...);
int runv(int silent, char *name, char **a0);
char *getmachine();
const char *extension(const char *p);
char *ltoac(long l);
char *basename(char *p);
char *strtail(char *p, int delim, int maxlen);
int strbcmp(char *s, char *b);
int match(unsigned char *name, unsigned char *pat);
int getmenu(void);
void downmenu(struct menu *m);
char *timestr(long tim);
int hexview(char *filename);
void viewpage(int line, int col);
void viewchar(int c);
void viewline(int line, int col, int scrline);
int offset(int l, int n);
int column(int l, int n);
int search(char *str, int l, int c, int *pline, int *pcol);
void runhelp(char *name);
char *getstring(int w, char *str, char *head, char *mesg);
void endmesg(void);
char *mdup(char *s);
int pagelen(int f);
char *getwstring(int w, char *str, char *head, char *mesg);
void syscmd(char *s);
void viewfile(int d, char *filename);
void editfile(int d, char *filename, char *basename);
void quitdeco(void);
int doexec(char *str);
void runset(char *dest);
void runarg(char *name);
int rundone(char *name, char *a0, char *a1, char *a2);
void runcancel();
int runl(int silent, char *name, ...);
void findfile(struct dir *d, char *name);
int compfile(const void *arg1, const void *arg2);
int exist(char *name);
void counttag(struct dir *d);
void rightcmd(void);
void mvcaddstr(int r, int c, char *s);
void inscmd(int key);
int quote(void);
void fullscreen(int k, int sk);
int tagged(void);
void draw(void);
void drawbanners(void);
int filetype(struct file *f);
char *filemode(int m);
void setinfo(struct file *f, struct dir *d);
char *filesize(struct file *f);
char *filedate(long d);
void execmd(int hit, int savhist);
void directory(int k, int sk);
void switchcmdreg();
void switchpanels();
void setstatus();
void reread(int k, int sk);
void setdwid();
void view();
void edit();
void menuedit();
void extedit();
void quit();
void swappanels();
void copy();
void renmove();
void makedir();
void delete();
void findname();
void tagall();
void untagall();
void drawcmd();
void namecmd();
void delcmd();
void homecmd();
void endcmd();
void leftcmd();
void upcmd();
void downcmd();
void nextcmd();
void prevcmd();
void histmenu();
void drawdir(struct dir *c, int statusflag);
void drawcursor();
void undrawcursor();
void excommand(char *cp, char *fname);
void usermenu();
void readinitfile();
void genhelp();
int hexview(char *filename);
void runmenu(int mkey);
void jobcmd(int pid, char *cmd);
RETSIGTYPE sigchild(int sig);
int mygroup(int gid);
