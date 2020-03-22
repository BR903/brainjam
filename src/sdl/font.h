/* sdl/font.h: retrieving fonts from the OS.
 */

#ifndef _sdl_font_h_
#define _sdl_font_h_

#include <SDL_ttf.h>

/* An opaque reference to the data of a truetype font.
 */
typedef struct fontrefinfo fontrefinfo;

/* Return a reference to an appropriate font for the program to use.
 * If the argument is not NULL, it provides the identifer of a
 * preferred font. If fontname is a valid filename, it will be used
 * directly; otherwise the operating system will be queried to find a
 * font with that name. If the name doesn't match a known font, the
 * system will select a general-purpose font to return instead. The
 * return value is NULL only in the case that no fonts could be found.
 */
extern fontrefinfo *findnamedfont(char const *fontname);

/* Load a usable SDL_ttf font from an existing fontref. The return
 * value is NULL if the fontref is invalid or if an error occurs.
 */
extern TTF_Font *getfontfromref(fontrefinfo const *fontref, int size);

/* Free all memory associated with a font reference. Do not call this
 * function while any derived fonts are still open.
 */
extern void deallocatefontref(fontrefinfo *fontref);

#endif
