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
extern int widewin;            /* wide window mode */
extern int H;                  /* file window height */
extern int cmdreg;             /* command line mode */
extern char command[];         /* command line */
extern int cpos;               /* command line cursor */
extern char *user;             /* user name */
extern char *group;            /* user group name */
extern char *tty;              /* tty name */
extern char *machine;          /* machine name */
extern int visualwin;          /* cursor in command line */
extern int showhidden;         /* show hidden files ".*" */
extern int uid;                /* real user id */
extern int main_pid;           /* our process id */
extern int ppid;               /* parent id */
extern int lang;               /* language */
extern char editname[40], viewname[40];
extern int useredit, userview, usecshell;
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
char *getmachine(void);
const char *extension(const char *p);
char *ltoac(long l);
char *basename(char *p);
char *strtail(char *p, int delim, int maxlen);
int strbcmp(char *s, char *b);
int match(char *name, char *pat);
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
void runcancel(void);
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
void switchcmdreg(void);
void switchpanels(void);
void setstatus(void);
void reread(int k, int sk);
void setdwid(void);
void view(void);
void edit(void);
void menuedit(void);
void extedit(void);
void quit(void);
void swappanels(void);
void copy(void);
void renmove(void);
void makedir(void);
void delete(void);
void findname(void);
void tagall(void);
void untagall(void);
void drawcmd(void);
void namecmd(void);
void delcmd(void);
void homecmd(void);
void endcmd(void);
void leftcmd(void);
void upcmd(void);
void downcmd(void);
void nextcmd(void);
void prevcmd(void);
void histmenu(void);
void drawdir(struct dir *c, int statusflag);
void drawcursor(void);
void undrawcursor(void);
void excommand(char *cp, char *fname);
void usermenu(void);
void readinitfile(void);
void genhelp(void);
int hexview(char *filename);
void runmenu(int mkey);
void jobcmd(int pid, char *cmd);
void sigchild(int sig);
int mygroup(int gid);
