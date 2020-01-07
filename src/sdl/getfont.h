/* sdl/getfont.h: obtaining a font filename.
 */

#ifndef _sdl_getfont_h_
#define _sdl_getfont_h_

/* Return the pathname of an appropriate font for the program to use.
 * If the argument is not NULL, it provides the name of a preferred
 * font. If the argument already contains a full pathname, that path
 * is returned directly. The caller is responsible for freeing the
 * returned string afterwards.
 */
extern char *getfontfilename(char const *fontname);

#endif
