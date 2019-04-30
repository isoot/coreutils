#include "filemode.h"

static void setst(unsigned short bits, char *chars);
static void rwx(unsigned short bits, char *chars);
static char ftypelet(mode_t bits);
void filemodestring(struct stat *statp, char *str)
{
    mode_string(statp->st_mode, str);
}

void mode_string(unsigned short mode, char *str)
{
    str[0] = ftypelet(mode);
    rwx((mode & 0700) << 0, &str[1]);
    rwx((mode & 0070) << 3, &str[4]);
    rwx((mode & 0007) << 6, &str[7]);
    setst(mode, str);
}

static char ftypelet(mode_t bits)
{
    if(S_ISBLK(bits))
        return 'b';
    if(S_ISCHR(bits))
        return 'c';
    if(S_ISDIR(bits))
        return 'd';
    if(S_ISREG(bits))
        return '-';
    if(S_ISFIFO(bits))
        return 'p';
    if(S_ISLNK(bits))
        return 'l';
    if(S_ISSOCK(bits))
        return 's';
    return '?';
}

static void rwx(unsigned short bits, char *chars)
{
    chars[0] = (bits & S_IREAD) ? 'r' : '-';
    chars[1] = (bits & S_IWRITE) ? 'w' : '-';
    chars[2] = (bits & S_IEXEC) ? 'x' : '-';
}

static void setst(unsigned short bits, char *chars)
{
    if (bits & S_ISUID)
    {
        if (chars[3] != 'x')
            /* Set-uid, but not executable by owner.  */
            chars[3] = 'S';
        else
            chars[3] = 's';
    }
    if (bits & S_ISGID)
    {
        if (chars[6] != 'x')
            /* Set-gid, but not executable by group.  */
            chars[6] = 'S';
        else
            chars[6] = 's';
    }
    if (bits & S_ISVTX)
    {
        if (chars[9] != 'x')
            /* Sticky, but not executable by others.  */
            chars[9] = 'T';
        else
            chars[9] = 't';
    }
}
