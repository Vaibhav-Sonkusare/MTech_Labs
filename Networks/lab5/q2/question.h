// questions.h

#ifndef LAB5_QUESTIONS_H
#define LAB5_QUESTIONS_H

#define MAX_QUESTION_DESCRIPTION_LEN 2048
#define MAX_QUESTION_OPTION_LEN 512
#define MAX_OPTION_COUNT 6

struct question {
    char *description;
    int options_count;
    char **options;
    int correct_option_index;
};

extern struct question *create_question(char *description, int options_count, char options[][MAX_QUESTION_OPTION_LEN], int correct_option_index);
extern void cleanup_question(struct question *ques);

#endif  //LAB5_QUESTIONS_H