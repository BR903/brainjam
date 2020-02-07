/* ./brainjam.c: main() and its associates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <getopt.h>
#include "./gen.h"
#include "./version.h"
#include "./types.h"
#include "./settings.h"
#include "./ui.h"
#include "configs/configs.h"
#include "solutions/solutions.h"
#include "files/files.h"
#include "redo/redo.h"
#include "game/game.h"

/* The program's version information and credits.
 */
static char const *versiontitle = "Credits";
static char const *versiontext =
    "Brain Jam: version " VERSION_ID "\n"
    "Copyright \302\251 2017 by Brian Raiter <breadbox@muppetlabs.com>\n"
    "License: GNU GPL version 3 or later; see"
    " <http://gnu.org/licenses/gpl.html>.\n"
    "\n"
    "This program is written by Brian Raiter. It is based on the original"
    " Windows program written by Peter Liepa. The configurations were created"
    " by Peter Liepa, with assistance from Bert van Oortmarssen, and are used"
    " here with their permission.\n"
    "\n"
    "The rules of Brainjam are based on \"Baker's Game\", as described by"
    " Martin Gardner in the June 1968 issue of Scientific American.";

/* The program's license information.
 */
static char const *licensetitle = "License";
static char const *licensetext =
    "This program is free software: you can redistribute it and/or modify it"
    " under the terms of the GNU General Public License as published by"
    " the Free Software Foundation, either version 3 of the License, or"
    " (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, but"
    " WITHOUT ANY WARRANTY; without even the implied warranty of"
    " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
    " GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License along"
    " with this program. If not, see <https://www.gnu.org/licenses/>.";

/* A description of the game's rules.
 */
static char const *rulestitle = "Rules of the Game";
static char const *rulestext =
    "At the start, use the list to select the number of a game to play. Each"
    " number corresponds to a different initial ordering of cards. There are"
    " no unsolvable games in the list. Once you select a game, you will see"
    " the starting layout of the game.\n"
    "\n"
    "The layout consists of three areas:\n"
    "\n"
    "\342\200\242 the four foundations at top left;\n"
    "\342\200\242 the four reserves at top right;\n"
    "\342\200\242 and the eight columns of the tableau.\n"
    "\n"
    "The game begins with the complete deck being dealt onto the tableau. The"
    " object of the game is to move all of the cards onto the four"
    " foundations.\n"
    "\n"
    "The foundations are to be built up, from Ace to King, in suit. A card can"
    " only be moved to a foundation if the next lower card of the same suit is"
    " currently showing (or if it an Ace being moved onto an empty"
    " foundation).\n"
    "\n"
    "Each of the four reserves can temporarily hold any one card. Cards in the"
    " reserve can always be moved, but cannot be built upon.\n"
    "\n"
    "In the tableau, only the last card in each column is available to move."
    " In addition, the last card in each column can be built upon, but only"
    " going downwards and staying in suit.\n"
    "\n"
    "For example, if the 3 of clubs was the last card in a tableau column, it"
    " could be moved onto a tableau column only if the 4 of clubs was the last"
    " card in that column. Or it could be moved to the clubs' foundation pile,"
    " but only if the 2 of clubs had already been moved there. If neither of"
    " these are possible, the card could still be moved onto an empty reserve"
    " or an empty tableau column.\n"
    "\n"
    "If a tableau column is emptied of cards, any available card can then be"
    " played there.\n"
    "\n"
    "Since it is never detrimental to do so, the program will automatically"
    " move cards onto the foundations once it becomes possible to do so."
    " However, if you find it distracting, you can turn off this behavior from"
    " the options menu.\n"
    "\n"
    "At any time, you can leave a game and return to the initial display of"
    " the list of available games. Your move history (and solution, if any)"
    " will be remembered, and if you return to the game at a later time you"
    " can use redo to pick up where you left off.";

/* A description of the game's branching redo feature.
 */
