// k-d_tree.c

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_POS_LEN 2048

struct data_node {
    int x;
    int y;
};

struct kdtree_node
{
    struct data_node *data;
    struct kdtree_node *left;
    struct kdtree_node *right;
    short level;
};

// Fucntion Prototype
struct data_node *create_data_node(int x, int y);
struct kdtree_node *create_node(struct data_node *data, short level);
struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data);
void search_data(struct kdtree_node *node, int xlow, int xhigh, int ylow, int yhigh);
struct kdtree_node *delete_data(struct kdtree_node *node, struct data_node *data);
static struct kdtree_node *find_min(struct kdtree_node *node, int dimension, short level);
void print_kdtree_inorder(struct kdtree_node *node);
static int compare_on_dim(struct data_node *data1, struct data_node *data2, int dimension);
void free_kdtree(struct kdtree_node *node);
void levelOrderTraversal(struct kdtree_node* root);
void printGivenLevel(struct kdtree_node* root, int level);


int main() {
    struct kdtree_node *root = NULL;

    root = insert_data(root, create_data_node(1, 8));
    root = insert_data(root, create_data_node(7, 15));
    root = insert_data(root, create_data_node(13, 15));
    root = insert_data(root, create_data_node(16, 12));
    root = insert_data(root, create_data_node(9, 1));
    root = insert_data(root, create_data_node(2, 7));
    root = insert_data(root, create_data_node(10, 19));

    printf("KD-Tree after insertions:\n");
    levelOrderTraversal(root);
    printf("\n");

    int xlow = 4;
    int xhigh = 13;
    int ylow = 12;
    int yhigh = 15;

    printf("Range search in:\n");
    printf("\t x\t y\n");
    printf("low \t%.2d\t%.2d\n", xlow, ylow);
    printf("high\t%.2d\t%.2d\n", xhigh, yhigh);
    search_data(root, xlow, xhigh, ylow, yhigh);
    printf("\n\n");

    struct data_node *data = create_data_node(1, 8);
    printf("Deleting node: (%d, %d)\n", data->x, data->y);
    root = delete_data(root, data);

    printf("KD-Tree after deletion:\n");
    levelOrderTraversal(root);
    printf("\n");

    free_kdtree(root);

    return 0;
}


struct data_node *create_data_node(int x, int y) {
    struct data_node *new_data_node = calloc(1, sizeof(struct data_node));

    new_data_node->x = x;
    new_data_node->y = y;

    return new_data_node;
}

struct kdtree_node *create_node(struct data_node *data, short level) {
    struct kdtree_node *new_node = calloc(1, sizeof(struct kdtree_node));

    new_node->data = data;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->level = level;

    return new_node;
}

struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data) {
    if (node == NULL) {
        return create_node(data, 0);
    }

    int val1, val2;
    if (node->level % 2 == 0) {         // --> x 
        val1 = node->data->x;
        val2 = data->x;
    } else {
        val1 = node->data->y;
        val2 = data->y;
    }

    // if (val1 == val2) {
    //     fprintf(stderr, "same level %d point!\n", node->level);
    // } else 
    if (val1 < val2) {
        // go to tree right
        node->right = insert_data(node->right, data);
        node->right->level = node->level + 1;
    } else {
        // go to tree left
        node->left = insert_data(node->left, data);
        node->left->level = node->level + 1;
    }

    return node;
}

