/* ./mainloop.h: the program's top-level loop.
 */

#ifndef _mainloop_h_
#define _mainloop_h_

/* Run the actual program, once everything has finished initializing.
 * The function brings up the game selection interface, and then
 * allows the user to play the game until they ask to leave.
 */
extern void gameselectionloop(void);

/* Examine the contents of every data file and return. Unreadable
 * files or files containing invalid data will result in error
 * messages displayed to the user.
 */
extern void filevalidationloop(void);

#endif
