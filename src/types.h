/* ./types.h: the program's type definitions.
 *
 * The program uses a variety of simple byte-based types, all of which
 * are defined here. In addition, several struct types are declared
 * here, so that code that only needs to use the opaque type name can
 * avoid including the full definition.
 */

#ifndef _types_h_
#define _types_h_

/* The range of integer values for a card is from 0-63. (The extra 12
 * values are used to indicate various non-card elements, such as the
 * empty foundation spaces.)
 */
typedef unsigned char card_t;

/* A place is a location in the layout that can contain a card that
 * can be moved or played upon. (There are sixteen total.)
 */
typedef signed char place_t;

/* A move command is a lowercase or uppercase letter, corresponding to
 * the user's keyboard commands for moving cards.
 */
typedef char movecmd_t;

/* A command is a value that either a move command or one of several
 * other possible values such as undo, quit, etc.
 */
typedef unsigned char command_t;

/* The following structs are fully defined elsewhere.
 */
typedef struct gameplayinfo gameplayinfo;
typedef struct solutioninfo solutioninfo;
typedef struct settingsinfo settingsinfo;
typedef struct renderparams renderparams;

#endif
