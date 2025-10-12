#include "paper.h"
#include "../../include/network_utils_v2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct paper *read_paper_from_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("read_paper_from_file: fopen");
        return NULL;
    }

    struct paper *p = calloc(1, sizeof(struct paper));
    if (!p) {
        perror("read_paper_from_file: calloc on paper");
        fclose(fp);
        return NULL;
    }

    // Read paper name
    p->paper_name = calloc(MAX_PAPER_NAME_LEN, 1);
    if (!p->paper_name) {
        perror("read_paper_from_file: calloc on paper_name");
        cleanup_paper(p);
        fclose(fp);
        return NULL;
    }
    if (!fgets(p->paper_name, MAX_PAPER_NAME_LEN, fp)) {
        perror("read_paper_from_file: fgets on paper_name");
        cleanup_paper(p);
        fclose(fp);
        return NULL;
    }
	if (debug > 1) {
    	fprintf(stderr, "fgets:%s^^^\n", p->paper_name);
	}
    p->paper_name[strcspn(p->paper_name, "\n")] = '\0';

    // Read question count
    char buffer[128];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        perror("read_paper_from_file: fgets on question_count");
        cleanup_paper(p);
        fclose(fp);
        return NULL;
    }
	if (debug > 1) {
    	fprintf(stderr, "fgets:%s^^^\n", buffer);
	}
    sscanf(buffer, " %d", &p->question_count);
    if (p->question_count <= 0 || p->question_count > MAX_QUESTION_COUNT) {
        perror("read_paper_from_file: sscanf on question_count");
		if (debug > 1) {
        	fprintf(stderr, "buffer:^^^\n");
		}
        cleanup_paper(p);
        fclose(fp);
        return NULL;
    }

    p->questions = calloc(p->question_count, sizeof(struct question *));
    if (!p->questions) {
        perror("read_paper_from_file: calloc on questions");
        cleanup_paper(p);
        fclose(fp);
        return NULL;
    }

    p->current_question = 0;

    for (int i = 0; i < p->question_count; i++) {
        char description[MAX_QUESTION_DESCRIPTION_LEN];
        if (!fgets(description, sizeof(description), fp)) break;
        description[strcspn(description, "\n")] = '\0';

        // Read options count
        int options_count;
        if (!fgets(buffer, sizeof(buffer), fp)) break;
        sscanf(buffer, "%d", &options_count);
        if (options_count < 2 || options_count > MAX_OPTION_COUNT) break;

        char options[MAX_OPTION_COUNT][MAX_QUESTION_OPTION_LEN];
        for (int j = 0; j < options_count; j++) {
            if (!fgets(options[j], sizeof(options[j]), fp)) break;
            options[j][strcspn(options[j], "\n")] = '\0';
        }

        // Read correct option index (1-based in file)
        int correct_option_index;
        if (!fgets(buffer, sizeof(buffer), fp)) break;
        sscanf(buffer, "%d", &correct_option_index);
        if (correct_option_index < 1 || correct_option_index > options_count) break;
        correct_option_index -= 1; // convert to 0-based

        struct question *q = create_question(description, options_count, options, correct_option_index);
        if (!q) break;

        p->questions[i] = q;
    }

    fclose(fp);
    return p;
}

void cleanup_paper(struct paper *p) {
    if (!p) return;
    free(p->paper_name);
    if (p->questions) {
        for (int i = 0; i < p->question_count; i++) {
            cleanup_question(p->questions[i]);
        }
        free(p->questions);
    }
    free(p);
}
