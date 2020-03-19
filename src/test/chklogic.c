/* test/chklogic.c: validation testing of game logic.
 */

#include <stdio.h>
#include <string.h>
#include "./gen.h"
#include "./decls.h"
#include "redo/redo.h"
#include "solutions/solutions.h"
#include "game/game.h"

/*
 */
static solutioninfo thesolution;

/* This function is normally supplied by the solution module, but the
 * test program doesn't include that module, so a replacement is
 * provided here.
 */
solutioninfo const *getsolutionfor(int id)
{
    return id == thesolution.id ? &thesolution : NULL;
}

/* Return a string representing a card suit.
 */
static char const *suitname(int suit)
{
    char const *names[NSUITS] = { "clubs", "diamonds", "hearts", "spades" };

    return names[suit];
}

/* Return a string representing a card. (The function uses a ring of
 * static buffers so that it can be called up to four times before the
 * buffer is reused.)
 */
static char const *cardname(card_t card)
{
    static int nbuf = 0;
    static char bufs[4][16];
    char const *ranks[] = {
        "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
    };

    nbuf = (nbuf + 1) % 4;
    if (isemptycard(card))
        sprintf(bufs[nbuf], "Empty(%d)", card);
    else if (card_rank(card) < ACE || card_rank(card) > KING)
        sprintf(bufs[nbuf], "Undef(%d)", card);
    else
        sprintf(bufs[nbuf], "%s%c",
                ranks[card_rank(card)], *suitname(card_suit(card)));
    return bufs[nbuf];
}

/* Return a string representing a place in the layout. (The function
 * uses a ring of static buffers so that it can be called up to four
 * times before the buffer is reused.)
 */
static char const *placename(place_t p)
{
    static int nbuf = 0;
    static char bufs[4][64];

    nbuf = (nbuf + 1) % 4;
    if (istableauplace(p))
        sprintf(bufs[nbuf], "tableau #%d [%c]", tableauplaceindex(p), 'A' + p);
    else if (isreserveplace(p))
        sprintf(bufs[nbuf], "reserve #%d [%c]", reserveplaceindex(p), 'A' + p);
    else if (isfoundationplace(p))
        sprintf(bufs[nbuf], "%s foundation",
                suitname(foundationplaceindex(p)));
    else
        sprintf(bufs[nbuf], "invalid-place-value (%d)", p);
    return bufs[nbuf];
}

/* Analyze the card layout and warn of inconsistencies. The function
 * checks that the depth and cardat array contents are consistent with
 * the covers array contents, that all cards are present somewhere in
 * the layout, that every card is atop exactly one other card or at
 * the bottom of a place, that all places are accounted for in the
 * covers array, that no place contains excess cards, and that no
 * foundation contains cards out of order or in the wrong suit.
 */
