#define _POSIX_C_SOURCE 1L
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "deco.h"
#include "env.h"

#define QUANT 16

//typedef int (*funcptr)();

static char **arg;
static int arglen, argp;

static int run(char *name, char **ap, int (*exe)(), int silent);

void runset(char *dest)
{
    arg    = (char **)malloc(QUANT * sizeof(char *));
    arglen = QUANT;
    arg[0] = dest;
    argp   = 1;
}

void runarg(char *name)
{
    mcheck(arg, char **, arglen, QUANT, argp);
    arg[argp++] = name;
}

int rundone(char *name, char *a0, char *a1, char *a2)
{
    register char **p, **q, *a;
    int ret;

    if (a2)
        runarg(a2);
    if (a1)
        runarg(a1);
    runarg(a0);
    for (p = arg, q = arg + argp - 1; p < q; ++p, --q) {
        a  = *p;
        *p = *q;
        *q = a;
    }
    runarg(0);
    ret = runv(1, name, arg) == 0;
    free(arg);
    return (ret);
}

void runcancel()
{
    free(arg);
}

static char *execat(char *s1, char *s2, char *si)
{
    register char *s;

    s = si;
    while (*s1 && *s1 != ':')
        *s++ = *s1++;
    if (si != s)
        *s++ = '/';
    while (*s2)
        *s++ = *s2++;
    *s = 0;
    return (*s1 ? ++s1 : 0);
}

static int execvpe(char *name, char **argv, char **envstr)
{
    static char *pathstr;
    char fname[128];
    char *newargs[256];
    int i;
    register char *cp;
    register unsigned etxtbsy = 1;
    register int eacces       = 0;

    if (!pathstr && !(pathstr = EnvGet("PATH")))
        pathstr = ":/bin:/usr/bin";
    for (cp = name;; ++cp) { /* check if name contains '/' */
        if (!*cp) {
            cp = pathstr; /* if no, set cp to pathstr */
            break;
        }
        if (*cp == '/') {
            cp = ""; /* else path is empty */
            break;
        }
    }
    do {
        cp = execat(cp, name, fname);
    retry:
        (void)execve(fname, argv, envstr);
        switch (errno) {
        case ENOEXEC:
            newargs[0] = "sh";
            newargs[1] = fname;
            for (i = 1; (newargs[i + 1] = argv[i]); ++i) {
                if (i >= 254) {
                    errno = E2BIG;
                    return (-1);
                }
            }
            (void)execve("/bin/sh", newargs, envstr);
            return (-1);
        case ETXTBSY:
            if (++etxtbsy > 5)
                return (-1);
            (void)sleep(etxtbsy);
            goto retry;
        case EACCES:
            ++eacces;
            break;
        case ENOMEM:
        case E2BIG:
            return (-1);
        }
    } while (cp);
    if (eacces)
        errno = EACCES;
    return (-1);
}

int runl(int silent, char *name, ...)
{
    va_list ap;
    int err;

    va_start(ap, name);
    err = run(name, (char**)ap, execve, silent);
    va_end(ap);
    return err;
}

int runv(int silent, char *name, char **a0)
{
    return (run(name, a0, execvpe, silent));
}

RETSIGTYPE sigchild(int sig)
{
    int status = 0;

    signal(SIGCHLD, sigchild);
    waitpid(-1, &status, WNOHANG | WUNTRACED);
}

void sigign()
{
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGCHLD, sigchild);
    signal(SIGTTOU, SIG_IGN);
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif
}

void sigdfl()
{
    signal(SIGTERM, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_DFL);
#endif
}

static int run(char *name, char **a0, int (*exe)(char*, char**, char**), int silent)
{
    register int t;
    int status = 0;
    static char **arg;

    arg = a0;
    signal(SIGCHLD, SIG_IGN);
    t = vfork();
    if (t == -1)
        /* cannot fork */
        return (0x7e00);
    if (t == 0) {
        /* Child. */
        sigdfl();
        if (silent) {
            close(0);
            close(1);
            close(2);
            open("/dev/null", 0);
            open("/dev/null", 1);
            dup(1);
        } else {
            /* Set for child the separate process group.
             * Give the terminal to the child. */
            int child_pid = getpid();
            setpgid(child_pid, child_pid);
            tcsetpgrp(2, child_pid);
        }
        signal(SIGTTOU, SIG_DFL);
        (*exe)(name, arg, EnvVector);
        if (!silent)
            outerr("%s: Command not found.\n", name);
        _exit(0x7f); /* file not found */
    }

    /* Parent. */
    waitpid(t, &status, WUNTRACED);

    /* Get back the terminal. */
    if (!silent) {
        tcsetpgrp(2, main_pid);
        killpg(t, SIGCONT);
    }
    signal(SIGCHLD, sigchild);

    if (WIFSTOPPED(status))
        return -t;
    return (status);
}
