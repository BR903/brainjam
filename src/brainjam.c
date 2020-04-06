/* ./brainjam.c: main() and its associates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <getopt.h>
#include "./gen.h"
#include "./version.h"
#include "./types.h"
#include "./glyphs.h"
#include "./settings.h"
#include "./decks.h"
#include "./ui.h"
#include "./mainloop.h"
#include "files/files.h"
#include "game/game.h"

/* The program's version information and credits.
 */
static char const *versiontitle = "Credits";
static char const *versiontext =
    "Brain Jam: version " VERSION_ID "\n"
    "Copyright " GLYPH_COPYRIGHT " 2017-2020 Brian Raiter"
    " <breadbox@muppetlabs.com>\n"
    "License: GNU GPL version 3 or later; see"
    " <http://gnu.org/licenses/gpl.html>.\n"
    "\n"
    "This program is written by Brian Raiter. It is based on the original"
    " Windows program, which was written by Peter Liepa. The game"
    " configurations were created by Peter Liepa, with assistance from Bert"
    " van Oortmarssen, and are used here with their permission.\n"
    "\n"
    "The rules of Brain Jam are based on \"Baker's Game\", as described by"
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
    "The card layout consists of three areas:\n"
    "\n"
    GLYPH_BULLET " the four foundations at top left;\n"
    GLYPH_BULLET " the four reserves at top right; and\n"
    GLYPH_BULLET " the eight columns of the tableau.\n"
    "\n"
    "The game begins with the complete deck being dealt to the tableau, face"
    " up. The object of the game is to move all of the cards onto the four"
    " foundations.\n"
    "\n"
    "The foundations are to be built up, from Ace to King, in suit. A card can"
    " only be moved to a foundation if the next lower card of the same suit is"
    " already moved there (or if it an Ace being moved onto an empty"
    " foundation).\n"
    "\n"
    "Each of the four reserves can temporarily hold any one card. Cards in the"
    " reserve can always be moved, but cannot be built upon.\n"
    "\n"
    "In the tableau, only the last-played card in each column is available to"
    " move. In addition, the last card in each column can be built upon, but"
    " only going downwards and staying in suit.\n"
    "\n"
    "For example, if the 3 of clubs was available to move, it could be moved"
    " onto a tableau column only if the 4 of clubs was the last card in that"
    " column. Or, it could be moved to the clubs' foundation pile, if the 2 of"
    " clubs had already been moved there. If neither of these are possible,"
    " the card could still be moved to any empty reserve.\n"
    "\n"
    "If a tableau column is emptied of cards, any available card can then be"
    " played there.\n"
    "\n"
    "Since it is never detrimental to do so, the program will automatically"
    " move cards onto the foundations once it becomes possible to do so."
    " However, if you find it distracting, you can turn this off via the"
    " options menu.\n"
    "\n"
    "At any time, you can leave a game and return to the initial display of"
    " the list of available games. Your move history (and solution, if any)"
    " will be remembered, and if you return to the game at a later time you"
    " can use the redo command to pick up where you left off.";

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
    " and you wish to improve upon your answer. It allows you to revisit your"
    " moves and experiment with changes at any point, while still keeping your"
    " working answer intact.\n"
    "\n"
    "When you are revisiting a solved game, the moves that are part of an"
    " answer are displayed differently: instead of a letter, the move is"
    " represented by the total number of moves in the answer. This allows"
    " you to more easily see which moves are part of shorter answers.\n"
    "\n"
    "Sometimes while trying a new sequence of moves, you will return to a"
    " point you had already reached via a different path. In that case, an"
    " indicator will appear below your current move count, showing the number"
    " of moves in the other path. When the other path is shorter, you can"
    " switch over to that path if you choose. If, on the other hand, your"
    " newer path is the shorter one, then the game will automatically update"
    " your history to prefer this newer path. If you have already solved this"
    " game, and this change creates a new, shorter answer, then it will"
    " immediately be saved as your current best answer.\n"
    "\n"
    "In addition to the above commands, the program also allows you to"
    " bookmark any point in your history, so that you can easily return to it"
    " again. When you do this, an bookmark indicator will appear on the"
    " right-hand side of the display. You can also jump back and forth between"
    " your current position and a bookmarked position.\n"
    "\n"
    "See the list of redo key commands for more details.";

/*
 * Cleanup callback.
 */

/* Ensure that the user's current settings are saved.
 */
