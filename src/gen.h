/* ./gen.h: shared general-purpose functionality.
 *
 * These are functions that are useful in multiple areas, and are not
 * specific to any one aspect of the program. They depend on nothing
 * outside of the standard C library.
 */

#ifndef _gen_h_
#define _gen_h_

#include <stddef.h>

#ifndef TRUE
#define TRUE  1
#define FALSE  0
#endif

/* Output a formatted message to stderr. Always returns false.
 */
extern int warn(char const *fmt, ...);

/* Wrappers around malloc(), realloc(), strdup(), and free() that exit
 * if memory cannot be allocated.
 */
extern void *allocate(size_t size);
extern void *reallocate(void *p, size_t size);
extern char *strallocate(char const *str);
extern void deallocate(void *p);

/* Print a formatted string directly into a newly allocated buffer.
 */
extern char *fmtallocate(char const *fmt, ...);

/* Find an appropriate place to break a string so as to fit in a line
 * of the given length. Newlines and the space characters are
 * recognized; all other characters are assumed to be non-whitespace.
 * This function is not UTF-8-aware, and blithely assumes each byte
 * represents one character cell width. The return value is the number
 * of characters to print, which is guaranteed to be nonzero unless
 * the string is already empty. The given string pointer is updated
 * upon return to skip past any initial spaces in the next line.
 */
extern int textbreak(char const **pstr, int width);

#endif
