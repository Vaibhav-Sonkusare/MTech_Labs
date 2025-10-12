#include "../../include/network_utils.h"
#include "question.h"
#include "paper.h"
#include <dirent.h>     // to list avaialble question papers

void *handle_client(void *);
struct paper *choose_question_paper(struct device *client);

int main () {
    tcp_server = initialize_tcp_server(DEFAULT_IP_ADDRESS, DEFAULT_PORT_NO);

    concurrently_handle_clients_with_handler(handle_client, 2);

    return 0;
}

void *handle_client(void *arg) {
    struct device *client = (struct device *) arg;
    int score = 0;

    // Ask client for name and registration no
    char *name_buffer = calloc(CLIENT_NAME_LEN, sizeof(char));
    if (name_buffer == NULL) {
        perror("handle_client: calloc failed");
        cleanup_client((struct device *) client);
        pthread_exit(NULL);
    }
    ssize_t bytes_read = receive_message(client, name_buffer, CLIENT_NAME_LEN);
    if (bytes_read < 0) {
        perror("handle_client: receive_message failed");
        free(name_buffer);
        cleanup_client((struct device *) client);
        pthread_exit(NULL);
    } else if (bytes_read == 0) {
        fprintf(stderr, "handle_client: client disconnected before sending name\n");
        free(name_buffer);
        cleanup_client((struct device *) client);
        pthread_exit(NULL);
    }
    // copy name to client_structure
    strncpy(client->name, name_buffer, CLIENT_NAME_LEN);
    free(name_buffer);

    char *client_ip_address = get_client_ip(client);
    int client_port = get_client_port(client);
    printf("Client on %s:%d is %s", client_ip_address, client_port, client->name);

    // Server will choose a question paper
    struct paper *question_paper = choose_question_paper(client);

    message_device_formatted(client, "You will be solving %d paper.\nBest of Luck!\n", question_paper->paper_name);

    while (question_paper->current_question < question_paper->question_count) {
        struct question c_question = question_paper->questions[question_paper->current_question];

        message_device_formatted(client, "\nQuestion %d/%d.\n", question_paper->current_question + 1, question_paper->question_count);
        message_device_formatted(client, "Description: %s\n", c_question.description);
        message_device_formatted(client, "Options:\n");
        for (int i=0; i< c_question.options_count; i++) {
            message_device_formatted(client, "%d. %s", (i + 1), c_question.options[i]);
        }

        int client_option_choice;
        char tmp_buffer[32];
        do {
            message_device_formatted(client, "Choose option index (1-%d): ", c_question.options_count);
            message_device_formatted(client, "#1");
            // get selected option from client
            receive_message(client, tmp_buffer, sizeof(tmp_buffer));
            sscanf(tmp_buffer, " %d", &client_option_choice);
        } while (client_option_choice < 1 || client_option_choice > c_question.options_count);

        if (client_option_choice == c_question.correct_option_index) {
            score++;
        }

        question_paper->current_question++;
    }
    
    message_device_formatted(client, "Exam Completed!\n");
    if (score > 0) {
        message_device_formatted(client, "*****You PASSED!*****\n");
    } else {
        message_device_formatted(client, "*****You FAILED!*****\n");
    }

    message_device_formatted(client, "Your Score: %d/%d\n", score, question_paper->question_count);
    message_device_formatted(client, "Thank you for giving the exam. See you again!\n\n");
    message_device_formatted(client, "#0");

    cleanup_paper(question_paper);
    cleanup_client(client);
    pthread_exit(NULL);
}

struct paper *choose_question_paper(struct device *client) {
    // get stdin lock
    // TODO: lock to prevent multiple clients from selecting paper

    // print current client's info
    printf("Requesting paper for client: %s", client->name);

    // selected paper name
    char paper_name[256];
    strncpy(paper_name, "default_paper.txt", sizeof(paper_name));

    const char *directory_path = "./question_papers";
    DIR *dir = opendir(directory_path);
    if (dir == NULL) {
        perror("choose_question_paper: Error listing paper");
        fprintf(stderr, "Choosing Default paper (%s/%s)", directory_path, paper_name);
    }

    struct dirent *entry;

    printf("Available papers are:");
    
    // read and print available papers
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        printf("%d. %s\n", ++count, entry->d_name);
    }


    printf("Selected paper (default paper: %s): ", paper_name);

    // if choosing default paper
    scanf("[^\n]%s", paper_name);
    if (strncmp(paper_name, "", sizeof(paper_name)) == 0) {
        strncpy(paper_name, "default_paper.txt", sizeof(paper_name));
    }

    //TODO: implement check if 'paper_name' paper exist or not.

    // create paper object
    return read_paper_from_file(paper_name);
}