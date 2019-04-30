#include "modechange.h"
#include "filemode.h"
#include "savedir.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
void usage();
int change_dir_mode(char *dir, struct mode_change *changes, struct stat *statp);
int change_file_mode(char *file, struct mode_change *changes);
void describe_change(char *file, unsigned short mode, int changed);

//程序运行名称
char *program_name;

//如果非零 递归更改目录所有权
static int recurse;

//是否静默输出
static int force_silent;

//描述处理文件信息
static int verbose;

//描述变化模式
static int changes_only;

int main(int argc, char **argv)
{
    struct mode_change *changes;
    int errors = 0;
    int modeind = 0;
    int thisind;
    int c;

    program_name = argv[0];
    recurse = force_silent = verbose = changes_only = 0;

    while(1)
    {
        thisind = optind ? optind : 1;
        c = getopt(argc, argv, "RcfvrwxXstugoa,+-=");
        if(c == EOF)
            break;
        switch(c)
        {
            case 'r':
            case 'w':
            case 'x':
            case 'X':
            case 's':
            case 't':
            case 'u':
            case 'g':
            case 'o':
            case 'a':
            case ',':
            case '+':
            case '-':
            case '=':
                if(modeind != 0 && modeind != thisind)
                    fprintf(stderr, "invalid mode");
                modeind = thisind;
                break;
            case 'R':
                recurse = 1;
                break;
            case 'c':
                verbose = 1;
                changes_only = 1;
                break;
            case 'f':
                force_silent = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                usage();
        }
    }

    if(modeind == 0)
        modeind = optind++;
    if(optind >= argc)
        usage();
    changes = mode_compile(argv[modeind],
            MODE_MASK_EQUALS | MODE_MASK_PLUS | MODE_MASK_MINUS);
    if(changes == MODE_INVALID)
        fprintf(stderr, "invalid mode");
    else if(changes == MODE_MEMORY_EXHAUSTED)
        fprintf(stderr, "virtual memory exhausted");

    for(; optind < argc; ++optind)
        errors |= change_file_mode(argv[optind], changes);
    exit(errors);
}

int change_file_mode(char *file, struct mode_change *changes)
{
    struct stat file_stats;
    unsigned short newmode;
    int errors = 0;
    if(lstat(file, &file_stats))
    {
        if(force_silent == 0)
        fprintf(stderr, "%s,%s", file, strerror(errno));
        return 1;
    }

#ifdef S_ISLNK
    if(S_ISLNK(file_stats.st_mode))
        return 0;
#endif

    newmode = mode_adjust(file_stats.st_mode, changes);

    if(newmode != (file_stats.st_mode & 077777))
    {
        if(verbose)
            describe_change(file, newmode, 1);
        if(chmod(file, (int)newmode))
        {
            if(force_silent == 0)
                fprintf(stderr, "%s, %s", file, strerror(errno));
            errors = 1;
        }
    }else if(verbose && changes_only == 0)
        describe_change(file, newmode, 0);

    if(recurse && S_ISDIR(file_stats.st_mode))
        errors |= change_dir_mode(file, changes, &file_stats);
    return errors;
}

int change_dir_mode(char *dir, struct mode_change *changes, struct stat *statp)
{
    char *name_space, *namep;
    char *path;
    unsigned int dirlength;
    unsigned int filelength;
    unsigned int pathlength;
    int errors = 0;
    errno = 0;

    name_space = savedir(dir, statp->st_size);
    if(name_space == NULL){
        if(errno){
            if(force_silent == 0)
                fprintf(stderr, "%s,%s", dir, strerror(errno));
            return 1;
        }else
            fprintf(stderr, "virtual memory exhausted");
    }

    dirlength = strlen(dir) +1;
    pathlength = dirlength + 1;
    path = malloc(pathlength);
    strcpy(path, dir);
    path[dirlength - 1] = '/';
    for(namep = name_space; *namep; namep += filelength - dirlength)
    {
        filelength = dirlength + strlen(namep) + 1;
        if(filelength > pathlength)
        {
            pathlength = filelength * 2;
            path = realloc(path, pathlength);
        }
        strcpy(path + dirlength, namep);
        errors |= change_file_mode(path, changes);
    }
    free(path);
    free(name_space);
    return errors;
}

void describe_change(char *file, unsigned short mode, int changed)
{
    char perms[11];
    mode_string(mode, perms);
    perms[10] = '\0';
    if(changed)
        printf("mode of %s changed to %04o (%s)\n",
                file, mode & 07777, &perms[1]);
    else
        printf("mode of %s retained as %04o (%s)\n",
                file, mode & 07777, &perms[1]);
}

void usage()
{
    fprintf(stderr, "\
            Usage: %s [-Refv] mode file...\n\
            mode is [ugoa...] [[+-=] [rwxXstugo...]...] [,...] or octal number\n",
            program_name);
    exit(1);
}
