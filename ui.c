/* ./ui.c: the API for the user interface.
 */

#include <stddef.h>
#include "./ui.h"
#include "curses/curses.h"
#include "sdl/sdl.h"

/* The global variable holding the functions of the API.
 */
uimap _ui;

/* Select the user interface to connect and have it initialize. If the
 * initialization program fails to provide a render function, then the
 * UI is unavailable.
 */
int initializeui(int uimode)
{
    switch (uimode) {
      case UI_SDL:      _ui = sdl_initializeui();       break;
      case UI_CURSES:   _ui = curses_initializeui();    break;
      default:          _ui.rendergame = NULL;          break;
    }
    return _ui.rendergame != NULL;
}
