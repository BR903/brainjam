/* files/rcio.c: reading and writing the initialization file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./gen.h"
#include "./types.h"
#include "./decks.h"
#include "./settings.h"
#include "files/files.h"
#include "internal.h"

/* A basic key-value pair. Note that both strings are stored in a
 * single allocated buffer, with key being the allocated pointer.
 * (This is reinforced by the value pointer being const.)
 */
typedef struct kvpair {
    char       *key;
    char const *value;
} kvpair;

/* Extra settings in the initialization file, not used directly by the
 * main program. These are either settings unique to one of the I/O
 * modules, or else are entries that are entirely unused by the
 * program, but are nonetheless preserved. This permits the program to
 * use the same file as the original Windows program uses.
 */
static kvpair *extras;
static int extrascount = 0;

/* Deallocate all stored miscellaneous key-values pairs.
 */
static void clearextras(void)
{
    int i;

    for (i = 0 ; i < extrascount ; ++i)
        deallocate(extras[i].key);
    deallocate(extras);
    extras = NULL;
    extrascount = 0;
}

/*
 * External functions.
 */

/* Check if the initialization file contains a field, and if so,
 * return the stored value. The return value is NULL if the field
 * is not present in the file.
 */
char const *lookuprcsetting(char const *key)
{
    int i;

    for (i = 0 ; i < extrascount ; ++i)
        if (!strcmp(key, extras[i].key))
            return extras[i].value;
    return NULL;
}

/* Insert a new key-value pair to the table of extra settings. If the
 * pair already exists in the table, it will be replaced.
 */
void storercsetting(char const *key, char const *value)
{
    char *p;
    int keysize, valuesize, i;

    keysize = strlen(key);
    valuesize = strlen(value);
    for (i = 0 ; i < extrascount ; ++i)
        if (!strcmp(key, extras[i].key))
            break;
    if (i == extrascount) {
        ++extrascount;
        extras = realloc(extras, extrascount * sizeof *extras);
    } else {
        deallocate(extras[i].key);
    }
    p = allocate(keysize + valuesize + 2);
    extras[i].key = p;
    memcpy(p, key, keysize + 1);
    p += keysize + 1;
    extras[i].value = p;
    memcpy(p, value, valuesize + 1);
}

/* Input the settings stored in the initialization file. Only
 * undefined fields in the given settingsinfo will be replaced with
 * the values from the file: fields that already have values will not
 * be changed. Unrecognized fields will be stored in the table of
 * extra settings.
 */
int loadrcfile(settingsinfo *settings)
{
    FILE *fp;
    char buf[128];
    char *filename;
    char *val;
    int lineno;
    int id, n, ch;

    filename = mksettingspath("brainjam.ini");
    fp = fopen(filename, "r");
    if (!fp) {
        if (errno == ENOENT) {
            deallocate(filename);
            return TRUE;
        } else {
            perror(filename);
            deallocate(filename);
            return FALSE;
        }
    }
    deallocate(filename);
    clearextras();
    for (lineno = 1 ; fgets(buf, sizeof buf, fp) ; ++lineno) {
        n = strlen(buf);
        if (n > 0 && buf[n - 1] == '\n') {
            buf[--n] = '\0';
            if (n > 0 && buf[n - 1] == '\r')
                buf[--n] = '\0';
        } else if (n == sizeof buf - 1) {
            for (;;) {
                ch = fgetc(fp);
                if (ch == '\n' || ch == EOF)
                    break;
            }
        }
        if (n == 0 || !strcmp(buf, "[General]"))
            continue;
        val = strchr(buf, '=');
        if (!val) {
            warn("brainjam.ini:%d: syntax error", lineno);
            continue;
        }
        *val++ = '\0';
        if (!strcmp(buf, "lastgame")) {
            if (sscanf(val, "%d", &id) == 1 &&
                        id >= 0 && id < getdeckcount()) {
                if (settings->gameid < 0)
                    settings->gameid = id;
            } else {
                warn("brainjam.ini:%d: invalid lastgame value", lineno);
                continue;
            }
        } else if (!strcmp(buf, "showkeys")) {
            if (settings->showkeys < 0)
                settings->showkeys = strcmp(val, "0");
        } else if (!strcmp(buf, "animation")) {
            if (settings->animation < 0)
                settings->animation = strcmp(val, "0");
        } else if (!strcmp(buf, "autoplay")) {
            if (settings->autoplay < 0)
                settings->autoplay = strcmp(val, "0");
        } else if (!strcmp(buf, "branching")) {
            if (settings->branching < 0)
                settings->branching = strcmp(val, "0");
        } else {
            storercsetting(buf, val);
        }
    }
    fclose(fp);
    return TRUE;
}

/* Store the current game ID and other settings to the initialization
 * file.
 */
int savercfile(settingsinfo const *settings)
{
    FILE *fp;
    char *filename;
    int i;

    if (getreadonly())
        return FALSE;
    filename = mksettingspath("brainjam.ini");
    fp = fopen(filename, "w");
    if (!fp) {
        perror(filename);
        deallocate(filename);
        return FALSE;
    }
    deallocate(filename);
    fprintf(fp, "\n[General]\n");
    if (settings->gameid >= 0)
        fprintf(fp, "lastgame=%04d\n", settings->gameid);
    if (settings->showkeys >= 0)
        fprintf(fp, "showkeys=%c\n", settings->showkeys ? '1' : '0');
    if (settings->animation >= 0)
        fprintf(fp, "animation=%c\n", settings->animation ? '1' : '0');
    if (settings->autoplay >= 0)
        fprintf(fp, "autoplay=%c\n", settings->autoplay ? '1' : '0');
    if (settings->branching >= 0)
        fprintf(fp, "branching=%c\n", settings->branching ? '1' : '0');
    for (i = 0 ; i < extrascount ; ++i)
        fprintf(fp, "%s=%s\n", extras[i].key, extras[i].value);
    fclose(fp);
    return TRUE;
}
