#include "question.h"
#include "../../include/network_utils.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>

struct question *create_question(const char *description, int options_count, char options[][MAX_QUESTION_OPTION_LEN], int correct_option_index) {
    if (!description || options_count < 2 || options_count > MAX_OPTION_COUNT || !options || correct_option_index < 0 || correct_option_index >= options_count) {
        errno = EINVAL;
        return NULL;
    }

    struct question *q = calloc(1, sizeof(struct question));
    if (!q) return NULL;

    q->description = calloc(MAX_QUESTION_DESCRIPTION_LEN, 1);
    if (!q->description) { free(q); return NULL; }
    strncpy(q->description, description, MAX_QUESTION_DESCRIPTION_LEN - 1);

    q->options_count = options_count;
    q->options = calloc(options_count, sizeof(char *));
    if (!q->options) { cleanup_question(q); return NULL; }

    for (int i = 0; i < options_count; i++) {
        q->options[i] = calloc(MAX_QUESTION_OPTION_LEN, 1);
        if (!q->options[i]) { cleanup_question(q); return NULL; }
        strncpy(q->options[i], options[i], MAX_QUESTION_OPTION_LEN - 1);
    }

    q->correct_option_index = correct_option_index;
    return q;
}

void cleanup_question(struct question *q) {
    if (!q) return;
    free(q->description);
    if (q->options) {
        for (int i = 0; i < q->options_count; i++) {
            free(q->options[i]);
        }
        free(q->options);
    }
    free(q);
}

