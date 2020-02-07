/* curses/curses.c: the textual user interface, implemented with ncurses.
 */

#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include "./types.h"
#include "./settings.h"
#include "redo/types.h"
#include "curses/curses.h"
#include "internal.h"

/* The general description of the game's user interface.
 */
static char const *gameplayhelptitle = "How to Play";
static char const *gameplayhelptext =
    "To move a card, use a letter key corresponding to its current"
    " location. Cards at the bottom of a tableau column can be moved with the"
    " letter keys A through H, each letter referring to the eight columns"
    " going from left to right. Cards in the reserve can be moved with the"
    " letter keys I through L, again reading from left to right.\n"
    "\n"
    "When you press a letter key, the card will be moved if it has a legal"
    " move available. If the card has more than one legal move, the program"
    " will choose one. The program will generally prefer moves that maximize"
    " your options (for example, it will prefer to move a card onto a empty"
    " tableau column rather than an empty reserve, since the former can also"
    " be built upon).\n"
    "\n"
    "You can select an alternate destination for your move by holding down the"
    " Shift key when pressing the letter key (or presumably by using the Caps"
    " Lock key).\n"
    "\n"
    "These letters will also appear underneath cards after undoing a move, to"
    " indicate which move the redo command will execute. A move is"
    " represented as a lower case letter on the left-hand side, or as an"
    " uppercase letter on the right-hand side for a move to the alternate"
    "destination.\n"
    "\n"
    "As you are playing, the current number of moves is displayed in the top"
    " right corner.\n"
    "\n"
    "If at any time, no legal moves are available, a \"STUCK\" indicator will"
    " appear below the move count, and you will need to use undo in order to"
    " proceed. When you complete a game, a \"DONE\" indicator will appear"
    " instead. In this situation you can also use undo if you wish to try to"
    " improve on your solution.\n"
    "\n"
    "When you are playing a game that you have already solved, the number of"
    " moves in your solution will be displayed at bottom right, so you can see"
    " what you are trying to improve upon. Directly below that will be the"
    " number of moves in the shortest possible solution, so you can also see"
    " how much room there is for improvement.";

/* The list of the basic game key commands.
 */
static char const *commandshelptitle = "Key Commands";
static char const *commandshelptext =
    "Move top card from a tableau column       A B C D E F G H\n"
    "Move a reserve card                       I J K L\n"
    "Move card to alternate spot               shift-A ... shift-L\n"
    "Undo previous move                        ctrl-Z\n"
    "Redo next move                            ctrl-Y\n"
    "Undo to the starting position             Home\n"
    "Redo all undone moves                     End\n"
    "Return to the previously viewed position  \342\200\223 \n"
    "Redraw the screen                         ctrl-L\n"
    "Display the options menu                  ctrl-O\n"
    "Display this help                         ? or F1\n"
    "Quit and select a new game                Q\n"
    "Quit and exit the program                 shift-Q\n"
    "\n"
    "The following commands are available when branching redo is enabled:\n"
    "\n"
    "Undo previous move                        \342\206\220 \n"
    "Redo next move                            \342\206\222 \n"
    "Undo and forget previous move             Bkspc\n"
    "Undo previous 10 moves                    PgUp\n"
    "Redo next 10 moves                        PgDn\n"
    "Undo backward to previous branch point    \342\206\221 \n"
    "Redo forward to next branch point         \342\206\223 \n"
    "Set redo moves to shortest solution       !\n"
    "Switch to \"better\" position               =\n"
    "Bookmark the current position             shift-M\n"
    "Forget the last bookmarked position       shift-P\n"
    "Restore the last bookmarked position      shift-R\n"
    "Swap with the last bookmarked position    shift-S";

/* The dimensions of the terminal. (Note that these numbers are not
 * used to lay out the screen -- the game actually looks better if it
 * remains constrained to a size of 80x24 and ignores any remaining
 * area that may be present. Instead, these dimensions are simply
 * cached to verify that the terminal hasn't been reduced to less than
 * 80x24 before rendering.)
 */
static int termheight = 0;
static int termwidth = 0;

/* The terminal text modes used by the interface. Each array element
 * contains a set of ncurses attributes that define the presentation
 * of text for that mode.
 */
