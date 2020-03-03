/* game/state.c: modifying game state in accordance with the rules.
 */

#include <stddef.h>
#include <string.h>
#include "./gen.h"
#include "./types.h"
#include "./decls.h"
#include "./decks.h"
#include "redo/redo.h"
#include "game/game.h"
#include "internal.h"

/* The redo state data consists of the combined covers and cardat
 * arrays. When comparing two states for equality, however, only the
 * covers array should be used. The cardat array is needed only for
 * consistency in layout display.
 */
#define SIZE_REDO_STATE (sizeof(gameplayinfo) - offsetof(gameplayinfo, covers))
#define CMPSIZE_REDO_STATE \
    (offsetof(gameplayinfo, cardat) - offsetof(gameplayinfo, covers))

/* Return all gameplay state to empty.
 */
static void clearstate(gameplayinfo *gameplay)
{
    int i;

    gameplay->moveable = 0;
    gameplay->locked = 0;
    gameplay->endpoint = FALSE;
    memset(gameplay->covers, 0, sizeof gameplay->covers);
    memset(gameplay->depth, 0, sizeof gameplay->depth);
    for (i = 0 ; i < TABLEAU_PLACE_COUNT ; ++i)
        gameplay->cardat[tableauplace(i)] = EMPTY_TABLEAU;
    for (i = 0 ; i < RESERVE_PLACE_COUNT ; ++i)
        gameplay->cardat[reserveplace(i)] = EMPTY_RESERVE;
    for (i = 0 ; i < FOUNDATION_PLACE_COUNT ; ++i)
        gameplay->cardat[foundationplace(i)] = EMPTY_FOUNDATION(i);
}

/* Set up the initial layout for the current game. Cards are dealt
 * from the deck to the tableau (left to right, top to bottom).
 */
static void dealcards(gameplayinfo *gameplay, int gameid)
{
    card_t deck[NCARDS], card;
    place_t place;
    int i, x, y;

    getgamedeck(deck, gameid);
    for (i = 0 ; i < NCARDS ; ++i) {
        card = deck[i];
        x = i % TABLEAU_PLACE_COUNT;
        y = i / TABLEAU_PLACE_COUNT;
        place = tableauplace(x);
        gameplay->covers[cardtoindex(card)] = gameplay->cardat[place];
        gameplay->cardat[place] = card;
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
    gameplay->cardat[move.from] = gameplay->covers[n];
    --gameplay->depth[move.from];
    gameplay->covers[n] = EMPTY_PLACE;
}

/* Complete the specified move that was begun by the beginmove()
 * function, by returning the card to the layout at the new position
 * and updating the game state accordingly.
 */
void finishmove(gameplayinfo *gameplay, moveinfo move)
{
    gameplay->covers[cardtoindex(move.card)] = gameplay->cardat[move.to];
    gameplay->cardat[move.to] = move.card;
    ++gameplay->depth[move.to];
    gameplay->locked &= ~((1 << move.from) | (1 << move.to));
    recalcmoveable(gameplay);
    gameplay->endpoint = isgamewon(gameplay);
}

/* Recursively update the saved state of a subtree. This function is
 * called after a graft has occurred. The state of the grafted
 * positions must necessarily match for the covers array, but can have
 * different values for the cardat array. This function therefore
 * recalculates the game state for every position in the subtree and
 * updates the saved state with the correct cardat array contents.
 */
void updategrafted(gameplayinfo *gameplay, redo_session *session,
                   redo_position *position)
{
    redo_branch *branch;

    for (branch = position->next ; branch ; branch = branch->cdr) {
        applymove(gameplay, moveidtocmd(gameplay, branch->move));
        if (memcmp(redo_getsavedstate(branch->p), &gameplay->covers,
                   SIZE_REDO_STATE)) {
            if (memcmp(redo_getsavedstate(branch->p), &gameplay->covers,
                       CMPSIZE_REDO_STATE)) {
                warn("ERROR: applying move at count %d"
                     " produced different state!", branch->p->movecount);
            }
            redo_updatesavedstate(session, branch->p, &gameplay->covers);
        }
        updategrafted(gameplay, session, branch->p);
        restoresavedstate(gameplay, position);
    }
}

/*
 * External functions.
 */

/* Initialize the gameplay game state to the starting point of a game.
 * All cards are dealt to the tableau, and the foundation and reserves
 * are set to empty. Return a newly started redo session.
 */
redo_session *initializegame(gameplayinfo *gameplay)
{
    clearstate(gameplay);
    dealcards(gameplay, gameplay->gameid);
    recalcmoveable(gameplay);
    return redo_beginsession(&gameplay->covers,
                             SIZE_REDO_STATE, CMPSIZE_REDO_STATE);
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

/* Call redo_addposition() for the given game state.
 */
redo_position *recordgamestate(gameplayinfo const *gameplay,
                               redo_session *session,
                               redo_position *fromposition,
                               int moveid, int checkequiv)
{
    return redo_addposition(session, fromposition, moveid,
                            &gameplay->covers, gameplay->endpoint, checkequiv);
}

/* Copy a saved game state back into the active game. The game state
 * is formatted as a byte array, one entry per card with each entry
 * identifying the card it covers. The in-memory state data also
 * contains the cardat array, so it is restored automatically as well
 * by the memcpy(). The depth field is not stored with the state,
 * however, so it needs need to be updated manually.
 */
void restoresavedstate(gameplayinfo *gameplay, redo_position const *position)
{
    card_t card;
    int i;

    clearstate(gameplay);
    memcpy(&gameplay->covers, redo_getsavedstate(position), SIZE_REDO_STATE);
    for (i = 0 ; i < NPLACES ; ++i) {
        gameplay->depth[i] = 0;
        card = gameplay->cardat[i];
        while (!isemptycard(card)) {
            ++gameplay->depth[i];
            card = gameplay->covers[cardtoindex(card)];
        }
    }
    recalcmoveable(gameplay);
    gameplay->endpoint = position->endpoint;
    if (gameplay->endpoint != isgamewon(gameplay))
        warn("restored game state claims endpoint = %s, but code disagrees!",
             position->endpoint ? "true" : "false");
}
