#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <grp.h>
#include <ctype.h>
#include <sys/dir.h>
#include <dirent.h>

#define NLENGTH(direct) ((direct)->d_namlen)
#define CLOSEDIR(dir) (closedir(dir))

//程序运行是的名称
char *program_name;

//递归更改目录的所有权
static int recurese;

//不做任何输出
static int force_silent;

//描述处理文件信息
static int verbose;

//描述处理组信息
static int changes_only;

static char *groupname;

static struct option long_options[] =
{
  {"recursive", 0, 0, 'R'},
  {"changes", 0, 0, 'c'},
  {"silent", 0, 0, 'f'},
  {"quiet", 0, 0, 'f'},
  {"verbose", 0, 0, 'v'},
  {0, 0, 0, 0}
};


void parse_group(char *name, int *g);
int change_file_group(char *file, int group);
int change_dir_group(char *dir, int group, struct stat *statp);
void describe_change(char *file, int changed);
int myisnumber(char *str);
char *savedir(char *dir, unsigned int name_size);
void usage();
int main(int argc, char **argv)
{
    int group;
    int errors = 0;
    int optc;

    //赋值程序运行时名称
    program_name = argv[0];
    //初始化参数
    recurese = force_silent = verbose = changes_only = 0;
    while((optc = getopt_long(argc, argv, "Rcfv", long_options, (int *)0)) != EOF)
    {
        switch(optc)
        {
            case 'R':
                //递归更改目录的所有权
                recurese = 1;
                break;
            case 'c':
                //描述处理的文件
                verbose = 1;
                //描述处理的组信息
                changes_only = 1;
                break;
            case 'f':
                //不反会任何消息
                force_silent = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                usage();
        }
    }

    //如果optind 比需要处理的下标数大 则错误
    if(optind >= argc - 1)
    {
        usage();
    }


    fprintf(stderr, "optind value is = %s\n", argv[optind]);
    //开始解析参数
    parse_group(argv[optind++], &group);

    for(; optind < argc; ++optind)
        errors |= change_file_group(argv[optind], group);
    exit(errors);
}

void parse_group(char *name, int *g)
{
    struct group *grp;
    groupname = name;

    if(*name == '\0')
        fprintf(stderr,"can not change to null group");
    grp = getgrnam(name);
    if(grp == NULL){
        if(!myisnumber(name))
            fprintf(stderr, "invalid group %s", name);
        *g = atoi(name);
    }else{
        *g = grp->gr_gid;
    }
    endgrent();
}

int change_file_group(char *file, int group)
{
    struct stat file_stats;
    int errors = 0;

    if(lstat(file, &file_stats))
    {
        if(force_silent == 0)
        {
            fprintf(stderr, "%s,%s", file, strerror(errno));
            return 1;
        }
    }

    if(group != file_stats.st_gid)
    {
        if(verbose)
            describe_change(file, 1);
        if(chown(file, file_stats.st_uid, group))
        {
            if(force_silent == 0)
                fprintf(stderr, "%s,%s", file, strerror(errno));
            return 1;
        }
    }else if(verbose && changes_only == 0)
        describe_change(file, 0);

    if(recurese && S_ISDIR(file_stats.st_mode))
        errors |= change_dir_group(file, group, &file_stats);
    return errors;
}

int change_dir_group(char *dir, int group, struct stat *statp)
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
        if(errno)
        {
            if(force_silent == 0)
                fprintf(stderr, "%s,%s", dir, strerror(errno));
            return 1;
        }else
            fprintf(stderr, "virtual memory exhausted");
    }

    dirlength = strlen(dir) + 1;
    pathlength = dirlength + 1;
    path = malloc(pathlength);
    strcpy(path, dir);
    path[dirlength - 1] = '/';

    for(namep = name_space; *namep; namep += filelength - dirlength){
        filelength = dirlength + strlen(namep) + 1;
        if(filelength > pathlength)
        {
            pathlength = filelength * 2;
            path = realloc(path, pathlength);
        }

        strcpy(path + dirlength, namep);
        errors |= change_file_group(path, group);
    }

    free(path);
    free(name_space);
    return errors;
}

void describe_change(char *file, int changed)
{
    if(changed)
        printf("group of %s changed to %s\n", file, groupname);
    else
        printf("group of %s retained as %s\n", file, groupname);
}

int myisnumber(char *str)
{
    for(; *str; str++)
        if(!isdigit(*str))
            return 0;
    return 1;
}
void usage()
{
    fprintf(stderr,  "\
            Usage: %s [-Rcfv] [--recursive] [--changes] [--silent] [--quit]\n\
            [--verbose] group file...\n", program_name);
    exit(-1);
}

char *savedir(char *dir, unsigned int name_size)
{
    DIR *dirp;
    struct direct *dp;
    char *name_space;
    char *namep;

    dirp = opendir(dir);
    if(dirp == NULL)
    return NULL;

    name_space = (char *)malloc(name_size);
    if(name_space == NULL)
    {
        closedir(dirp);
        return NULL;
    }

    namep = name_space;

    while((dp = readdir(dirp)) != NULL)
    {
        if(dp->d_name[0] != '.'
            || (dp->d_name[1] != '\0'
                && (dp->d_name[1] != '.'
                    || dp->d_name[2] != '\0')))
        {
            unsigned int size_needed = (namep - name_space) + NLENGTH(dp) + 2;
            if(size_needed > name_size)
            {
                char *new_name_space;
                while(size_needed > name_size)
                    name_size += 1024;
                new_name_space = realloc(name_space, name_size);
                if(new_name_space == NULL)
                {
                    closedir(dirp);
                    return NULL;
                }

                namep += new_name_space - name_space;
                name_space = new_name_space;
            }
            namep = strcpy(namep, dp->d_name) + 1;
        }
    }

    *namep = '\0';
    if(CLOSEDIR(dirp))
    {
        free(name_space);
        return NULL;
    }

    return name_space;
}
