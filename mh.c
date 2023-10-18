#include "deco.h"
#include "dir.h"
#include "scr.h"
#include "menu.h"

/* menu handler */

menufunc setfull, setdwid, reread, setstatus, directory;
menufunc genhelp, view, edit, quit, setpattern, makelink;
menufunc viewinfo, editinfo, setlang, redraw;
menufunc swappanels, switchpanels, shell, usermenu, cmpdir;
menufunc switchcmdreg, fullscreen, delete, findname;
menufunc copy, renmove, makedir, histmenu;
menufunc setshellname, setviewname, seteditname, writeinitfile;
menufunc switchalign, setsort, setrevsort, settypesort;
menufunc tagall, untagall, menuedit, extedit, switchhidden;

#ifdef S_IFLNK
menufunc makeslink;
#endif

static struct submenu lmenu[] = {
    // clang-format off
    { "Brief",                'b',  setfull,     },
    { "Long",                 'l',  setfull,     },
    { "Wide",                 'w',  setfull,     },
    { "Full",                 'f',  setfull,     },
    { "Info",                 'i',  setfull,     },
    { "Status           ^L",  's',  setstatus,   },
    { "Align extensions",     'a',  switchalign, },
    { "",                     0,    0,           },
    { "sort by Name",         'n',  setsort,     },
    { "eXtension",            'x',  setsort,     },
    { "Time",                 't',  setsort,     },
    { "siZe",                 'z',  setsort,     },
    { "Unsorted",             'u',  setsort,     },
    { "Reverse",              'r',  setrevsort,  },
    { "sort by tYpe",         'y',  settypesort, },
    { "",                     0,    0,           },
    { "Re-read          ^R",  'r',  reread,      },
    { "Pattern",              'p',  setpattern,  },
    { 0,                      0,    0,           },
    // clang-format on
};

static struct submenu rmenu[] = {
    // clang-format off
    { "Brief",                'b',  setfull,     },
    { "Long",                 'l',  setfull,     },
    { "Wide",                 'w',  setfull,     },
    { "Full",                 'f',  setfull,     },
    { "Info",                 'i',  setfull,     },
    { "Status           ^L",  's',  setstatus,   },
    { "Align extensions",     'a',  switchalign, },
    { "",                     0,    0,           },
    { "sort by Name",         'n',  setsort,     },
    { "eXtension",            'x',  setsort,     },
    { "Time",                 't',  setsort,     },
    { "siZe",                 'z',  setsort,     },
    { "Unsorted",             'u',  setsort,     },
    { "Reverse",              'r',  setrevsort,  },
    { "sort by tYpe",         'y',  settypesort, },
    { "",                     0,    0,           },
    { "Re-read          ^R",  'r',  reread,      },
    { "Pattern",              'p',  setpattern,  },
    { 0,                      0,    0,           },
    // clang-format on
};

static struct submenu fmenu[] = {
    // clang-format off
    { "Help             F1",  'h',  genhelp,     },
    { "User menu        F2",  'u',  usermenu,    },
    { "View             F3",  'v',  view,        },
    { "View Info",            'i',  viewinfo,    },
    { "Edit             F4",  'e',  edit,        },
    { "Edit Info",            'n',  editinfo,    },
    { "Copy             F5",  'c',  copy,        },
    { "Link",                 'l',  makelink,    },
#ifdef S_IFLNK
    { "Symlink",              's',  makeslink,   },
#endif
    { "Rename/move      F6",  'r',  renmove,     },
    { "Make directory   F7",  'm',  makedir,     },
    { "Delete           F8",  'd',  delete,      },
    { "",                     0,    0,           },
    { "select Group     +",   'g',  tagall,      },
    { "Unselect group   -",   'u',  untagall,    },
    { "",                     0,    0,           },
    { "Quit             F10", 'q',  quit,        },
    { 0,                      0,    0,           },
    // clang-format on
};

static struct submenu cmenu[] = {
    // clang-format off
    { "run subShell",             's',  shell,     },
    { "Compare directories",      'c',  cmpdir,    },
    { "Find file            ^K",  'f',  findname,  },
    { "hIstory              ^B",  'i',  histmenu,  },
    { "",                         0,    0,         },
    { "hOme directory       ^\\", 'o',  directory, },
    { "Root directory       ^^",  'r',  directory, },
    { "reDraw screen        ^]",  'd',  redraw,    },
    { "",                         0,    0,         },
    { "Menu file edit",           'm',  menuedit,  },
    { "eXtensions file edit",     'x',  extedit,   },
    { 0,                          0,    0,         },
    // clang-format on
};

