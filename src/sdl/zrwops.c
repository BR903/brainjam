/* sdl/zrwops.c: reading compressed buffers.
 */

#include <stdlib.h>
#include <zlib.h>
#include <SDL.h>
#include "zrwops.h"

/* The data that needs to be retained in between invocations. This is
 * what we store as our private context.
 */
typedef struct zrwops {
    z_stream    zstr;           /* zlib stream state object */
    void       *input;          /* initial input pointer */
    int         size;           /* initial input size */
    int         outpos;         /* current position in the expanded output */
} zrwops;

/* Initialize our internal state at the start of the data stream.
 */
static int startinflate(zrwops *zrw)
{
    zrw->zstr.next_in = zrw->input;
    zrw->zstr.avail_in = zrw->size;
    zrw->zstr.next_out = Z_NULL;
    zrw->zstr.avail_out = 0;
    zrw->zstr.zalloc = Z_NULL;
    zrw->zstr.zfree = Z_NULL;
    zrw->outpos = 0;
    if (inflateInit2(&zrw->zstr, 0x2F) != Z_OK) {
        if (zrw->zstr.msg)
            SDL_SetError("zlib: %s\n", zrw->zstr.msg);
        else
            SDL_SetError("zlib: inflateInit2() failed\n");
        return 0;
    }
    return 1;
}

/* The read function. The zlib inflate function is invoked until the
 * requested amount of data is retrieved, or the end of the stream is
 * found, or an error occurs. As with fread(), the return value is the
 * number of complete elements stored.
 */
static size_t zrwops_read(SDL_RWops *context, void *ptr,
                          size_t size, size_t maxnum)
{
    zrwops *zrw;
    size_t n;
    int f;

    if (!context)
        return -1;
    zrw = context->hidden.unknown.data1;
    zrw->zstr.next_out = ptr;
    zrw->zstr.avail_out = size * maxnum;
    for (;;) {
        f = inflate(&zrw->zstr, Z_NO_FLUSH);
        if (f == Z_BUF_ERROR || f == Z_STREAM_END)
            break;
        if (f != Z_OK) {
            if (zrw->zstr.msg)
                SDL_SetError("zlib: inflate: %s\n", zrw->zstr.msg);
            else
                SDL_SetError("zlib: inflate failed\n");
            return -1;
        }
        if (zrw->zstr.avail_out == 0)
            break;
    }
    n = zrw->zstr.next_out - (Bytef*)ptr;
    zrw->outpos += n;
    return n / size;
}

/* The seek function. Seeking can go either forwards or backwards. If
 * the latter, the stream is rewound to the beginning before seeking
 * forwards. Seeking forwards is done by reading and discarding the
 * result. Seeking relative to the end of the stream is not supported.
 */
static Sint64 zrwops_seek(SDL_RWops *context, Sint64 offset, int whence)
{
    zrwops *zrw;
    int pos, n;

    if (!context)
        return -1;
    zrw = context->hidden.unknown.data1;

    if (whence == SEEK_SET) {
        pos = offset;
    } else if (whence == SEEK_CUR) {
        pos = zrw->outpos + offset;
    } else {
        SDL_Unsupported();
        return -1;
    }

    if (pos < zrw->outpos) {
        inflateEnd(&zrw->zstr);
        if (!startinflate(zrw))
            return -1;
    }

    n = pos - zrw->outpos;
    if (n > 0) {
        Bytef *dummy = malloc(n);
        if (dummy) {
            zrw->zstr.next_out = dummy;
            zrw->zstr.avail_out = n;
            inflate(&zrw->zstr, Z_SYNC_FLUSH);
            zrw->outpos += zrw->zstr.next_out - dummy;
            free(dummy);
        } else {
            SDL_OutOfMemory();
        }
    }

    return zrw->outpos;
}

/* The close function frees everything.
 */
static int zrwops_close(SDL_RWops *context)
{
    zrwops *zrw;

    if (!context)
        return -1;
    zrw = context->hidden.unknown.data1;
    inflateEnd(&zrw->zstr);
    SDL_FreeRW(context);
    free(zrw);
    return 0;
}

/* This program has no need to implement a write function.
 */
static size_t zrwops_write(SDL_RWops *context, void const *ptr,
                           size_t size, size_t num)
{
    (void)context;
    (void)ptr;
    (void)size;
    (void)num;
    SDL_Unsupported();
    return (size_t)-1;
}

/* The size function is also not needed.
 */
static Sint64 zrwops_size(SDL_RWops *context)
{
    (void)context;
    SDL_Unsupported();
    return -1;
}

/*
 * Internal function.
 */

/* A new read-write object is allocated, initialized with a zlib
 * stream object, and returned. The return value is NULL if an error
 * occurred.
 */
SDL_RWops *RWFromZStream(void const *input, int size)
{
    SDL_RWops *rwops;
    zrwops *zrw;

    zrw = malloc(sizeof *zrw);
    if (!zrw) {
        SDL_OutOfMemory();
        return NULL;
    }
    rwops = SDL_AllocRW();
    if (!rwops) {
        free(zrw);
        SDL_OutOfMemory();
        return NULL;
    }

    zrw->input = (void*)input;
    zrw->size = size;
    if (!startinflate(zrw)) {
        free(zrw);
        SDL_FreeRW(rwops);
        return NULL;
    }

    rwops->size = zrwops_size;
    rwops->seek = zrwops_seek;
    rwops->read = zrwops_read;
    rwops->write = zrwops_write;
    rwops->close = zrwops_close;
    rwops->hidden.unknown.data1 = zrw;
    return rwops;
}
