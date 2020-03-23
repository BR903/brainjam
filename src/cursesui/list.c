/* cursesui/list.c: the list selection display.
 */

#include <stdlib.h>
#include <ncurses.h>
#include "./gen.h"
#include "./glyphs.h"
#include "./decks.h"
#include "answers/answers.h"
#include "game/game.h"
#include "internal.h"

/* The help text for the game selection display. Unlike the other help
 * sections, this text is only made available when the user enters
 * help from this display.
 */
static char const *listhelptitle = "Selection Key Commands";
static char const *listhelptext =
    "Select a game              Ret or Spc\n"
    "Scroll selection           " GLYPH_UPARROW " " GLYPH_DOWNARROW " \n"
    "Scroll one screen's worth  PgUp PgDn\n"
    "Scroll to top              Home\n"
    "Scroll to bottom           End\n"
    "Scroll to next unsolved    Tab\n"
    "Scroll to prev unsolved    Shift-Tab\n"
    "Scroll to random           Ctrl-R\n"
    "Display this help          ? or F1\n"
    "Quit the program           Q";

/* Placement of the list display elements.
 */
static int const titlelinex = 11;       /* x-coordinate of the screen title */
static int const titleliney = 0;        /* y-coordinate of the screen title */
static int const gamelistx = 8;         /* coordinates of the game list's */
static int const gamelisty = 2;         /*    top-left corner */
static int const directionsx = 48;      /* coordinates of the temporary help */
static int const directionsy = 10;      /*    box's top-left corner */
static int const pageheight = 21;       /* lines in the list, sans header */

/*
 * Rendering the selection list.
 */

/* Render a subset of game IDs as a list, starting at first and
 * showing count entries, with selected indicating the ID to
 * highlight. For games that have an answer recorded, the size of the
 * answer is shown, along with the shortest answer size possible. (If
 * the user's answer size is already the shortest, then it is rendered
 * in dim text.)
 */
static void drawgamelist(int first, int count, int selected)
{
    answerinfo const *answer;
    int id, best, i;

    move(gamelisty, gamelistx);
    textmode(MODEID_DARKER);
    if (getanswercount() > 0)
        addstr("Game    Moves    Best");
    else
        addstr("Select a Game");
    textmode(MODEID_NORMAL);

    answer = getnearestanswer(first);
    for (i = 0 ; i < count ; ++i) {
        id = first + i;
        move(gamelisty + 1 + i, gamelistx);
        if (id == selected)
            textmode(MODEID_SELECTED);
        if (answer) {
            best = bestknownanswersize(id);
            if (id != selected)
                if (answer->id == id && answer->size == best)
                    textmode(MODEID_DIMMED);
            printw("%04d", id);
            if (answer->id == id) {
                printw("%8d%9d", answer->size, best);
                answer = getnextanswer(answer);
            }
        } else {
            printw("%04d", id);
        }
        textmode(MODEID_NORMAL);
    }
}

/* Render the title of the selection list. If it appears that the user
 * is still relatively new to the program, add some helpful text
 * explaining what to do here.
 */
static void drawlisttext(void)
{
    move(titleliney, titlelinex);
    textmode(MODEID_TITLE);
    addstr("B R A I N J A M");
    textmode(MODEID_NORMAL);
    if (getanswercount() == 0) {
        mvaddstr(directionsy + 0, directionsx, "Welcome to Brain Jam.");
        mvaddstr(directionsy + 1, directionsx, "Select one of the games");
        mvaddstr(directionsy + 2, directionsx, "from the list and press Ret");
        mvaddstr(directionsy + 3, directionsx, "to begin playing.");
        mvaddstr(directionsy + 5, directionsx, "Press ? or F1 to view help.");
    } else if (getanswercount() < 3) {
        mvaddstr(directionsy + 0, directionsx, "The middle column shows");
        mvaddstr(directionsy + 1, directionsx, "the number of moves in");
        mvaddstr(directionsy + 2, directionsx, "your answer. The");
        mvaddstr(directionsy + 3, directionsx, "right column shows the");
        mvaddstr(directionsy + 4, directionsx, "number of moves in the");
        mvaddstr(directionsy + 5, directionsx, "best possible answer.");
    }
}

/*
 * Running the list display.
 */

/* Return the entry in the (visible) game list that was clicked on, or
 * -1 if the most recent mouse event did not represent a selection of
 * a list entry.
 */
static int findmouseselection(void)
{
    MEVENT event;

    if (getmouse(&event) != OK)
        return -1;
    if (!(event.bstate & BUTTON1_CLICKED))
        return -1;
    if (event.y <= gamelisty || event.y >= gamelisty + pageheight)
        return -1;
    if (event.x < gamelistx || event.x >= directionsx)
        return -1;
    return event.y - gamelisty - 1;
}

/* Display a scrollable list of games, initially centered on a chosen
 * game, and manage the I/O for moving around and selecting an entry.
 * The return value is the game ID that was selected by the user, or
 * -1 if the user opted to exit the program instead.
 */
static int runselectionloop(int selected)
{
    int total, top, n;

    total = getdeckcount();
    for (;;) {
        if (selected < 0)
            selected = 0;
        if (selected >= total)
            selected = total - 1;
        top = selected - pageheight / 2;
        if (top < 0)
            top = 0;
        if (top > total - pageheight)
            top = total - pageheight;
        if (validatesize()) {
            erase();
            drawgamelist(top, pageheight, selected);
            drawlisttext();
            move(23, 78);
            refresh();
        }
        switch (getkey()) {
          case KEY_UP:      --selected;                                 break;
          case KEY_DOWN:    ++selected;                                 break;
          case KEY_PPAGE:   selected = top - 1;                         break;
          case KEY_NPAGE:   selected = top + pageheight;                break;
          case KEY_HOME:    selected = 0;                               break;
          case KEY_END:     selected = total;                           break;
          case '\t':        selected = findnextunsolved(selected, +1);  break;
          case KEY_BTAB:    selected = findnextunsolved(selected, -1);  break;
          case '\b':        selected = findnextunsolved(selected, -1);  break;
          case '\022':      selected = pickrandomunsolved();            break;
          case ' ':         return selected;
          case '\n':        return selected;
          case 'q':         return -1;
          case 'Q':         return -1;
          case '?':
            if (!runhelp(listhelptitle))
                return -1;
            break;
          case KEY_MOUSE:
            n = findmouseselection();
            if (n >= 0)
                return top + n;
            break;
        }
    }
}

/*
 * API functions.
 */

/* A wrapper around runselectionloop() that adds and then removes the
 * relevant help topic.
 */
int cursesui_selectgame(int gameid)
{
    cursesui_addhelpsection(listhelptitle, listhelptext, TRUE);
    gameid = runselectionloop(gameid);
    cursesui_addhelpsection(listhelptitle, NULL, TRUE);
    return gameid;
}
