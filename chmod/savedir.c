#include "savedir.h"

char *savedir(char *dir, unsigned int name_size)
{
    DIR *dirp;
    struct direct *dp;
    char *name_space;
    char *namep;

    dirp = opendir(dir);
    if(NULL == dirp)
        return NULL;
    name_space = (char *)malloc(name_size);

    if(NULL == name_space)
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
