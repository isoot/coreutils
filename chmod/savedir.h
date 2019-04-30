#include <dirent.h>
#include <sys/dir.h>
#include <stdlib.h>
#include <string.h>
#define CLOSEDIR(dir) closedir(dir)

#define NLENGTH(dirent) ((dirent)->d_namlen)
char *savedir(char *dir, unsigned int name_size);
