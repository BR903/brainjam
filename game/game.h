/* game/game.h: manipulating the game state.
 *
 * These functions provide all of the game logic -- of the gameplay
 * user interface as well as the rules of Brain Jam itself. The
 * gameplayinfo struct holds all of the data that tracks the game
 * state. Code in other modules can examine its fields, but this is
 * the only module that is allowed to directly alter them.
 */

#ifndef _game_h_
#define _game_h_

#include "./types.h"
#include "./decls.h"
#include "redo/types.h"

/* All the information used to run the game. It includes the data
 * involved in managing changes to the game state, as well as several
 * fields that provide quick access to information that is complexly
 * embedded in the game state.
 */
struct gameplayinfo {
    int configid;               /* the current configuration's ID number */
    int bestsolution;           /* the size of the user's best solution */
    int moveable;               /* bitmask of places with legal moves */
    int locked;                 /* bitmask of places with a move in progress */
    int endpoint;               /* true if the user has reached an endpoint */
    card_t inplay[NPLACES];     /* the (top) card at each place */
    signed char depth[NPLACES]; /* the number of cards at each place */
    position_t state[NCARDS];   /* the current location of each card */
};

/* Enable or disable the auto-play feature. When the feature is
 * disabled, cards will not be automatically played on the
 * foundations.
 */
extern void setautoplay(int enabled);

/* Enable or disable the animation of card movements. When the feature
 * is disabled, a moved card will complete instantly. This feature is
 * not available on all UIs.
 */
extern void setanimation(int enabled);

/* Enable or disable the branching redo feature. When the feature is
 * disabled, only linear undo and redo commands are available.
 */
extern void setbranching(int enabled);

/* Initialize the game state to the beginning of a game. The
 * gameplay's configid field is used to choose the deck to use.
 */
extern void initializegame(gameplayinfo *gameplay);

/* Change the game state by making the given move. The return value is
 * false if the move is invalid.
 */
extern int applymove(gameplayinfo *gameplay, movecmd_t movechar);

/* Return the game state to the one associated with the given redo
 * position.
 */
extern void restoresavedstate(gameplayinfo *gameplay,
                              redo_position const *position);

/* Run the user interface for the current setup, using the given game
 * state and redo session to store progress. When invoked, the game
 * state must be initialized to the configuration's starting point.
 * The redo session can be empty, or it can contain redo history from
 * an earlier play session. The function returns when the user leaves
 * the game. If an improved solution is discovered during game play,
 * it will be automatically saved to the user's solution file. The
 * return value is true if the program should return the user to the
 * list of configurations, or false if the program should exit.
 */
extern int gameplayloop(gameplayinfo *gameplay, redo_session *session);

#endif
