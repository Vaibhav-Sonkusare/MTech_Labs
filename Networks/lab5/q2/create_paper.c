// create_paper.c

#include "paper.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_FILE_NAME_LEN 32

int main(int argc, char **argv) {
    char file_name[MAX_FILE_NAME_LEN + 1];
    if (argc > 1) {
        strncpy(file_name, argv[1], MAX_FILE_NAME_LEN);
        file_name[MAX_FILE_NAME_LEN] = '\0';
    } else {
        fprintf(stderr, "Invalid Params!\n");
        fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    char _mode = 'w';
    FILE *paper = fopen(file_name, &_mode);
    if (paper == NULL) {
        fprintf(stderr, "Unable to open file '%s' in %c mode\n", file_name, _mode);
        fprintf(stderr, "%s\n", strerror(errno));
        return 2;
    }

    // Print question paper name to paper
    file_name[strcspn(file_name, ".")] = '\0';
    fprintf(paper, "%s\n", file_name);

    int question_count;
    printf("Enter the number of questions: ");
    scanf(" %d", &question_count);
    while (getchar() != '\n' && !feof(stdin));

    fprintf(paper, "%d\n", question_count);

    char *question = malloc((MAX_QUESTION_DESCRIPTION_LEN + 1) * sizeof(char));
    int option_count;
    char *option = malloc((MAX_QUESTION_OPTION_LEN + 1) * sizeof(char));
    int correct_option_index;

    for (int i=0; i< question_count; i++) {
        memset(question, '\0', MAX_QUESTION_DESCRIPTION_LEN);
        printf("Question Description: ");
        fgets(question, MAX_QUESTION_DESCRIPTION_LEN, stdin);
        question[MAX_QUESTION_DESCRIPTION_LEN] = '\0';


        fprintf(paper, "%s", question);

        do {
            printf("Number of options (1-%d): ", MAX_OPTION_COUNT);
            scanf(" %d", &option_count);
            while (getchar() != '\n' && !feof(stdin));
        } while (option_count <= 1 || option_count > MAX_OPTION_COUNT);
        

        fprintf(paper, "%d\n", option_count);

        for (int i=0; i< option_count; i++) {
            memset(option, '\0', MAX_QUESTION_OPTION_LEN);
            printf("Option %d: ", (i + 1));
            fgets(option, MAX_QUESTION_OPTION_LEN, stdin);
            option[MAX_QUESTION_OPTION_LEN] = '\0';

            fprintf(paper, "%s", option);
        }

        do {
            printf("Correct Option index (1-%d): ", option_count);
            scanf(" %d", &correct_option_index);
            while (getchar() != '\n' && !feof(stdin));
        } while (correct_option_index < 1 || correct_option_index > option_count);

        fprintf(paper, "%d\n", correct_option_index);
    }

    fclose(paper);
    free(question);
    free(option);

    return 0;
}