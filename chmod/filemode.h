#include <sys/types.h>
#include <sys/stat.h>

void filemodestring(struct stat *statp, char *str);

void mode_string(unsigned short mode, char *str);

