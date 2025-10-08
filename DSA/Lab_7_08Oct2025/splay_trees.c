// splay_trees.c
#include <stdio.h>
#include <stdlib.h>

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_POS_LEN 2048

struct data_node {
    int value;
    char *word;
    char *meaning;
    char *parts_of_speach;
};

struct splay_node {
    struct data_node *data;
    struct splay_node *left;
    struct splay_node *right;
    struct splay_node *parent;
};

struct splay_node *create_node(int value);
void free_node(struct splay_node *node);
struct splay_node *right_rotate(struct splay_node *x);
struct splay_node *left_rotate(struct splay_node *x);
struct splay_node *splay(struct splay_node *root, struct splay_node *x);
struct splay_node *insert_splay_tree(struct splay_node *root, int value);
struct splay_node *search_splay_tree(struct splay_node *root, int value);
struct splay_node *delete_splay_tree(struct splay_node *root, int value);
void inorder(struct splay_node *root);

struct splay_node *create_node(int value) {
    struct splay_node *node = malloc(sizeof(struct splay_node));
    if (!node) { perror("malloc"); exit(EXIT_FAILURE); }
    node->data = malloc(sizeof(struct data_node));
    if (!node->data) { perror("malloc"); exit(EXIT_FAILURE); }
    node->data->value = value;
    node->left = node->right = node->parent = NULL;
    return node;
}

void free_node(struct splay_node *node) {
    if (!node) return;
    free(node->data);
    free(node);
}

struct splay_node *right_rotate(struct splay_node *x) {
    if (!x) return NULL;
    struct splay_node *y = x->left;
    if (!y) return x;

    x->left = y->right;
    if (y->right) y->right->parent = x;

    y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
    }

    y->right = x;
    x->parent = y;

    return y;
}

struct splay_node *left_rotate(struct splay_node *x) {
    if (!x) return NULL;
    struct splay_node *y = x->right;
    if (!y) return x;

    x->right = y->left;
    if (y->left) y->left->parent = x;

    y->parent = x->parent;
    if (x->parent) {
        if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
    }

    y->left = x;
    x->parent = y;

    return y;
}

struct splay_node *splay(struct splay_node *root, struct splay_node *x) {
    if (!x) return root;

    while (x->parent) {
        struct splay_node *p = x->parent;
        struct splay_node *g = p->parent;

        if (!g) {
            /* Zig */
            if (x == p->left) {
                right_rotate(p);
            } else {
                left_rotate(p);
            }
        } else if (x == p->left && p == g->left) {
            /* Zig-Zig left-left */
            right_rotate(g);
            right_rotate(p);
        } else if (x == p->right && p == g->right) {
            /* Zig-Zig right-right */
            left_rotate(g);
            left_rotate(p);
        } else if (x == p->right && p == g->left) {
            /* Zig-Zag left-right */
            left_rotate(p);
            right_rotate(g);
        } else { /* x == p->left && p == g->right */
            /* Zig-Zag right-left */
            right_rotate(p);
            left_rotate(g);
        }
    }

    return x;
}

struct splay_node *insert_splay_tree(struct splay_node *root, int value) {
    struct splay_node *z = root;
    struct splay_node *p = NULL;

    while (z) {
        p = z;
        if (value < z->data->value) z = z->left;
        else if (value > z->data->value) z = z->right;
        else {
            return splay(root, z);
        }
    }

    struct splay_node *node = create_node(value);
    node->parent = p;

    if (!p)
        root = node;
    else if (value < p->data->value)
        p->left = node;
    else
        p->right = node;

    return splay(root, node);
}

struct splay_node *search_splay_tree(struct splay_node *root, int value) {
    struct splay_node *z = root;
    struct splay_node *last = NULL;

    while (z) {
        last = z;
        if (value < z->data->value) z = z->left;
        else if (value > z->data->value) z = z->right;
        else return splay(root, z);
    }

    if (last) return splay(root, last);
    return root;
}

struct splay_node *delete_splay_tree(struct splay_node *root, int value) {
    if (!root) return NULL;

    root = search_splay_tree(root, value);

    if (!root || root->data->value != value) {
        return root;
    }

    struct splay_node *left = root->left;
    struct splay_node *right = root->right;

    if (left) left->parent = NULL;
    if (right) right->parent = NULL;

    free_node(root);

    if (!left) {
        return right;
    } else {
        struct splay_node *max = left;
        while (max->right) max = max->right;
        left = splay(left, max); 
        left->right = right;
        if (right) right->parent = left;
        return left;
    }
}

void inorder(struct splay_node *root) {
    if (!root) return;
    inorder(root->left);
    printf("%d ", root->data->value);
    inorder(root->right);
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int height(struct splay_node* root) {
    if (root == NULL) return 0;
    int leftHeight = height(root->left);
    int rightHeight = height(root->right);
    return max(leftHeight,rightHeight) + 1;
}

void printGivenLevel(struct splay_node* root, int level) {
    if (root == NULL) {
        printf("NULL\t");
        return;
    }
    if (level == 1) printf("%d ", root->data->value);
    else if (level > 1) {
        printGivenLevel(root->left, level - 1);
        printGivenLevel(root->right, level - 1);
    }
}

void levelOrderTraversal(struct splay_node* root) {
    int h = height(root);
    for (int i = 1; i <= h; i++) {
        printGivenLevel(root, i);
        printf("\n");
    }
}

int main(void) {
    struct splay_node *root = NULL;

    char arr[] = {"10", "15", "7", "30", "38", "25", "26"};

    for (int i=0; i< sizeof(arr)/sizeof(arr[0]); i++) {
        root = insert_splay_tree(root, arr[i]);
    }

    printf("Level Order traversal after insertions: \n");
    levelOrderTraversal(root);
    printf("\n");

    root = search_splay_tree(root, 15);
    if (root) printf("Root after searching 15: %d\n", root->data->value);

    root = delete_splay_tree(root, 10);
    printf("Level Order after deleting 10: \n");
    levelOrderTraversal(root);
    printf("\n");

    while (root) {
        root = delete_splay_tree(root, root->data->value);
    }

    return 0;
}
