// item_uploader.c

#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT_NO 5000
#define BUFFER_SIZE 1024
#define ITEM_NAME_LEN 50
#define ITEM_DESC_LEN 200
#define ITEM_FILE "auction_items.txt"   // file with items

struct device {
    int fd;
    struct sockaddr_in addr;
};

/* Connect to server */
struct device *initialize_client() {
    struct device *client = calloc(1, sizeof(struct device));
    if (!client) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->fd == -1) {
        perror("socket");
        free(client);
        exit(EXIT_FAILURE);
    }

    client->addr.sin_family = AF_INET;
    inet_pton(AF_INET, IP_ADDRESS, &client->addr.sin_addr);
    client->addr.sin_port = htons(PORT_NO);

    if (connect(client->fd, (struct sockaddr *)&client->addr, sizeof(client->addr)) == -1) {
        perror("connect");
        close(client->fd);
        free(client);
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Connected to server %s:%d\n", IP_ADDRESS, PORT_NO);
    return client;
}

/* Cleanup */
void cleanup_device(struct device *client) {
    if (!client) return;
    close(client->fd);
    free(client);
}

/* Send formatted item to server */
int send_item(struct device *client, const char *name, const char *desc, const char *price) {
    char payload[BUFFER_SIZE];
    int ret = snprintf(payload, sizeof(payload),
                       "ITEM_NAME:%s#ITEM_DESC:%s#BASE_PRICE:%s",
                       name, desc, price);

    if (ret < 0 || ret >= (int)sizeof(payload)) {
        fprintf(stderr, "Item too long, skipping.\n");
        return -1;
    }

    if (send(client->fd, payload, strlen(payload), 0) == -1) {
        perror("send");
        return -1;
    }

    printf("[INFO] Sent item: %s (base: %s)\n", name, price);
    return 0;
}

int main(void) {
    FILE *fp = fopen(ITEM_FILE, "r");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    struct device *client = initialize_client();
    if (!client) {
        fclose(fp);
        return EXIT_FAILURE;
    }

    char name[ITEM_NAME_LEN];
    char desc[ITEM_DESC_LEN];
    char price[64];

    /* File format: name|description|base_price */
    while (fscanf(fp, "%49[^|]|%199[^|]|%63s\n", name, desc, price) == 3) {
        send_item(client, name, desc, price);
        sleep(1); // small delay so server isn’t flooded
    }

    fclose(fp);
    cleanup_device(client);
    printf("[INFO] Done sending items.\n");
    return EXIT_SUCCESS;
}
