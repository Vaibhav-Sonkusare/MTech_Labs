#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct data_node {
    char *word;
    char *meaning;
    char *dummy_str;
};

struct AVL_node {
    struct data_node *data;
    struct AVL_node *left;
    struct AVL_node *right;
    struct AVL_node *parent;
    int height;
};

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_DUMMY_STR_LEN 32

struct AVL_node *AVL_insert(struct AVL_node *, struct data_node *);
struct AVL_node *AVL_delete(struct AVL_node *, struct data_node *);
struct AVL_node *AVL_search(struct AVL_node *, struct data_node *);
void _AVL_delete(struct AVL_node *, struct data_node *);

// Helper functions
struct data_node *create_data_node(char *, char *, char *);
void free_data_node(struct data_node *);
struct AVL_node *create_AVL_node(struct data_node *);
void free_AVL_node(struct AVL_node *);
int compare_data_node(struct data_node *, struct data_node *);
int get_height(struct AVL_node *);
int get_balance(struct AVL_node *);
void update_AVL_node_height(struct AVL_node *);
struct AVL_node *rotate_right(struct AVL_node *);
struct AVL_node *rotate_left(struct AVL_node *);
struct AVL_node *LL_rotation(struct AVL_node *);
struct AVL_node *LR_rotation(struct AVL_node *);
struct AVL_node *RL_rotation(struct AVL_node *);
struct AVL_node *RR_rotation(struct AVL_node *);
struct AVL_node *get_inorder_successor(struct AVL_node *);
struct AVL_node *get_inorder_predecessor(struct AVL_node *);
struct AVL_node *min_value_node(struct AVL_node *);
int max(int, int);
void inorder_traversal(struct AVL_node *);
void level_order_traversal(struct AVL_node *);

int main(int argc, char **argv) {
    struct AVL_node *head = NULL;

    head = AVL_insert(head, create_data_node("070", "numeric", "dummy"));
    head = AVL_insert(head, create_data_node("004", "numeric", "dummy"));
    head = AVL_insert(head, create_data_node("021", "numeric", "dummy"));
    head = AVL_insert(head, create_data_node("001", "numeric", "dummy"));

    inorder_traversal(head);

    struct AVL_node *searched_node;
    struct data_node *data_to_search;

    printf("Searching (\"004\", \"numeric\", \"dummy\"): ");
    data_to_search = create_data_node("004", "numeric", "dummy");
    searched_node = AVL_search(head, data_to_search);
    if (searched_node != NULL) {
        printf("Found: %s, %s, %s\n", searched_node->data->word, searched_node->data->meaning, searched_node->data->dummy_str);
    } else {
        printf("Not Found!\n");
    }

    printf("Searching (\"005\", \"numeric\", \"dummy\"): ");
    data_to_search = create_data_node("005", "numeric", "dummy");
    searched_node = AVL_search(head, data_to_search);
    if (searched_node != NULL) {
        printf("Found: %s, %s, %s\n", searched_node->data->word, searched_node->data->meaning, searched_node->data->dummy_str);
    } else {
        printf("Not Found!\n");
    }

    printf("Searching (\"070\", \"numeric\", \"dummy\"): ");
    data_to_search = create_data_node("070", "numeric", "dummy");
    searched_node = AVL_search(head, data_to_search);
    if (searched_node != NULL) {
        printf("Found: %s, %s, %s\n", searched_node->data->word, searched_node->data->meaning, searched_node->data->dummy_str);
    } else {
        printf("Not Found!\n");
    }

    // head = AVL_delete(head, head->data);
    _AVL_delete(head, head->data);
    inorder_traversal(head);

    return 0;
}

struct AVL_node *AVL_insert(struct AVL_node *node, struct data_node *data) {
    if (node == NULL) {
        return create_AVL_node(data);
    }

    if (compare_data_node(node->data, data) > 0) {
        if (node->left != NULL) {
            node->left = AVL_insert(node->left, data);
        } else {
            node->left = create_AVL_node(data);
            node->left->parent = node;
        }
    } else if (compare_data_node(node->data, data) < 0) {
        if (node->right != NULL) {
            node->right = AVL_insert(node->right, data);
        } else {
            node->right = create_AVL_node(data);
            node->right->parent = node;
        }
    } else {
        // same data already exists.
        fprintf(stderr, "AVL_insert: %s already exists.", data->word);
        return node;
    }
    
