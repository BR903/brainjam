/* game/game.c: manipulating the game state.
 */

#include "./types.h"
#include "./decls.h"
#include "game/game.h"
#include "internal.h"

/*
 * Internal function.
 */

/* Expand a movecmd, which defines the starting place of a move, into
 * a full moveinfo, naming the destination place and the card being
 * moved. The function must examine the game state for possible
 * destinations in order to determine which one is being requested.
 *
 * If the movecmd represents an invalid move, the returned moveinfo
 * will have a value of zero in the cmd field.
 *
 * When more than one legal move is available, the order of preference
 * is for moving first to a foundation pile, second to a tableau stack
 * with the next-higher card showing, third to an empty tableau stack,
 * and fourth to an empty reserve place. Finally, if the movecmd is
 * uppercase, then the second available move is taken instead of the
 * first. (Note that there is no mechanism for selecting a third
 * choice, even though situations can arise where it is helpful to be
 * able to do so. This mirrors the functionality of the original
 * Windows game, so that the best possible solutions from that program
 * continue to apply to this program.)
 */
moveinfo findmoveinfo(gameplayinfo const *gameplay, movecmd_t movecmd)
{
    moveinfo move;
    int usefirst;

    move.cmd = movecmd;
    move.from = movecmdtoplace(movecmd);
    move.card = gameplay->cardat[move.from];
    if (gameplay->depth[move.from] == 0)
        goto invalid;

    if (ismovecmd1(movecmd))
        usefirst = 1;
    else if (ismovecmd2(movecmd))
        usefirst = 0;
    else
        goto invalid;

    move.to = foundationplace(card_suit(move.card));
    if (gameplay->cardat[move.to] + RANK_INCR == move.card)
        if (usefirst++)
            return move;
    for (move.to = TABLEAU_PLACE_1ST ; move.to < TABLEAU_PLACE_END ; ++move.to)
        if (move.from != move.to)
            if (move.card + RANK_INCR == gameplay->cardat[move.to])
                if (usefirst++)
                    return move;
    for (move.to = TABLEAU_PLACE_1ST ; move.to < TABLEAU_PLACE_END ; ++move.to)
        if (gameplay->depth[move.to] == 0)
            if (usefirst++)
                return move;
    for (move.to = RESERVE_PLACE_1ST ; move.to < RESERVE_PLACE_END ; ++move.to)
        if (gameplay->depth[move.to] == 0)
            if (usefirst++)
                return move;

  invalid:
    move.cmd = 0;
    return move;
}

/*
 * External function.
 */

/* Translate a move ID into a move command using the current game
 * state. Zero is returned if the move ID is not currently valid.
 */
movecmd_t moveidtocmd(gameplayinfo const *gameplay, int moveid)
{
    place_t p;

    for (p = MOVEABLE_PLACE_1ST ; p < MOVEABLE_PLACE_END ; ++p)
        if (gameplay->cardat[p] == moveidtocard(moveid))
            return ismoveid1(moveid) ? placetomovecmd1(p)
                                     : placetomovecmd2(p);
    return 0;
}
