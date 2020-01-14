/* files/files.c: managing the program's directories.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include "./gen.h"
#include "files/files.h"
#include "internal.h"

/* The directory where the user's initialization file is stored.
 */
static char *settingsdir = NULL;

/* The directory where all of the user's data files are stored.
 */
static char *datadir = NULL;

/* When readonly is true, all save functions are disabled.
 */
static int readonly = FALSE;

/* If forcereadonly is true, even setreadonly() is disabled.
 */
static int forcereadonly = FALSE;

/*
 * Platform-specific code.
 */

/* Create a directory. This function hides the fact that the mkdir()
 * in the Windows API only takes one argument.
 */
static int createdir(char const *path, int mode)
{
#ifdef WIN32
    (void)mode;
    return mkdir(path);
#else
    return mkdir(path, mode);
#endif
}

/* Return the offset of the last directory separator in the given
 * path, or -1 if no such separator is present. This function hides
 * the fact that Windows permits either type of slash to be used in
 * pathnames interchangeably.
 */
static int dirsepindex(char const *path)
{
    char const *p;
    char const *pp;

#ifdef WIN32
    pp = strrchr(path, '\\');
#else
    pp = NULL;
#endif
    p = strrchr(path, '/');
    if (!pp || (p && p > pp))
        pp = p;
    return pp ? pp - path : -1;
}

/*
 * Directory validation.
 */

/* Return TRUE if the path exists and is an accesible directory.
 */
static int isdir(char const *path)
{
    DIR *fp;

    fp = opendir(path);
    if (!fp)
        return FALSE;
    closedir(fp);
    return TRUE;
}

/* Verify that a directory is present within a parent directory, or
 * create it if it doesn't exist. Note that the parent directory must
 * already exist; this function will not create a new hierarchy. The
 * return value is the full pathname to the queried directory, or NULL
 * if the requested directory was not available. The caller assumes
 * ownership of the returned string.
 */
static char *verifydirindir(char const *dir, char const *subdir)
{
    char *path;

    if (!isdir(dir))
        return NULL;
    path = fmtallocate("%s/%s", dir, subdir);
    if (isdir(path))
        return path;
    if (errno == ENOENT) {
        if (getreadonly())
            goto quit;
        if (createdir(path, 0777) == 0)
            return path;
    }
    warn("%s: %s", path, strerror(errno));

  quit:
    deallocate(path);
    return NULL;
}

/* Extract the directory from the given path, if possible. Since
 * Windows paths can use both kinds of slashes as directory
 * separators, some extra logic is needed just for that platform. If
 * the directory cannot be isolated, the return value is NULL,
 * otherwise it points to a valid pathname. The caller is responsible
 * for freeing the buffer returned.
 */
static char *getdirfrompath(char const *path)
{
    char *buf;
    int n;

    if (!path)
        return NULL;
    n = dirsepindex(path);
    if (n < 0)
        return NULL;
    buf = allocate(n + 1);
    memcpy(buf, path, n);
    buf[n] = '\0';
    return buf;
}

/*
 * Directory selection.
 */

/* Select program directories, specifically settingsdir for storing
 * the user's settings, and datadir for storing the user's solutions
 * and sessions. These directories will be initialized by the standard
 * XDG_CONFIG_HOME and XDG_DATA_HOME values. The return value is false
 * if these environment variables are unset and their default values
 * cannot be determined. If their values are set but the directories
 * they point to do not exist (or are not accessible), then
 * settingsdir and/or datadir will still be NULL when this function
 * returns.
 */
static int choosedirectories(void)
{
    char *settingsroot;
    char *dataroot;
    char *homedir = NULL;

    settingsroot = getenv("XDG_CONFIG_HOME");
    dataroot = getenv("XDG_DATA_HOME");
    if (!settingsroot || !dataroot) {
        homedir = getenv("HOME");
        if (!homedir)
            return FALSE;
    }

    if (settingsroot)
        settingsroot = strallocate(settingsroot);
    else
        settingsroot = fmtallocate("%s/.config", homedir);
    if (dataroot)
        dataroot = strallocate(dataroot);
    else
        dataroot = fmtallocate("%s/.local/share", homedir);

    settingsdir = verifydirindir(settingsroot, "brainjam");
    datadir = verifydirindir(dataroot, "brainjam");

    deallocate(settingsroot);
    deallocate(dataroot);
    return TRUE;
}

/* Select program directories when the user doesn't have a home
 * directory, by locating the directory of the executable and
 * attempting to create a save directory there. (This strategy is
 * aimed at Windows users, who are the only ones likely to not have a
 * HOME environment variable set.)
 */
static int choosehomelessdirectories(char const *executablepath)
{
    char *executabledir;

    executabledir = getdirfrompath(executablepath);
    if (!executabledir)
        return FALSE;

    settingsdir = verifydirindir(executabledir, "save");
    datadir = verifydirindir(executabledir, "save");

    deallocate(executabledir);
    return TRUE;
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

/* Turn a filename into a pathname, using datadir as the starting
 * directory.
 */
char *mkdatapath(char const *filename)
{
    if (datadir)
        return fmtallocate("%s/%s", datadir, filename);
    else
        return strallocate(filename);
}

/* Turn a filename into a pathname, using settingsdir as the starting
 * directory.
 */
char *mksettingspath(char const *filename)
{
    if (settingsdir)
        return fmtallocate("%s/%s", settingsdir, filename);
    else if (datadir)
        return fmtallocate("%s/%s", datadir, filename);
    else
        return strallocate(filename);
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

/* Locate the directories that the program will use for its files,
 * creating them if they do not already exist. If overridedir names a
 * valid directory, then it will be used instead of the program's
 * default directories.
 */
int setfiledirectories(char const *overridedir, char const *executablepath)
{
    if (overridedir) {
        if (isdir(overridedir)) {
            settingsdir = strallocate(overridedir);
            datadir = strallocate(overridedir);
        } else {
            warn("%s: %s", overridedir, strerror(errno));
        }
    } else {
        if (!choosedirectories())
            choosehomelessdirectories(executablepath);
    }
    if (!datadir)
        forcereadonly = TRUE;
    return TRUE;
}