    // Recursively update height
    update_AVL_node_height(node);

    // Correct balance
    if (get_balance(node) < -1) {
        if (get_balance(node->right) <= 0) {
            node = RR_rotation(node);
        } else {
            node = RL_rotation(node);
        }
    } else if (get_balance(node) > 1) {
        if (get_balance(node->left) >= 0) {
            node = LL_rotation(node);
        } else {
            node = LR_rotation(node);
        }
    }

    return node;
}

struct AVL_node *AVL_delete(struct AVL_node *root, struct data_node *data) {
    if (root == NULL)
        return root;

    if (compare_data_node(data, root->data) > 0)
        root->left = AVL_delete(root->left, data);
    else if (compare_data_node(data, root->data) < 0)
        root->right = AVL_delete(root->right, data);
    else {
        // Node with only one child or no child
        if ((root->left == NULL) || (root->right == NULL)) {
            struct AVL_node *temp = root->left ? root->left : root->right;

            if (!temp) {
                temp = root;
                root = NULL;
            } else
                *root = *temp;

            free_AVL_node(temp);
        } else {
            struct AVL_node *temp = min_value_node(root->right);
            root->data = temp->data;
            root->right = AVL_delete(root->right, temp->data);
        }
    }

    if (!root)
        return root;

    root->height = 1 + max(get_height(root->left), get_height(root->right));

    int balance = get_balance(root);

    // Left Left
    if (balance > 1 && get_balance(root->left) >= 0)
        return rotate_right(root);

    // Left Right
    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }

    // Right Right
    if (balance < -1 && get_balance(root->right) <= 0)
        return rotate_left(root);

    // Right Left
    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

void _AVL_delete(struct AVL_node *node, struct data_node *data) {
    if (node == NULL || data == NULL) {
        fprintf(stderr, "AVL_delete: Invalid parameters");
        return;
    }

    if (compare_data_node(node->data, data) == 0) {
        if ((node->left == NULL) || (node->right == NULL)) {
            struct AVL_node *child = (node->left != NULL)? node->left: node->right;

            if (node = node->parent->left) {
                node->parent->left = child;
            } else {
                node->parent->right = child;
            }

            node->left = NULL;
            node->right = NULL;

            free_AVL_node(node);
        } else {
            struct AVL_node *temp = min_value_node(node->right);
            node->data = temp->data;
            node->right = AVL_delete(node->right, temp->data);
        }
    } else if (compare_data_node(node->data, data) > 0) {
        AVL_delete(node->left, data);
    } else {
        AVL_delete(node->right, data);
    }
    
    // Recursively update height
    update_AVL_node_height(node);

    // Correct balance
    if (get_balance(node) < -1) {
        if (get_balance(node->right) <= 0) {
            node = RR_rotation(node);
        } else {
            node = RL_rotation(node);
        }
    } else if (get_balance(node) > 1) {
        if (get_balance(node->left) >= 0) {
            node = LL_rotation(node);
        } else {
            node = LR_rotation(node);
        }
    }
    
}

struct AVL_node *AVL_search(struct AVL_node *node, struct data_node *data) {
    if (node == NULL) {
        return NULL;
    }

    if (compare_data_node(node->data, data) == 0) {
        // printf("\nfound node->data->word= %s\t data->word= %s\n", node->data->word, data->word);
        return node;
    } else if (compare_data_node(node->data, data) > 0) {
        // printf("\nleft node->data->word= %s\t data->word= %s\n", node->data->word, data->word);
        return AVL_search(node->left, data);
    } else {
        // printf("\nright node->data->word= %s\t data->word= %s\n", node->data->word, data->word);
        return AVL_search(node->right, data);
    }

    // controll should never reach here!
    return NULL;
}

struct data_node *create_data_node(char *word, char *meaning, char *dummy_str) {
    struct data_node *new_data_node;
    new_data_node = calloc(1, sizeof(struct data_node));

