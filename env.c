/*
 *      EnvInit (char **env)
 *              Initialixe Env packadge by environment vector.
 *
 *      char *EnvGet (char *name)
 *              Get variable from environment.
 *
 *      EnvPut (char *name, char *value)
 *              Change value of variable.
 *
 *      char **EnvVector
 *              Current environment vector.
 */
#include <stdlib.h>
#include <string.h>

#include "deco.h"

#define QUANT 10

char **EnvVector;

static int nenv;
static int last;

static char *nvmatch(char *s1, char *s2);
static char *findvar(char *name, char ***ptr);

void EnvInit(char **env)
{
    register char **p;
    register i;

    if (EnvVector) {
        for (p = EnvVector; *p; ++p)
            free(*p);
        free(EnvVector);
        EnvVector = 0;
        nenv      = 0;
        last = nenv = 0;
    }
    if (!env)
        return;
    for (i = 0, p = env; *p; ++p, ++i) {
        mcheck(EnvVector, char **, nenv, QUANT, i);
        EnvVector[i] = malloc(strlen(*p) + 1);
        strcpy(EnvVector[i], *p);
    }
    mcheck(EnvVector, char **, nenv, QUANT, i);
    last            = i;
    EnvVector[last] = 0;
}

void EnvPut(char *name, char *value)
{
    register char *v;
    char **p;

    if (!name)
        return;
    if (!value)
        value = "";
    v = findvar(name, &p);
    if (v)
        free(*p);
    else {
        p = EnvVector + last;
        ++last;
        mcheck(EnvVector, char **, nenv, QUANT, last);
        EnvVector[last] = 0;
    }
    *p = malloc(strlen(name) + 1 + strlen(value) + 1);
    strcpy(*p, name);
    strcat(*p, "=");
    strcat(*p, value);
}

void EnvDelete(char *name)
{
    register char *v;
    char **p;

    if (!name)
        return;
    v = findvar(name, &p);
    if (!v)
        return;
    free(*p);
    for (++p; *p; ++p)
        *(p - 1) = *p;
    EnvVector[--last] = 0;
}

char *EnvGet(char *name)
{
    char **p;

    return (findvar(name, &p));
}

static char *findvar(char *name, char ***ptr)
{
    register char *v, **p;

    p = EnvVector;
    if (!p)
        return (0);
    for (; *p; ++p)
        if ((v = nvmatch(name, *p))) {
            *ptr = p;
            return (v);
        }
    return (0);
}

/*
 *	s1 is either name, or name=value
 *	s2 is name=value
 *	if names match, return value of s2, else NULL
 *	used for environment searching: see getenv
 */
static char *nvmatch(char *s1, char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == '=')
            return (s2);
    if (*s1 == '\0' && *(s2 - 1) == '=')
        return (s2);
    return (0);
}
