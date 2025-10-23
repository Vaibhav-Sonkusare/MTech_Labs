#include "../../include/network_utils_v2.h"
#include "question.h"
#include "paper.h"
#include <dirent.h>     // to list avaialble question papers

#define PAPER_NAME "paper1.txt"

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

    printf("Client connected with IP: %s\tPort: %d\n", get_client_ip(client), get_client_port(client));

    // Ask client for name and registration no
    char *name_buffer = calloc(CLIENT_NAME_LEN, sizeof(char));
    if (name_buffer == NULL) {
        perror("handle_client: calloc failed");
        cleanup_client((struct device *) client);
        pthread_exit(NULL);
    }

    if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Enter your name: ") < 0) {
		perror("message_device_formatted: returned error");
		
	}
    if (message_device_formatted(client, MESSAGE_TYPE_WAITINP, "") < 0) {
		perror("message_device_formatted: returned error");
		
	}
    uint16_t responce_type;
    ssize_t bytes_read = receive_message(client, name_buffer, CLIENT_NAME_LEN, &responce_type);
    if (bytes_read <= 0 || responce_type == MESSAGE_TYPE_CLOSURE || responce_type != MESSAGE_TYPE_NORMAL) {
        if (bytes_read <0) {
            fprintf(stderr, "handle_client: receive_message failed\n");
            fprintf(stderr, "Disconnecting and Closing client\n");
        } else if (responce_type == MESSAGE_TYPE_CLOSURE) {
            fprintf(stderr, "Client disconnected successfully\n");
        } else if (responce_type != MESSAGE_TYPE_NORMAL) {
            fprintf(stderr, "Invalid responce type %d\n", responce_type);
            fprintf(stderr, "Disconnecting and Closing client\n");
        }
        
        free(name_buffer);
        cleanup_client((struct device *) client);
        pthread_exit(NULL);
    }
    // copy name to client_structure
    strncpy(client->name, name_buffer, CLIENT_NAME_LEN);
    free(name_buffer);

    printf("Client on %s:%d is %s\n", get_client_ip(client), get_client_port(client), client->name);

    // Server will choose a question paper
    if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Wait till paper is selected\n") < 0) {
		perror("message_device_formatted: returned error");
		
	}
    struct paper *question_paper = choose_question_paper(client);
    // struct paper *question_paper = read_paper_from_file(PAPER_NAME);
    if (question_paper == NULL) {
        perror("choose_question_paper: returned NULL");
        pthread_exit(NULL);
    }
	if (debug > 1) {
    	printf("question paper selected: %s\n", question_paper->paper_name);
	}

    printf("Starting Paper(%s) for `%s`.\n", question_paper->paper_name, client->name);
    if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "You will be solving %s paper.\nBest of Luck!\n", question_paper->paper_name) < 0) {
		perror("message_device_formatted: returned error");
		
	}

    while (question_paper->current_question < question_paper->question_count) {
        struct question *c_question = question_paper->questions[question_paper->current_question];

        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "\nQuestion %d/%d.\n", question_paper->current_question + 1, question_paper->question_count) < 0) {
			perror("message_device_formatted: returned error");
			
		}
        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Description: %s\n", c_question->description) < 0) {
			perror("message_device_formatted: returned error");
			
		}
        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Options:\n") < 0) {
			perror("message_device_formatted: returned error");
			
		}
        for (int i=0; i< c_question->options_count; i++) {
            if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "%d. %s\n", (i + 1), c_question->options[i]) < 0) {
				perror("message_device_formatted: returned error");
				
			}
        }

        int client_option_choice;
        char tmp_buffer[32];
        uint16_t msg_type;
        do {
            if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Choose option index (1-%d): ", c_question->options_count) < 0) {
				perror("message_device_formatted: returned error");
				
			}
            if (message_device_formatted(client, MESSAGE_TYPE_WAITINP, "") < 0) {
				perror("message_device_formatted: returned error");
				
			}
            // get selected option from client
            receive_message(client, tmp_buffer, sizeof(tmp_buffer), &msg_type);
            if (msg_type == MESSAGE_TYPE_CLOSURE) {
                fprintf(stderr, "Error! Client closed early!\n");
                client_option_choice = 0;
                question_paper->current_question = question_paper->question_count;
                break;
            } else if (msg_type != MESSAGE_TYPE_NORMAL) {
                fprintf(stderr, "Error! invalid message from client\n");
            }
            sscanf(tmp_buffer, " %d", &client_option_choice);
        } while (client_option_choice < 1 || client_option_choice > c_question->options_count);

        if (client_option_choice - 1 == c_question->correct_option_index) {
            score++;
        }
		if (debug > 1) {
        	printf("Correct option: %d, your option: %d\n---Score: %d---\n", c_question->correct_option_index, client_option_choice, score);
		}

        question_paper->current_question++;
    }
    
    if (question_paper->current_question == question_paper->question_count) {
        printf("\nExam Completed by `%s`.\n", client->name);
        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Exam Completed!\n") < 0) {
			perror("message_device_formatted: returned error");
			
		}
        if (score >= ((question_paper->question_count) / 3)) {
            printf("%s passed the exam.\n", client->name);
            if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "*****You PASSED!*****\n") < 0) {
				perror("message_device_formatted: returned error");
				
			}
        } else {
            printf("%s failed the exam.\n", client->name);
            if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "*****You FAILED!*****\n") < 0) {
				perror("message_device_formatted: returned error");
				
			}
        }

        printf("%s obtained score %d/%d\n", client->name, score, question_paper->question_count);
        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Your Score: %d/%d\n", score, question_paper->question_count) < 0) {
			perror("message_device_formatted: returned error");
			
		}
        if (message_device_formatted(client, MESSAGE_TYPE_NORMAL, "Thank you for giving the exam. See you again!\n\n") < 0) {
			perror("message_device_formatted: returned error");
			
		}
        if (message_device_formatted(client, MESSAGE_TYPE_CLOSURE, "") < 0) {
			perror("message_device_formatted: returned error");
			
		}
    } else {
        printf("Exam incomplete!\n");
    }

    printf("%s Disconnected!\n", client->name);

    cleanup_paper(question_paper);
    cleanup_client(client);
    pthread_exit(NULL);
}

