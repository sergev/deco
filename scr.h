#ifndef cntrl
#define cntrl(c) (c & 037)
#endif
#ifndef meta
#define meta(c) (c | 0400)
#endif

typedef struct {
    short cury, curx;
    short flgs;
    char clear;
    short **y;
    short *firstch;
    short *lastch;
    short *lnum;
} WINDOW;

typedef struct {
    short y, x;
    short ny, nx;
    short *mem;
} BOX;

typedef long CURSOR;

extern int LINES, COLS;
extern WINDOW VScreen;

#define VMPutChar(y, x, ch)    (VMove(y, x), VPutChar(ch))
#define VMPutString(y, x, str) (VMove(y, x), VPutString(str))
#define VMGetChar(yy, x)       (VMove(yy, x), (VScreen.y[VScreen.cury][VScreen.curx] & 0377))

int VInit(void);
void VOpen(void);
void VReopen(void);
void VClose(void);
void VRestore(void);
void VRedraw(void);
void VSyncLine(int wy);
void VSync(void);
void VInsLine(int n);
void VDelLine(int n);
void VMove(int y, int x);
void VClearLine(void);
void VClear(void);
void VPutString(char *str);
void VPrint(char *fmt, ...);
void VSetPalette(int n, int nb, int nr, int nrb, int b, int bb, int br, int brb, int d, int db,
                 int dr, int drb);
int VStandOut(void);
void VStandEnd(void);
void VSetNormal(void);
void VSetPrev(void);
int VSetDim(void);
int VSetBold(void);
void VPutChar(int c);
void VFlush(void);
void VBeep(void);
BOX *VGetBox(int y, int x, int ny, int nx);
CURSOR VGetCursor(void);
void VSetCursor(CURSOR c);
void VUngetBox(BOX *box);
void VPrintBox(BOX *box);
void VFreeBox(BOX *box);
void VClearBox(int r, int c, int nr, int nc);
void VFillBox(int r, int c, int nr, int nc, int sym);
void VHorLine(int r, int c, int nc);
void VVertLine(int c, int r, int nr);
void VCorner(int r, int c, int n);
void VDrawBox(int r, int c, int nr, int nc);
void VExpandString(char *s, char *d);
void _prscreen(void);

struct KeyMap {
    char *tcap;
    char *str;
    short val;
};

void KeyInit(struct KeyMap *map, void (*flushproc)(void));
int KeyGetChar(void);
int KeyGet(void);
void KeyUnget(int key);

#define CAPNUM 1
#define CAPFLG 2
#define CAPSTR 3

struct CapTab {
    char tname[3];
    char ttype;
    char tdef;
    char *tc;
    int *ti;
    char **ts;
};

int CapInit(char *bp);
void CapGet(struct CapTab *t);
char *CapGoto(char *CM, int destcol, int destline);

void TtySet(void);
void TtyReset(void);
void TtyFlushInput(void);