static struct submenu omenu[] = {
    // clang-format off
    { "sWitch panels      ^I",  'w',  switchpanels,  },
    { "Swap panels        ^U",  's',  swappanels,    },
    { "Full screen        ^F",  'f',  fullscreen,    },
    { "Double width       ^W",  'd',  setdwid,       },
    { "Command line mode  ^P",  'c',  switchcmdreg,  },
    { "show hiddeN files",      'n',  switchhidden,  },
    { "",                       0,    0,             },
    { "Language...",            'l',  setlang,       },
    { "Viewer...",              'v',  setviewname,   },
    { "Editor...",              'e',  seteditname,   },
    { "sHell...",               'h',  setshellname,  },
    { "",                       0,    0,             },
    { "sAve setup",             'a',  writeinitfile, },
    { 0,                        0,    0,             },
    // clang-format on
};

struct menu menu[] = {
    // clang-format off
    { "Left",         'l',  lmenu, },
    { "Files",        'f',  fmenu, },
    { "Commands",     'c',  cmenu, },
    { "Options",      'o',  omenu, },
    { "Right",        'r',  rmenu, },
    { 0,              0,    0,     },
    // clang-format on
};

int nmenu;

static int tag(int k, int sk);

void runmenu(int mkey)
{
    register struct menu *m;
    register struct submenu *sm;
    int ret;

    nmenu = 0;
    for (m = menu; m->name; ++m) {
        if (m->key == mkey)
            nmenu = m - menu;
        for (sm = m->submenu; sm->name; ++sm)
            sm->active = sm->exec != 0;
        m->nsm = -1;
        downmenu(m);
    }
oncemore:
    for (m = menu; m->name; ++m) {
        for (sm = m->submenu; sm->name; ++sm) {
            sm->active = sm->exec != 0;
            sm->tag    = tag(m->key, sm->key);
        }
    }
    ret = getmenu();
    if (!ret)
        return;
    m  = &menu[nmenu];
    sm = &m->submenu[m->nsm];
    if (sm->exec)
        (*sm->exec)(m->key, sm->key);
    if (ret > 1)
        goto oncemore;
}

static int tag(int k, int sk)
{
    switch (k) {
    case 'l':
        switch (sk) {
        case 'b':
            return (left.view == VIEW_BRIEF);
        case 'w':
            return (left.view == VIEW_WIDE);
        case 'l':
            return (left.view == VIEW_LONG || (!widewin && left.view == VIEW_FULL));
        case 'f':
            return (widewin && left.view == VIEW_FULL);
        case 'i':
            return (widewin && left.view == VIEW_INFO);
        case 's':
            return (right.status);
        case 'p':
            return (left.pattern[0]);
        case 'n':
            return (left.sort == SORTNAME);
        case 'x':
            return (left.sort == SORTEXT);
        case 't':
            return (left.sort == SORTTIME);
        case 'z':
            return (left.sort == SORTSIZE);
        case 'u':
            return (left.sort == SORTSKIP);
        case 'a':
            return (left.alignext);
        case 'y':
            return (left.typesort);
        case 'r':
            return (left.revsort);
        }
        break;
    case 'r':
        switch (sk) {
        case 'b':
            return (right.view == VIEW_BRIEF);
        case 'w':
            return (right.view == VIEW_WIDE);
        case 'l':
            return (right.view == VIEW_LONG || (!widewin && right.view == VIEW_FULL));
        case 'f':
            return (widewin && right.view == VIEW_FULL);
        case 'i':
            return (widewin && right.view == VIEW_INFO);
        case 's':
            return (left.status);
        case 'p':
            return (right.pattern[0]);
        case 'n':
            return (right.sort == SORTNAME);
        case 'x':
            return (right.sort == SORTEXT);
        case 't':
            return (right.sort == SORTTIME);
        case 'z':
            return (right.sort == SORTSIZE);
        case 'u':
            return (right.sort == SORTSKIP);
        case 'a':
            return (right.alignext);
        case 'y':
            return (right.typesort);
        case 'r':
            return (right.revsort);
        }
        break;
    case 'o':
        switch (sk) {
        case 's':
            return (left.basecol != 0);
        case 'f':
            return (H == LINES - 7);
        case 'h':
            return (usecshell);
        case 'd':
            return (widewin);
        case 'c':
            return (cmdreg);
        case 'v':
            return (userview);
        case 'e':
            return (useredit);
        case 'n':
            return (showhidden);
        }
        break;
    }
    return (0);
}
