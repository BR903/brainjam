/* sdlui/help.c: the help reader display.
 */

#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "./gen.h"
#include "./types.h"
#include "./commands.h"
#include "internal.h"
#include "image.h"
#include "button.h"
#include "scroll.h"

/* The data comprising each section of help.
 */
typedef struct sectioninfo {
    char const *title;          /* name of the section */
    char const *text;           /* full text of this section */
    int         w;              /* width in pixels of the text */
    int         h;              /* height in pixels of the text */
    int         scrollpos;      /* scrolled position of the text */
    SDL_Texture *texture;       /* the texture holding the full text */
    SDL_Texture *titletexture;  /* the texture holding the title */
} sectioninfo;

/* A substring making up one (renderable) line of a multi-line string.
 */
typedef struct lineinfo {
    int         offset;         /* offset of this line in the string */
    int         len;            /* length of this line in the string */
} lineinfo;

/* The complete list of sections making up the display.
 */
static sectioninfo *sections;
static int sectioncount = 0;

/* The width of the section list, wide enough to show each title.
 */
static int listwidth = 0;

/* The index of the section currently being displayed.
 */
static int currentsection = -1;

/* The help display's scrollbar.
 */
static scrollbar scroll;

/* The help display's back button.
 */
static button backbutton;

/* The locations of the elements of the display.
 */
static SDL_Point displaysize;           /* size of the window */
static SDL_Rect listrect;               /* location for the section list */
static SDL_Rect textrect;               /* location for the main text */
static SDL_Point listoutline[5];        /* outline around the section list */
static SDL_Point textoutline[5];        /* outline around the main text */
static int lineheight;                  /* height of one line of text */

/*
 * Managing the help sections.
 */

/* Free all cached resources for rendering the section's contents on
 * the display that is specific to its size. This function is called
 * whenever the display changes size.
 */
static void resetsections(void)
{
    int i;

    for (i = 0 ; i < sectioncount ; ++i) {
        if (sections[i].texture && sections[i].w != textrect.w) {
            SDL_DestroyTexture(sections[i].texture);
            sections[i].texture = NULL;
        }
    }
}

/* Replace the text of the section at index, or remove the section
 * entirely if text is NULL.
 */
static void updatehelpsection(int index, char const *text)
{
    if (text) {
        sections[index].text = text;
        if (sections[index].texture) {
            SDL_DestroyTexture(sections[index].texture);
            sections[index].texture = NULL;
        }
    } else {
        memmove(sections + index, sections + index + 1,
                (sectioncount - index - 1) * sizeof *sections);
        --sectioncount;
        if (currentsection >= sectioncount)
            currentsection = sectioncount - 1;
    }
}

/* Add a new section to the array, with the given title and text body.
 * If first is true, the section is placed before the others,
 * otherwise it is added to the end. The title texture is rendered
 * without color, so that color can be added dynamically. The body
 * text is not rendered to a texture until it is actually displayed.
 */
static void inserthelpsection(char const *title, char const *text, int first)
{
    SDL_Color const white = { 255, 255, 255, 255 };
    SDL_Surface *image;
    int n;

    sections = reallocate(sections, (sectioncount + 1) * sizeof *sections);
    if (first) {
        memmove(sections + 1, sections, sectioncount * sizeof *sections);
        n = 0;
    } else {
        n = sectioncount;
    }
    ++sectioncount;
    sections[n].title = title;
    sections[n].text = text;
    sections[n].texture = NULL;
    image = TTF_RenderUTF8_Blended(_graph.smallfont, title, white);
    sections[n].titletexture =
                SDL_CreateTextureFromSurface(_graph.renderer, image);
    SDL_FreeSurface(image);
}

/*
 * Display element placement.
 */

/* Calculate the locations and positions of the elements that make up
 * the help display: The section list and its outline, the main text
 * display and its outline, the scrollbar, and the back button.
 */
