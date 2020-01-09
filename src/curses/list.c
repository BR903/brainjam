/* curses/list.c: the list selection display.
 */

#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include "./gen.h"
#include "configs/configs.h"
#include "solutions/solutions.h"
#include "game/game.h"
#include "internal.h"

/* The help text for the configuration selection display. Unlike the
 * other help sections, this text is only made available when the user
 * enters help from this display.
 */
static char const *listhelptitle = "Selection Key Commands";
static char const *listhelptext =
    "Select a configuration     Ret or Spc\n"
    "Scroll selection           \342\206\221 \342\206\223\n"
    "Scroll one screen's worth  PgUp PgDn\n"
    "Scroll to top              Home\n"
    "Scroll to bottom           End\n"
    "Scroll to next unsolved    Tab\n"
    "Scroll to prev unsolved    Shift-Tab\n"
    "Scroll to random           Ctrl-R\n"
    "Display this help          ? or F1\n"
    "Quit the program           Q";

/* Placement of the list display elements, using an 80x24 active area.
 */
static int const titlelinex = 11;       /* x-coordinate of the screen title */
static int const titleliney = 0;        /* y-coordinate of the screen title */
static int const configlistx = 8;       /* coordinates of the configuration */
static int const configlisty = 2;       /*    list's top-left corner */
static int const directionsx = 48;      /* coordinates of the temporary help */
static int const directionsy = 10;      /*    box's top-left corner */
static int const pageheight = 21;       /* lines in the list, sans header */

/*
 * Rendering the selection list.
 */

/* Render a subset of configuration IDs as a list, starting at first
 * and showing count entries, with selected indicating the ID to
 * highlight. For configurations that have a solution recorded, the
 * size of the solution is shown, along with the shortest solution
 * size possible. (If the user's solution size is already the
 * shortest, then it is rendered in dim text.)
 */
static void drawconfiglist(int first, int count, int selected)
{
    solutioninfo const *solution;
    int id, best, i;

    move(configlisty, configlistx);
    textmode(MODEID_MARKED);
    if (getsolutioncount() > 0)
        addstr("Game    Moves    Best");
    else
        addstr("Select a Game");
    textmode(MODEID_NORMAL);

    solution = getnearestsolution(first);
    for (i = 0 ; i < count ; ++i) {
        id = first + i;
        move(configlisty + 1 + i, configlistx);
        if (id == selected)
            textmode(MODEID_SELECTED);
        if (solution) {
            best = bestknownsolutionsize(id);
            if (id != selected)
                if (solution->id == id && solution->size == best)
                    textmode(MODEID_DIMMED);
            printw("%04d", id);
            if (solution->id == id) {
                printw("%8d%9d", solution->size, best);
                solution = getnextsolution(solution);
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
    if (getsolutioncount() == 0) {
        mvaddstr(directionsy + 0, directionsx, "Welcome to Brain Jam.");
        mvaddstr(directionsy + 1, directionsx, "Select one of the");
        mvaddstr(directionsy + 2, directionsx, "configurations from the");
        mvaddstr(directionsy + 3, directionsx, "list and press Ret");
        mvaddstr(directionsy + 4, directionsx, "to begin playing.");
        mvaddstr(directionsy + 6, directionsx, "Press ? or F1 to view help.");
    } else if (getsolutioncount() < 3) {
        mvaddstr(directionsy + 0, directionsx, "The middle column shows");
        mvaddstr(directionsy + 1, directionsx, "the number of moves in");
        mvaddstr(directionsy + 2, directionsx, "your solution. The");
        mvaddstr(directionsy + 3, directionsx, "right column shows the");
        mvaddstr(directionsy + 4, directionsx, "number of moves in the");
        mvaddstr(directionsy + 5, directionsx, "best possible solution.");
    }
}

/*
 * Running the list display.
 */

/* Output a scrollable list of configurations, initially centered on a
 * chosen configuration, and manage the I/O for moving around and
 * selecting an entry. The return value is the configuration selected
 * by the user, or -1 if the user opted to exit the program instead.
 */
static int runselectionloop(int selected)
{
    int total;
    int top;

    total = getconfigurationcount();
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
            drawconfiglist(top, pageheight, selected);
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
        }
    }
}

/*
 * API functions.
 */

/* A wrapper around runselectionloop() that adds and then removes the
 * relevant help topic.
 */
int curses_selectconfig(int configid)
{
    curses_addhelpsection(listhelptitle, listhelptext, TRUE);
    configid = runselectionloop(configid);
    curses_addhelpsection(listhelptitle, NULL, TRUE);
    return configid;
}