/* files/files.c: managing data file I/O.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "./gen.h"
#include "files/files.h"
#include "internal.h"

/* The directory where all of the user's data files are stored.
 */
static char *datadir = NULL;

/* If true, all save functions are disabled.
 */
static int readonly = FALSE;

/* If true, even the setreadonly() function is disabled.
 */
static int forcereadonly = FALSE;

/* Verify that the given directory is present, creating it if it
 * doesn't exist. The return value is false if the directory could not
 * be created.
 */
static int finddir(char const *dir)
{
    DIR *dirfp;

    dirfp = opendir(dir);
    if (dirfp) {
        closedir(dirfp);
        return TRUE;
    }
    if (errno == ENOENT)
        if (mkdir(dir, 0777) == 0)
            return TRUE;
    warn("%s: %s", dir, strerror(errno));
    return FALSE;
}

/* Extract a directory from the given path, if possible. If the return
 * value is not NULL, the caller is responsible for freeing the buffer.
 */
static char const *getdirfrompath(char const *path)
{
    char *p;
    DIR *dirfp;
    int n;

    if (!path || !*path)
        return NULL;
    p = strrchr(path, '/');
    if (!p) {
        p = strrchr(path, '\\');
        if (!p)
            return NULL;
    }
    n = p - path;
    p = allocate(n + 1);
    memcpy(p, path, n);
    p[n] = '\0';
    dirfp = opendir(p);
    if (dirfp) {
        closedir(dirfp);
        return p;
    } else {
        deallocate(p);
        return NULL;
    }
}

/*
 * Internal functions.
 */

/* Return the state of the read-only flag (unless the forcereadonly
 * flag is set, in which case it takes precedence).
 */
int getreadonly(void)
{
    return readonly || forcereadonly;
}

/* Copy a complete pathname, using datadir as the starting
 * directory if the file is not already an absolute pathname.
 */
char *mkpath(char const *file)
{
    if (!datadir || *file == '/')
        return strallocate(file);
    return fmtallocate("%s/%s", datadir, file);
}

/*
 * External functions.
 */

/* Forbid or permit writing to files.
 */
void setreadonly(int flag)
{
    readonly = flag;
}

/* Set the data directory, creating it if it does not already exist.
 * If an explicit path for the data directory is not given, then
 * select a path in the user's home directory, or in the program's
 * directory if the home directory is not known, or in the current
 * directory if the program's directory is also not known. If, after
 * all this, the chosen path cannot be used, then the program is
 * forced into read-only mode and false is returned.
 */
int setdatadirectory(char const *dir, char const *programpath)
{
    deallocate(datadir);
    if (dir) {
        datadir = strallocate(dir);
    } else {
        dir = getenv("HOME");
        if (dir) {
            datadir = fmtallocate("%s/.brainjam", dir);
        } else {
            dir = getdirfrompath(programpath);
            if (dir)
                datadir = fmtallocate("%s/save", dir);
            else
                datadir = strallocate("./save");
        }
    }
    if (!finddir(datadir)) {
        deallocate(datadir);
        datadir = NULL;
        forcereadonly = TRUE;
        return FALSE;
    }
    return TRUE;
}