static SDL_Point setlayout(SDL_Point display)
{
    SDL_Rect rect;
    SDL_Point size;
    int textmargin;

    displaysize = display;

    lineheight = TTF_FontLineSkip(_graph.smallfont);

    textmargin = _graph.margin / 2;

    rect.x = _graph.margin;
    rect.y = _graph.margin;
    rect.w = display.x - rect.x - _graph.margin;
    rect.h = display.y - rect.y - _graph.margin;

    backbutton.pos.x = rect.x + rect.w - backbutton.pos.w;
    backbutton.pos.y = rect.y + rect.h - backbutton.pos.h;
    rect.w -= backbutton.pos.w + _graph.margin;

    listoutline[0].x = rect.x - 1;
    listoutline[0].y = rect.y - 1;
    listoutline[1].x = listoutline[0].x;
    listoutline[1].y = rect.y + rect.h + 1;
    listoutline[2].x = rect.x + listwidth + 2 * textmargin + 1;
    listoutline[2].y = listoutline[1].y;
    listoutline[3].x = listoutline[2].x;
    listoutline[3].y = listoutline[0].y;
    listoutline[4].x = listoutline[0].x;
    listoutline[4].y = listoutline[0].y;

    listrect.x = listoutline[0].x + textmargin;
    listrect.y = listoutline[0].y + textmargin;
    listrect.w = listwidth;
    listrect.h = rect.h - 2 * textmargin;

    textoutline[0].x = listoutline[3].x + _graph.margin - 1;
    textoutline[0].y = rect.y - 1;
    textoutline[1].x = textoutline[0].x;
    textoutline[1].y = rect.y + rect.h + 1;
    textoutline[2].x = rect.x + rect.w - _graph.margin + 1;
    textoutline[2].y = textoutline[1].y;
    textoutline[3].x = textoutline[2].x;
    textoutline[3].y = listoutline[0].y;
    textoutline[4].x = textoutline[0].x;
    textoutline[4].y = textoutline[0].y;

    textrect.x = textoutline[0].x + textmargin;
    textrect.y = textoutline[0].y + textmargin;
    textrect.w = textoutline[2].x - textrect.x - textmargin;
    textrect.h = textoutline[2].y - textrect.y - textmargin;

    scroll.pos.x = rect.x + rect.w - textmargin;
    scroll.pos.y = textoutline[3].y;
    scroll.pos.w = textmargin;
    scroll.pos.h = textoutline[2].y - scroll.pos.y + 2;
    scroll.pagesize = textrect.h;
    scroll.linesize = lineheight;
    if (currentsection >= 0)
        scroll.range = sections[currentsection].h - textrect.h;

    resetsections();

    TTF_SizeUTF8(_graph.smallfont, "An average section title.",
                 &rect.w, &rect.h);
    size.x = listrect.w + 2 * rect.w + backbutton.pos.w + 5 * _graph.margin;
    size.y = 8 * lineheight + 2 * _graph.margin;
    return size;
}

/* Determine the minimum width necessary for rendering the list of
 * section names. If the width needs to change, then recompute the
 * full layout.
 */
static int updatelistwidth(void)
{
    int maxwidth, w, i;

    maxwidth = 0;
    for (i = 0 ; i < sectioncount ; ++i) {
        SDL_QueryTexture(sections[i].titletexture, NULL, NULL, &w, NULL);
        if (maxwidth < w)
            maxwidth = w;
    }
    if (maxwidth <= listwidth)
        return FALSE;

    listwidth = maxwidth;
    setlayout(displaysize);
    return TRUE;
}

/*
 * Laying out text.
 */

/* Provide a buffer for temporarily holding strings. The argument
 * specifies the size of the string, not including the terminating NUL
 * byte. For code that just needs a sequence of temporary buffers,
 * this function avoids having to allocate and deallocate repeatedly.
 */
static char *getbuffer(int size)
{
    static char *buffer = NULL;
    static int buffersize = 0;

    if (size >= buffersize) {
        if (!buffersize)
            buffersize = 256;
        while (size >= buffersize)
            buffersize *= 2;
        buffer = reallocate(buffer, buffersize);
    }
    return buffer;
}

