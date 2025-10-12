// paper.c

#include "paper.h"
#include "../../include/network_utils.h"

struct Paper *read_paper_from_file(char *filename) {
    FILE *paper = fopen(filename, "r");
    if (paper == NULL) {
        return NULL;
    }

    struct Paper *new_paper = malloc(sizeof(struct Paper));
    if (new_paper == NULL) {
        fclose(paper);
        return NULL;
    }

    // Auxiliary buffers and variables
    int buffer_size = (max(MAX_QUESTION_DESCRIPTION_LEN, MAX_QUESTION_OPTION_LEN) + 2) * sizeof(char);
    char buffer[buffer_size];
    char description[MAX_QUESTION_DESCRIPTION_LEN + 1];
    int options_count;
    char *options[MAX_OPTION_COUNT];
    int correct_option_index;

    // Read question count
    memset(buffer, '\0', buffer_size);
    fgets(buffer, MAX_QUESTION_COUNT, paper);
    sscanf(buffer, "%d", &new_paper->question_count);

    if (new_paper->question_count <= 0 || new_paper->question_count > MAX_QUESTION_COUNT) {
        // fprintf(stderr, "Invalid question count: %d\n", new_paper->question_count);
        free(new_paper);
        fclose(paper);
        return NULL;
    }

    new_paper->questions = malloc(new_paper->question_count * sizeof(struct Question));
    if (new_paper->questions == NULL) {
        free(new_paper);
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
            options[j] = malloc((strlen(buffer) + 1) * sizeof(char));
            if (options[j] == NULL) {
                break;
            }
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
        correct_option_index -= 1; // convert to 0-based index
    
        // Create question and add to paper
        struct Question *question = create_question(description, options_count, options, correct_option_index);
        if (question == NULL) {
            break;
        }
        new_paper->questions[i] = *question;
    }

    fclose(paper);
    for (int i=0; i< options_count; i++) {
        free(options[i]);
    }
    return new_paper;
}