/* ./settings.h: managing the settings for the program.
 *
 * The settings are a small set of values (mostly boolean values),
 * that affect program behavior and persist across invocations. The
 * user can modify them via the options menu, the initialization file,
 * and/or the command-line options. These functions manage access and
 * modification of these values.
 */

#ifndef _settings_h_
#define _settings_h_

/* The list of settings. A negative value indicates that that field is
 * currently unset.
 */
struct settingsinfo {
    int configid;               /* the ID of the last visited configuration */
    int autoplay;               /* setting for autoplaying on foundations */
    int animation;              /* setting for animating card movements */
    int branching;              /* setting for enabling branching undo */
    int forcetextmode;          /* true if the terminal UI should be used */
    int readonly;               /* true to prevent files from being changed */
};

/* Initialize the program settings. All fields will be unset.
 */
extern void initializesettings(void);

/* Force any unset fields to take on a default value. Settings that
 * already have a value will be unchanged.
 */
extern void setdefaultsettings(void);

/* Get a pointer to the current settings. If the caller modifies any
 * of the settings through the returned pointer, applysettings() must
 * be called afterwards for the changes to take effect.
 */
extern settingsinfo *getcurrentsettings(void);

/* Apply the current settings to the running program. If write is
 * true, the current settings are also written to the initialization
 * file. Note that this function does not apply the configid and
 * forcetextmode fields, which unlike the other settings can only be
 * applied at specific times.
 */
extern void applysettings(int write);

#endif
