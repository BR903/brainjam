/* ./settings.c: managing the settings for the program.
 */

#include <stddef.h>
#include "./gen.h"
#include "./types.h"
#include "./settings.h"
#include "./ui.h"
#include "files/files.h"
#include "game/game.h"

/* The default values for the program's settings. Typically these are
 * only used the first time the program runs, before an initialization
 * file has been created.
 */
#define DEFAULT_GAMEID  0
#define DEFAULT_SHOWKEYS  0
#define DEFAULT_ANIMATION  1
#define DEFAULT_AUTOPLAY  1
#define DEFAULT_BRANCHING  0
#define DEFAULT_READONLY  0
#define DEFAULT_FORCETEXTMODE  0

/* The global settings are stored here.
 */
static settingsinfo *settings = NULL;

/*
 * External functions.
 */

/* Initialize the settings to all-unset.
 */
void initializesettings(void)
{
    settings = reallocate(settings, sizeof *settings);
    settings->gameid = -1;
    settings->showkeys = -1;
    settings->animation = -1;
    settings->autoplay = -1;
    settings->branching = -1;
    settings->readonly = -1;
    settings->forcetextmode = -1;
}

/* Update any unset fields to their default values.
 */
void setdefaultsettings(void)
{
    if (settings->gameid < 0)
        settings->gameid = DEFAULT_GAMEID;
    if (settings->showkeys < 0)
        settings->showkeys = DEFAULT_SHOWKEYS;
    if (settings->animation < 0)
        settings->animation = DEFAULT_ANIMATION;
    if (settings->autoplay < 0)
        settings->autoplay = DEFAULT_AUTOPLAY;
    if (settings->branching < 0)
        settings->branching = DEFAULT_BRANCHING;
    if (settings->readonly < 0)
        settings->readonly = DEFAULT_READONLY;
    if (settings->forcetextmode < 0)
        settings->forcetextmode = DEFAULT_FORCETEXTMODE;
}

/* Return a pointer to the current settings. The caller is permitted
 * to modify the settings through this pointer.
 */
settingsinfo *getcurrentsettings(void)
{
    if (!settings)
        initializesettings();
    return settings;
}

/* Apply the current settings to the running program. If write is
 * true, the initialization file is updated.
 */
void applysettings(int write)
{
    if (!settings)
        return;
    if (settings->showkeys >= 0)
        setshowkeyguidesflag(settings->showkeys);
    if (settings->animation >= 0)
        setanimation(settings->animation);
    if (settings->autoplay >= 0)
        setautoplay(settings->autoplay);
    if (settings->branching >= 0)
        setbranching(settings->branching);
    if (settings->readonly >= 0)
        setreadonly(settings->readonly);
    if (write)
        saveinitfile(settings);
}
