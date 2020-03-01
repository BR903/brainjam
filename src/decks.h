/* ./decks.h: accessing the decks for each game.
 *
 * Each ID corresponds to a unique initial deck order, which is what
 * makes each game different. (Also stored with the deck order is the
 * size of the shortest possible solution for that game.)
 */

#ifndef _decks_h_
#define _decks_h_

#include "./types.h"
#include "./decls.h"

/* Return the number of decks available.
 */
extern int getdeckcount(void);

/* Return the size of the best known solution for a deck.
 */
extern int bestknownsolutionsize(int id);

/* Retrieve the deck of cards for the given ID. The cards are stored
 * in the given array, top card first.
 */
extern void getgamedeck(card_t cards[NCARDS], int id);

#endif
