/* files/files.h: managing data file I/O.
 *
 * The game uses several different files for storing information. All
 * file access is done through the functions in this module. The
 * initialization file stores the user's settings. The solutions file
 * stores all the user's solutions. Finally, there are the session
 * files, one for each game configuration that the user has played.
 * These store the history of the user's moves in that game.
 *
 * All files are kept in a single data directory, which is chosen (and
 * created if it doesn't already exist) during program initialization.
 */

#ifndef _files_files_h_
#define _files_files_h_

#include "./types.h"
#include "redo/types.h"

/* Set the directory for storing the program's data files. If dir is
 * NULL, then a default directory location is selected. The directory
 * is created if it does not already exist. The programpath argument
 * should be pathname of the program. If the program cannot determine
 * the usual default location, then it will attempt to locate the data
 * directory in the same directory as the program. Regardless of how
 * the directory is found, if the chosen directory is not usable, then
 * the return value is false and the program is put in read-only mode.
 */
extern int setdatadirectory(char const *dir, char const *programpath);

/* Set the value of the read-only flag. If the read-only flag is true,
 * all functions that write to files will automatically fail.
 */
extern void setreadonly(int flag);

/*
 * The initialization file.
 */

/* Read the game settings from the initialization file into the given
 * settingsinfo struct. Any settings in the struct that already have
 * values will be preserved, and the value from the file will be
 * ignored. Fields which do not appear in the initialization file will
 * be remain unchanged in either case. The return value is false if an
 * error occurred. (Note that a nonexistent file is treated the same
 * as a file that is empty.)
 */
extern int loadrcfile(settingsinfo *settings);

/* Update the initialization file to reflect the current settings.
 */
extern int savercfile(settingsinfo const *settings);

/* Look up an "extra" entry in the initialization file, one not
 * captured in the fields of the settingsinfo struct. The return value
 * is the value of the requested setting, or NULL if the key is not
 * present in the initialization file. The returned pointer is owned
 * by this module, and the caller should not retain a copy of it.
 */
extern char const *lookuprcsetting(char const *key);

/* Change the value of a setting, creating the setting if it doesn't
 * already exist. This entry will be written out to the initialization
 * file the next time it is updated. (It is an error to use this
 * function to change the value of one of the standard settings
 * captured in the fields of the settingsinfo struct.)
 */
extern void storercsetting(char const *key, char const *value);

/*
 * The solutions file.
 */

/* Read the solution file and return the array of solutions, or NULL
 * if the solution file cannot be read. pcount will receive the number
 * of solutions in the returned array. The caller is responsible for
 * freeing the array.
 */
extern solutioninfo *loadsolutionfile(int *pcount);

/* Write the given array of solutions to the solution file. The return
 * value is false if an error occurs.
 */
extern int savesolutionfile(solutioninfo const *solutions, int count);

/*
 * The session files.
 */

/* Set the name of the current session file to filename. The default
 * data directory will be used if filename is not an absolute path.
 */
extern void setsessionfilename(char const *filename);

/* Read the game tree stored in the session file and add it to the
 * redo session, recreating every move. The game state should be
 * initialized to the starting state. (The function temporarily alters
 * the state, and then restores it before returning.) The return value
 * is false if the file exists but cannot be read.
 */
extern int loadsession(redo_session *session, gameplayinfo *gameplay);

/* Write the redo_session contents to the current session file. The
 * return value is false if an error occurred while saving the data.
 */
extern int savesession(redo_session const *session);

#endif