void search_data(struct kdtree_node *node, int xlow, int xhigh, int ylow, int yhigh) {
    if (node == NULL || xlow > xhigh || ylow > yhigh) {
        return;
    }

    int ckey, clow, chigh;
    if (node->level % 2 == 0) {         // --> x 
        ckey = node->data->x;
        clow = xlow;
        chigh = xhigh;
    } else {
        ckey = node->data->y;
        clow = ylow;
        chigh = yhigh;
    }

    if (ckey >= clow || ckey <= chigh) {
        bool xsatisfy = (node->data->x >= xlow) && (node->data->x <= xhigh)? true: false;
        bool ysatisfy = (node->data->y >= ylow) && (node->data->y <= yhigh)? true: false;

        if (xsatisfy && ysatisfy) {
            printf("(%d, %d), ", node->data->x, node->data->y);
        }

        search_data(node->left, xlow, xhigh, ylow, yhigh);
        search_data(node->right, xlow, xhigh, ylow, yhigh);
    } else if (ckey < clow) {
        // go to tree right
        search_data(node->right, xlow, xhigh, ylow, yhigh);
    } else if (ckey > chigh) {
        // go to tree left
        search_data(node->left, xlow, xhigh, ylow, yhigh);
    }

    return;
}

struct kdtree_node *delete_data(struct kdtree_node *node, struct data_node *data) {
    if (node == NULL)
        return NULL;

    int cd = node->level % 2;

    if (node->data->x == data->x && node->data->y == data->y) {
        if (node->right != NULL) {
            struct kdtree_node *min = find_min(node->right, cd, node->right->level);
            node->data->x = min->data->x;
            node->data->y = min->data->y;
            node->right = delete_data(node->right, min->data);
        } else if (node->left != NULL) {
            struct kdtree_node *min = find_min(node->left, cd, node->left->level);
            node->data->x = min->data->x;
            node->data->y = min->data->y;
            node->right = delete_data(node->left, min->data);
            node->left = NULL;
        } else {
            free(node->data);
            free(node);
            return NULL;
        }
        return node;
    }

    if (compare_on_dim(data, node->data, cd) < 0)
        node->left = delete_data(node->left, data);
    else
        node->right = delete_data(node->right, data);

    return node;
}

static struct kdtree_node *find_min(struct kdtree_node *node, int dimension, short level) {
    if (node == NULL)
        return NULL;

    int cd = level % 2;

    if (cd == dimension) {
        if (node->left == NULL)
            return node;
        return find_min(node->left, dimension, node->left->level);
    }

    struct kdtree_node *left_min = find_min(node->left, dimension, node->left ? node->left->level : level + 1);
    struct kdtree_node *right_min = find_min(node->right, dimension, node->right ? node->right->level : level + 1);
    struct kdtree_node *min = node;

    if (cd == 0) {

    }
    if (left_min && compare_on_dim(left_min->data, min->data, dimension) < 0)
        min = left_min;
    if (right_min && compare_on_dim(right_min->data, min->data, dimension) < 0)
        min = right_min;

    return min;
}

void print_kdtree_inorder(struct kdtree_node *node) {
    if (node == NULL)
        return;
    print_kdtree_inorder(node->left);
    printf("(x=%d, y=%d, level=%d)\n", node->data->x, node->data->y, node->level);
    print_kdtree_inorder(node->right);
}

static int compare_on_dim(struct data_node *a, struct data_node *b, int dim) {
    return (dim == 0) ? (a->x - b->x) : (a->y - b->y);
}

void free_kdtree(struct kdtree_node *node) {
    if (node == NULL)
        return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node->data);
    free(node);
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int height(struct kdtree_node* root) {
    if (root == NULL) return 0;
    int leftHeight = height(root->left);
    int rightHeight = height(root->right);
    return max(leftHeight,rightHeight) + 1;
}

void printGivenLevel(struct kdtree_node* node, int level) {
    if (node == NULL) {
        printf("NULL\t");
        return;
    }
    if (level == 1) printf("(%.2d, %.2d) ", node->data->x, node->data->y);
    else if (level > 1) {
        printGivenLevel(node->left, level - 1);
        printGivenLevel(node->right, level - 1);
    }
}

void levelOrderTraversal(struct kdtree_node* root) {
    int h = height(root);
    for (int i = 1; i <= h; i++) {
        printGivenLevel(root, i);
        printf("\n");
    }
}