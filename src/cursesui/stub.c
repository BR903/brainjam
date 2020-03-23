/* cursesui/stub.c: empty placeholder for a removed user interface.
 */

#include <stddef.h>
#include "cursesui/cursesui.h"

/* Don't create the requested user interface.
 */
uimap cursesui_initializeui(void)
{
    uimap ui;

    ui.rendergame = NULL;
    ui.getinput = NULL;
    ui.ungetinput = NULL;
    ui.setcardanimationflag = NULL;
    ui.ding = NULL;
    ui.showwriteindicator = NULL;
    ui.movecard = NULL;
    ui.changesettings = NULL;
    ui.selectgame = NULL;
    ui.addhelpsection = NULL;
    return ui;
}
