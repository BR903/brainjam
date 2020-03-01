/* curses/game.c: the game display.
 */

#include <string.h>
#include <time.h>
#include <ncurses.h>
#include "./types.h"
#include "./commands.h"
#include "./decks.h"
#include "./ui.h"
#include "redo/redo.h"
#include "game/game.h"
#include "internal.h"

/* The placement of the various elements of the game display.
 */
static int const foundationx = 0;       /* left edge of the foundations */
static int const tableaux = 2;          /* left edge of the tableau stacks */
static int const reservex = 36;         /* left edge of the reserves */
static int const toprowy = 1;           /* y-coordinate of the top row */
static int const tableauy = 4;          /* top edge of the tableau stacks */
static int const cardspacingx = 8;      /* horizontal space between cards */
static int const rightcolumnx = 70;     /* left edge of information column */
static int const bottomareay = 19;      /* top edge of bottom-right corner */

/* If nonzero, indicates the time at which the save indicator should
 * stop being visible.
 */
static time_t saveiconshown = 0;

/* True if the move key guides should be displayed.
 */
static int showkeyguides;

/*
 * Input management.
 */

/* Map a mouse click event onto the location of the cards in the
 * layout, and return the appropriate move command. The return value
 * is cmd_nop if the mouse event is not located at a moveable place.
 */
static command_t translatemouseinput(void)
{
    MEVENT event;
    place_t place;
    int usefirst, n;

    if (getmouse(&event) != OK)
        return cmd_nop;

    if (event.bstate & BUTTON2_CLICKED)
        usefirst = FALSE;
    else if (event.bstate & BUTTON1_CLICKED)
        usefirst = !(event.bstate & BUTTON_SHIFT);
    else
        return cmd_nop;

    if (event.y >= toprowy && event.y < tableauy) {
        n = (event.x - reservex) / cardspacingx;
        if (n < 0 || n >= RESERVE_PLACE_COUNT)
            return cmd_nop;
        place = reserveplace(n);
    } else if (event.y >= tableauy && event.y < tableauy + MAX_TABLEAU_DEPTH) {
        n = (event.x - tableaux) / cardspacingx;
        if (n < 0 || n >= TABLEAU_PLACE_COUNT)
            return cmd_nop;
        place = tableauplace(n);
    } else {
        return cmd_nop;
    }

    return usefirst ? placetomovecmd1(place) : placetomovecmd2(place);
}

/* Map keyboard input events to user commands. Any normal key event
 * not mapped to a special command value (such as move commands) is
 * returned unchanged.
 */
static command_t translategameinput(int ch)
{
    switch (ch) {
      case KEY_MOUSE:       return translatemouseinput();
      case ' ':             return cmd_autoplay;
      case '\n':            return cmd_autoplay;
      case '\032':          return cmd_undo;
      case '\031':          return cmd_redo;
      case KEY_LEFT:        return cmd_undo;
      case KEY_RIGHT:       return cmd_redo;
      case KEY_PPAGE:       return cmd_undo10;
      case KEY_NPAGE:       return cmd_redo10;
      case KEY_UP:          return cmd_undotobranch;
      case KEY_DOWN:        return cmd_redotobranch;
      case '\b':            return cmd_erase;
      case KEY_HOME:        return cmd_jumptostart;
      case KEY_END:         return cmd_jumptoend;
      case '=':             return cmd_switchtobetter;
      case '-':             return cmd_switchtoprevious;
      case 'M':             return cmd_pushbookmark;
      case 'P':             return cmd_dropbookmark;
      case 'R':             return cmd_popbookmark;
      case 'S':             return cmd_swapbookmark;
      case '!':             return cmd_setminimalpath;
      case '\017':          return cmd_changesettings;
      case '?':             return cmd_showhelp;
      case 'q':             return cmd_quit;
      case 'Q':             return cmd_quitprogram;
    }
    return (ch < 256) ? ch : cmd_nop;
}

/*
 * Rendering the game display.
 */

/* Return true if there are no empty slots in the current layout, and
 * so it would be helpful to mark which cards can actually be moved.
 */
