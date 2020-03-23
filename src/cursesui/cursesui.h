/* cursesui/cursesui.h: the textual user interface, implemented with ncurses.
 */

#ifndef _cursesui_cursesui_h_
#define _cursesui_cursesui_h_

#include "./ui.h"

/* Initialize the curses user interface and return the set of function
 * pointers that comprise the API. If the interface cannot be
 * initialized, the returned structure will have the first field
 * ("rendergame") set to NULL.
 */
extern uimap cursesui_initializeui(void);

#endif
