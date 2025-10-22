// k-d_tree.c

#include <stdio.h>
#include <stdlib.h>

// ----- STRUCTURES -----
struct data_node {
    int x;
    int y;
};

struct kdtree_node {
    struct data_node *data;
    struct kdtree_node *left;
    struct kdtree_node *right;
    short level; // used to decide which dimension to compare
};

// ----- FUNCTION PROTOTYPES -----
struct data_node *create_data_node(int x, int y);
struct kdtree_node *create_node(struct data_node *data, short level);
struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data);
struct kdtree_node *search_data(struct kdtree_node *node, struct data_node *data);
struct kdtree_node *delete_data(struct kdtree_node *node, struct data_node *data);

// helper prototypes
void print_kdtree(struct kdtree_node *node);
void free_kdtree(struct kdtree_node *node);
static struct kdtree_node *find_min(struct kdtree_node *node, int dim, short level);

// ----- FUNCTION IMPLEMENTATIONS -----

struct data_node *create_data_node(int x, int y) {
    struct data_node *new_data_node = calloc(1, sizeof(struct data_node));
    new_data_node->x = x;
    new_data_node->y = y;
    return new_data_node;
}

struct kdtree_node *create_node(struct data_node *data, short level) {
    struct kdtree_node *new_node = calloc(1, sizeof(struct kdtree_node));
    new_node->data = data;
    new_node->level = level;
    new_node->left = NULL;
    new_node->right = NULL;
    return new_node;
}

// Compare two data nodes on given dimension (0 for x, 1 for y)
static int compare_on_dim(struct data_node *a, struct data_node *b, int dim) {
    return (dim == 0) ? (a->x - b->x) : (a->y - b->y);
}

// Insert data into KD-tree
struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data) {
    if (node == NULL)
        return create_node(data, 0);

    int cd = node->level % 2; // current dimension

    if (compare_on_dim(data, node->data, cd) < 0) {
        if (node->left == NULL)
            node->left = create_node(data, node->level + 1);
        else
            node->left = insert_data(node->left, data);
    } else {
        if (node->right == NULL)
            node->right = create_node(data, node->level + 1);
        else
            node->right = insert_data(node->right, data);
    }

    return node;
}

// Search for a data point in KD-tree
struct kdtree_node *search_data(struct kdtree_node *node, struct data_node *data) {
    if (node == NULL)
        return NULL;

    if (node->data->x == data->x && node->data->y == data->y)
        return node;

    int cd = node->level % 2;

    if (compare_on_dim(data, node->data, cd) < 0)
        return search_data(node->left, data);
    else
        return search_data(node->right, data);
}

// Find node with minimum value in given dimension
static struct kdtree_node *find_min(struct kdtree_node *node, int dim, short level) {
    if (node == NULL)
        return NULL;

    int cd = level % 2;

    if (cd == dim) {
        if (node->left == NULL)
            return node;
        return find_min(node->left, dim, node->left->level);
    }

    struct kdtree_node *left_min = find_min(node->left, dim, node->left ? node->left->level : level + 1);
    struct kdtree_node *right_min = find_min(node->right, dim, node->right ? node->right->level : level + 1);
    struct kdtree_node *min = node;

    if (left_min && compare_on_dim(left_min->data, min->data, dim) < 0)
        min = left_min;
    if (right_min && compare_on_dim(right_min->data, min->data, dim) < 0)
        min = right_min;

    return min;
}

// Delete a data node
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

// Print KD-tree (inorder traversal)
void print_kdtree(struct kdtree_node *node) {
    if (node == NULL)
        return;
    print_kdtree(node->left);
    printf("(x=%d, y=%d, level=%d)\n", node->data->x, node->data->y, node->level);
    print_kdtree(node->right);
}

// Free KD-tree
void free_kdtree(struct kdtree_node *node) {
    if (node == NULL)
        return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free(node->data);
    free(node);
}

// ----- SAMPLE MAIN -----
int main() {
    struct kdtree_node *root = NULL;

    // Insert sample points
    root = insert_data(root, create_data_node(3, 6));
    root = insert_data(root, create_data_node(17, 15));
    root = insert_data(root, create_data_node(13, 15));
    root = insert_data(root, create_data_node(6, 12));
    root = insert_data(root, create_data_node(9, 1));
    root = insert_data(root, create_data_node(2, 7));
    root = insert_data(root, create_data_node(10, 19));

    printf("KD-Tree after insertions:\n");
    print_kdtree(root);
    printf("\n");

    // // Search example
    // struct data_node target = {10, 19};
    // struct kdtree_node *found = search_data(root, &target);
    // if (found)
    //     printf("Found node: (%d, %d)\n", found->data->x, found->data->y);
    // else
    //     printf("Node not found!\n");


    // // Delete example
    // printf("\nDeleting node (3,6)...\n");
    // struct data_node to_delete = {3, 6};
    // root = delete_data(root, &to_delete);

    // printf("\nKD-Tree after deletion:\n");
    // print_kdtree(root);

    free_kdtree(root);
    return 0;
}