static chtype modes[MODEID_COUNT];

/* A cached input command, to be returned as user input in the future.
 */
static command_t cachedcmd = 0;

/* The time at which to return the current cached input command.
 */
static struct timespec cachedcmdtime;

/*
 * Basic terminal functions.
 */

/* Restore the terminal's original state on shutdown.
 */
static void shutdown(void)
{
    if (!isendwin())
        endwin();
}

/* Initialize curses, and define the text attributes. The color pairs
 * are selected to provide clear yet understated contrast with each
 * other.
 */
static int startup(void)
{
    if (!initscr())
        return FALSE;
    getmaxyx(stdscr, termheight, termwidth);
    if (termwidth < 80 || termheight < 24) {
        endwin();
        fputs("Program requires a terminal size of at least 80x24.\n", stderr);
        return FALSE;
    }

    atexit(shutdown);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON_SHIFT, NULL);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLACK, 0);
        init_pair(2, COLOR_CYAN, 0);
        init_pair(3, COLOR_YELLOW, 0);
        init_pair(4, COLOR_BLUE, 0);
        init_pair(5, COLOR_BLACK, COLOR_WHITE);
        init_pair(6, COLOR_RED, COLOR_WHITE);
        init_pair(7, 0, COLOR_CYAN);
        init_pair(8, 0, COLOR_YELLOW);
        modes[MODEID_NORMAL] = A_NORMAL;
        modes[MODEID_SELECTED] = COLOR_PAIR(3);
        modes[MODEID_MARKED] = COLOR_PAIR(2);
        modes[MODEID_HIGHLIGHT] = A_BOLD;
        modes[MODEID_DIMMED] = COLOR_PAIR(1) | A_BOLD;
        modes[MODEID_TITLE] = COLOR_PAIR(4) | A_BOLD;
        modes[MODEID_BLACKCARD] = COLOR_PAIR(5);
        modes[MODEID_REDCARD] = COLOR_PAIR(6);
        modes[MODEID_FOUNDATION] = COLOR_PAIR(7);
        modes[MODEID_RESERVE] = COLOR_PAIR(8);
    } else {
        modes[MODEID_NORMAL] = A_NORMAL;
        modes[MODEID_SELECTED] = A_STANDOUT;
        modes[MODEID_MARKED] = A_DIM;
        modes[MODEID_HIGHLIGHT] = A_BOLD;
        modes[MODEID_DIMMED] = A_DIM;
        modes[MODEID_TITLE] = A_BOLD;
        modes[MODEID_BLACKCARD] = A_REVERSE;
        modes[MODEID_REDCARD] = A_REVERSE;
        modes[MODEID_FOUNDATION] = A_REVERSE | A_DIM;
        modes[MODEID_RESERVE] = A_REVERSE | A_DIM;
    }
    return TRUE;
}

/*
 * Internal functions.
 */

/* Change the mode for subsequent output.
 */
void textmode(int attrid)
{
    attrset(modes[attrid]);
}

/* Ensure that the terminal is of minimum size. A return value of
 * false indicates that the terminal is too small to correctly render
 * the game.
 */
int validatesize(void)
{
    if (termwidth >= 80 && termheight >= 24)
        return TRUE;
    clear();
    addstr("Program needs a display size of at least 80x24.\n");
    refresh();
    return FALSE;
}

/* Wait for keyboard input from the user and return it. If a ungotten
 * command becomes available before then, it is returned instead.
 * Certain special key values are normalized here, such as the
 * multiple possible encodings for the backspace key. In addition,
 * ctrl-L is automatically handled as requesting a full redraw of the
 * current display.
 */
