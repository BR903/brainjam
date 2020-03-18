/* ./commands.h: user commands available during game play.
 *
 * The most important commands during game play are the letter
 * commands that move cards around. Those commands do not appear here;
 * they are simply mapped to their ASCII values directly. All other
 * commands are defined below, and are assigned non-ASCII values to
 * avoid collision.
 */

#ifndef _commands_h_
#define _commands_h_

enum {
    cmd_none = 0,
    cmd_firstcmd = 128,
    cmd_nop = cmd_firstcmd,     /* do nothing */
    cmd_quit,                   /* exit the current game */
    cmd_quitprogram,            /* exit the program immediately */
    cmd_autoplay,               /* auto-play cards to the foundations */
    cmd_undo,                   /* undo the last move */
    cmd_redo,                   /* redo the last undone move */
    cmd_erase,                  /* undo and forget the last move */
    cmd_undo10,                 /* undo the last 10 moves */
    cmd_redo10,                 /* redo the last 10 undone move */
    cmd_undotobranch,           /* undo to a branch point */
    cmd_redotobranch,           /* redo to a branch point */
    cmd_jumptostart,            /* return to the starting point */
    cmd_jumptoend,              /* redo all undone moves */
    cmd_switchtobetter,         /* jump to the indicated better position */
    cmd_switchtoprevious,       /* go back to the last viewed position */
    cmd_pushbookmark,           /* push the current position */
    cmd_popbookmark,            /* restore the last pushed position */
    cmd_swapbookmark,           /* swap the current and the pushed position */
    cmd_dropbookmark,           /* forget the last pushed position */
    cmd_setminimalpath,         /* make the best solution the redo default */
    cmd_changesettings,         /* display the options settings */
    cmd_select,                 /* select a game (list display) */
    cmd_showhelp,               /* display the online help */
    cmd_redraw,                 /* re-render the game display */
    cmd_lastcmd
};

#endif
