/* ./gen.c: shared general-purpose functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "./gen.h"

/* Display a formatted message on stderr and return false.
 */
int warn(char const *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    return FALSE;
}

/* A wrapper around malloc() that dies if memory is unavailable.
 */
void *allocate(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        fputs("memory allocation failed\n", stderr);
        exit(EXIT_FAILURE);
    }
    return p;
}

/* A wrapper around realloc() that dies if memory is unavailable.
 */
void *reallocate(void *p, size_t size)
{
    p = realloc(p, size);
    if (!p && size) {
        fputs("memory allocation failed\n", stderr);
        exit(EXIT_FAILURE);
    }
    return p;
}

/* free() doesn't need to be wrapped, but this is provided for the
 * sake of completeness.
 */
void deallocate(void *p)
{
    free(p);
}

/* A version of strdup() that dies if memory is unavailable.
 */
char *strallocate(char const *str)
{
    unsigned int size;

    if (!str)
        return NULL;
    size = strlen(str) + 1;
    return memcpy(allocate(size), str, size);
}

/* Return a newly-allocated formatted string.
 */
char *fmtallocate(char const *fmt, ...)
{
    va_list args;
    size_t size;
    char *p;

    va_start(args, fmt);
    size = vsnprintf(NULL, 0, fmt, args) + 1;
    va_end(args);
    p = allocate(size);
    va_start(args, fmt);
    vsnprintf(p, size, fmt, args);
    va_end(args);
    return p;
}

/* Find the next place to break a word-wrapped string of text,
 * automatically skipping leading whitespace.
 */
int textbreak(char const **pstr, int width)
{
    char const *str;
    int brk, n;

    str = *pstr;
    if (*str == '\n')
        ++str;
    while (*str == ' ')
        ++str;
    *pstr = str;
    if (*str == '\0' || *str == '\n')
        return 0;
    brk = 0;
    for (n = 1 ; n <= width ; ++n) {
        if (str[n] == '\0' || str[n] == '\n')
            return n;
        else if (str[n] == ' ' && str[n - 1] != ' ')
            brk = n;
    }
    return brk ? brk : width;
}