int getkey(void)
{
    struct timespec ts;
    int ch;
    long delay;

    if (cachedcmd) {
        clock_gettime(CLOCK_REALTIME, &ts);
        delay = (cachedcmdtime.tv_sec - ts.tv_sec) * 1000;
        delay += (cachedcmdtime.tv_nsec - ts.tv_nsec) / 1000000;
        if (delay <= 0) {
            ch = ERR;
        } else {
            timeout(delay);
            ch = getch();
        }
        if (ch == ERR) {
            timeout(-1);
            ch = cachedcmd;
            cachedcmd = 0;
        }
    } else {
        ch = getch();
    }
    switch (ch) {
      case '\177':          return '\b';
      case KEY_BACKSPACE:   return '\b';
      case '\r':            return '\n';
      case KEY_ENTER:       return '\n';
      case KEY_F(1):        return '?';
      case '\003':          return 'Q';
      case KEY_RESIZE:
        getmaxyx(stdscr, termheight, termwidth);
        ch = '\f';
        break;
    }
    if (ch == '\f')
        clearok(stdscr, TRUE);
    return ch;
}

/*
 * The options display.
 */

/* Display the settings for autoplay and branching redo, and allow the
 * user to update them.
 */
static int runoptions(settingsinfo *settings)
{
    char const *checked[2] = { "\342\227\246", "\342\200\242" };
    char const *able[2] = { "disable", "enable" };
    char const *abled[2] = { "disabled", "enabled" };
    int autoplay, branching;

    autoplay = settings->autoplay ? 1 : 0;
    branching = settings->branching ? 1 : 0;
    for (;;) {
        erase();
        textmode(MODEID_MARKED);
        mvaddstr(0, 16, "OPTIONS");
        textmode(MODEID_NORMAL);
        mvaddstr(2, 1, checked[autoplay]);
        mvprintw(2, 4, "Autoplay on foundations is %s.", abled[autoplay]);
        mvprintw(3, 4, "Use ctrl-A to %s this feature.", able[1 - autoplay]);
        mvaddstr(4, 4, "When this feature is enabled, cards that can be");
        mvaddstr(5, 4, "played on a foundation pile are moved automatically.");
        mvaddstr(7, 1, checked[branching]);
        mvprintw(7, 4, "Branching redo is %s.", abled[branching]);
        mvprintw(8, 4, "Use ctrl-B to %s this feature.", able[1 - branching]);
        mvaddstr(9, 4, "When this feature is enabled, all undone states are");
        mvaddstr(10,4, "remembered, and can be revisited at any later point.");
        mvaddstr(12,4, "Use Q or Ret to return to the game.");
        refresh();
        switch (getkey()) {
          case '\001':  autoplay = 1 - autoplay;        break;
          case '\002':  branching = 1 - branching;      break;
          case '\n':    goto done;
          case 'q':     goto done;
          case 'Q':     return FALSE;
        }
    }
  done:
    settings->autoplay = autoplay;
    settings->branching = branching;
    return TRUE;
}

/*
 * API functions.
 */

/* Alert the user.
 */
static void curses_ding(void)
{
    flash();
}

/* Accept a key to be injected into the input stream after a delay.
 */
static void curses_ungetinput(command_t cmd, int msec)
{
    cachedcmd = cmd;
    clock_gettime(CLOCK_REALTIME, &cachedcmdtime);
    cachedcmdtime.tv_nsec += msec * 1000000L;
    while (cachedcmdtime.tv_nsec >= 1000000000L) {
        cachedcmdtime.tv_nsec -= 1000000000L;
        ++cachedcmdtime.tv_sec;
    }
}

/* Run the options display.
 */
static int curses_changesettings(settingsinfo *settings)
{
    return runoptions(settings);
}

/*
 * External function.
 */

/* Create the curses user interface.
 */
uimap curses_initializeui(void)
{
    uimap ui;

    if (!startup()) {
        ui.rendergame = NULL;
        return ui;
    }
    atexit(shutdown);

    curses_addhelpsection(commandshelptitle, commandshelptext, TRUE);
    curses_addhelpsection(gameplayhelptitle, gameplayhelptext, TRUE);

    ui.rendergame = curses_rendergame;
    ui.getinput = curses_getinput;
    ui.ungetinput = curses_ungetinput;
    ui.setcardanimationflag = curses_setcardanimationflag;
    ui.ding = curses_ding;
    ui.showsolutionwrite = curses_showsolutionwrite;
    ui.movecard = curses_movecard;
    ui.changesettings = curses_changesettings;
    ui.selectconfig = curses_selectconfig;
    ui.addhelpsection = curses_addhelpsection;
    return ui;
}
