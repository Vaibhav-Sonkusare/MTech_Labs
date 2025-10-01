#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

struct QueueNode {
    struct TreeNode* treeNode;
    struct QueueNode* next;
};

struct Queue {
    struct QueueNode *front, *rear;
};

struct QueueNode* newQueueNode(struct TreeNode *);
struct Queue* createQueue();
void enqueue(struct Queue *, struct TreeNode *);
struct TreeNode* dequeue(struct Queue *);
bool isEmpty(struct Queue *);
int queue_length(struct Queue *);
struct TreeNode* createNode(int);
struct TreeNode* buildBST(int *, int, int);
struct TreeNode* sortedArrayToBST(int *, int);
void printInOrder(struct TreeNode *);
void printLevelOrder(struct TreeNode *);
void printLevelOrder2(struct TreeNode *);
void freeTree(struct TreeNode *);

int main() {
    int nums[] = {-10, -3, 0, 5, 9};
    int numsSize = sizeof(nums) / sizeof(nums[0]);

    printf("Original sorted array: ");
    for (int i = 0; i < numsSize; i++) {
        printf("%d ", nums[i]);
    }
    printf("\n\n");

    struct TreeNode* root = sortedArrayToBST(nums, numsSize);

    printf("In-order traversal (verification): ");
    printInOrder(root);
    printf("\n");

    // printf("Level-order traversal (visualization): ");
    // printLevelOrder(root);
    // printf("\n\n");
    
    freeTree(root);
    
    return 0;
}

struct QueueNode* newQueueNode(struct TreeNode* node) {
    struct QueueNode* temp = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    temp->treeNode = node;
    temp->next = NULL;
    return temp;
}

struct Queue* createQueue() {
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

void enqueue(struct Queue* q, struct TreeNode* node) {
    struct QueueNode* temp = newQueueNode(node);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

struct TreeNode* dequeue(struct Queue* q) {
    if (q->front == NULL) {
        return NULL;
    }
    struct QueueNode* temp = q->front;
    struct TreeNode* dequeuedNode = temp->treeNode;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }
    free(temp);
    return dequeuedNode;
}

bool isEmpty(struct Queue* q) {
    return q->front == NULL;
}

struct TreeNode* createNode(int val) {
    struct TreeNode* newNode = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    newNode->val = val;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

struct TreeNode* buildBST(int* nums, int start, int end) {
    if (start > end) {
        return NULL;
    }
    int mid = start + (end - start) / 2;
    struct TreeNode* root = createNode(nums[mid]);
    root->left = buildBST(nums, start, mid - 1);
    root->right = buildBST(nums, mid + 1, end);
    return root;
}

struct TreeNode* sortedArrayToBST(int* nums, int numsSize) {
    if (numsSize <= 0) {
        return NULL;
    }
    return buildBST(nums, 0, numsSize - 1);
}

void printInOrder(struct TreeNode* node) {
    if (node == NULL) return;
    printInOrder(node->left);
    printf("%d ", node->val);
    printInOrder(node->right);
}

void printLevelOrder2(struct TreeNode* root) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }
    
    struct Queue* q = createQueue();
    enqueue(q, root);
    
    while (!isEmpty(q)) {
        struct TreeNode* currentNode = dequeue(q);
        printf("%d ", currentNode->val);
        
        if (currentNode->left != NULL) {
            enqueue(q, currentNode->left);
        }
        
        if (currentNode->right != NULL) {
            enqueue(q, currentNode->right);
        }
    }
    free(q);
}

void printLevelOrder(struct TreeNode* root) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }
    
    struct Queue* q = createQueue();
    enqueue(q, root);
    
    while (!isEmpty(q)) {
        int level_nodes = queue_length(q);
        printf("%d\n", level_nodes);

        while (level_nodes-- != 0) {
            struct TreeNode* currentNode = dequeue(q);
            printf("%d ", currentNode->val);
            
            if (currentNode->left != NULL) {
                enqueue(q, currentNode->left);
            }
            
            if (currentNode->right != NULL) {
                enqueue(q, currentNode->right);
            }
        }

        printf("\n");
    }
    free(q);
}

int queue_length(struct Queue* q) {
    int len = 0;
    struct QueueNode *itr = q->front;

    while (itr != q->rear) {
        len++;
        itr = itr->next;
    }

    return len;
}

void freeTree(struct TreeNode* node) {
    if (node == NULL) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}