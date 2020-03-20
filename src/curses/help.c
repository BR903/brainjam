/* curses/help.c: the help reader display.
 */

#include <string.h>
#include <ncurses.h>
#include "./gen.h"
#include "./glyphs.h"
#include "internal.h"

/* The data required to properly display each section of help text.
 */
typedef struct sectioninfo {
    char const *title;          /* the title of the section */
    char const *text;           /* the text of this section */
    int         linecount;      /* the number of lines the text requires */
    int         topline;        /* the current first visible line */
} sectioninfo;

/* Placement of the help display elements.
 */
static int const helptexty = 2;         /* top line of help text */
static int const maxlinewidth = 76;     /* maximum line length for help text */
static int const pageheight = 21;       /* lines of text plus section list */

/* The current list of sections of help.
 */
static sectioninfo *sections = NULL;
static int sectioncount = 0;

/* The section currently being displayed.
 */
static int currentsection = -1;

/*
 * Managing the sections of help text.
 */

/* Count the number of lines a given text takes up when word-wrapped.
 */
static int countlines(char const *text)
{
    char const *p;
    int count, n;

    if (!text || !*text)
        return 0;
    count = 0;
    for (p = text ; *p ; p += n) {
        n = textbreak(&p, maxlinewidth);
        ++count;
    }
    return count;
}

/* Replace the text of the section at index. If text is NULL, then the
 * section is removed instead.
 */
static void updatehelpsection(int index, char const *text)
{
    if (text) {
        sections[index].text = text;
        sections[index].linecount = countlines(text);
        sections[index].topline = 0;
    } else {
        memmove(sections + index, sections + index + 1,
                (sectioncount - index - 1) * sizeof *sections);
        --sectioncount;
        if (currentsection >= sectioncount)
            currentsection = sectioncount - 1;
    }
}

/* Add a new section, appending it to the array of sections. (Or
 * prepending it, if putfirst is true.) If title is the title of a
 * section that already exists, then the original section is replaced
 * instead. If text is NULL, then nothing is added, and any previously
 * added section is removed.
 */
static void sethelpsection(char const *title, char const *text, int putfirst)
{
    int i, n;

    for (i = 0 ; i < sectioncount ; ++i) {
        if (!strcmp(title, sections[i].title)) {
            updatehelpsection(i, text);
            return;
        }
    }
    if (!text)
        return;

    sections = reallocate(sections, (sectioncount + 1) * sizeof *sections);
    if (putfirst) {
        memmove(sections + 1, sections, sectioncount * sizeof *sections);
        n = 0;
    } else {
        n = sectioncount;
    }
    sections[n].title = title;
    sections[n].text = text;
    sections[n].linecount = countlines(text);
    sections[n].topline = 0;
    ++sectioncount;
    if (currentsection < 0)
        currentsection = 0;
}

/*
 * Rendering the help display.
 */

/* Render the text of the current section, starting at the current
 * line. (The cursor is assumed to be in the far left column when this
 * function is called.) If the text too long to fit within pagesize
 * lines, then the section's topline field is used to determine how
 * far into the text to begin displaying, and a scroll thumb is
 * rendered to the left of the text.
 */
static void drawhelptext(sectioninfo *section, int pagesize)
{
    char const *p;
    int thumbpos, thumbsize;
    int start, i, n;

    p = section->text;
    if (section->linecount <= pagesize) {
        start = 0;
        thumbpos = -1;
        thumbsize = 0;
    } else {
        start = section->topline;
        if (start > section->linecount - pagesize)
            start -= section->linecount - pagesize;
        if (start < 0)
            start = 0;
        thumbsize = 2 * pagesize - section->linecount;
        if (thumbsize >= 1) {
            thumbpos = start;
        } else {
            thumbsize = 1;
            n = section->linecount - pagesize;
            thumbpos = (int)(((float)start / n) * (pagesize - 1));
        }
    }
    for (i = -start ; *p && i < pagesize ; ++i) {
        n = textbreak(&p, maxlinewidth);
        if (i >= 0) {
            if (i >= thumbpos && i < thumbpos + thumbsize) {
                textmode(MODEID_DIMMED);
                addstr(GLYPH_BLOCK " ");
                textmode(MODEID_NORMAL);
            } else {
                addstr("  ");
            }
            addnstr(p, n);
            addch('\n');
        }
        p += n;
    }
}

