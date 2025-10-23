// k-d_tree.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_WORD_LEN 32
#define MAX_MEANING_LEN 32
#define MAX_POS_LEN 2048

struct data_node {
    int value;
    char *word;
    char *meaning;
    char *parts_of_speach;
};

struct kdtree_node
{
    struct data_node *data;
    struct kdtree_node *left;
    struct kdtree_node *right;
    short level; // depth of this node (root level usually 0)
};

// Function prototypes (user-facing)
struct data_node *create_data_node(int value, char *word, char *meaning, char *parts_of_speach);
struct kdtree_node *create_node(struct data_node *data, short level);
struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data);
struct kdtree_node *search_data(struct kdtree_node *node, struct data_node *data);
struct kdtree_node *delete_data(struct kdtree_node *node, struct data_node *data);

// Helper prototypes
static void free_data_node(struct data_node *d);
static int compare_on_dim(const struct data_node *a, const struct data_node *b, int dim);
static struct kdtree_node *find_min(struct kdtree_node *node, int dim, short level);
static struct kdtree_node *delete_node_recursive(struct kdtree_node *node, const struct data_node *target);
static void print_kdtree(struct kdtree_node *node);
static void free_kdtree(struct kdtree_node *node);

/* -------------------------
   Implementation
   ------------------------- */

