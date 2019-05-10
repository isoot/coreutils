#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
static int isnumber(char *str);

char *parse_user_spec(char *name, uid_t *uid, gid_t *gid, char **username, char **groupname)
{
    static char *tired = "virtual memeory exhausted";
    struct passwd *pwd;
    struct group *grp;
    char *cp;
    int use_login_group = 0;

    *username = *groupname = NULL;

    cp = index(name, ':');
    if(cp == NULL)
        cp = index(name, '.');
    if(cp != NULL)
    {
        *cp++ = '\0';
        if(*cp == '\0')
        {
            if(cp == name + 1)
                return "cant not omit both user and group";
            else
                use_login_group = 1;
        }else {
            *groupname = strdup(cp);
            if(*groupname == NULL)
                return tired;
            grp = getgrnam(cp);
            if(grp == NULL){
                if(!isnumber(cp))
                    return "invalid group";
                *gid = atoi(cp);
            }else
                *gid = grp->gr_gid;
            endgrent();
        }
    }

    if(name[0] == '\0')
        return NULL;

    *username = strdup(name);
    if(*username == NULL)
        return tired;

    pwd = getpwnam(name);
    if(pwd == NULL){
        if(!isnumber(name))
        {
            return "invalid user";
        }
        if(use_login_group)
            return "cannot get the login group of a numeric UID";
        *uid = atoi(name);
    }else
    {
        *uid = pwd->pw_uid;
        if(use_login_group)
        {
            *gid = pwd->pw_gid;
            grp = getgrgid(pwd->pw_gid);
            if(grp == NULL)
            {
                *groupname = malloc(15);
                if(*groupname == NULL)
                    return tired;
                sprintf(*groupname, "%u", pwd->pw_gid);
            }else {
                *groupname = strdup(grp->gr_name);
                if(*groupname == NULL)
                    return tired;
            }
            endgrent();
        }
    }

    endpwent();
    return NULL;
}

static int isnumber(char *str)
{
    for(; *str; str++)
    {
        if(!ISDIGIT(*str))
            return 0;
    }

    return 1;
}