static int shouldmarkmoveable(gameplayinfo const *gameplay)
{
    int i;

    for (i = MOVEABLE_PLACE_1ST ; i < MOVEABLE_PLACE_END ; ++i)
        if (gameplay->depth[i] == 0)
            return FALSE;
    return TRUE;
}

/* Output the representation for the given card. emptymode indicates
 * the text mode to use when no card is present.
 */
static void drawcard(int card, int emptymode)
{
    static char const *ranks[] = {
        "  ", "A ", "2 ", "3 ", "4 ", "5 ", "6 ",
        "7 ", "8 ", "9 ", "10", "J ", "Q ", "K "
    };
    static char const *suits[NSUITS] = {
        "\342\231\243", "\342\231\246", "\342\231\245", "\342\231\240"
    };
    static int const suitmodes[NSUITS] = {
        MODEID_BLACKCARD, MODEID_REDCARD, MODEID_REDCARD, MODEID_BLACKCARD
    };

    int r, s;

    r = card_rank(card);
    s = card_suit(card);
    if (r > 0) {
        textmode(suitmodes[s]);
        printw(" %s%s ", ranks[r], suits[s]);
    } else {
        textmode(emptymode);
        addstr("     ");
    }
    textmode(MODEID_NORMAL);
}

/* Output markers showing moves from the given place. If showmoveable
 * is true, a dot is added when the card at this place can be moved.
 */
static void drawnavinfo(gameplayinfo const *gameplay,
                        redo_position const *position,
                        int place, int showmoveable)
{
    redo_position const *unshiftpos;
    redo_position const *shiftpos;
    redo_branch const *branch;

    unshiftpos = NULL;
    shiftpos = NULL;
    for (branch = position->next ; branch ; branch = branch->cdr) {
        if (branch->move == cardtomoveid1(gameplay->inplay[place]))
            unshiftpos = branch->p;
        else if (branch->move == cardtomoveid2(gameplay->inplay[place]))
            shiftpos = branch->p;
    }

    if (unshiftpos) {
        if (unshiftpos->solutionsize)
            printw("%3d", unshiftpos->solutionsize);
        else
            printw(" %c ", placetomovecmd1(place));
    } else {
        addstr("   ");
    }
    if (showmoveable)
        showmoveable = gameplay->moveable & (1 << place);
    addstr(showmoveable ? "\342\200\242" : " ");
    if (shiftpos) {
        if (shiftpos->solutionsize)
            printw("%-3d", shiftpos->solutionsize);
        else
            printw(" %c ", placetomovecmd2(place));
    } else {
        addstr("   ");
    }
}

/* Output an indicator of whether a better seequence of moves exists
 * for the current position.
 */
static void drawbetterinfo(redo_position const *position)
{
    char buf[8];
    redo_position const *bp;
    int isbetter;

    if (!position || !position->better)
        return;

    for (bp = position ; bp->better ; bp = bp->better) ;
    if (bp->movecount < position->movecount) {
        isbetter = TRUE;
    } else if (bp->movecount == position->movecount && bp->solutionsize) {
        isbetter = !position->solutionsize ||
                        bp->solutionsize < position->solutionsize;
    } else {
        isbetter = FALSE;
    }
    if (isbetter) {
        if (!bp->solutionsize)
            textmode(MODEID_DIMMED);
        sprintf(buf, "= %d", bp->movecount);
        printw("%6s", buf);
        if (!bp->solutionsize)
            textmode(MODEID_NORMAL);
    }
}

/* Render the game display. At the top are placed the foundations and
 * the reserves, and below this is the main tableau of the layout.
 * After the cards are drawn, information about the game state is
 * placed in the right-hand column.
 */