/* Given a section and a display width, apply word-wrapping to the
 * section's text using that width. The return value is an array
 * pointing to each substring making up one line of text. The size of
 * the array is returned through plinecount. The caller assumes
 * responsibility for freeing the returned array.
 *
 * The function iteratively copies words from a substring into a
 * separate buffer and measures its pixel width. If a substring
 * exceeds maxwidth, the previous substring measured is stored as one
 * line of display, and the function resumes scanning at the first
 * non-space character following. If a line exceeds the display width
 * before the first space character is found, the line will be broken
 * mid-word; otherwise, lines are always broken at spaces or line
 * breaks.
 */
static lineinfo *wraptext(sectioninfo const *section, int maxwidth,
                          int *plinecount)
{
    lineinfo *lines;
    char *buffer;
    char const *text;
    int linecount;
    int lastlen, pastfirst;
    int ch, i, w;

    text = section->text;
    lines = NULL;
    linecount = 0;

    while (*text) {
        lastlen = 0;
        if (*text != '\n') {
            i = 0;
            pastfirst = FALSE;
            for (;;) {
                ch = text[i];
                if (ch != ' ' && ch != '\n' && ch != '\0') {
                    buffer = getbuffer(i);
                    buffer[i] = ch;
                    ++i;
                    if (pastfirst)
                        continue;
                } else {
                    pastfirst = TRUE;
                }
                buffer = getbuffer(i);
                buffer[i] = '\0';
                TTF_SizeUTF8(_graph.smallfont, buffer, &w, NULL);
                if (w > maxwidth)
                    break;
                lastlen = i;
                if (ch == '\n' || ch == '\0')
                    break;
                if (pastfirst) {
                    while (text[i] == ' ') {
                        buffer = getbuffer(i);
                        buffer[i] = ' ';
                        ++i;
                    }
                }
            }
        }
        lines = reallocate(lines, (linecount + 1) * sizeof *lines);
        lines[linecount].offset = text - section->text;
        lines[linecount].len = lastlen;
        ++linecount;
        text += lastlen;
        if (*text == '\n')
            ++text;
        while (*text == ' ')
            ++text;
    }

    *plinecount = linecount;
    return lines;
}

/* Given a section that is formatted as a table, with newlines
 * separating rows and tabs separating columns, return an array of
 * substrings for each entry in the table, as well as an array giving
 * the required width in pixels of each column. The latter array is
 * returned through pwidths, and the size of the array is returned
 * through pcolumncount.
 *
 * The return value is an array of substrings and is organized into
 * columns instead of rows, so that the first elements of the array
 * list the entries in the leftmost column, followed by the entries
 * that make up the next column, etc. The caller assumes
 * responsibility for freeing both returned arrays.
 */
static lineinfo *tabulatetext(sectioninfo const *section, int **pwidths,
                              int *plinecount, int *pcolumncount)
{
    char const *text;
    lineinfo *lines;
    int *widths;
    char *buffer;
    int linecount, columncount, fullcount, i, n, w;

    text = section->text;
    linecount = 0;
    columncount = 1;
    n = 1;
    i = 0;
    for (;;) {
        if (text[i] == '\t') {
            ++n;
        } else if (text[i] == '\n' || text[i] == '\0') {
            ++linecount;
            if (columncount < n)
                columncount = n;
            n = 1;
            if (text[i] == '\0')
                break;
        }
        ++i;
    }
    fullcount = linecount * columncount;
    lines = allocate(fullcount * sizeof *lines);
    memset(lines, 0, fullcount * sizeof *lines);
    widths = allocate(columncount * sizeof *widths);
    memset(widths, 0, columncount * sizeof *widths);
    i = n = 0;
    for (;;) {
        if (text[i] == '\t' || text[i] == '\n' || text[i] == '\0') {
            lines[n].len = i - lines[n].offset;
            buffer = getbuffer(lines[n].len);
            memcpy(buffer, text + lines[n].offset, lines[n].len);
            buffer[lines[n].len] = '\0';
            TTF_SizeUTF8(_graph.smallfont, buffer, &w, NULL);
            if (widths[n / linecount] < w)
                widths[n / linecount] = w;
            if (text[i] == '\0')
                break;
            else if (text[i] == '\n')
                n = (n % linecount) + 1;
            else if (text[i] == '\t')
                n += linecount;
            lines[n].offset = i + 1;
        }
        ++i;
    }
    *pwidths = widths;
    *plinecount = linecount;
    *pcolumncount = columncount;
    return lines;
}