/* Render the list of help topics at the current cursor position,
 * arranged into two columns. This function assumes that there is
 * enough room on the display for all titles to be rendered.
 */
static void drawhelpsections(void)
{
    int half, i, n;

    half = (sectioncount + 1) / 2;
    textmode(MODEID_TITLE);
    addstr("Help Topics");
    textmode(MODEID_NORMAL);
    addch('\n');
    for (i = n = 0 ; i < sectioncount ; ++i) {
        n = i % 2 ? half + i / 2 : i / 2;
        if (n == currentsection) {
            textmode(MODEID_SELECTED);
            printw("%c - %-32s", '1' + n, sections[n].title);
            textmode(MODEID_NORMAL);
            addch(' ');
        } else {
            textmode(MODEID_DARKER);
            addch('1' + n);
            textmode(MODEID_NORMAL);
            printw(" - %-32s ", sections[n].title);
        }
        if (i % 2)
            addch('\n');
    }
}

/* Render the help display.
 */
static void drawhelpdisplay(int maxtextlines)
{
    erase();
    move(0, 0);
    textmode(MODEID_SELECTED);
    addstr(sections[currentsection].title);
    textmode(MODEID_NORMAL);
    move(helptexty, 0);
    drawhelptext(&sections[currentsection], maxtextlines);
    move(helptexty + maxtextlines + 1, 0);
    drawhelpsections();
    move(23, 78);
    refresh();
}

/*
 * Managing user input.
 */

/* Wait for a keypress. If the key is a scrolling command or a section
 * number, apply its effect and return zero. Otherwise, return the key.
 * that was pressed.
 */
static int processkey(int pagesize)
{
    sectioninfo *section;
    int ch;

    ch = getkey();
    if (ch >= '1' && ch < '1' + sectioncount) {
        currentsection = ch - '1';
        return 0;
    }
    section = &sections[currentsection];
    switch (ch) {
      case KEY_UP:    --section->topline;                       break;
      case KEY_DOWN:  ++section->topline;                       break;
      case KEY_PPAGE: section->topline -= pagesize;             break;
      case KEY_NPAGE: section->topline += pagesize;             break;
      case KEY_HOME:  section->topline = 0;                     break;
      case KEY_END:   section->topline = section->linecount;    break;
      case KEY_LEFT:  return 0;
      case KEY_RIGHT: return 0;
      default:        return ch;
    }
    if (section->topline < 0)
        section->topline = 0;
    if (section->topline > section->linecount - pagesize)
        section->topline = section->linecount - pagesize;
    return 0;
}

/*
 * Internal function.
 */

/* Display help text, allowing the user to switch between sections and
 * to scroll up and down through each section's text. The argument, if
 * it is not NULL, should be the title of a section to display first.
 */
int runhelp(char const *title)
{
    int maxtextlines;
    int ch, i;

    if (sectioncount == 0)
        return 1;

    for (i = 0 ; i < sectioncount ; ++i) {
        sections[i].topline = 0;
        if (title && !strcmp(title, sections[i].title))
            currentsection = i;
    }

    maxtextlines = pageheight - (sectioncount + 1) / 2;
    for (;;) {
        if (validatesize())
            drawhelpdisplay(maxtextlines);
        ch = processkey(maxtextlines);
        if (ch)
            break;
    }

    return ch != 'Q';
}

/*
 * API function.
 */

/* Add a topic to the list of sections.
 */
void curses_addhelpsection(char const *title, char const *text, int putfirst)
{
    sethelpsection(title, text, putfirst);
}
