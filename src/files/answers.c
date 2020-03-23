/* files/answers.c: reading and writing the answer file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "./gen.h"
#include "./types.h"
#include "./decks.h"
#include "answers/answers.h"
#include "files/files.h"
#include "internal.h"

/*
 * External functions.
 */

/* Read the answer file, if it hasn't been read already, and return
 * the array of answerinfo structs through panswers. The caller
 * inherits ownership of the array. (The unusual formatting of the
 * information in this file is inherited from the original Windows
 * program.)
 */
int loadanswerfile(answerinfo **panswers)
{
    char buf[512];
    answerinfo *answers;
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
    if (!fgets(buf, sizeof buf, fp) || memcmp(buf, "[Solutions]", 11)) {
        fprintf(stderr, "%s: invalid answer file\n", filename);
        deallocate(filename);
        fclose(fp);
        return -1;
    }
    maxcount = getdeckcount();
    answers = allocate(maxcount * sizeof *answers);
    n = 0;
    for (lineno = 1 ; fgets(buf, sizeof buf, fp) ; ++lineno) {
        if (sscanf(buf, "%4d=000%*[A-La-l](%d)", &id, &size) != 2 || id < 0) {
            fprintf(stderr, "%s:%d: invalid answer file entry\n",
                    filename, lineno);
            continue;
        }
        if (id >= maxcount) {
            fprintf(stderr, "%s:%d: invalid id: %d\n", filename, lineno, id);
            continue;
        }
        answers[n].id = id;
        answers[n].size = size;
        answers[n].text = allocate(size + 1);
        memcpy(answers[n].text, buf + 8, size);
        answers[n].text[size] = '\0';
        ++n;
    }
    fclose(fp);
    deallocate(filename);

    *panswers = reallocate(answers, n * sizeof *answers);
    return n;
}

/* Store the given array of answers to the answer file.
 */
int saveanswerfile(answerinfo const *answers, int count)
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
    fputs("[Solutions]\n", fp);
    for (i = 0 ; i < count ; ++i)
        fprintf(fp, "%04d=000%s(%d)\n",
                answers[i].id, answers[i].text, answers[i].size);
    fclose(fp);
    deallocate(filename);
    return TRUE;
}
