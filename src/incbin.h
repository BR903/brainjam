/* ./incbin.h: including binary data as C objects.
 *
 * This file defines a macro that allows a file containing binary data
 * to be compiled into the program as a simple array of bytes. In
 * addition to the main object, the macro also defines a pointer to
 * the end of the data, so that its size can be easily computed.
 */

#ifndef _incbin_h_
#define _incbin_h_

/* Some platforms prefix exported symbols with an underscore. Some
 * platforms do not.
 */
#if __APPLE__ || (_WIN32 && !_WIN64)
#define _ "_"
#else
#define _
#endif

/* The INCBIN macro takes three arguments. The first argument is the
 * filename that contains the data. (As with #include, it must be a
 * literal string constant). The second argument is a symbol that will
 * be used as the name of the variable that points to the included
 * data. This variable will be declared as an array of const bytes.
 * The final argument is a symbol that will point to the end of the
 * data. It will also be declared as a byte array, but it should never
 * be dereferenced; it is only useful for computing the size of the
 * data.
 */
#define INCBIN(filename, symbol, endsymbol)             \
    extern unsigned char const symbol[], endsymbol[];   \
    __asm__(".globl " _ #symbol "\n"                    \
            ".globl " _ #endsymbol "\n"                 \
            _ #symbol ":\n"                             \
            ".incbin " #filename "\n"                   \
            _ #endsymbol ":\n")

#endif
