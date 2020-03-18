/* ./settings.h: managing the settings for the program.
 *
 * The settings are a small set of values (mostly boolean), that
 * affect program behavior and persist across invocations. The user
 * can modify them via the options menu, the initialization file, and
 * the command-line options. These functions manage the access and
 * modification of these values.
 */

#ifndef _settings_h_
#define _settings_h_

/* The list of settings. A negative value indicates that that field is
 * currently unset.
 */
struct settingsinfo {
    int gameid;                 /* the ID of the game most recently played */
    int autoplay;               /* setting for autoplaying on foundations */
    int animation;              /* setting for animating card movements */
    int showkeys;               /* setting for displaying move key guides */
    int branching;              /* setting for enabling branching undo */
    int forcetextmode;          /* true if the terminal UI should be used */
    int readonly;               /* true to prevent files from being changed */
};

/* Initialize the program settings by marking all fields as unset.
 */
extern void initializesettings(void);

/* Force any unset fields to take on a default value. Settings that
 * already have a value will be unchanged.
 */
extern void setdefaultsettings(void);

/* Get a pointer to the current settings. This pointer is writeable.
 * If the caller uses it to modify any settings, applysettings() must
 * be called subsequently for the changes to properly take effect.
 */
extern settingsinfo *getcurrentsettings(void);

/* Apply the current settings to the running program. If write is
 * true, the current settings are also saved to the initialization
 * file. Note that this function does not apply the gameid and
 * forcetextmode settings, which unlike the other settings can only be
 * applied at specific times.
 */
extern void applysettings(int write);

#endif
