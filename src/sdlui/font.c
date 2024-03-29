/* sdlui/font.h: retrieving fonts from the OS.
 */

#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "font.h"

/* A font reference consists of a buffer containing the actual font
 * data, or else the path to a font file.
 */
struct fontrefinfo {
    char       *fontname;       /* an identifying name of some kind */
    char       *filename;       /* a font file name */
    void       *databuf;        /* a buffer containing font data */
    size_t      bufsize;        /* the size of the data buffer */
};

/*
 * Platform-specific font lookup.
 *
 * This file contains multiple implementations of font lookup code.
 * Which one will be included in the program will depend on the
 * target platform (and potentially the configuration).
 */

/* Use the fontname field of the fontref to identify an installed font
 * on the system with that name, and fill in either the filename field
 * or the databuf and bufsize fields, so that getfontfromref() can
 * use it to retrieve the font.
 */
static int lookupfont(fontrefinfo *fontref);

#if _WITH_FONTCONFIG

#include <fontconfig/fontconfig.h>

/* Use the fontconfig API to look up the filename of a font that the
 * program can use. The font is chosen by the given family/face name,
 * along with a requirement for various glyphs that appear in the UI.
 * The fontname pointer can be NULL to select a default font to use.
 */
static int lookupfont(fontrefinfo *fontref)
{
    FcPattern *pattern;
    FcPattern *match;
    FcCharSet *charset;
    FcFontSet *set;
    FcValue value;
    FcResult result;
    int i;

    charset = NULL;
    pattern = NULL;
    set = NULL;
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

    result = FALSE;
    if (fontref->fontname)
        pattern = FcNameParse((FcChar8 const*)fontref->fontname);
    else
        pattern = FcPatternCreate();
    if (!pattern)
        goto done;
    result = FcPatternAddCharSet(pattern, "charset", charset);
    if (!result)
        goto done;
    result = FcConfigSubstitute(NULL, pattern, FcMatchPattern);
    if (!result)
        goto done;
    FcDefaultSubstitute(pattern);
    match = FcFontMatch(NULL, pattern, &result);
    if (!match)
        goto done;
    set = FcFontSetCreate();
    FcFontSetAdd(set, match);
    for (i = 0 ; i < set->nfont ; ++i) {
        FcPatternGet(set->fonts[i], "file", 0, &value);
        if (value.type == FcTypeString) {
            fontref->filename = strallocate((char const*)value.u.s);
            result = TRUE;
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
    return result;
}

#elif _WIN32

#include <windows.h>

/* Use the Windows system API to look up a font by name. Unfortunately
 * the object that Windows returns cannot be turned into a filename.
 * Instead the actual font data must be copied into a private buffer.
 */
static int lookupfont(fontrefinfo *fontref)
{
    LOGFONT lf;
    HFONT hfont;
    HDC hdc;
    DWORD size;
    int result;

    lf.lfHeight = 0;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = FW_REGULAR;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;
    lf.lfStrikeOut = FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    if (fontref->fontname)
        snprintf(lf.lfFaceName, sizeof lf.lfFaceName, "%s", fontref->fontname);
    else
        lf.lfFaceName[0] = '\0';

    hfont = CreateFontIndirect(&lf);
    if (!hfont)
        return FALSE;

    result = FALSE;
    hdc = CreateCompatibleDC(NULL);
    if (!hdc)
        goto done;
    hfont = SelectObject(hdc, hfont);
    size = GetFontData(hdc, 0, 0, NULL, 0);
    if (size == GDI_ERROR)
        goto done;
    fontref->databuf = allocate(size);
    fontref->bufsize = size;
    GetFontData(hdc, 0, 0, fontref->databuf, size);
    result = TRUE;

  done:
    if (hdc) {
        hfont = SelectObject(hdc, hfont);
        DeleteDC(hdc);
    }
    if (hfont)
        DeleteObject(hfont);
    return result;
}

#else

/* As a last ditch effort, this variable can be modified to point to a
 * preferred font on the local machine. Replace this with the path of
 * your favorite font if you want to build without libfontconfig.
 */
static char const *defaultfontfilename =
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

/* Without an external service to locate system fonts, all that can be
 * done is offer up a likely path where a usable font might be found,
 * and hope for the best.
 */
static int lookupfont(fontrefinfo *fontref)
{
    fontref->filename = strallocate(defaultfontfilename);
    return TRUE;
}

#endif

/*
 * Internal functions.
 */

/* Use the given name to look up a font. If the argument is a valid
 * filename, it is returned directly. Otherwise, the argument is
 * assumed to be the name of an installed font. If the argument
 * doesn't match an available font, then a generic font will be
 * selected instead.
 */
fontrefinfo *findnamedfont(char const *fontname)
{
    fontrefinfo *fontref;
    FILE *fp;

    fontref = allocate(sizeof *fontref);
    fontref->fontname = fontname ? strallocate(fontname) : NULL;
    fontref->filename = NULL;
    fontref->databuf = NULL;
    fontref->bufsize = 0;

    if (fontname) {
        fp = fopen(fontname, "rb");
        if (fp) {
            fclose(fp);
            fontref->filename = fontref->fontname;
            fontref->fontname = NULL;
            return fontref;
        }
    }

    if (lookupfont(fontref))
        return fontref;

    deallocatefontref(fontref);
    return NULL;
}

/* Use an existing font reference to create a usable font object.
 */
TTF_Font *getfontfromref(fontrefinfo const *fontref, int size)
{
    SDL_RWops *rw;

    if (fontref->databuf) {
        rw = SDL_RWFromConstMem(fontref->databuf, fontref->bufsize);
        if (rw)
            return TTF_OpenFontRW(rw, TRUE, size);
    } else if (fontref->filename) {
        return TTF_OpenFont(fontref->filename, size);
    }
    return NULL;
}

/* Free all memory associated with a font reference.
 */
void deallocatefontref(fontrefinfo *fontref)
{
    if (fontref->fontname)
        deallocate(fontref->fontname);
    if (fontref->filename)
        deallocate(fontref->filename);
    if (fontref->databuf)
        deallocate(fontref->databuf);
    deallocate(fontref);
}