static void savesettings(void)
{
    settingsinfo const *settings;

    settings = getcurrentsettings();
    if (settings)
        saveinitfile(settings);
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
        "  -C, --cfgdir=DIR      Store user settings in DIR\n"
        "  -D, --datadir=DIR     Store all program data in DIR\n"
        "  -t, --textmode        Use the non-graphical interface\n"
        "  -r, --readonly        Don't modify any files\n"
        "      --validate        Check user files for invalid data and exit\n"
        "      --dirs            Display the output directories and exit\n"
        "      --help            Display this help text and exit\n"
        "      --version         Display program version and exit\n"
        "      --license         Display program license and exit\n"
        "      --rules           Display the rules of the game and exit\n"
        "\n";
    char const *detailstext =
        "If a game ID is not specified, the most recently played game will"
        " be resumed.\n"
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

/* Parse and validate the command-line arguments. If errors are
 * detected, or if an argument indicates that normal program behavior
 * is not requested (e.g. the --help option is used), the function
 * will exit().
 */
static void readcmdline(int argc, char *argv[], settingsinfo *settings)
{
    static char const *optstring = "C:D:tr";
    static struct option const options[] = {
        { "cfgdir", required_argument, NULL, 'C' },
        { "datadir", required_argument, NULL, 'D' },
        { "textmode", no_argument, NULL, 't' },
        { "readonly", no_argument, NULL, 'r' },
        { "validate", no_argument, NULL, 'v' },
        { "dirs", no_argument, NULL, 'd' },
        { "help", no_argument, NULL, 'H' },
        { "version", no_argument, NULL, 'V' },
        { "license", no_argument, NULL, 'L' },
        { "rules", no_argument, NULL, 'R' },
        { 0, 0, 0, 0 }
    };

    char *cfgdir = NULL;
    char *datadir = NULL;
    int validateonly = FALSE;
    int dirdisplayonly = FALSE;
    char *p;
    long id;
    int ch;

    while ((ch = getopt_long(argc, argv, optstring, options, NULL)) != EOF) {
        switch (ch) {
          case 'C':     cfgdir = optarg;                        break;
          case 'D':     datadir = optarg;                       break;
          case 't':     settings->forcetextmode = TRUE;         break;
          case 'r':     settings->readonly = TRUE;              break;
          case 'v':     validateonly = TRUE;                    break;
          case 'd':     dirdisplayonly = TRUE;                  break;
          case 'H':     yowzitch();
          case 'V':     printflowedtext(versiontext);
          case 'L':     printflowedtext(licensetext);
          case 'R':     rhoulz();
          default:
            warn("(try \"--help\" for more information)");
            exit(EXIT_FAILURE);
        }
    }
    if (!cfgdir && datadir)
        cfgdir = datadir;

    if (optind + 1 < argc) {
        warn("%s: invalid argument: \"%s\"", argv[0], argv[optind + 1]);
        warn("(try \"--help\" for more inforfmation)");
        exit(EXIT_FAILURE);
    }
    if (optind < argc) {
        id = strtol(argv[optind], &p, 10);
        if (*p || id < 0 || id >= getdeckcount()) {
            warn("%s: invalid game ID: \"%s\"",
                 argv[0], argv[optind]);
            warn("(valid range is 0000-%04d)", getdeckcount() - 1);
            exit(EXIT_FAILURE);
        }
        settings->gameid = (int)id;
    }

    if (settings->readonly == TRUE || validateonly)
        setreadonly(TRUE);
    setfiledirectories(cfgdir, datadir, argv[0]);
    if (validateonly) {
        filevalidationloop();
        exit(EXIT_SUCCESS);
    }
    if (dirdisplayonly) {
        printfiledirectories();
        exit(EXIT_SUCCESS);
    }
}

/*
 * The main() function.
 */

/* Parse the command line, initialize the
 * settings, start the UI, register the interface-independent help
 * texts, and enter the game selection loop.
 */
int main(int argc, char *argv[])
{
    settingsinfo *settings;

    setlocale(LC_ALL, "");

    settings = getcurrentsettings();
    readcmdline(argc, argv, settings);
    loadinitfile(settings);
    setdefaultsettings();

    if (settings->forcetextmode || !initializeui(UI_SDL)) {
        if (!initializeui(UI_CURSES)) {
            fputs("error: unable to initialize the user interface\n", stderr);
            exit(EXIT_FAILURE);
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
