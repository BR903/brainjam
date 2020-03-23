/* files/files.h: managing data file I/O.
 *
 * The game uses a few different files for storing information. All
 * file access is done through the functions in this module. The
 * initialization file stores the user's settings. The answers file
 * stores all the user's answers. Finally, there are the session
 * files, one for each game that the user has played. These store the
 * history of the user's moves in that game.
 *
 * All files are kept in one or two directories, which are chosen (and
 * created if they don't already exist) during program initialization.
 * These two directories are referred to as the config directory (or
 * the settings directory), and the data directory. They can be the
 * same directory, and in many cases they are.
 */

#ifndef _files_files_h_
#define _files_files_h_

#include "./types.h"
#include "redo/redo.h"

/* Select the directories for storing the program's files. cfgdir
 * specifies the directory for storing the game's settings, and
 * datadir specifies the directory for storing the user's data
 * (specifically, game history). If either cfgdir or datadir is NULL,
 * then default directories are selected, and they will be created if
 * they do not already exist. (Though note that the program will not
 * create a full path; the parent directories at least must already be
 * present.) If values are provided for either directory, those
 * directories must already exist; the program will not create them.
 * The argument executable should be the path to the program itself,
 * i.e. argv[0]. This is only used if the function cannot determine
 * the usual default locations. In this case it will attempt to use a
 * directory in the same directory as the program. Regardless of how
 * the directories are chosen, if they cannot be accessed or written
 * to, then the return value is false and the program is forced into
 * read-only mode.
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
 *
 * The main purpose of the initialization file is to preserve the
 * configuration values stored in the settingsinfo struct fields.
 * However, it is permitted for other settings to appear in the
 * initialization file, and even if they are not recognized, the
 * program will preserve them across updates. These "extra" settings
 * are useful to the UI modules, in which one may need to store
 * settings for a configuration that is not applicable to the other.
 */

/* Read the game settings from the initialization file into the given
 * settingsinfo struct. Fields which already have set values will not
 * be overwritten by this function; only unset fields will be changed.
 * Fields which do not appear in the initialization file will remain
 * unchanged in either case. The return value is false if an error
 * occurred. (Note that a nonexistent file is treated the same as a
 * file that is empty.)
 */
extern int loadinitfile(settingsinfo *settings);

/* Update the initialization file to reflect the current settings.
 */
extern int saveinitfile(settingsinfo const *settings);

/* Look up an "extra" entry in the initialization file, one not
 * captured in the fields of the settingsinfo struct. The return value
 * is the value of the requested setting, or NULL if the key is not
 * present in the initialization file. The returned pointer is owned
 * by this module, and the caller should not retain a copy of it.
 */
extern char const *lookupinitsetting(char const *key);

/* Change the value of a setting, creating the setting if it doesn't
 * already exist. This entry will be written out to the initialization
 * file the next time it is updated. (It is an error to use this
 * function to change the value of one of the standard settings
 * captured in the fields of the settingsinfo struct.)
 */
extern void storeinitsetting(char const *key, char const *value);

/*
 * The answers file.
 */

/* Read the answers file and return the array of answers through the
 * provided pointer. The return value is the size of the array on
 * success, or -1 if the answers file cannot be read. The caller is
 * responsible for freeing the array.
 */
extern int loadanswerfile(answerinfo **panswers);

/* Write the given array of answers to the answers file. The return
 * value is false if an error occurs.
 */
extern int saveanswerfile(answerinfo const *answers, int count);

/*
 * The session files.
 */

/* Set the name of the current session file to filename. The default
 * data directory will be used if filename is not an absolute path.
 */
extern void setsessionfilename(char const *filename);

/* Read the game tree stored in the session file and add it to the
 * redo session, recreating every move. The game state should be
 * initialized to the starting state before calling this function.
 * (The function temporarily alters the state, and then restores it
 * before returning.) The return value is false if the file exists but
 * cannot be read.
 */
extern int loadsession(redo_session *session, gameplayinfo *gameplay);

/* Write the complete redo_session contents to the current session
 * file. The return value is false if an error occurs while saving the
 * data.
 */
extern int savesession(redo_session const *session);

#endif
