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

/* Macro to paper over the fact that the mkdir() provided by the
 * Windows runtime only takes one argument.
 */
#ifdef WIN32
#define createdir(path, perm)  mkdir(path)
#else
#define createdir(path, perm)  mkdir(path, perm)
#endif

/* The directory where all of the user's data files are stored.
 */
static char *datadir = NULL;

/* If true, all save functions are disabled.
 */
static int readonly = FALSE;

/* If true, even the setreadonly() function is disabled.
 */
static int forcereadonly = FALSE;

/* Verify that the given directory is present, or create it if it
 * doesn't exist. The return value is false if the directory could not
 * be accessed or created.
 */
static int finddir(char const *dir)
{
    DIR *dirfp;

    dirfp = opendir(dir);
    if (dirfp) {
        closedir(dirfp);
        return TRUE;
    } else if (errno == ENOENT) {
        if (createdir(dir, 0777) == 0)
            return TRUE;
    }
    warn("%s: %s", dir, strerror(errno));
    return FALSE;
}

/* Extract a directory from the given path, if possible. Since Windows
 * paths can use both kinds of slashes as directory separators, some
 * extra logic is needed just for that platform. If the return value
 * is not NULL, the caller is responsible for freeing the buffer.
 */
static char const *getdirfrompath(char const *path)
{
    char *p;
    DIR *dirfp;
    int n;

    if (!path || !*path)
        return NULL;
    p = strrchr(path, '/');
#ifdef WIN32
    if (!p)
        p = strrchr(path, '\\');
#endif
    if (!p)
        return NULL;
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

/* Turn a file into a pathname, using datadir as the starting
 * directory.
 */
char *mkpath(char const *file)
{
    if (datadir)
        return fmtallocate("%s/%s", datadir, file);
    else
        return strallocate(file);
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
 * If no explicit directory is provided, the function will select one,
 * located under the user's home directory. If the user's home
 * directory is not known, then the function attempts to use a
 * directory in the same location as programpath. If programpath does
 * not include a directory, the function will attempt to use the
 * current directory. If, in the end, the chosen directory cannot be
 * used, then false is returned, and the program enforces read-only
 * mode for the duration.
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
    forcereadonly = FALSE;
    return TRUE;
}