struct paper *choose_question_paper(struct device *client) {
    // get stdin lock
    // TODO: lock to prevent multiple clients from selecting paper

    // print current client's info
    printf("Requesting paper for client: %s\n", client->name);

    // selected paper name
    char paper_name[256];
    strncpy(paper_name, "default_paper.txt", sizeof(paper_name));

    const char *directory_path = "./question_papers";
    DIR *dir = opendir(directory_path);
    if (dir == NULL) {
        perror("choose_question_paper: Error listing paper");
        fprintf(stderr, "Choosing Default paper (%s/%s)\n", directory_path, paper_name);
    }

    struct dirent *entry;

    printf("Available papers are:\n");
    
    // read and print available papers
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        printf("%d. %s\n", ++count, entry->d_name);
    }


    printf("Selected paper (default paper: %s): ", paper_name);

    // if choosing default paper
    // scanf("[^\n]%s", paper_name);
    fgets(paper_name, sizeof(paper_name), stdin);
    paper_name[strcspn(paper_name, "\n")] = '\0';
    if (strncmp(paper_name, "", sizeof(paper_name)) == 0) {
        strncpy(paper_name, "default_paper.txt", sizeof(paper_name));
    }

    //TODO: implement check if 'paper_name' paper exist or not.

    // create paper object
    char paper_file_path[512];
    memset(paper_file_path, '\0', 512);
    strncpy(paper_file_path, directory_path, 256);
    strcpy(paper_file_path + strlen(paper_file_path), "/");
    strncpy(paper_file_path + strlen(paper_file_path), paper_name, 256);
    if (debug > 1) {
        printf("Opening paper:^^%s^^\n", paper_file_path);
    }
    return read_paper_from_file(paper_file_path);
}