static char const *branchingredotitle = "Branching Redo";
static char const *branchingredotext =
    "By default, the game provides the familiar undo and redo commands."
    " However, from the options menu you can choose to enable the branching"
    " redo feature, which provides a fuller set of commands.\n"
    "\n"
    "When the branching redo feature is enabled, the game will maintain a"
    " complete history of all moves made. When you use undo to return to a"
    " previous state and then try another set of moves going forward, the game"
    " will still remember the old moves. So if you later use undo to return to"
    " this point again, either path will be available to be redone.\n"
    "\n"
    "You can see when there are multiple paths forward because more than one"
    " card will have redoable moves displayed underneath. By default, the redo"
    " command will choose the most recently visited move, but you can visit"
    " the other path by specifying the other move directly.\n"
    "\n"
    "The branching redo feature is most useful after you have solved a game"
    " and you wish to improve upon your solution. It allows you to revisit"
    " your moves and experiment with changes at any point, while still keeping"
    " your working solution intact.\n"
    "\n"
    "When you are revisiting a solved game, the moves that are part of a"
    " solution are displayed differently: instead of a letter, the move is"
    " represented by the total number of moves in the solution. This allows"
    " you to more easily see which moves are part of shorter solutions.\n"
    "\n"
    "Sometimes while trying a new sequence of moves, you will return to a"
    " state you had already visited via a different path. In that case, an"
    " indicator will appear below your current move count, showing the number"
    " of moves in the other path. When the other path is shorter, you can"
    " switch over to that path if you choose. If, on the other hand, your"
    " newer path is the shorter one, then the game will automatically update"
    " your history to prefer this newer path. If you have already solved this"
    " game, and this change creates a new, shorter solution, then it will"
    " immediately be saved as your current best solution.\n"
    "\n"
    "In addition to the above commands, the program also allows you to"
    " bookmark a state, so that you can more easily return to it again. When a"
    " state is bookmarked, an indicator appear on the right-hand side. You can"
    " switch between the current state and the bookmarked state.\n"
    "\n"
    "See the list of redo key commands for more details.";

/*
 * The top-level game loop.
 */

/* Set up a configuration. Initialize the game data, load the
 * previously saved session data and solution (if any). If a solution
 * exists separately from the session, then the solution is "replayed"
 * into the session data.
 */