static void drawgamedisplay(gameplayinfo const *gameplay,
                            redo_position const *position, int bookmark)
{
    card_t card;
    int showmoveable;
    int i, y;

    showmoveable = shouldmarkmoveable(gameplay);

    erase();
    for (i = 0 ; i < FOUNDATION_PLACE_COUNT ; ++i) {
        move(toprowy, foundationx + i * cardspacingx);
        drawcard(gameplay->inplay[foundationplace(i)], MODEID_FOUNDATION);
    }
    for (i = 0 ; i < RESERVE_PLACE_COUNT ; ++i) {
        move(toprowy, reservex + i * cardspacingx);
        drawcard(gameplay->inplay[reserveplace(i)], MODEID_RESERVE);
        move(toprowy + 1, reservex - 1 + i * cardspacingx);
        drawnavinfo(gameplay, position, reserveplace(i), showmoveable);
        if (showkeyguides)
            mvaddch(toprowy - 1, reservex + i * cardspacingx + 2,
                    placetomovecmd2(reserveplace(i)));
    }
    for (i = 0 ; i < TABLEAU_PLACE_COUNT ; ++i) {
        y = gameplay->depth[tableauplace(i)];
        move(tableauy + y, tableaux + i * cardspacingx - 1);
        drawnavinfo(gameplay, position, tableauplace(i), showmoveable);
        card = gameplay->inplay[tableauplace(i)];
        while (y--) {
            move(tableauy + y, tableaux + i * cardspacingx);
            drawcard(card, MODEID_NORMAL);
            card = gameplay->state[cardtoindex(card)];
        }
        if (showkeyguides)
            mvaddch(tableauy - 1, tableaux + i * cardspacingx + 2,
                    placetomovecmd2(tableauplace(i)));
    }

    textmode(MODEID_HIGHLIGHT);
    mvprintw(toprowy, rightcolumnx, "%5d", position->movecount);
    textmode(MODEID_NORMAL);
    move(toprowy + 1, rightcolumnx);
    drawbetterinfo(position);
    if (gameplay->endpoint)
        mvaddstr(toprowy + 2, rightcolumnx, " done");
    else if (!gameplay->moveable)
        mvaddstr(toprowy + 2, rightcolumnx, "stuck");
    if (bookmark)
        mvaddstr(toprowy + 3, rightcolumnx, "mark ");

    if (saveiconshown) {
        if (time(NULL) > saveiconshown)
            saveiconshown = 0;
        else
            mvaddstr(bottomareay, rightcolumnx, "  save");
    }
    if (gameplay->bestsolution) {
        mvprintw(bottomareay + 1, rightcolumnx + 3, "%4d",
                 gameplay->bestsolution);
        mvprintw(bottomareay + 2, rightcolumnx + 3, "%4d",
                 bestknownsolutionsize(gameplay->gameid));
    }
    mvprintw(bottomareay + 4, rightcolumnx - 2,
             "Game %04d ", gameplay->gameid);

    refresh();
}

/*
 * API functions.
 */

/* Display the game in its current state, along with navigational
 * indicators as supplied by the given redo position.
 */
void curses_rendergame(renderparams const *params)
{
    if (validatesize())
        drawgamedisplay(params->gameplay, params->position, params->bookmark);
}

/* Retrieve a single key event. Commands to view help and redraw the
 * display are handled automatically; everything else is passed back
 * up to the caller.
 */
command_t curses_getinput(void)
{
    command_t cmd;

    cmd = translategameinput(getkey());
    if (cmd == cmd_showhelp)
        cmd = runhelp(NULL) ? cmd_nop : cmd_quitprogram;
    return cmd;
}

/* Enable or disable the display of move key guides.
 */
int curses_setshowkeyguidesflag(int flag)
{
    showkeyguides = flag;
    return flag;
}

/* Animations are not available in this UI.
 */
int curses_setcardanimationflag(int flag)
{
    (void)flag;
    return FALSE;
}

/* Animations are not available in this UI, so this function simply
 * invokes the callback directly.
 */
void curses_movecard(card_t card, position_t from, position_t to,
                     void (*callback)(void*), void *data)
{
    (void)card;
    (void)from;
    (void)to;
    callback(data);
}

/* Display a temporary indicator that the solution file has been
 * updated.
 */
void curses_showsolutionwrite(void)
{
    saveiconshown = time(NULL) + 2;
}
