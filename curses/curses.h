/* curses/curses.h: the textual user interface, implemented with ncurses.
 */

#ifndef _curses_curses_h_
#define _curses_curses_h_

#include "./ui.h"

/* Initialize the curses user interface and return the set of function
 * pointers that comprise the API. If the interface cannot be
 * initialized, the returned structure will have the first field
 * ("rendergame") set to NULL.
 */
extern uimap curses_initializeui(void);

#endif
