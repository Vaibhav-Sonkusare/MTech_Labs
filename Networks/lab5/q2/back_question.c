// questions.c

#include "question.h"
#include "../../include/network_utils.h"
#include <errno.h>

extern struct question *create_question(char *description, int options_count, char options[][MAX_QUESTION_OPTION_LEN], int correct_option_index) {
    // Check parameters
    if ((description == NULL) || (options_count <= 1 || options_count > MAX_OPTION_COUNT) || (options == NULL) || (correct_option_index < 0 || correct_option_index >= options_count)) {
        errno = EINVAL;         // set errno to Invalid Parameters
        return NULL;
    }

    // allocate memory
    struct question *new_question = calloc(1, sizeof(struct question));
    if (new_question == NULL) {
        return NULL;
    }

    // process struct members
    new_question->description = calloc(MAX_QUESTION_DESCRIPTION_LEN, sizeof(char));
    if (new_question->description == NULL) {
        cleanup_question(new_question);
        return NULL;
    }
    strncpy(new_question->description, description, MAX_QUESTION_DESCRIPTION_LEN);

    new_question->options_count = options_count;

    new_question->options = calloc(options_count, sizeof(char *));
    if (new_question->options == NULL) {
        cleanup_question(new_question);
        return NULL;
    }
    for (int i=0; i< options_count; i++) {
        new_question->options[i] = calloc(MAX_QUESTION_OPTION_LEN, sizeof(char));
        if (new_question->options[i] == NULL) {
            cleanup_question(new_question);
            return NULL;
        }

        strncpy(new_question->options[i], options[i], MAX_QUESTION_OPTION_LEN);
    }
    
    new_question->correct_option_index = correct_option_index;

    return new_question;
}

// extern struct question *copy_question(struct question *q1) {
//     return create_question(q1->description, q1->options_count, q1->options, q1->correct_option_index);
// }

extern void cleanup_question(struct question *ques) {
    if (ques != NULL) {
        free(ques->description);
        if (ques != NULL) {   
            for (int i=0; i< ques->options_count; i++) {
                free(ques->options[i]);
            }
            free(ques->options);
        }
        free(ques);
    }
}