/* files/solutionio.c: reading and writing the solution file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./gen.h"
#include "./types.h"
#include "./decks.h"
#include "solutions/solutions.h"
#include "files/files.h"
#include "internal.h"

/*
 * External functions.
 */

/* Read the solution file, if it hasn't been read already, and return
 * the array of solutioninfo structs through psolutions. The caller
 * inherits ownership of the array. The unusual formatting of the
 * information in this file is inherited from the original Windows
 * program.
 */
int loadsolutionfile(solutioninfo **psolutions)
{
    char buf[512];
    solutioninfo *solutions;
    FILE *fp;
    char *filename;
    int lineno, maxcount;
    int size, id, n;

    filename = mksettingspath("brainjam.sol");
    fp = fopen(filename, "r");
    if (!fp) {
        if (errno == ENOENT) {
            deallocate(filename);
            return 0;
        } else {
            perror(filename);
            deallocate(filename);
            return -1;
        }
    }
    deallocate(filename);
    if (!fgets(buf, sizeof buf, fp) || memcmp(buf, "[Solutions]", 11)) {
        fclose(fp);
        fprintf(stderr, "brainjam.sol: invalid solution file\n");
        return -1;
    }
    maxcount = getdeckcount();
    solutions = allocate(maxcount * sizeof *solutions);
    n = 0;
    for (lineno = 1 ; fgets(buf, sizeof buf, fp) ; ++lineno) {
        if (sscanf(buf, "%4d=000%*[A-La-l](%d)", &id, &size) != 2 || id < 0) {
            fprintf(stderr, "brainjam.sol:%d: invalid solution file entry\n",
                    lineno);
            continue;
        }
        if (id >= maxcount) {
            fprintf(stderr, "brainjam.sol:%d: invalid id: %d\n", lineno, id);
            continue;
        }
        solutions[n].id = id;
        solutions[n].size = size;
        solutions[n].text = allocate(size + 1);
        memcpy(solutions[n].text, buf + 8, size);
        solutions[n].text[size] = '\0';
        ++n;
    }
    fclose(fp);

    *psolutions = reallocate(solutions, n * sizeof *solutions);
    return n;
}

/* Rewrite the solution file.
 */
int savesolutionfile(solutioninfo const *solutions, int count)
{
    FILE *fp;
    char *filename;
    int i;

    if (getreadonly())
        return FALSE;
    filename = mksettingspath("brainjam.sol");
    fp = fopen(filename, "w");
    if (!fp) {
        perror(filename);
        deallocate(filename);
        return FALSE;
    }
    deallocate(filename);
    fputs("[Solutions]\n", fp);
    for (i = 0 ; i < count ; ++i)
        fprintf(fp, "%04d=000%s(%d)\n",
                solutions[i].id, solutions[i].text, solutions[i].size);
    fclose(fp);
    return TRUE;
}