// safe strdup with max length limit (copies up to max_len-1 and null terminates)
static char *safe_strdup_limit(const char *src, size_t max_len) {
    if (!src) return NULL;
    size_t len = strlen(src);
    size_t copy_len = (len < (max_len - 1)) ? len : (max_len - 1);
    char *dst = malloc(copy_len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
    return dst;
}

struct data_node *create_data_node(int value, char *word, char *meaning, char *parts_of_speach) {
    struct data_node *d = malloc(sizeof(*d));
    if (!d) return NULL;
    d->value = value;
    // limit strings by the defined max lengths
    d->word = safe_strdup_limit(word ? word : "", MAX_WORD_LEN);
    d->meaning = safe_strdup_limit(meaning ? meaning : "", MAX_MEANING_LEN);
    d->parts_of_speach = safe_strdup_limit(parts_of_speach ? parts_of_speach : "", MAX_POS_LEN);
    return d;
}

struct kdtree_node *create_node(struct data_node *data, short level) {
    struct kdtree_node *n = malloc(sizeof(*n));
    if (!n) return NULL;
    n->data = data;
    n->left = NULL;
    n->right = NULL;
    n->level = level;
    return n;
}

// compare a and b on dimension dim (0 => value, 1 => word)
// returns <0 if a < b, 0 if equal, >0 if a > b on that dimension
static int compare_on_dim(const struct data_node *a, const struct data_node *b, int dim) {
    if (dim == 0) {
        if (a->value < b->value) return -1;
        if (a->value > b->value) return 1;
        return 0;
    } else {
        // compare words lexicographically
        return strcmp(a->word ? a->word : "", b->word ? b->word : "");
    }
}

struct kdtree_node *insert_data(struct kdtree_node *node, struct data_node *data) {
    if (!data) return node;
    if (node == NULL) {
        return create_node(data, 0); // root level assumed 0 if inserted into empty tree
    }
    // recursion with level info: we will descend passing increased level
    short level = node->level;
    int dim = level % 2;
    int cmp = compare_on_dim(data, node->data, dim);
    if (cmp < 0) {
        if (node->left)
            node->left = insert_data(node->left, data);
        else
            node->left = create_node(data, level + 1);
    } else {
        if (node->right)
            node->right = insert_data(node->right, data);
        else
            node->right = create_node(data, level + 1);
    }
    return node;
}

// search: matches on both dimensions (value && word). Returns pointer to tree node if found, NULL otherwise.
struct kdtree_node *search_data(struct kdtree_node *node, struct data_node *data) {
    if (node == NULL || data == NULL) return NULL;
    // check for exact match: both value and word
    if (node->data->value == data->value && strcmp(node->data->word ? node->data->word : "",
                                                    data->word ? data->word : "") == 0) {
        return node;
    }
    int dim = node->level % 2;
    int cmp = compare_on_dim(data, node->data, dim);
    if (cmp < 0) return search_data(node->left, data);
    else return search_data(node->right, data);
}

// find minimum node in subtree 'node' for dimension dim (0 or 1)
static struct kdtree_node *find_min(struct kdtree_node *node, int dim, short level) {
    if (node == NULL) return NULL;
    int curr_dim = level % 2;
    if (curr_dim == dim) {
        // minimum must be in left subtree if exists
        if (node->left == NULL) return node;
        return find_min(node->left, dim, node->left->level);
    } else {
        // need to find min among node, left subtree, right subtree
        struct kdtree_node *min = node;
        struct kdtree_node *lmin = find_min(node->left, dim, node->left ? node->left->level : level + 1);
        struct kdtree_node *rmin = find_min(node->right, dim, node->right ? node->right->level : level + 1);
        if (lmin) {
            if (compare_on_dim(lmin->data, min->data, dim) < 0) min = lmin;
        }
        if (rmin) {
            if (compare_on_dim(rmin->data, min->data, dim) < 0) min = rmin;
        }
        return min;
    }
}

// internal recursive delete which expects target data to match by value & word
static struct kdtree_node *delete_node_recursive(struct kdtree_node *node, const struct data_node *target) {
    if (node == NULL) return NULL;
    // check whether this is the node to delete (match on value and word)
    if (node->data->value == target->value &&
        strcmp(node->data->word ? node->data->word : "", target->word ? target->word : "") == 0) {
        // if right subtree exists, find min in right subtree in current dimension and replace
        int dim = node->level % 2;
        if (node->right != NULL) {
            struct kdtree_node *min = find_min(node->right, dim, node->right->level);
            // copy min->data into node->data (we'll free node->data and duplicate min->data)
            // To keep memory simpler: free current node->data and deep copy min->data
            struct data_node *old = node->data;
            // create a copy of min->data
            struct data_node *replacement = create_data_node(min->data->value,
                                                             min->data->word,
                                                             min->data->meaning,
                                                             min->data->parts_of_speach);
            node->data = replacement;
            // delete the min node from right subtree
            node->right = delete_node_recursive(node->right, min->data);
            // free old data
            free_data_node(old);
        } else if (node->left != NULL) {
            // no right subtree, use left subtree's min
            struct kdtree_node *min = find_min(node->left, dim, node->left->level);
            struct data_node *old = node->data;
            struct data_node *replacement = create_data_node(min->data->value,
                                                             min->data->word,
                                                             min->data->meaning,
                                                             min->data->parts_of_speach);
            node->data = replacement;
            // delete that min node from left subtree; then attach left subtree's result to right
            node->right = delete_node_recursive(node->left, min->data);
            node->left = NULL;
            free_data_node(old);
        } else {
            // leaf node: remove it
            free_data_node(node->data);
            free(node);
            return NULL;
        }
        return node;
    }
    // not matched yet: descend based on current dim
    int dim = node->level % 2;
    int cmp = compare_on_dim((const struct data_node *)target, node->data, dim);
    if (cmp < 0) node->left = delete_node_recursive(node->left, target);
    else node->right = delete_node_recursive(node->right, target);
    return node;
}

struct kdtree_node *delete_data(struct kdtree_node *node, struct data_node *data) {
    if (!data) return node;
    return delete_node_recursive(node, (const struct data_node *)data);
}

// free a data_node (strings + struct)
static void free_data_node(struct data_node *d) {
    if (!d) return;
    free(d->word);
    free(d->meaning);
    free(d->parts_of_speach);
    free(d);
}

// utility: in-order-ish print (this is NOT a balanced tree in KD sense but for debugging)
static void print_kdtree(struct kdtree_node *node) {
    if (!node) return;
    print_kdtree(node->left);
    printf("level=%d | value=%d | word=\"%s\" | meaning=\"%s\" | pos=\"%s\"\n",
           node->level,
           node->data->value,
           node->data->word ? node->data->word : "",
           node->data->meaning ? node->data->meaning : "",
           node->data->parts_of_speach ? node->data->parts_of_speach : "");
    print_kdtree(node->right);
}

static void free_kdtree(struct kdtree_node *node) {
    if (!node) return;
    free_kdtree(node->left);
    free_kdtree(node->right);
    free_data_node(node->data);
    free(node);
}

/* -------------------------
   Example usage in main()
   ------------------------- */
int main() {
    struct kdtree_node *root = NULL;

    // create some sample data nodes
    struct data_node *d1 = create_data_node(10, "apple", "a fruit", "noun");
    struct data_node *d2 = create_data_node(20, "banana", "another fruit", "noun");
    struct data_node *d3 = create_data_node(5,  "apricot", "yet another fruit", "noun");
    struct data_node *d4 = create_data_node(10, "avocado", "green fruit", "noun");
    struct data_node *d5 = create_data_node(15, "blueberry", "berry", "noun");

    // insert nodes
    // note: insert_data will attach the provided data_node pointers into the tree;
    // do not free them after insertion, tree owns them
    root = insert_data(root, d1);
    // ensure root->level is 0; if tree existed we left levels as created when nodes were created.
    if (root) root->level = 0;
    root = insert_data(root, d2);
    root = insert_data(root, d3);
    root = insert_data(root, d4);
    root = insert_data(root, d5);

    printf("Tree after inserts:\n");
    print_kdtree(root);
    printf("\n");

    // search
    struct data_node query = {10, "avocado", NULL, NULL};
    struct kdtree_node *found = search_data(root, &query);
    if (found) {
        printf("Found node: value=%d word=%s meaning=%s\n",
               found->data->value,
               found->data->word,
               found->data->meaning);
    } else {
        printf("Node not found for value=%d word=%s\n", query.value, query.word);
    }
    printf("\n");

    // delete a node (delete avocado)
    struct data_node target = {10, "avocado", NULL, NULL};
    root = delete_data(root, &target);

    printf("Tree after deleting (10,\"avocado\"):\n");
    print_kdtree(root);
    printf("\n");

    // attempt to delete a node that doesn't exist
    struct data_node missing = {999, "nope", NULL, NULL};
    root = delete_data(root, &missing); // safe no-op

    // cleanup entire tree
    free_kdtree(root);
    root = NULL;

    return 0;
}
