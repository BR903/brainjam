/* ./decls.h: macros for working with the special type values.
 *
 * This file defines numerous macros for examining and manipulating
 * all of the basic types, as well as some related constants. The
 * macros are all simple operations, involving little more than basic
 * arithmetic or bit-masking.
 */

#ifndef _decls_h_
#define _decls_h_

/*
 * The card_t type. Each value represents a playing card.
 */

/* Number of cards in the deck.
 */
#define NRANKS 13
#define NSUITS 4
#define NCARDS (NRANKS * NSUITS)

/* Some convenience names.
 */
#define CLUBS 0
#define HEARTS 1
#define DIAMONDS 2
#define SPADES 3
#define ACE 1
#define KING 13

/* Macros for translating a playing card to and from its component
 * features. RANK_INCR is a constant that can be added or subtracted
 * to a card value to change its rank while preserving its suit.
 */
#define card_rank(c) (((c) >> 2) - 1)
#define card_suit(c) ((c) & 3)
#define mkcard(r, s) ((((r) + 1) << 2) | ((s) & 3))
#define RANK_INCR (1 << 2)

/* Macros for translating between a (non-empty) card value and a
 * zero-based index value.
 */
#define cardtoindex(c) ((c) - mkcard(ACE, 0))
#define indextocard(n) ((n) + mkcard(ACE, 0))

/* Special values representing jokers and non-cards.
 */
#define EMPTY_PLACE  mkcard(-1, 1)
#define EMPTY_FOUNDATION(s)  mkcard(0, s)
#define isemptycard(c) ((c) < mkcard(1, 0))
#define JOKER mkcard(14, 0)
#define JOKER2 mkcard(14, 1)
#define CARDBACK1 mkcard(14, 2)
#define CARDBACK2 mkcard(14, 3)

/*
 * The place_t type. A "place" is a destination that a given card can
 * be moved to. The first eight places are the eight stacks of the
 * tableau. After this comes the four reserves, and lastly are the
 * four foundations.
 */

/* The places of the layout.
 */
#define TABLEAU_PLACE_1ST 0
#define TABLEAU_PLACE_END 8
#define TABLEAU_PLACE_COUNT 8
#define RESERVE_PLACE_1ST 8
#define RESERVE_PLACE_END 12
#define RESERVE_PLACE_COUNT 4
#define FOUNDATION_PLACE_1ST 12
#define FOUNDATION_PLACE_END 16
#define FOUNDATION_PLACE_COUNT 4
#define MOVEABLE_PLACE_1ST 0
#define MOVEABLE_PLACE_END 12
#define MOVEABLE_PLACE_COUNT 12
#define NPLACES 16

/* Macros for working with place values.
 */
#define isplace(p) ((p) >= 0 && (p) < NPLACES)
#define istableauplace(p) ((p) < TABLEAU_PLACE_END)
#define isfoundationplace(p) ((p) >= FOUNDATION_PLACE_1ST)
#define isreserveplace(p) ((p) >= RESERVE_PLACE_1ST && (p) < RESERVE_PLACE_END)
#define tableauplace(n) (n)
#define reserveplace(n) (RESERVE_PLACE_1ST + (n))
#define foundationplace(n) (FOUNDATION_PLACE_1ST + (n))
#define tableauplaceindex(p) (p)
#define reserveplaceindex(p) ((p) - RESERVE_PLACE_1ST)
#define foundationplaceindex(p) ((p) - FOUNDATION_PLACE_1ST)

/* Places are basically identical to indexes, but these macros are
 * defined for completeness.
 */
#define placetoindex(p) ((p) - MOVEABLE_PLACE_1ST)
#define indextoplace(n) ((n) + MOVEABLE_PLACE_1ST)

/*
 * The position_t type. A "position" is more specific than a place; in
 * the case of a tableau stack, it also specifies the relative
 * location of the card within the stack.
 */

/* In practice, the maximum tableau depth is 19. This value is rounded
 * up to provide a margin of safety.
 */
#define MAX_TABLEAU_DEPTH 24

/* The positions of the layout.
 */
#define POS_FACTOR MAX_TABLEAU_DEPTH
#define POS_RESERVE_OFFSET (TABLEAU_PLACE_COUNT * POS_FACTOR)
#define POS_FOUNDATION_OFFSET \
    (TABLEAU_PLACE_COUNT * POS_FACTOR + RESERVE_PLACE_COUNT)