static redo_session *setupsession(gameplayinfo *gameplay)
{
    redo_session *session;
    char buf[16];

    sprintf(buf, "session-%04d", gameplay->configid);
    setsessionfilename(buf);
    session = redo_beginsession(&gameplay->state, sizeof gameplay->state, 0);
    redo_setgraftbehavior(session, redo_graftandcopy);
    loadsession(session, gameplay);

    redo_clearsessionchanged(session);
    if (!redo_getfirstposition(session)->solutionsize)
        replaysolution(gameplay, session);
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
 * return to the configuration selection interface, or false if the
 * user is done with the program for now.
 */
static int playgame(int configid)
{
    gameplayinfo thegame;
    redo_session *session;
    int f;

    thegame.configid = configid;
    initializegame(&thegame);
    session = setupsession(&thegame);
    f = gameplayloop(&thegame, session);
    closesession(session);
    return f;
}

/* The program's top-level loop. Allow the user to select a
 * configuration, and then interact with it. Continue until the user
 * asks to leave.
 */
static void gameselectionloop(void)
{
    settingsinfo *settings;
    int id, f;

    initializesolutions();
    for (;;) {
        settings = getcurrentsettings();
        id = selectconfig(settings->configid);
        if (id < 0)
            break;
        settings->configid = id;
        f = playgame(id);
        if (!f)
            break;
    }
}

/* Save the user's current settings to disk.
 */
static void savesettings(void)
{
    settingsinfo const *settings;

    settings = getcurrentsettings();
    if (settings)
        savercfile(settings);
}

/*
 * Command-line parsing code.
 */

/* Output some text with word-wrapped lines and exit.
 */
static void printflowedtext(char const *text)
{
    int const maxlinesize = 78;
    char const *p;
    int n;

    for (p = text ; *p ; p += n) {
        n = textbreak(&p, maxlinesize);
        printf("%.*s\n", n, p);
    }
    exit(EXIT_SUCCESS);
}

/* Display help text for the command-line options and exit.
 */
static void yowzitch(void)
{
    char const *optionstext =
        "Usage: brainjam [OPTIONS] [ID]\n"
        "Play Brain Jam.\n"
        "\n"
        "  -C, --cfgdir=DIR      Store config data in DIR\n"
        "  -D, --datadir=DIR     Store session data in DIR\n"
        "  -t, --textmode        Use the non-graphical interface\n"
        "  -r, --readonly        Don't modify any files\n"
        "      --help            Display this help text and exit\n"
        "      --version         Display program version and exit\n"
        "      --license         Display program license and exit\n"
        "      --rules           Display the rules of the game and exit\n"
        "\n";
    char const *detailstext =
        "If a configuration ID is not specified, the most recently used"
        " configuration will be resumed.\n"
        "\n"
        "While the program is running, use ? or F1 to display information"
        " on how to play the game.";

    fputs(optionstext, stdout);
    printflowedtext(detailstext);
}

/* Display the rules text with an appropriate header and exit.
 */
static void rhoulz(void)
{
    printf("Brain Jam: %s\n\n", rulestitle);
    printflowedtext(rulestext);
}

/* Parse and validate the command-line arguments.
 */
static int readcmdline(int argc, char *argv[], settingsinfo *settings)
{
    static char const *optstring = "C:D:tr";
    static struct option const options[] = {
        { "cfgdir", required_argument, NULL, 'C' },
        { "datadir", required_argument, NULL, 'D' },
        { "textmode", no_argument, NULL, 't' },
        { "readonly", no_argument, NULL, 'r' },
        { "help", no_argument, NULL, 'H' },
        { "version", no_argument, NULL, 'V' },
        { "license", no_argument, NULL, 'L' },
        { "rules", no_argument, NULL, 'R' },
        { 0, 0, 0, 0 }
    };

    char *cfgdir = NULL;
    char *datadir = NULL;
    char *p;
    long id;
    int ch;

    while ((ch = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
        switch (ch) {
          case 'C':     cfgdir = optarg;                        break;
          case 'D':     datadir = optarg;                       break;
          case 't':     settings->forcetextmode = TRUE;         break;
          case 'r':     settings->readonly = TRUE;              break;
          case 'H':     yowzitch();
          case 'V':     printflowedtext(versiontext);
          case 'L':     printflowedtext(licensetext);
          case 'R':     rhoulz();
          default:
            warn("(try \"--help\" for more information)");
            return FALSE;
        }
    }
    if (optind + 1 < argc) {
        warn("%s: invalid argument: \"%s\"", argv[0], argv[optind + 1]);
        warn("(try \"--help\" for more inforfmation)");
        return FALSE;
    }
    if (optind < argc) {
        id = strtol(argv[optind], &p, 10);
        if (*p || id < 0 || id >= getconfigurationcount()) {
            warn("%s: invalid configuration ID: \"%s\"",
                 argv[0], argv[optind]);
            warn("(valid range is 0000-%04d)", getconfigurationcount() - 1);
            return FALSE;
        }
        settings->configid = (int)id;
    }

    if (settings->readonly == TRUE)
        setreadonly(settings->readonly);
    setfiledirectories(cfgdir, datadir, argv[0]);
    return TRUE;
}

/* The main() function. Parse the command line, initialize the
 * settings, start the UI, register the interface-independent help
 * texts, and enter the game selection loop.
 */
int main(int argc, char *argv[])
{
    settingsinfo *settings;

    setlocale(LC_ALL, "");
    srand(time(NULL));

    settings = getcurrentsettings();
    if (!readcmdline(argc, argv, settings))
        return EXIT_FAILURE;
    loadrcfile(settings);
    setdefaultsettings();

    if (settings->forcetextmode || !initializeui(UI_SDL)) {
        if (!initializeui(UI_CURSES)) {
            fputs("error: unable to initialize the user interface\n", stderr);
            return EXIT_FAILURE;
        }
    }
    applysettings(FALSE);
    atexit(savesettings);

    addhelpsection(rulestitle, rulestext, TRUE);
    addhelpsection(branchingredotitle, branchingredotext, FALSE);
    addhelpsection(versiontitle, versiontext, FALSE);
    addhelpsection(licensetitle, licensetext, FALSE);

    gameselectionloop();

    return 0;
}
