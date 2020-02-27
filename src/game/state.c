/* game/state.c: modifying game state in accordance with the rules.
 */

#include <string.h>
#include "./gen.h"
#include "./types.h"
#include "./decls.h"
#include "configs/configs.h"
#include "redo/redo.h"
#include "game/game.h"
#include "internal.h"

/* Return all gameplay state to empty.
 */
static void clearstate(gameplayinfo *gameplay)
{
    int i;

    gameplay->moveable = 0;
    gameplay->locked = 0;
    gameplay->endpoint = FALSE;
    memset(gameplay->state, 0, sizeof gameplay->state);
    memset(gameplay->depth, 0, sizeof gameplay->depth);
    for (i = 0 ; i < TABLEAU_PLACE_COUNT ; ++i)
        gameplay->inplay[tableauplace(i)] = EMPTY_TABLEAU;
    for (i = 0 ; i < RESERVE_PLACE_COUNT ; ++i)
        gameplay->inplay[reserveplace(i)] = EMPTY_RESERVE;
    for (i = 0 ; i < FOUNDATION_PLACE_COUNT ; ++i)
        gameplay->inplay[foundationplace(i)] = EMPTY_FOUNDATION(i);
}

/* Set up the initial layout for the current configuration. Cards are
 * dealt from the deck to the tableau (left to right, top to bottom).
 */
static void dealcards(gameplayinfo *gameplay, int configid)
{
    card_t deck[NCARDS], card;
    place_t place;
    int i, x, y;

    getdeckforconfiguration(deck, configid);
    for (i = 0 ; i < NCARDS ; ++i) {
        card = deck[i];
        x = i % TABLEAU_PLACE_COUNT;
        y = i / TABLEAU_PLACE_COUNT;
        place = tableauplace(x);
        gameplay->state[cardtoindex(card)] = gameplay->inplay[place];
        gameplay->inplay[place] = card;
        gameplay->depth[place] = y + 1;
    }
}

/* Return true if the game is solved.
 */
static int isgamewon(gameplayinfo const *gameplay)
{
    int i;

    for (i = 0 ; i < FOUNDATION_PLACE_COUNT ; ++i)
        if (gameplay->depth[foundationplace(i)] < NRANKS)
            return FALSE;
    return TRUE;
}

/* Update the moveable field of the gameplay structure. The card at
 * each place is tested for having a valid move, and if so the
 * corresponding bit is set in the moveable field. Note that if any
 * place is currently empty, then everything is automatically moveable
 * (since anytyhing can be moved to an empty place).
 */
static void recalcmoveable(gameplayinfo *gameplay)
{
    moveinfo move;
    place_t from;

    gameplay->moveable = 0;
    for (from = MOVEABLE_PLACE_1ST ; from < MOVEABLE_PLACE_END ; ++from) {
        if (!gameplay->depth[from]) {
            gameplay->moveable = (1 << MOVEABLE_PLACE_END) - 1;
            return;
        }
        move = findmoveinfo(gameplay, placetomovecmd1(from));
        if (move.cmd)
            gameplay->moveable |= 1 << from;
    }
}

/*
 * Internal functions.
 */

/* Begin making the specified move by removing the card from the game
 * state. This function assumes that the requested move has already
 * been validated as legal. After calling this function, the card
 * being moved will not be present in the layout. No other changes can
 * be made to the game state until finishmove() is called and the
 * layout has been restored to a legal state.
 */
void beginmove(gameplayinfo *gameplay, moveinfo move)
{
    int n;

    n = cardtoindex(move.card);
    gameplay->locked |= (1 << move.from) | (1 << move.to);
    gameplay->inplay[move.from] = gameplay->state[n];
    --gameplay->depth[move.from];
    gameplay->state[n] = EMPTY_PLACE;
}

/* Complete the specified move that was begun by the beginmove()
 * function, by returning the card to the layout at the new position
 * and updating the game state accordingly.
 */
void finishmove(gameplayinfo *gameplay, moveinfo move)
{
    gameplay->state[cardtoindex(move.card)] = gameplay->inplay[move.to];
    gameplay->inplay[move.to] = move.card;
    ++gameplay->depth[move.to];
    gameplay->locked &= ~((1 << move.from) | (1 << move.to));
    recalcmoveable(gameplay);
    gameplay->endpoint = isgamewon(gameplay);
}

/*
 * External functions.
 */

/* Initialize the gameplay game state to the starting point of a game.
 * All cards are dealt to the tableau, and the foundation and reserves
 * are set to empty.
 */
void initializegame(gameplayinfo *gameplay)
{
    clearstate(gameplay);
    dealcards(gameplay, gameplay->configid);
    recalcmoveable(gameplay);
}

/* Apply a move command to the game state directly, without any UI
 * component. The return value is false if the move is not valid.
 */
int applymove(gameplayinfo *gameplay, movecmd_t movecmd)
{
    moveinfo move;

    move = findmoveinfo(gameplay, movecmd);
    if (!move.cmd)
        return FALSE;
    beginmove(gameplay, move);
    finishmove(gameplay, move);
    return TRUE;
}

/* Copy a saved game state back into the active game. The game state
 * is formatted as a byte array, one entry per card with each entry
 * identifying the card it covers. The in-memory state data also
 * contains the inplay array, so it is restored automatically as well
 * by the memcpy(). The depth field is not stored with the state,
 * however, so it needs need to be updated manually.
 */
void restoresavedstate(gameplayinfo *gameplay, redo_position const *position)
{
    card_t card;
    int i;

    clearstate(gameplay);
    memcpy(gameplay->state, redo_getsavedstate(position), SIZE_REDO_STATE);
    for (i = 0 ; i < NPLACES ; ++i) {
        gameplay->depth[i] = 0;
        card = gameplay->inplay[i];
        while (!isemptycard(card)) {
            ++gameplay->depth[i];
            card = gameplay->state[cardtoindex(card)];
        }
    }
    recalcmoveable(gameplay);
    gameplay->endpoint = position->endpoint;
    if (gameplay->endpoint != isgamewon(gameplay))
        warn("restored game state claims endpoint = %s, but code disagrees!",
             position->endpoint ? "true" : "false");
}
