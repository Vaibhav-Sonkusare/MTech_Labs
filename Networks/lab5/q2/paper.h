// paper.h

#ifndef LAB5_PAPER_H
#define LAB5_PAPER_H

#include "question.h"

#define MAX_QUESTION_COUNT 100

struct Paper
{
    int question_count;
    struct Question *questions;
    int current_question;
};

struct Paper *read_paper_from_file(char *filename);

#endif  //LAB5_PAPER_H