static int validatelayout(gameplayinfo const *gameplay)
{
    char const *prefix = "card layout validation";
    char seen[NCARDS];
    place_t p;
    card_t card, prevcard;
    int brokenplaces, errors;
    int cardcount, tableaucount, reservecount, foundationcount;
    int depth, n;

    brokenplaces = 0;
    errors = 0;

    for (n = 0 ; n < NCARDS ; ++n) {
        if (gameplay->covers[n] == 0) {
            warn("%s: card %s removed from layout!", prefix, cardname(n));
            ++errors;
        } else if (gameplay->covers[n] >= mkcard(14, 0)) {
            warn("%s: illegal value (%d) for covers[%d]",
                 prefix, gameplay->covers[n], n);
            ++errors;
        }
    }

    depth = NCARDS / TABLEAU_PLACE_COUNT + NRANKS;
    for (p = TABLEAU_PLACE_1ST ; p < TABLEAU_PLACE_END ; ++p) {
        if (gameplay->depth[p] > depth) {
            warn("%s: %d cards at %s (over %d)",
                 prefix, gameplay->depth[p], placename(p), depth);
            ++errors;
            brokenplaces |= 1 << p;
        }
    }
    for (p = RESERVE_PLACE_1ST ; p < RESERVE_PLACE_END ; ++p) {
        if (gameplay->depth[p] > 1) {
            warn("%s: %d cards at %s (over 1)",
                 prefix, gameplay->depth[p], placename(p));
            ++errors;
            brokenplaces |= 1 << p;
        }
    }
    for (p = FOUNDATION_PLACE_1ST ; p < FOUNDATION_PLACE_END ; ++p) {
        if (gameplay->depth[p] > NRANKS) {
            warn("%s: %d cards at %s (over %d)",
                 prefix, gameplay->depth[p], placename(p), NRANKS);
            ++errors;
            brokenplaces |= 1 << p;
        }
    }

    memset(seen, FALSE, sizeof seen);
    cardcount = tableaucount = reservecount = foundationcount = 0;
    for (p = 0 ; p < NPLACES ; ++p) {
        if (brokenplaces & (1 << p))
            continue;
        card = gameplay->cardat[p];
        if (!card) {
            warn("%s: missing cardat card at %s", prefix, placename(p));
            ++errors;
            continue;
        }
        depth = 0;
        while (!isemptycard(card)) {
            n = cardtoindex(card);
            if (seen[n]) {
                warn("%s: multiple cards atop %s!", prefix, cardname(card));
                ++errors;
                brokenplaces |= 1 << p;
                card = 0;
                break;
            }
            seen[n] = TRUE;
            ++cardcount;
            ++depth;
            if (!gameplay->covers[n]) {
                warn("%s: missing card under %s!", prefix, cardname(card));
                ++errors;
                brokenplaces |= 1 << p;
            }
            card = gameplay->covers[n];
        }
        if (card == 0)
            continue;
        switch (card) {
          case EMPTY_TABLEAU:
            ++tableaucount;
            break;
          case EMPTY_RESERVE:
            ++reservecount;
            break;
          case EMPTY_FOUNDATION(0):
          case EMPTY_FOUNDATION(1):
          case EMPTY_FOUNDATION(2):
          case EMPTY_FOUNDATION(3):
            ++foundationcount;
            break;
          default:
            warn("%s: unexpected card at bottom of %s: %s",
                 prefix, placename(p), cardname(card));
            ++errors;
            break;
        }
        if (gameplay->depth[p] != depth) {
            warn("%s: incorrect depth for %s (expectd %d, found %d)",
                 prefix, gameplay->depth[p], depth);
            ++errors;
        }
    }

    if (cardcount != NCARDS) {
        warn("%s: %d cards found in layout (expected %d)!",
             prefix, cardcount, NCARDS);
        ++errors;
    }
    if (tableaucount != TABLEAU_PLACE_COUNT) {
        warn("%s: %d tableau places found in layout (expected %d)!",
             prefix, tableaucount, TABLEAU_PLACE_COUNT);
        ++errors;
    }
    if (reservecount != RESERVE_PLACE_COUNT) {
        warn("%s: %d reserve places found in layout (expected %d)!",
             prefix, reservecount, RESERVE_PLACE_COUNT);
        ++errors;
    }
    if (foundationcount != FOUNDATION_PLACE_COUNT) {
        warn("%s: %d foundations found in layout (expected %d)!",
             prefix, foundationcount, FOUNDATION_PLACE_COUNT);
        ++errors;
    }

    for (p = FOUNDATION_PLACE_1ST ; p < FOUNDATION_PLACE_END ; ++p) {
        if (brokenplaces & (1 << p))
            continue;
        card = gameplay->cardat[p];
        while (!isemptycard(card)) {
            prevcard = gameplay->covers[cardtoindex(card)];
            if (prevcard + RANK_INCR != card) {
                warn("%s: %s covers %s at %s",
                     prefix, cardname(card), cardname(prevcard), placename(p));
                ++errors;
                brokenplaces |= 1 << p;
                break;
            }
            card = prevcard;
        }
    }

    return errors;
}

/* Validate that the endpoint flag is consistent with the card layout.
 */
static int validateendpoint(gameplayinfo const *gameplay)
{
    char const *prefix = "game state validation";
    place_t p;
    int endpoint;

    endpoint = TRUE;
    for (p = TABLEAU_PLACE_1ST ; p < TABLEAU_PLACE_END ; ++p)
        if (gameplay->depth[p] != 0)
            endpoint = FALSE;
    for (p = RESERVE_PLACE_1ST ; p < RESERVE_PLACE_END ; ++p)
        if (gameplay->depth[p] != 0)
            endpoint = FALSE;
    for (p = FOUNDATION_PLACE_1ST ; p < FOUNDATION_PLACE_END ; ++p)
        if (gameplay->depth[p] != NRANKS)
            endpoint = FALSE;
    if (endpoint && !gameplay->endpoint) {
        warn("%s: game not over but all cards are in foundations", prefix);
        return 1;
    } else if (!endpoint && gameplay->endpoint) {
        warn("%s: game over but not all cards are in foundations", prefix);
        return 1;
    } else {
        return 0;
    }
}