    new_data_node->word = calloc(MAX_WORD_LEN, sizeof(char));
    new_data_node->meaning = calloc(MAX_MEANING_LEN, sizeof(char));
    new_data_node->dummy_str = calloc(MAX_DUMMY_STR_LEN, sizeof(char));

    strncpy(new_data_node->word, word, MAX_WORD_LEN - 1);
    strncpy(new_data_node->meaning, meaning, MAX_MEANING_LEN - 1);
    strncpy(new_data_node->dummy_str, dummy_str, MAX_DUMMY_STR_LEN - 1);

    return new_data_node;
}

void free_data_node(struct data_node *node) {
    free(node->word);
    free(node->meaning);
    free(node->dummy_str);
    free(node);
}

struct AVL_node *create_AVL_node(struct data_node *data) {
    struct AVL_node *new_AVL_node;
    new_AVL_node = calloc(1, sizeof(struct AVL_node));

    new_AVL_node->data = data;
    new_AVL_node->left = NULL;
    new_AVL_node->right = NULL;
    new_AVL_node->parent = NULL;
    new_AVL_node->height = 0;

    return new_AVL_node;
}

void free_AVL_node(struct AVL_node *node) {
    if (node == NULL) return;
    free_AVL_node(node->left);
    free_AVL_node(node->right);
    free_data_node(node->data);
    free(node);
}

int compare_data_node(struct data_node *node1, struct data_node *node2) {
    return strncmp(node1->word, node2->word, MAX_WORD_LEN);
}

int get_height(struct AVL_node *node) {
    return (node != NULL)? node->height: -1;
}

int get_balance(struct AVL_node *node) {
    if (node == NULL) return 0;
    return get_height(node->left) - get_height(node->right);
}

void update_AVL_node_height(struct AVL_node *node) {
    node->height = 1 + max(get_height(node->left), get_height(node->right));
}

struct AVL_node *rotate_right(struct AVL_node *node) {
    struct AVL_node *new_parent = node->left;
    struct AVL_node *dangling_right = new_parent->right;

    node->left = dangling_right;
    new_parent->right = node;

    // Update node heights
    update_AVL_node_height(node);
    update_AVL_node_height(new_parent);

    return new_parent;
}

struct AVL_node *rotate_left(struct AVL_node *node) {
    struct AVL_node *new_parent = node->right;
    struct AVL_node *dangling_left = new_parent->left;

    node->right = dangling_left;
    new_parent->left = node;

    // UPdate node heights
    update_AVL_node_height(node);
    update_AVL_node_height(new_parent);

    return new_parent;
}

struct AVL_node *LL_rotation(struct AVL_node *node) {
    return rotate_right(node);
}

struct AVL_node *LR_rotation(struct AVL_node *node) {
    node->left = rotate_left(node->left);
    return rotate_right(node);
}

struct AVL_node *RL_rotation(struct AVL_node *node) {
    node->right = rotate_right(node->right);
    return rotate_left(node);
}

struct AVL_node *RR_rotation(struct AVL_node *node) {
    return rotate_left(node);
}

struct AVL_node *get_inorder_successor(struct AVL_node *node) {
    struct AVL_node *inorder_successor = node->right;

    if (inorder_successor == NULL) {
        return NULL;
    }

    while (inorder_successor->left != NULL) {
        inorder_successor = inorder_successor->left;
    }
    
    return inorder_successor;
}

struct AVL_node *get_inorder_predecessor(struct AVL_node *node) {
    struct AVL_node *inorderpredecessor = node->left;

    if (inorderpredecessor == NULL) {
        return NULL;
    }

    while (inorderpredecessor->right != NULL)
    {
        inorderpredecessor = inorderpredecessor->right;
    }
    
    return inorderpredecessor;
}

struct AVL_node *min_value_node(struct AVL_node *node) {
    struct AVL_node *current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

int max(int a, int b) {
    return a > b? a: b;
}

void inorder_traversal(struct AVL_node *node) {
    if (node) {
        inorder_traversal(node->left);
        printf("%s, %s, %s\n", node->data->word, node->data->meaning, node->data->dummy_str);
        inorder_traversal(node->right);
    }
}

void level_order_traversal(struct AVL_node *node) {

}