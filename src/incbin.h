/* ./incbin.h: including binary data as C objects.
 *
 * This file defines a macro that allows a file containing binary data
 * to be compiled into the program as a simple array of bytes. In
 * addition to the main object, the macro also defines a pointer to
 * the end of the data, so that its size can be easily computed.
 */

/* The INCBIN macro takes three arguments. The first argument is the
 * filename that contains the data. The second argument is a symbol
 * that will be used as the name of the variable that points to the
 * included data. This variable will be declared as an array of const
 * bytes. The final argument is a symbol that will point to the end of
 * the data. It will also be declared as a byte array, but it should
 * never be dereferenced; it is only used for computing the size of
 * the data.
 */
#ifdef __APPLE__
#define INCBIN(filename, symbol, endsymbol)             \
    static unsigned char const symbol[], endsymbol[];   \
    __asm__(".globl _" #symbol "\n"                     \
            ".globl _" #endsymbol "\n"                  \
            "_" #symbol ":\n"                           \
            ".incbin " #filename "\n"                   \
            "_" #endsymbol ":\n")
#else
#define INCBIN(filename, symbol, endsymbol)             \
    static unsigned char const symbol[], endsymbol[];   \
    __asm__(".globl " #symbol "\n"                      \
            ".globl " #endsymbol "\n"                   \
            #symbol ":\n"                               \
            ".incbin " #filename "\n"                   \
            #endsymbol ":\n")
#endif
