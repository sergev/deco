#include <string.h>

#include "deco.h"
#include "scr.h"
#include "menu.h"

static void drawhead(int nm);
static void clrmenu(struct menu *m);
static void drawmenu(struct menu *m);
static void upmenu(struct menu *m);
static int hotkey(int c, struct menu *m);

static void initmenu()
{
    register struct menu *m;
    register struct submenu *sm;
    int col, w;

    col = 3;
    for (m = menu; m->name; ++m) {
        m->len = 4 + strlen(m->name);
        m->col = col;
        col += m->len;
        m->hgh = 0;
        m->wid = 0;
        for (sm = m->submenu; sm->name; ++sm) {
            sm->row = m->hgh;
            w       = 4 + strlen(sm->name);
            if (w > m->wid)
                m->wid = w;
            ++m->hgh;
        }
    }
}

static void getboxes()
{
    register struct menu *m;

    for (m = menu; m->name; ++m) {
        if (m->box)
            VFreeBox(m->box);
        m->box = VGetBox(1, m->col - 1, m->hgh + 3, m->wid + 4);
    }
}

int getmenu()
{
    if (!menu[0].len)
        initmenu();

    getboxes();
    VMPutString(LINES - 1, 0,
                "\0011\16      \17 2\16      \17 3\16      \17 4\16      \17 5\16      \17 6\16    "
                "  \17 7\16      \17 8\16      \17 9\16      \01710\16Quit \17\2");
    for (;;) {
        drawhead(nmenu);
        for (;;) {
            int c, k;

            drawmenu(&menu[nmenu]);
            hidecursor();
            VSync();
            c = KeyGet();
            switch (c) {
            default:
                k = hotkey(c, &menu[nmenu]);
                if (k) {
                    clrmenu(&menu[nmenu]);
                    return (k);
                }
                VBeep();
                continue;
            case cntrl(']'): /* redraw screen */
                VRedraw();
                continue;
            case cntrl('M'):
                clrmenu(&menu[nmenu]);
                return (1);
            case cntrl('J'):
                clrmenu(&menu[nmenu]);
                return (2);
            case cntrl('C'):
            case cntrl('['):
            case meta('J'): /* f0 */
                clrmenu(&menu[nmenu]);
                return (0);
            case meta('r'): /* right */
                clrmenu(&menu[nmenu]);
                if (!menu[++nmenu].name)
                    nmenu = 0;
                break;
            case meta('l'): /* left */
                clrmenu(&menu[nmenu]);
                if (--nmenu < 0) {
                    for (nmenu = 0; menu[nmenu].name; ++nmenu)
                        ;
                    --nmenu;
                }
                break;
            case meta('u'): /* up */
                upmenu(&menu[nmenu]);
                continue;
            case meta('d'): /* down */
                downmenu(&menu[nmenu]);
                continue;
            }
            break;
        }
    }
}

static void drawhead(int nm)
{
    register int i, reverse;

    VSetDim();
    reverse = VStandOut();
    VMove(0, 0);
    for (i = 0; i < 80; ++i)
        VPutChar(' ');
    VMove(0, 2);
    for (i = 0; menu[i].name; ++i) {
        if (i == nm) {
            VStandEnd();
            VSetBold();
        }
        VPutChar(' ');
        VPutChar(i == nm && !reverse ? '[' : ' ');
        VPutString(menu[i].name);
        VPutChar(i == nm && !reverse ? ']' : ' ');
        VPutChar(' ');
        if (i == nm) {
            VStandOut();
            VSetDim();
        }
    }
    VSetNormal();
    VStandEnd();
}

static void clrmenu(struct menu *m)
{
    VUngetBox(m->box);
}

static void drawmenu(struct menu *m)
{
    register int i, reverse;

    VClearBox(1, m->col - 1, m->hgh + 3, m->wid + 4);
    VSetBold();
    VDrawBox(1, m->col, m->hgh + 2, m->wid + 2);
    VSetDim();
    for (i = 0; m->submenu[i].name; ++i)
        if (!m->submenu[i].name[0])
            VHorLine(i + 2, m->col + 2, m->wid - 2);
    VSetNormal();
    for (i = 0; m->submenu[i].name; ++i) {
        if (!m->submenu[i].name[0])
            continue;
        reverse = 0;
        if (i == m->nsm)
            reverse = VStandOut();
        else if (!m->submenu[i].active)
            VSetDim();
        VMove(2 + i, m->col + 1);
        VPutChar(i == m->nsm && !reverse ? '[' : ' ');
        VPrint("%c %-*s", m->submenu[i].tag ? '*' : ' ', m->wid - 4, m->submenu[i].name);
        VPutChar(i == m->nsm && !reverse ? ']' : ' ');
        if (i == m->nsm)
            VStandEnd();
        else if (!m->submenu[i].active)
            VSetNormal();
    }
}

static void upmenu(struct menu *m)
{
    for (;;) {
        if (--m->nsm < 0) {
            for (m->nsm = 0; m->submenu[m->nsm].name; ++m->nsm)
                ;
            --m->nsm;
        }
        if (m->submenu[m->nsm].active)
            break;
    }
}

void downmenu(struct menu *m)
{
    for (;;) {
        if (!m->submenu[++m->nsm].name)
            m->nsm = 0;
        if (m->submenu[m->nsm].active)
            break;
    }
}

static int hotkey(int c, struct menu *m)
{
    register int nsm, ret;

    if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';
        ret = 2;
    } else
        ret = 1;
    for (nsm = 0; m->submenu[nsm].name; ++nsm)
        if (c == m->submenu[nsm].key && m->submenu[nsm].active) {
            m->nsm = nsm;
            return (ret);
        }
    return (0);
}
