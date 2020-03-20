/* ./glyphs.h: UTF-8 characters.
 *
 * These macros are defined for the UTF-8 characters that are used by
 * the program. This allows the source code to stick to the basic
 * ASCII character set, without having opaque octal escape sequences
 * sprinkled through the program's string literals.
 *
 * In addition, defining the ASCIIONLY preprocessor symbol will
 * replace these characters with plain ASCII equivalents, for maximal
 * portability.
 */

#ifndef _glyphs_h_
#define _glyphs_h_

#ifdef ASCIIONLY

#define GLYPH_COPYRIGHT  "(C)"
#define GLYPH_DASH       "-"
#define GLYPH_BULLET     "*"
#define GLYPH_LEFTARROW  "Left"
#define GLYPH_UPARROW    "Up"
#define GLYPH_RIGHTARROW "Right"
#define GLYPH_DOWNARROW  "Down"
#define GLYPH_BLOCK      "#"
#define GLYPH_OPENDOT    "-"
#define GLYPH_SPADE      "s"
#define GLYPH_CLUB       "c"
#define GLYPH_HEART      "h"
#define GLYPH_DIAMOND    "d"

#else

#define GLYPH_COPYRIGHT  "\302\251"
#define GLYPH_DASH       "\342\200\223"
#define GLYPH_BULLET     "\342\200\242"
#define GLYPH_LEFTARROW  "\342\206\220"
#define GLYPH_UPARROW    "\342\206\221"
#define GLYPH_RIGHTARROW "\342\206\222"
#define GLYPH_DOWNARROW  "\342\206\223"
#define GLYPH_BLOCK      "\342\226\210"
#define GLYPH_OPENDOT    "\342\227\246"
#define GLYPH_SPADE      "\342\231\240"
#define GLYPH_CLUB       "\342\231\243"
#define GLYPH_HEART      "\342\231\245"
#define GLYPH_DIAMOND    "\342\231\246"

#endif

#endif
