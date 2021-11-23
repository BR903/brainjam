/* ./decls.h: macros for working with the special type values.
 *
 * This file defines basic constants associated with the types, and
 * numerous macros for manipulating values and translating between
 * types. All macros are composed of inexpensive operations on their
 * inputs.
 */

#ifndef _decls_h_
#define _decls_h_

/*
 * The card_t type. The representation of a playing card.
 */

/* Number of cards in the deck.
 */
#define NRANKS  13
#define NSUITS  4
#define NCARDS  (NRANKS * NSUITS)

/* Some convenience names.
 */
#define CLUBS  0
#define HEARTS  1
#define DIAMONDS  2
#define SPADES  3
#define ACE  1
#define KING  13

/* Macros for translating a playing card to and from its component
 * features. RANK_INCR is a constant that can be added or subtracted
 * to a card value to change its rank while preserving its suit.
 */
#define card_rank(c)  (((c) >> 2) - 1)
#define card_suit(c)  ((c) & 3)
#define mkcard(r, s)  ((((r) + 1) << 2) | ((s) & 3))
#define RANK_INCR  (1 << 2)

/* Macros for translating between a (non-empty) card value and a
 * zero-based index value.
 */
#define cardtoindex(c)  ((c) - mkcard(ACE, 0))
#define indextocard(n)  ((n) + mkcard(ACE, 0))

/* Special values representing things that aren't technically cards
 * but are card_t values. EMPTY_PLACE is the graphic representation of
 * a generic place with no card on it. Internally, however, the
 * different parts of the layout use different values to represent an
 * empty place.
 */
#define EMPTY_PLACE  mkcard(-1, 1)
#define EMPTY_TABLEAU  mkcard(-1, 2)
#define EMPTY_RESERVE  mkcard(-1, 3)
#define EMPTY_FOUNDATION(s)  mkcard(0, s)
#define isemptycard(c)  ((c) < mkcard(1, 0))
#define JOKER_LOW  mkcard(14, 0)
#define JOKER_HIGH  mkcard(14, 1)
#define JOKER  JOKER_HIGH
#define CARDBACK1  mkcard(14, 2)
#define CARDBACK2  mkcard(14, 3)

/*
 * The place_t type. A "place" is a destination that a given card can
 * be moved to. The first eight places are the eight stacks of the
 * tableau. After this comes the four reserves, and lastly are the
 * four foundations.
 */

/* The places of the layout.
 */
#define TABLEAU_PLACE_1ST  0
#define TABLEAU_PLACE_END  8
#define TABLEAU_PLACE_COUNT  8
#define RESERVE_PLACE_1ST  8
#define RESERVE_PLACE_END  12
#define RESERVE_PLACE_COUNT  4
#define FOUNDATION_PLACE_1ST  12
#define FOUNDATION_PLACE_END  16
#define FOUNDATION_PLACE_COUNT  4
#define MOVEABLE_PLACE_1ST  0
#define MOVEABLE_PLACE_END  12
#define MOVEABLE_PLACE_COUNT  12
#define NPLACES  16

/* Macros for working with place values.
 */
#define istableauplace(p)  \
    ((p) >= TABLEAU_PLACE_1ST && (p) < TABLEAU_PLACE_END)
#define isfoundationplace(p)  \
    ((p) >= FOUNDATION_PLACE_1ST && (p) < FOUNDATION_PLACE_END)
#define isreserveplace(p)  \
    ((p) >= RESERVE_PLACE_1ST && (p) < RESERVE_PLACE_END)
#define tableauplace(n)  (TABLEAU_PLACE_1ST + (n))
#define reserveplace(n)  (RESERVE_PLACE_1ST + (n))
#define foundationplace(n)  (FOUNDATION_PLACE_1ST + (n))
#define tableauplaceindex(p)  ((p) - TABLEAU_PLACE_1ST)
#define reserveplaceindex(p)  ((p) - RESERVE_PLACE_1ST)
#define foundationplaceindex(p)  ((p) - FOUNDATION_PLACE_1ST)

/* Places are basically identical to indexes, but these macros are
 * defined for completeness.
 */
#define placetoindex(p) ((p) - MOVEABLE_PLACE_1ST)
#define indextoplace(n) ((n) + MOVEABLE_PLACE_1ST)

/*
 * The movecmd_t type. A "move command" is an ASCII letter that
 * indicates the place a card is moved from, either a lowercase or
 * capital letter 'A' through 'L'. A lowercase letter indicates that
 * the move should be to the first available destination, while a
 * capital letter selects the second available destination.
 */

/* Testing for a valid move command.
 */
#define ismovecmd1(ch)  ((ch) >= 'a' && (ch) < 'a' + MOVEABLE_PLACE_COUNT)
#define ismovecmd2(ch)  ((ch) >= 'A' && (ch) < 'A' + MOVEABLE_PLACE_COUNT)
#define ismovecmd(ch)  (ismovecmd1(ch) || ismovecmd2(ch))

/* Macros for translating between a move command and a place.
 */
#define movecmdtoplace(m)  (((m) - 'A') & ~('a' - 'A'))
#define placetomovecmd1(p)  ('a' + placetoindex(p))
#define placetomovecmd2(p)  ('A' + placetoindex(p))

/*
 * A "move ID" identifies the card being moved, rather than the place
 * it is moved from. An extra bit is used to indicate whether the
 * first-choice or second-choice destination is selected. (Move IDs
 * are only used to identify moves within the redo session, which is
 * why they don't have their own typedef.)
 */

/* Testing for a move ID's type.
 */
#define MOVEID_CARD_MASK  0x3F
#define MOVEID_ALT_FLAG  0x40
#define ismoveid1(moveid)  (((moveid) & MOVEID_ALT_FLAG) == 0)
#define ismoveid2(moveid)  (((moveid) & MOVEID_ALT_FLAG) != 0)

/* Macros for translating between a move ID and a card.
 */
#define moveidtocard(moveid)  ((moveid) & MOVEID_CARD_MASK)
#define moveidtocardindex(moveid)  cardtoindex(moveidtocard(moveid))
#define cardtomoveid1(card)  (card)
#define cardtomoveid2(card)  ((card) | MOVEID_ALT_FLAG)

/* Macro to generate a move ID.
 */
#define mkmoveid(card, alt)  \
    ((alt) ? cardtomoveid2(card) : cardtomoveid1(card))

#endif
