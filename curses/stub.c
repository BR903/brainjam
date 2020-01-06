/* curses/stub.c: empty placeholder for a removed user interface.
 */

#include <stddef.h>
#include "curses/curses.h"

/* Don't create the requested user interface.
 */
uimap curses_initializeui(void)
{
    uimap ui;

    ui.rendergame = NULL;
    ui.getinput = NULL;
    ui.ungetinput = NULL;
    ui.setcardanimationflag = NULL;
    ui.ding = NULL;
    ui.showsolutionwrite = NULL;
    ui.movecard = NULL;
    ui.changesettings = NULL;
    ui.selectconfig = NULL;
    ui.addhelpsection = NULL;
    return ui;
}
