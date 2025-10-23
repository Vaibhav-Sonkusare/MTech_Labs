// paper.c

#include "paper.h"
#include "../../include/network_utils.h"

extern struct paper *read_paper_from_file(char *filename) {
    FILE *paper = fopen(filename, "r");
    if (paper == NULL) {
        return NULL;
    }

    struct paper *new_paper = calloc(1, sizeof(struct paper));
    if (new_paper == NULL) {
        int ret_val = errno;
        cleanup_paper(new_paper);
        fclose(paper);
        errno = ret_val;
        return NULL;
    }

    // Auxiliary buffers and variables
    int buffer_size = (max(MAX_QUESTION_DESCRIPTION_LEN, MAX_QUESTION_OPTION_LEN) + 2) * sizeof(char);
    char buffer[buffer_size];
    char description[MAX_QUESTION_DESCRIPTION_LEN + 1];
    int options_count;
    char options[MAX_OPTION_COUNT][MAX_QUESTION_OPTION_LEN];
    int correct_option_index;

    // allocate memory and read paper name
    new_paper->paper_name = calloc(MAX_PAPER_NAME_LEN, sizeof(char));
    if (new_paper->paper_name == NULL) {
        int ret_val = errno;
        fclose(paper);
        errno = ret_val;
        return NULL;
    }
    memset(buffer, '\0', buffer_size);
    fgets(buffer, MAX_PAPER_NAME_LEN, paper);
    sscanf(buffer, "%s", new_paper->paper_name);

    // Read question count
    memset(buffer, '\0', buffer_size);
    fgets(buffer, MAX_QUESTION_COUNT, paper);
    sscanf(buffer, "%d", &new_paper->question_count);

    if (new_paper->question_count <= 0 || new_paper->question_count > MAX_QUESTION_COUNT) {
        // fprintf(stderr, "Invalid question count: %d\n", new_paper->question_count);
        cleanup_paper(new_paper);
        fclose(paper);
        return NULL;
    }

    new_paper->questions = calloc(new_paper->question_count, sizeof(struct question));
    if (new_paper->questions == NULL) {
        cleanup_paper(new_paper);
        fclose(paper);
        return NULL;
    }

    new_paper->current_question = 0;

    for (int i=0; i< new_paper->question_count; i++) {
        // Read question description
        memset(buffer, '\0', buffer_size);
        fgets(buffer, MAX_QUESTION_DESCRIPTION_LEN + 1, paper);
        sscanf(buffer, "%[^\n]", description);

        // Read options count
        memset(buffer, '\0', buffer_size);
        fgets(buffer, MAX_OPTION_COUNT, paper);
        sscanf(buffer, "%d", &options_count);

        if (options_count <= 1 || options_count > MAX_OPTION_COUNT) {
            // fprintf(stderr, "Invalid options count: %d\n", options_count);
            break;
        }

        // Read options
        for (int j=0; j< options_count; j++) {
            memset(buffer, '\0', buffer_size);
            fgets(buffer, MAX_QUESTION_OPTION_LEN + 1, paper);
            memset(options[j], '\0', MAX_QUESTION_OPTION_LEN);
            sscanf(buffer, "%[^\n]", options[j]);
        }

        // Read correct option index
        memset(buffer, '\0', buffer_size);
        fgets(buffer, MAX_OPTION_COUNT, paper);
        sscanf(buffer, "%d", &correct_option_index);

        if (correct_option_index < 1 || correct_option_index > options_count) {
            // fprintf(stderr, "Invalid correct option index: %d\n", correct_option_index);
            break;
        }
    
        // Create question and add to paper
        struct question *question = create_question(description, options_count, options, correct_option_index);
        if (question == NULL) {
            perror("create_question: returned NULL");
            break;
        }
        new_paper->questions[i] = *question;
    }

    fclose(paper);
    return new_paper;
}

extern void cleanup_paper(struct paper *question_paper) {
    if (question_paper != NULL) {
        free(question_paper->paper_name);
        if (question_paper->questions != NULL) {
            for (int i=0; i< question_paper->question_count; i++) {
                cleanup_question(&(question_paper->questions[i]));
            }
            free(question_paper->questions);
        }
        free(question_paper);
    }
}