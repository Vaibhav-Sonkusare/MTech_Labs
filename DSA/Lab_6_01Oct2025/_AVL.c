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
    int height;
};

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_DUMMY_STR_LEN 32

struct AVL_node *AVL_insert(struct AVL_node *, struct data_node *);
struct AVL_node *AVL_delete(struct AVL_node *, struct data_node *);
struct AVL_node *AVL_search(struct AVL_node *, struct data_node *);
struct data_node *create_data_node(char *, char *, char *);
void free_data_node(struct data_node *);
struct AVL_node *create_AVL_node(struct data_node *);
void free_AVL_node(struct AVL_node *);
int compare_data_node(struct data_node *, struct data_node *);
int max(int, int);

// AVL helper functions
int get_height(struct AVL_node *node) {
    return node ? node->height : -1;
}

int get_balance(struct AVL_node *node) {
    if (!node) return 0;
    return get_height(node->left) - get_height(node->right);
}

// Rotations
struct AVL_node *rotate_right(struct AVL_node *y) {
    struct AVL_node *x = y->left;
    struct AVL_node *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = 1 + max(get_height(y->left), get_height(y->right));
    x->height = 1 + max(get_height(x->left), get_height(x->right));

    return x;
}

struct AVL_node *rotate_left(struct AVL_node *x) {
    struct AVL_node *y = x->right;
    struct AVL_node *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = 1 + max(get_height(x->left), get_height(x->right));
    y->height = 1 + max(get_height(y->left), get_height(y->right));

    return y;
}

// Insertion with balancing
struct AVL_node *AVL_insert(struct AVL_node *node, struct data_node *data) {
    if (node == NULL)
        return create_AVL_node(data);

    if (compare_data_node(data, node->data) < 0)
        node->left = AVL_insert(node->left, data);
    else if (compare_data_node(data, node->data) > 0)
        node->right = AVL_insert(node->right, data);
    else {
        fprintf(stderr, "AVL_insert: %s already exists.\n", data->word);
        return node;
    }

    node->height = 1 + max(get_height(node->left), get_height(node->right));

    int balance = get_balance(node);

    // Left Left Case
    if (balance > 1 && compare_data_node(data, node->left->data) < 0)
        return rotate_right(node);

    // Right Right Case
    if (balance < -1 && compare_data_node(data, node->right->data) > 0)
        return rotate_left(node);

    // Left Right Case
    if (balance > 1 && compare_data_node(data, node->left->data) > 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // Right Left Case
    if (balance < -1 && compare_data_node(data, node->right->data) < 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

// Find minimum node
struct AVL_node *min_value_node(struct AVL_node *node) {
    struct AVL_node *current = node;
    while (current->left != NULL)
        current = current->left;
    return current;
}

// Deletion with balancing
struct AVL_node *AVL_delete(struct AVL_node *root, struct data_node *data) {
    if (root == NULL)
        return root;

    if (compare_data_node(data, root->data) < 0)
        root->left = AVL_delete(root->left, data);
    else if (compare_data_node(data, root->data) > 0)
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

// Search
struct AVL_node *AVL_search(struct AVL_node *root, struct data_node *data) {
    if (!root) return NULL;

    if (compare_data_node(data, root->data) == 0)
        return root;
    else if (compare_data_node(data, root->data) < 0)
        return AVL_search(root->left, data);
    else
        return AVL_search(root->right, data);
}

// Data node functions
struct data_node *create_data_node(char *word, char *meaning, char *dummy_str) {
    struct data_node *new_data_node = calloc(1, sizeof(struct data_node));
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
    struct AVL_node *new_AVL_node = calloc(1, sizeof(struct AVL_node));
    new_AVL_node->data = data;
    new_AVL_node->left = new_AVL_node->right = NULL;
    new_AVL_node->height = 0;
    return new_AVL_node;
}

void free_AVL_node(struct AVL_node *node) {
    if (!node) return;
    free_AVL_node(node->left);
    free_AVL_node(node->right);
    free_data_node(node->data);
    free(node);
}

int compare_data_node(struct data_node *node1, struct data_node *node2) {
    return strncmp(node1->word, node2->word, MAX_WORD_LEN);
}

int max(int a, int b) {
    return a > b ? a : b;
}

// Optional: Inorder traversal for debugging
void inorder_traversal(struct AVL_node *root) {
    if (!root) return;
    inorder_traversal(root->left);
    printf("%s: %s\n", root->data->word, root->data->meaning);
    inorder_traversal(root->right);
}

int main() {
    struct AVL_node *root = NULL;

    struct data_node *d1 = create_data_node("apple", "fruit", "dummy1");
    struct data_node *d2 = create_data_node("banana", "fruit", "dummy2");
    struct data_node *d3 = create_data_node("carrot", "vegetable", "dummy3");

    root = AVL_insert(root, d1);
    root = AVL_insert(root, d2);
    root = AVL_insert(root, d3);

    printf("Inorder traversal after insertions:\n");
    inorder_traversal(root);

    struct AVL_node *found = AVL_search(root, create_data_node("banana", "", ""));
    if (found)
        printf("Found: %s -> %s\n", found->data->word, found->data->meaning);

    root = AVL_delete(root, create_data_node("banana", "", ""));
    printf("Inorder traversal after deletion of banana:\n");
    inorder_traversal(root);

    free_AVL_node(root);
    return 0;
}
