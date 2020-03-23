/* ./mainloop.c: the program's top-level loop.
 */

#include <stdio.h>
#include "./types.h"
#include "./ui.h"
#include "./settings.h"
#include "./decks.h"
#include "answers/answers.h"
#include "redo/redo.h"
#include "game/game.h"
#include "files/files.h"

/*
 * Entering the program's inner loop: playing a game.
 */

/* Set up a game and a redo session. Lay out the cards for the given
 * game, and load the previously saved session data and answer (if
 * any). If an answer exists separately from the session, then the
 * answer is "replayed" into the session data.
 */
static redo_session *setupgame(gameplayinfo *gameplay)
{
    redo_session *session;
    char buf[16];

    sprintf(buf, "session-%04d", gameplay->gameid);
    setsessionfilename(buf);
    session = initializegame(gameplay);
    redo_setgraftbehavior(session, redo_graftandcopy);
    loadsession(session, gameplay);

    redo_clearsessionchanged(session);
    if (!redo_getfirstposition(session)->solutionsize)
        replayanswer(gameplay, session);
    return session;
}

/* Retire the current redo session, saving any changes to disk.
 */
static void closesession(redo_session *session)
{
    if (redo_hassessionchanged(session))
        savesession(session);
    redo_endsession(session);
}

/* Create the game state and the redo session, and hand them off to
 * the game engine. The return value is true if the program should
 * return to the game selection interface, or false if the user is
 * done with the program for now.
 */
static int playgame(int gameid)
{
    gameplayinfo thegame;
    redo_session *session;
    int f;

    thegame.gameid = gameid;
    session = setupgame(&thegame);
    f = gameplayloop(&thegame, session);
    closesession(session);
    return f;
}

/*
 * External functions.
 */

/* The program's top-level loop. Allow the user to select a game, and
 * then interact with it. Continue until the user asks to leave.
 */
void gameselectionloop(void)
{
    settingsinfo *settings;
    int id, f;

    initializeanswers();
    for (;;) {
        settings = getcurrentsettings();
        id = selectgame(settings->gameid);
        if (id < 0)
            break;
        settings->gameid = id;
        f = playgame(id);
        if (!f)
            break;
    }
}

/* An alternate main loop, this function briefly loads every data file
 * in the user's directories and then returns. Any unreadable files or
 * invalid data will trigger error messages.
 */
void filevalidationloop(void)
{
    answerinfo *answers;
    gameplayinfo g;

    loadinitfile(getcurrentsettings());
    loadanswerfile(&answers);
    for (g.gameid = 0 ; g.gameid < getdeckcount() ; ++g.gameid)
        closesession(setupgame(&g));
}
