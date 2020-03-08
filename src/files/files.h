/* files/files.h: managing data file I/O.
 *
 * The game uses several different files for storing information. All
 * file access is done through the functions in this module. The
 * initialization file stores the user's settings. The solutions file
 * stores all the user's solutions. Finally, there are the session
 * files, one for each game that the user has played. These store the
 * history of the user's moves in that game.
 *
 * All files are kept in a single data directory, which is chosen (and
 * created if it doesn't already exist) during program initialization.
 */

#ifndef _files_files_h_
#define _files_files_h_

#include "./types.h"
#include "redo/types.h"

/* Select the directories for storing the program's data files. cfgdir
 * specifies the directory for storing the game's settings, and
 * datadir specifies the directory for storing the user's data
 * (specifically, game history). If either argument is NULL, then
 * default locations are selected. The directories are created if they
 * do not already exist. The executable argument should be the path to
 * the program itself, i.e. argv[0]. If the function cannot determine
 * the usual default locations, then it will attempt to use a
 * directory in the same directory as the program. Regardless of how
 * the directories are chosen, if they cannot be accessed, then the
 * return value is false and the program is put in read-only mode.
 */
extern int setfiledirectories(char const *cfgdir, char const *datadir,
                              char const *executable);

/* Output the directories on standard output. (This functions is
 * mainly provided to help the user with configuration issues.)
 */
extern void printfiledirectories(void);

/* Set read-only mode on or off. When in read-only mode, all functions
 * that write to files will quietly fail.
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

/* Read the solution file and return the array of solutions through
 * the provided pointer. The return value is the size of the array on
 * success, or -1 if the solution file cannot be read. The caller is
 * responsible for freeing the array.
 */
extern int loadsolutionfile(solutioninfo **psolutions);

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
