#include "strip.h"
#include "bname.h"

void remove_suffix(register char *name, register char *ssuffix);
/**
 * install core version
 */
int main(int argc, char **argv)
{
    char *name;

    if(argc == 1 || argc > 3)
    {
        fprintf(stderr, "Usage: %s name [suffix]\n", argv[0]);
        return -1;
    }

    strip_trailing_slashes(argv[1]);

    name = basename(argv[1]);

    if(argc == 3)
    {
        remove_suffix(name, argv[2]);
    }

    puts(name);

    return 0;
}

void remove_suffix(register char *name, register char *ssuffix)
{
    register char *np, *sp;
    np = name +strlen(name);
    sp = ssuffix + strlen(ssuffix);

    while(np > name && sp > ssuffix)
        if(--*np != --*sp)
            return ;
    if(np > name)
        *np = '\0';
}