/* Validate the bit flags of the moveable field correspond to the
 * cards that can be moved. Note that an empty place is permitted to
 * have its flag in either state, so those bits are skipped over.
 */
static int validatemoveable(gameplayinfo const *gameplay)
{
    char const *prefix = "moveable flags validation";
    place_t p, q;
    card_t card;
    int errors, f;

    errors = 0;
    for (p = MOVEABLE_PLACE_1ST ; p < MOVEABLE_PLACE_END ; ++p) {
        card = gameplay->cardat[p];
        if (isemptycard(card))
            continue;
        f = FALSE;
        q = foundationplace(card_suit(card));
        if (card == gameplay->cardat[q] + RANK_INCR) {
            f = TRUE;
            goto found;
        }
        for (q = MOVEABLE_PLACE_1ST ; q < MOVEABLE_PLACE_END ; ++q) {
            if (isemptycard(gameplay->cardat[q])) {
                f = TRUE;
                goto found;
            }
        }
        for (q = TABLEAU_PLACE_1ST ; q < TABLEAU_PLACE_END ; ++q) {
            if (card + RANK_INCR == gameplay->cardat[q]) {
                f = TRUE;
                goto found;
            }
        }
      found:
        if (f != ((gameplay->moveable & (1 << p)) ? TRUE : FALSE)) {
            warn("%s: moveable bit value %d incorrect for %s",
                 prefix, !f, placename(p));
            ++errors;
        }
    }
    return errors;
}

/* Report any inconsistencies in the game state to the user.
 */
static int validategamestate(gameplayinfo const *gameplay)
{
    if (gameplay->locked) {
        warn("error: validategamestate called while game state is locked!");
        return 0;
    }
    return validatelayout(gameplay) + validateendpoint(gameplay) +
           validatemoveable(gameplay);
}

/*
 * Test cases.
 */

/* Initialize a sample game and run through all the moves in a
 * solution, verifying that the game state never enters an
 * inconsistent state. Then repeat the same checks when running
 * backwards through the saved solutions. Errors and inconsistencies
 * are reported on stderr to the user. The return value is the number
 * of errors seen throughout the entire test.
 */
int main(void)
{
    char const *prefix = "sample game solution test";
    int const gameid = 223;
    char solutionstring[] =
        "hcgggggckgfhhgjaaaaaeeeeelkifccccjggjkFFfkjccfkjgggkjFFfkjffaaaBBbbk"
        "jbbbfffibBjhhjihhlkcccckjiDDDDdddbbbbbbddddeeeijklcdaagggfffhhhhhhh";
    signed char depths[] = { 7, 7, 7, 7, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0 };

    gameplayinfo thegame;
    redo_session *session;
    redo_position *position;
    int errors, i, f;

    thesolution.id = gameid;
    thesolution.text = solutionstring;
    thesolution.size = sizeof solutionstring - 1;

    errors = 0;

    thegame.gameid = gameid;
    session = initializegame(&thegame);
    if (memcmp(thegame.depth, depths, sizeof thegame.depth)) {
        warn("%s: initializegame created invalid initial state", prefix);
        ++errors;
    }

    position = redo_getfirstposition(session);
    for (i = 0 ; i < thesolution.size ; ++i) {
        f = applymove(&thegame, thesolution.text[i]);
        if (!f) {
            warn("%s: move #%d (%c) could not be made in test game",
                 prefix, i, thesolution.text[i]);
            ++errors;
            break;
        }
        position = recordgamestate(&thegame, session, position,
                                   thesolution.text[i], redo_nocheck);
        errors += validategamestate(&thegame);
    }

    if (!thegame.endpoint) {
        warn("%s: sample game solution finished without completing game",
             prefix);
        ++errors;
    }

    while (position->prev) {
        position = position->prev;
        restoresavedstate(&thegame, position);
        errors += validategamestate(&thegame);
    }
    if (memcmp(thegame.depth, depths, sizeof thegame.depth)) {
        warn("%s: restored game not in valid initial state", prefix);
        ++errors;
    }

    redo_endsession(session);
    if (errors)
        warn("Total errors: %d", errors);
    return errors;
}