/*
 * Rendering text.
 */

/* Render the contents of a section into a texture, formatted as
 * paragraphs of running text. The resulting texture is stored in the
 * sectioninfo structure.
 */
static void makeparagraphtexture(sectioninfo *section)
{
    SDL_Surface *image;
    SDL_Surface *lineimage;
    lineinfo *lines;
    char *text;
    SDL_Rect rect;
    Uint32 color;
    int count, i;

    lines = wraptext(section, textrect.w, &count);
    image = NULL;
    text = NULL;
    for (i = 0 ; i < count ; ++i) {
        if (lines[i].len == 0)
            continue;
        text = reallocate(text, lines[i].len + 1);
        memcpy(text, section->text + lines[i].offset, lines[i].len);
        text[lines[i].len] = '\0';
        lineimage = TTF_RenderUTF8_Blended(_graph.smallfont, text,
                                           _graph.defaultcolor);
        if (!image) {
            image = SDL_CreateRGBSurface(0, textrect.w, count * lineheight,
                                         lineimage->format->BitsPerPixel,
                                         lineimage->format->Rmask,
                                         lineimage->format->Gmask,
                                         lineimage->format->Bmask,
                                         lineimage->format->Amask);
            color = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
            SDL_FillRect(image, NULL, color);
        }
        rect.x = 0;
        rect.y = i * lineheight;
        rect.w = lineimage->w;
        rect.h = lineimage->h;
        SDL_BlitSurface(lineimage, NULL, image, &rect);
        SDL_FreeSurface(lineimage);
    }
    deallocate(text);
    deallocate(lines);
    section->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    section->w = image->w;
    section->h = image->h;
    section->scrollpos = 0;
    SDL_FreeSurface(image);
}

/* Render the contents of a section into a texture, formatted as a
 * table. The function does not check that the rendered table will fit
 * into the available horizontal space. The resulting texture is
 * stored in the sectioninfo structure.
 */
static void maketabletexture(sectioninfo *section)
{
    SDL_Surface *image;
    SDL_Surface *lineimage;
    lineinfo *lines;
    int *widths;
    char *text;
    SDL_Rect rect;
    Uint32 color;
    int linecount, columncount, i;

    lines = tabulatetext(section, &widths, &linecount, &columncount);
    image = NULL;
    text = NULL;
    rect.x = 0;
    rect.y = 0;
    for (i = 0 ; i < linecount * columncount ; ++i) {
        text = reallocate(text, lines[i].len + 1);
        memcpy(text, section->text + lines[i].offset, lines[i].len);
        text[lines[i].len] = '\0';
        lineimage = TTF_RenderUTF8_Blended(_graph.smallfont, text,
                                           _graph.defaultcolor);
        if (!image) {
            image = SDL_CreateRGBSurface(0, textrect.w, linecount * lineheight,
                                         lineimage->format->BitsPerPixel,
                                         lineimage->format->Rmask,
                                         lineimage->format->Gmask,
                                         lineimage->format->Bmask,
                                         lineimage->format->Amask);
            color = SDL_MapRGB(image->format, colors3(_graph.bkgndcolor));
            SDL_FillRect(image, NULL, color);
        }
        rect.w = lineimage->w;
        rect.h = lineimage->h;
        SDL_BlitSurface(lineimage, NULL, image, &rect);
        SDL_FreeSurface(lineimage);
        if (i % linecount == linecount - 1) {
            rect.x += widths[i / linecount] + 2 * _graph.margin;
            rect.y = 0;
        } else {
            rect.y += lineheight;
        }
    }
    deallocate(text);
    deallocate(lines);
    deallocate(widths);
    section->texture = SDL_CreateTextureFromSurface(_graph.renderer, image);
    section->w = image->w;
    section->h = image->h;
    section->scrollpos = 0;
    SDL_FreeSurface(image);
}

