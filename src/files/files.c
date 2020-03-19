/* files/files.c: managing the program's directories.
 *
 * Much of the code in this file is platform-specific. It is easy to
 * write platform-independent file I/O code. It is harder to write
 * platform-independent directory I/O code. Selecting sensible default
 * directories is even harder. So all of that logic is done here.
 */

#include <stdio.h>
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
 * Platform-specific directory management.
 */

/* Create a directory. This function hides the fact that the mkdir()
 * in the Windows API only takes one argument.
 */
static int createdir(char const *path, int mode)
{
#if _WIN32
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

#if _WIN32
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

/* Return TRUE if the path exists and is an accesible directory, or if
 * the path can be created. The latter option requires that the parent
 * directory must already exist and be accessible. If the return value
 * is false, errno will be set appropriately.
 */
static int canbedir(char const *path)
{
    DIR *fp;

    fp = opendir(path);
    if (!fp) {
        if (errno == ENOENT)
            if (!getreadonly() && createdir(path, 0777) == 0)
                return TRUE;
        return FALSE;
    }
    closedir(fp);
    return TRUE;
}

/* Verify that a directory is present within a parent directory, or
 * create it if it doesn't exist. The return value is the full
 * pathname to the queried directory, or NULL if the requested
 * directory was not available. The caller assumes ownership of the
 * returned string.
 */
static char *verifydirindir(char const *dir, char const *subdir)
{
    char *path;

    if (!isdir(dir))
        return NULL;
    path = fmtallocate("%s/%s", dir, subdir);
    if (canbedir(path))
        return path;
    perror(path);
    deallocate(path);
    return NULL;
}

/* Extract the directory from the given path, if possible. If the
 * directory cannot be isolated, the return value is NULL, otherwise
 * it points to a valid pathname. The caller is responsible for
 * freeing the buffer returned.
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
 * Platform-specific paths.
 *
 * Each platform has a different default for where applications can
 * set up per-user writeable directories. Each platform also has a
 * different default for how to find the path to those directories. So
 * the function findroots() needs a completely different definition
 * for each platform.
 */

/* Locate the root directories for applications to create directories
 * for storing settings and user data. The return value is false if
 * the root directories could not be located. Otherwise, the caller is
 * responsible for freeing the returned strings. Note that the
 * function does not verify that the directories exist, or that they
 * are useable.
 */
static int findroots(char **psettingsroot, char **pdataroot);

#if __APPLE__

/* The application directory is in a standard location under the
 * user's home directory.
 */
static int findroots(char **psettingsroot, char **pdataroot)
{
    char const *homedir;

    homedir = getenv("HOME");
    if (!homedir)
        return FALSE;
    *pdataroot = fmtallocate("%s/Library/Application Support", homedir);
    *psettingsroot = strallocate(*pdataroot);
    return TRUE;
}

#elif _WIN32

/* The application directory should be identified by the APPDATA
 * environment variable.
 */
static int findroots(char **psettingsroot, char **pdataroot)
{
    char const *dataroot;
    char const *homedir;

    dataroot = getenv("APPDATA");
    if (dataroot) {
        *pdataroot = strallocate(dataroot);
    } else {
        homedir = getenv("HOMEPATH");
        if (!homedir) {
            homedir = getenv("HOME");
            if (!homedir)
                return FALSE;
        }
        *pdataroot = fmtallocate("%s\\Application Data", homedir);
    }
    *psettingsroot = strallocate(*pdataroot);
    return TRUE;
}

#else

/* The XDG_CONFIG_HOME and XDG_DATA_HOME environment variables
 * identify the root directories for configuration files and user data
 * files, respectively.
 */
static int findroots(char **psettingsroot, char **pdataroot)
{
    char const *settingsroot;
    char const *dataroot;
    char const *homedir = NULL;

    settingsroot = getenv("XDG_CONFIG_HOME");
    dataroot = getenv("XDG_DATA_HOME");
    if (!settingsroot || !dataroot) {
        homedir = getenv("HOME");
        if (!homedir)
            return FALSE;
    }

    if (settingsroot)
        *psettingsroot = strallocate(settingsroot);
    else
        *psettingsroot = fmtallocate("%s/.config", homedir);
    if (dataroot)
        *pdataroot = strallocate(dataroot);
    else
        *pdataroot = fmtallocate("%s/.local/share", homedir);
    return TRUE;
}

#endif

/*
 * Directory selection.
 */

/* Determine values for the settingsdir and datadir variables, for
 * storing the user's configuration data and session data,
 * respectively. False is returned if no proper values for these
 * directories could not be found. If this function returns a true
 * value, however, it is still possible for settingsdir or datadir to
 * still be set to NULL. This indicates that appropriate values were
 * determined but the directories could not be accessed, and therefore
 * instead of finding directories elsewhere, the program should forgo
 * writing data.
 */
static int choosedirectories(void)
{
    char *settingsroot;
    char *dataroot;

    if (!findroots(&settingsroot, &dataroot))
        return FALSE;
    settingsdir = verifydirindir(settingsroot, "brainjam");
    datadir = verifydirindir(dataroot, "brainjam");
    deallocate(settingsroot);
    deallocate(dataroot);
    return TRUE;
}

/* Select program directories when the user doesn't have any of the
 * necessary environment variables set to determine what path they
 * should use. As a last-ditch measure, this function attempts to use
 * a save subdirectory in the directory containing the executable.
 */
static int choosehomelessdirectories(char const *executablepath)
{
    char *executabledir;

    executabledir = getdirfrompath(executablepath);
    if (!executabledir)
        return FALSE;

    if (!settingsdir)
        settingsdir = verifydirindir(executabledir, "save");
    if (!datadir)
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
 * directory. If datadir is unset, the current directory will be used.
 */
char *mkdatapath(char const *filename)
{
    if (datadir)
        return fmtallocate("%s/%s", datadir, filename);
    else
        return strallocate(filename);
}

/* Turn a filename into a pathname, using settingsdir as the starting
 * directory. If settingsdir is unset, use the current directory.
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

/* Identify the directories that the program will use to hold its
 * files. Either or both override directory arguments can be NULL, in
 * which case the program will supply default directories, creating
 * them if they do not already exist.
 */
int setfiledirectories(char const *overridecfgdir,
                       char const *overridedatadir,
                       char const *executablepath)
{
    if (overridedatadir) {
        if (canbedir(overridedatadir))
            datadir = strallocate(overridedatadir);
        else
            perror(overridedatadir);
    }
    if (overridecfgdir) {
        if (overridedatadir && !strcmp(overridecfgdir, overridedatadir))
            settingsdir = datadir;
        else if (canbedir(overridecfgdir))
            settingsdir = strallocate(overridecfgdir);
        else
            perror(overridecfgdir);
    }
    if (!settingsdir || !datadir) {
        if (!choosedirectories())
            choosehomelessdirectories(executablepath);
    }
    if (!settingsdir && datadir)
        settingsdir = datadir;

    if (!settingsdir || !datadir)
        forcereadonly = TRUE;
    return TRUE;
}

/* Print the program's chosen directories on standard output.
 */
void printfiledirectories(void)
{
    printf("configuration data: %s\n", settingsdir ? settingsdir : "(unset)");
    printf("saved session data: %s\n", datadir ? datadir : "(unset)");
}
