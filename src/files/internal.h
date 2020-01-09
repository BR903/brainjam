/* files/internal.h: internal functions of the files module.
 */

#ifndef _files_internal_h_
#define _files_internal_h_

/* Copy the given filename to a new buffer. The value of datadir is
 * used to produce a full pathname. The caller is responsible for
 * freeing the returned buffer. (If datadir has not been defined, then
 * a copy of the original filename is returned.)
 */
extern char *mkpath(char const *filename);

/* Return the state of the read-only flag.
 */
extern int getreadonly(void);

#endif