/* Render the section's text into a single texture.
 */
static void makesectiontexture(sectioninfo *section)
{
    if (strchr(section->text, '\t'))
        maketabletexture(section);
    else
        makeparagraphtexture(section);
}

/*
 * Rendering the display.
 */

/* Draw all the elements of the help display. Draw the box outlines,
 * populate the boxes with content, and add the scroll bar as needed.
 */
static void render(void)
{
    SDL_Rect rect;
    int i;

    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.bkgndcolor));
    SDL_RenderClear(_graph.renderer);
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.dimmedcolor));
    SDL_RenderDrawLines(_graph.renderer, listoutline,
                        sizeof listoutline / sizeof *listoutline);
    SDL_RenderDrawLines(_graph.renderer, textoutline,
                        sizeof textoutline / sizeof *textoutline);
    SDL_SetRenderDrawColor(_graph.renderer, colors4(_graph.defaultcolor));

    rect.x = listrect.x;
    rect.y = listrect.y;
    for (i = 0 ; i < sectioncount ; ++i) {
        SDL_QueryTexture(sections[i].titletexture,
                         NULL, NULL, &rect.w, &rect.h);
        if (i == currentsection)
            SDL_SetTextureColorMod(sections[i].titletexture,
                                   colors3(_graph.highlightcolor));
        else
            SDL_SetTextureColorMod(sections[i].titletexture,
                                   colors3(_graph.defaultcolor));
        SDL_RenderCopy(_graph.renderer, sections[i].titletexture, NULL, &rect);
        rect.y += lineheight;
    }

    if (currentsection < 0)
        return;

    if (!sections[currentsection].texture) {
        makesectiontexture(&sections[currentsection]);
        scroll.range = sections[currentsection].h - textrect.h;
        scroll.value = 0;
    }

    if (sections[currentsection].h <= textrect.h) {
        rect.x = textrect.x;
        rect.y = textrect.y;
        rect.w = textrect.w;
        rect.h = sections[currentsection].h;
        SDL_RenderCopy(_graph.renderer,
                       sections[currentsection].texture, NULL, &rect);
    } else {
        rect.x = 0;
        rect.y = scroll.value;
        rect.w = textrect.w;
        rect.h = textrect.h;
        SDL_RenderCopy(_graph.renderer,
                       sections[currentsection].texture, &rect, &textrect);
    }
    scrollrender(&scroll);
}

/*
 * Managing user input.
 */

/* Scroll the current text by a relative amount. If useanim is true,
 * use animation to keep the movement smooth. The return value is
 * cmd_redraw or cmd_none, depending on whether or not a visible
 * change took place.
 */
static command_t scrolltext(int delta, int useanim)
{
    animinfo *anim;
    int value;

    value = scroll.value + delta;
    if (value < 0)
        value = 0;
    if (value >= scroll.range)
        value = scroll.range - 1;
    if (value == scroll.value)
        return cmd_none;
    if (!useanim) {
        scroll.value = value;
        return cmd_redraw;
    }

    anim = getaniminfo();
    anim->steps = 8;
    anim->pval1 = &scroll.value;
    anim->destval1 = value;
    anim->pval2 = NULL;
    anim->callback = NULL;
    startanimation(anim, 10);
    return cmd_redraw;
}

/* Change the section currently being displayed. The return value is
 * cmd_none if the given section is already current, and cmd_redraw
 * otherwise.
 */
