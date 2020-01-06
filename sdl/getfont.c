/* sdl/getfont.c: obtaining a font filename.
 */

#include <stdlib.h>
#include "./gen.h"
#include "getfont.h"

/* This is the family name of the program's default font.
 */
static char const *defaultfontname = "freeserif";

/* This pathname is provided as a last resort, to be used if no other
 * means are available for obtaining a working font. Please feel free
 * to replace this with the pathname of a valid font on your system.
 * (Especially if you have removed fontconfig support!)
 */
static char const *defaultfontfile =
    "/usr/share/fonts/opentype/freefont/FreeSerif.otf";

#if _USE_FONTCONFIG

#include <fontconfig/fontconfig.h>

/* Use fontconfig to look up the filename of a font that the program
 * can use. The font is chosen by the given family name, along with a
 * requirement for various glyphs that appear in the help text. The
 * caller retains ownership of the returned string buffer.
 */
static char *fontconfiglookup(char const *familyname)
{
    FcPattern *pattern;
    FcPattern *match;
    FcCharSet *charset;
    FcFontSet *set;
    FcValue value;
    FcResult r;
    char *filename;
    int i;

    filename = NULL;
    set = NULL;
    pattern = NULL;
    FcInit();

    charset = FcCharSetCreate();
    FcCharSetAddChar(charset, 0x003D);
    FcCharSetAddChar(charset, 0x0061);
    FcCharSetAddChar(charset, 0x00A9);
    FcCharSetAddChar(charset, 0x00F8);
    FcCharSetAddChar(charset, 0x2013);
    FcCharSetAddChar(charset, 0x2022);
    FcCharSetAddChar(charset, 0x2190);
    FcCharSetAddChar(charset, 0x2191);
    FcCharSetAddChar(charset, 0x2192);
    FcCharSetAddChar(charset, 0x2193);

    pattern = FcNameParse((FcChar8 const*)familyname);
    if (!pattern)
        goto done;
    r = FcPatternAddCharSet(pattern, "charset", charset);
    if (r == FcFalse)
        goto done;
    r = FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    if (r == FcFalse)
        goto done;
    FcDefaultSubstitute(pattern);
    match = FcFontMatch(NULL, pattern, &r);
    if (!match)
        goto done;
    set = FcFontSetCreate();
    FcFontSetAdd(set, match);
    for (i = 0 ; i < set->nfont ; ++i) {
        FcPatternGet(set->fonts[i], "file", 0, &value);
        if (value.type == FcTypeString) {
            filename = strallocate((char const*)value.u.s);
            break;
        }
    }

  done:
    if (set)
        FcFontSetDestroy(set);
    if (pattern)
        FcPatternDestroy(pattern);
    if (charset)
        FcCharSetDestroy(charset);
    FcFini();

    return filename;
}

#else

/* If fontconfig support has not been included, this function is used
 * instead. The return value is either a copy of the argument, if it
 * holds a full pathname, or NULL.
 */
static char *fontconfiglookup(char const *hint)
{
    if (hint && *hint == '/')
        return strallocate(hint);
    return NULL;
}

#endif

/*
 * Internal functions.
 */

/* Return the full pathname of a font. If the argument already
 * contains a full pathname, it is returned directly. Otherwise, the
 * name is used in a lookup of the available fonts. The return value
 * is always a newly allocated buffer.
 */
char *getfontfilename(char const *fontname)
{
    char *filename = NULL;

    if (!fontname)
        fontname = defaultfontname;
    else if (*fontname == '/')
        return strallocate(fontname);
    filename = fontconfiglookup(fontname);
    if (!filename)
        filename = strallocate(defaultfontfile);
    return filename;
}
