// paper.h

#ifndef LAB5_PAPER_H
#define LAB5_PAPER_H

#include "question.h"

#define MAX_PAPER_NAME_LEN 256
#define MAX_QUESTION_COUNT 100

struct paper
{
    char *paper_name;
    int question_count;
    struct question *questions;
    int current_question;
};

extern struct paper *read_paper_from_file(char *filename);
extern void cleanup_paper(struct paper *question_paper);

#endif  //LAB5_PAPER_H