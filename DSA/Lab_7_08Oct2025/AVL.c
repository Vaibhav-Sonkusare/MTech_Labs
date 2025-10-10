#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_POS_LEN 2048

struct data_node {
    char word[MAX_WORD_LEN];
    char meaning[MAX_MEANING_LEN];
    char parts_of_speach[MAX_POS_LEN];
};

struct AVL_node {
    struct data_node data;
    struct AVL_node *left;
    struct AVL_node *right;
    int height;
};

int max(int a, int b) { return (a > b) ? a : b; }
int height(struct AVL_node *n) { return n ? n->height : 0; }

int compare_data(const struct data_node *a, const struct data_node *b) {
    return strcmp(a->word, b->word);
}

struct AVL_node *create_node(const char *word, const char *meaning, const char *parts_of_speach) {
    struct AVL_node *node = malloc(sizeof(struct AVL_node));
    if (!node) {
        perror("malloc");
        exit(1);
    }
    strncpy(node->data.word, word, MAX_WORD_LEN - 1);
    strncpy(node->data.meaning, meaning, MAX_MEANING_LEN - 1);
    strncpy(node->data.parts_of_speach, parts_of_speach, MAX_POS_LEN - 1);
    node->data.word[MAX_WORD_LEN - 1] = '\0';
    node->data.meaning[MAX_MEANING_LEN - 1] = '\0';
    node->data.parts_of_speach[MAX_POS_LEN - 1] = '\0';
    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

void *append_node(struct AVL_node *node, const char *new_pos) {
    size_t curr_pos_len = strlen(node->data.parts_of_speach);
    if (curr_pos_len >= MAX_POS_LEN - MAX_WORD_LEN) {
        printf("Overflow!\n");
        return node;
    }

    char *seperator = ", ";
    strncpy(node->data.parts_of_speach + curr_pos_len, seperator, strlen(seperator));
    curr_pos_len += strlen(seperator);
    strncpy(node->data.parts_of_speach + curr_pos_len, new_pos, MAX_WORD_LEN);
}

/* Rotations */
struct AVL_node *rotate_right(struct AVL_node *y) {
    struct AVL_node *x = y->left;
    struct AVL_node *T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = 1 + max(height(y->left), height(y->right));
    x->height = 1 + max(height(x->left), height(x->right));
    return x;
}

struct AVL_node *rotate_left(struct AVL_node *x) {
    struct AVL_node *y = x->right;
    struct AVL_node *T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = 1 + max(height(x->left), height(x->right));
    y->height = 1 + max(height(y->left), height(y->right));
    return y;
}

/* Get balance factor */
int get_balance(struct AVL_node *n) {
    return n ? height(n->left) - height(n->right) : 0;
}

/* Insertion */
struct AVL_node *insert(struct AVL_node *node, const char *word, const char *meaning, const char *pos) {
    if (node == NULL)
        return create_node(word, meaning, pos);

    struct data_node temp;
    strncpy(temp.word, word, MAX_WORD_LEN - 1);
    temp.word[MAX_WORD_LEN - 1] = '\0';

    if (strcmp(word, node->data.word) < 0) {
        node->left = insert(node->left, word, meaning, pos);
    } else if (strcmp(word, node->data.word) > 0) {
        node->right = insert(node->right, word, meaning, pos);
    } else {
        append_node(node, pos);
        return node;
    }

    node->height = 1 + max(height(node->left), height(node->right));

    int balance = get_balance(node);

    // Rebalance
    if (balance > 1 && strcmp(word, node->left->data.word) < 0)
        return rotate_right(node);
    if (balance < -1 && strcmp(word, node->right->data.word) > 0)
        return rotate_left(node);
    if (balance > 1 && strcmp(word, node->left->data.word) > 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (balance < -1 && strcmp(word, node->right->data.word) < 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

/* Find minimum node */
struct AVL_node *min_value_node(struct AVL_node *node) {
    struct AVL_node *current = node;
    while (current->left)
        current = current->left;
    return current;
}

/* Deletion */
struct AVL_node *delete_node(struct AVL_node *root, const char *word) {
    if (root == NULL)
        return root;

    if (strcmp(word, root->data.word) < 0)
        root->left = delete_node(root->left, word);
    else if (strcmp(word, root->data.word) > 0)
        root->right = delete_node(root->right, word);
    else {
        // Node with one or no child
        if (root->left == NULL || root->right == NULL) {
            struct AVL_node *temp = root->left ? root->left : root->right;
            free(root);
            return temp;
        } else {
            // Two children: get inorder successor
            struct AVL_node *temp = min_value_node(root->right);
            root->data = temp->data; // copy struct, not pointers
            root->right = delete_node(root->right, temp->data.word);
        }
    }

    root->height = 1 + max(height(root->left), height(root->right));
    int balance = get_balance(root);

    // Rebalance
    if (balance > 1 && get_balance(root->left) >= 0)
        return rotate_right(root);
    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (balance < -1 && get_balance(root->right) <= 0)
        return rotate_left(root);
    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

/* Search */
struct AVL_node *search(struct AVL_node *root, const char *word) {
    if (!root)
        return NULL;
    int cmp = strcmp(word, root->data.word);
    if (cmp == 0)
        return root;
    if (cmp < 0)
        return search(root->left, word);
    else
        return search(root->right, word);
}

/* Traversals */
void inorder(struct AVL_node *root) {
    if (!root) return;
    inorder(root->left);
    printf("word:%s, meaning:%s, pos:%s\n", root->data.word, root->data.meaning, root->data.parts_of_speach);
    inorder(root->right);
}

// Function to get the height of the tree
int _height(struct AVL_node* root) {
    if (root == NULL) return 0;
    int leftHeight = height(root->left);
    int rightHeight = height(root->right);
    return max(leftHeight,rightHeight) + 1;
}

void printGivenLevel(struct AVL_node* root, int level) {
    if (root == NULL) {
        printf("NULL\t");
        return;
    }
    if (level == 1) printf("w:%s, m:%s, p:%s\t ", root->data.word, root->data.meaning, root->data.parts_of_speach);
    else if (level > 1) {
        printGivenLevel(root->left, level - 1);
        printGivenLevel(root->right, level - 1);
    }
}

// Function for level order traversal of the tree without using a queue
void levelOrderTraversal(struct AVL_node* root) {
    int h = _height(root);
    for (int i = 1; i <= h; i++) {
        printGivenLevel(root, i);
        printf("\n");
    }
}

/* Free tree */
void free_tree(struct AVL_node *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

void print_2_empty_lines() {
    printf("\n\n");
}

/* Main */
int main() {
    struct AVL_node *root = NULL;
    root = insert(root, "banana", "word", "fruit");
    root = insert(root, "banana", "word", "make (in hindi)");
    root = insert(root, "apple", "word", "fruit");
    root = insert(root, "copy", "word", "notebook");
    root = insert(root, "copy", "word", "duplicate");
    root = insert(root, "carrot", "word", "vegetable");
    root = insert(root, "copy", "word", "cheat");

    printf("Level Order traversal after insertions:\n");
    levelOrderTraversal(root);
    print_2_empty_lines();

    struct AVL_node *found = search(root, "copy");
    if (found)
        printf("Found:\tw:%s, m:%s, p:%s\n", found->data.word, found->data.meaning, found->data.parts_of_speach);

    print_2_empty_lines();
    root = delete_node(root, "banana");

    printf("Level Order traversal after deletion of banana:\n");
    levelOrderTraversal(root);

    free_tree(root);
    return 0;
}