/* Macros for working with position values.
 */
#define tableaupos(n, d) ((n) * POS_FACTOR + (d))
#define reservepos(n) (POS_RESERVE_OFFSET + (n))
#define foundationpos(n) (POS_FOUNDATION_OFFSET + (n))
#define istableaupos(p) ((p) < POS_RESERVE_OFFSET)
#define isreservepos(p) \
    ((p) >= POS_RESERVE_OFFSET && (p) < POS_FOUNDATION_OFFSET)
#define isfoundationpos(p) ((p) >= POS_FOUNDATION_OFFSET)
#define tableauposindex(p) ((p) / POS_FACTOR)
#define tableauposdepth(p) ((p) % POS_FACTOR)
#define reserveposindex(p) ((p) - POS_RESERVE_OFFSET)
#define foundationposindex(p) ((p) - POS_FOUNDATION_OFFSET)

/* Translating between places and positions. In the case of a tableau
 * stack, placetopos() will return the first position in the stack
 * (i.e. the position furthest from the playable card).
 */
#define postoplace(pos) \
    (istableaupos(pos) ? tableauplace(tableauposindex(pos)) : \
     isreservepos(pos) ? reserveplace(reserveposindex(pos)) : \
                       foundationplace(foundationposindex(pos)))
#define placetopos(place) \
    (istableauplace(place) ? tableaupos(tableauplaceindex(place), 0) : \
     isreserveplace(place) ? reservepos(reserveplaceindex(place)) : \
                             foundationpos(foundationplaceindex(place)))

/* An invalid value for both positions and places.
 */
#define NOWHERE (POS_FOUNDATION_OFFSET + 5)

/*
 * The three move types.
 *
 * The move index: A move index indicates which place to move from,
 * but doubled so as to indicate whether the destination should be the
 * first available place or the second. Move indexes are only used in
 * the redo session, and thus do not have their own typedef.
 *
 * The movecmd_t type: A move command maps a move index value to
 * letters, either a lowercase letter 'a' through 'l', or a capital
 * letter in the case of a second-choice desination.
 *
 * The moveid_t type: Unlike the other types, a moveid specifies both
 * the source and the destination place, by packing the two place
 * values into a single byte.
 */

/* The number of possible moves.
 */
#define MOVE_SOURCE_COUNT MOVEABLE_PLACE_COUNT
#define MOVE_TOTAL_COUNT (2 * MOVE_SOURCE_COUNT)

/* A first-choice move index is therefore identical to a place value. A
 * second-choice move index is a place with a constant added.
 */
#define moveindex1(place) (place)
#define moveindex2(place) ((place) + MOVE_SOURCE_COUNT)
#define ismoveindex2(n) ((n) >= MOVE_SOURCE_COUNT)
#define moveindextoplace(n) ((n) % MOVE_SOURCE_COUNT)

/* Testing for a valid move command.
 */
#define ismovecmd1(ch) ((ch) >= 'a' && (ch) < 'a' + MOVE_SOURCE_COUNT)
#define ismovecmd2(ch) ((ch) >= 'A' && (ch) < 'A' + MOVE_SOURCE_COUNT)
#define ismovecmd(ch) (ismovecmd1(ch) || ismovecmd2(ch))

/* Macros to translate between a move index and a move command.
 */
#define indextomovecmd(n) \
    (ismoveindex2(n) ? 'A' + (n) - MOVE_SOURCE_COUNT : 'a' + (n))
#define movecmdtoindex(m) \
    (ismovecmd1(m) ? moveindex1((m) - 'a') : moveindex2((m) - 'A'))

/* Macros for translating between a move command and a place.
 */
#define movecmdtoplace(m) (moveindextoplace(movecmdtoindex(m)))
#define placetomovecmd1(p) (indextomovecmd(moveindex1(placetoindex(p))))
#define placetomovecmd2(p) (indextomovecmd(moveindex2(placetoindex(p))))

/* Macros for using a moveid.
 */
#define mkmoveid(from, to) ((placetoindex(from) << 4) | placetoindex(to))
#define moveid_from(moveid) (indextoplace((moveid) >> 4))
#define moveid_to(moveid) (indextoplace((moveid) & 15))

/* A special value indicating an invalid move, regardless of type.
 */
#define BADMOVE mkmoveid(FOUNDATION_PLACE_1ST, FOUNDATION_PLACE_1ST)

#endif
