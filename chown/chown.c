#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <getopt.h>
#include <sys/stat.h>
char *program_name;

static int recurse;

static int force_silent;

static int verbose;

static int changes_only;

static char *username;

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

void usage();
int change_file_owner(char *file, uid_t user, gid_t group);
int change_dir_owner(char *dir, uid_t user, gid_t group, struct stat *statp);
void describe_change(char *file, int changed);
char *parse_user_spec(char *name, uid_t *uid, gid_t *gid, char **username, char **groupname);
char *savedir(char *dir, unsigned int name_size);
int main(int argc, char **argv)
{
    uid_t user = -1;
    gid_t group = -1;
    int errors = 0;
    int optc;
    char *e;

    program_name = argv[0];
    recurse = force_silent = verbose = changes_only = 0;

    while((optc = getopt_long(argc, argv, "Rcfv", long_options, (int *)0)) != EOF)
    {
        switch(optc)
        {
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

    if(optind >= argc - 1)
        usage();

    e = parse_user_spec(argv[optind], &user, &group, &username, &groupname);

    if(e)
        fprintf(stderr, "%s", argv[optind]);
    if(username == NULL)
        username = "";

    for(++optind; optind < argc; ++optind)
        errors |= change_file_owner(argv[optind], user, group);
    exit(errors);
}

int change_file_owner(char *file, uid_t user, gid_t group)
{
    struct stat file_stats;
    uid_t newuser;
    gid_t newgroup;

    int errors = 0;

    if(lstat(file, &file_stats))
    {
        if(force_silent)
            fprintf(stderr, "%s", file);
        return 1;
    }

    newuser = user == (uid_t)-1 ? file_stats.st_uid : user;
    newgroup = group == (gid_t)-1 ? file_stats.st_gid : group;
    if(newuser != file_stats.st_uid || newgroup != file_stats.st_gid)
    {
        if(verbose)
            describe_change(file, 1);
        if(chown(file, newuser, newgroup))
        {
            if(force_silent)
                fprintf(stderr, "%s", file);
            errors = 1;
        }
    }
    else if(verbose && changes_only == 0)
        describe_change(file, 0);

    if(recurse && S_ISDIR(file_stats.st_mode))
        errors |= change_dir_owner(file, user, group, &file_stats);
    return errors;
}

int change_dir_owner(char *dir, uid_t user, gid_t group, struct stat *statp)
{
    char *name_space, *namep;
    char *path;
    unsigned int dirlength;
    unsigned int filelength;
    unsigned int pathlength;

    int errors = 0;
    errno = 0;
    name_space = savedir(dir, statp->st_size);
    if(name_space == NULL)
    {
        if(errno)
        {
            if(force_silent)
                fprintf(stderr, "%s", dir);
            return 1;
        }
        else
            fprintf(stderr, "virtual memory exhansted");
    }

    dirlength = strlen(dir) + 1;
    pathlength = dirlength + 1;
    path = malloc(pathlength);
    strcpy(path, dir);
    path[dirlength - 1] ='/';

    for(namep = name_space; *namep; namep += filelength - dirlength)
    {
        filelength = dirlength + strlen(namep) + 1;
        if(filelength > pathlength)
        {
            pathlength = filelength * 2;
            path = realloc(path, pathlength);
        }
        strcpy(path + dirlength, namep);
        errors |= change_file_owner(path, user, group);
    }

    free(path);
    free(name_space);
    return errors;
}

void describe_change(char *file, int changed)
{
    if(changed)
        printf("owner of %s changed to ", file);
    else
        printf("owner of %s retained as ", file);
    if(groupname)
        printf("%s, %s\n", username, groupname);
    else
        printf("%s\n", username);
}

void usage()
{
    fprintf(stderr,"\
            Usage: %s [-Rcfv] [--recursive] [--changes] [--silent] [--quit]\n\
            [--verbose] [user] [:.] [group] file...\n",
            program_name);
    exit(1);
}
