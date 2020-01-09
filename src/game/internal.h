/* game/internal.h: internal functions of the game module.
 */

#ifndef _game_internal_h_
#define _game_internal_h_

#include "./types.h"

/* A complete description of a move, capturing all of the different
 * aspects of a move that are needed by different functions.
 */
typedef struct moveinfo {
    movecmd_t   cmd;            /* the user's move command */
    card_t      card;           /* the card that is being moved */
    place_t     from;           /* the place that the card is currently */
    place_t     to;             /* the place that the card should move to */
} moveinfo;

/* Expand a move command into a complete moveinfo, indentifying both
 * the source and the destination as well as the card involved. If an
 * illegal move is specified, the cmd field of the returned moveinfo
 * will be set to zero.
 */
extern moveinfo findmoveinfo(gameplayinfo const *gameplay, movecmd_t movecmd);

/* Start the given move by removing the card from the game. This
 * functions assumes that the move specified is legal. No further
 * modifications to the game state can be made until finishmove() is
 * called.
 */
extern void beginmove(gameplayinfo *gameplay, moveinfo move);

/* Complete a begun move by returning the card being moved to the
 * game, at its new position. All relevant game state (such as the
 * moveable bitfield) is appropriately updated.
 */
extern void finishmove(gameplayinfo *gameplay, moveinfo move);

#endif