static command_t setcurrentsection(int index)
{
    if (index == currentsection)
        return cmd_none;

    sections[currentsection].scrollpos = scroll.value;
    currentsection = index < 0 ? 0 :
                     index >= sectioncount ? sectioncount - 1 : index;
    if (sections[currentsection].texture) {
        scroll.range = sections[currentsection].h - textrect.h;
        scroll.value = sections[currentsection].scrollpos;
    }
    return cmd_redraw;
}

/* Change the displayed section relative to the currently displayed
 * section. If wraparound is true, then the current section index
 * rotates through the possible values, wrapping around on either end.
 * Otherwise, the current section index is clamped on either end.
 */
static command_t changesection(int delta, int wraparound)
{
    int index;

    if (wraparound) {
        index = (currentsection + delta + sectioncount) % sectioncount;
    } else {
        index = currentsection + delta;
        if (index < 0 || index >= sectioncount)
            return cmd_none;
    }
    return setcurrentsection(index);
}

/* Handle keyboard input for the help display. The return value is
 * cmd_none if the key has no defined action (or did not change the
 * appearance of the help display), or cmd_redraw otherwise.
 */
static command_t applykeycmd(SDL_Keysym keysym)
{
    switch (keysym.sym) {
      case SDLK_UP:             return scrolltext(-lineheight, TRUE);
      case SDLK_DOWN:           return scrolltext(+lineheight, TRUE);
      case SDLK_PAGEUP:         return scrolltext(-scroll.pagesize, TRUE);
      case SDLK_PAGEDOWN:       return scrolltext(+scroll.pagesize, TRUE);
      case SDLK_HOME:           return scrolltext(-scroll.range, TRUE);
      case SDLK_END:            return scrolltext(+scroll.range, TRUE);
      case SDLK_TAB:
        return changesection(keysym.mod & KMOD_SHIFT ? -1 : +1, TRUE);
    }
    return cmd_none;
}

/* Filter the input events for interactions with the help display. In
 * addition to keyboard commands, the mouse can be used to change the
 * current section, and the mouse wheel can be used to scroll either
 * the text or the selected section.
 */
static command_t eventhandler(SDL_Event *event)
{
    int x, y;

    if (scrolleventhandler(event, &scroll))
        return cmd_redraw;
    switch (event->type) {
      case SDL_KEYDOWN:
        return applykeycmd(event->key.keysym);
      case SDL_MOUSEBUTTONDOWN:
        if (rectcontains(&listrect, event->button.x, event->button.y)) {
            y = (event->button.y - listrect.y) / lineheight;
            if (y >= 0 && y < sectioncount)
                return setcurrentsection(y);
        }
        break;
      case SDL_MOUSEWHEEL:
        SDL_GetMouseState(&x, &y);
        if (rectcontains(&listrect, x, y))
            return changesection(event->wheel.y > 0 ? -1 : +1, FALSE);
        if (rectcontains(&textrect, x, y))
            return scrolltext(-lineheight * event->wheel.y, FALSE);
        break;
    }
    return cmd_none;
}

/*
 * Internal function.
 */

/* Initialize resources and return the help display's displaymap.
 */
displaymap inithelpdisplay(int displayid)
{
    displaymap display;

    makeimagebutton(&backbutton, IMAGE_BACK);
    backbutton.display = displayid;
    backbutton.cmd = cmd_quit;
    addbutton(&backbutton);

    display.setlayout = setlayout;
    display.render = render;
    display.eventhandler = eventhandler;
    return display;
}

/*
 * API function.
 */

/* Add a new topic to the help display. The function first looks to
 * see if the title is replacing (or removing) an existing section. If
 * not, then the section is inserted.
 */
void sdlui_addhelpsection(char const *title, char const *text, int placefirst)
{
    int i;

    for (i = 0 ; i < sectioncount ; ++i) {
        if (!strcmp(title, sections[i].title)) {
            updatehelpsection(i, text);
            updatelistwidth();
            return;
        }
    }
    if (text) {
        inserthelpsection(title, text, placefirst);
        updatelistwidth();
        if (currentsection < 0)
            currentsection = 0;
    }
}
