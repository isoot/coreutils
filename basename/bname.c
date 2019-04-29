#include "bname.h"

/**
 * /usr/bin/
 */
char *basename(char *name)
{
    char *base;
    base  = rindex(name, '/');
    return base ? base + 1 : name;
}
