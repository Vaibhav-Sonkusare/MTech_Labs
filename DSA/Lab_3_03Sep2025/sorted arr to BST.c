#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // For bool type in isEmpty

// Definition for a binary tree node.
struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

// --- Queue Implementation for Level Order Traversal ---

// A queue node (to store tree nodes)
struct QueueNode {
    struct TreeNode* treeNode;
    struct QueueNode* next;
};

// The queue, front stores the front node and rear stores the last node
struct Queue {
    struct QueueNode *front, *rear;
};

// Helper function to create a new queue node
struct QueueNode* newQueueNode(struct TreeNode* node) {
    struct QueueNode* temp = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    temp->treeNode = node;
    temp->next = NULL;
    return temp;
}

// Helper function to create an empty queue
struct Queue* createQueue() {
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

// Function to add a tree node to the queue (enqueue)
void enqueue(struct Queue* q, struct TreeNode* node) {
    struct QueueNode* temp = newQueueNode(node);
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    q->rear->next = temp;
    q->rear = temp;
}

// Function to remove a tree node from the queue (dequeue)
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
    free(temp); // Free the queue node
    return dequeuedNode;
}

// Function to check if the queue is empty
bool isEmpty(struct Queue* q) {
    return q->front == NULL;
}

// --- BST Creation (Divide and Conquer) ---

// Helper function to create a new tree node.
struct TreeNode* createNode(int val) {
    struct TreeNode* newNode = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    newNode->val = val;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

// The core recursive function that builds the BST.
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

// Main function to convert a sorted array to a height-balanced BST.
struct TreeNode* sortedArrayToBST(int* nums, int numsSize) {
    if (numsSize <= 0) {
        return NULL;
    }
    return buildBST(nums, 0, numsSize - 1);
}

// --- Tree Traversal and Utility Functions ---

// Utility function to print the tree in-order (for verification)
void printInOrder(struct TreeNode* node) {
    if (node == NULL) return;
    printInOrder(node->left);
    printf("%d ", node->val);
    printInOrder(node->right);
}

// 🌳 Function to print the tree level by level
void printLevelOrder(struct TreeNode* root) {
    if (root == NULL) {
        printf("Tree is empty.\n");
        return;
    }
    
    struct Queue* q = createQueue();
    enqueue(q, root);
    
    while (!isEmpty(q)) {
        // Dequeue the current node
        struct TreeNode* currentNode = dequeue(q);
        printf("%d ", currentNode->val);
        
        // Enqueue left child if it exists
        if (currentNode->left != NULL) {
            enqueue(q, currentNode->left);
        }
        
        // Enqueue right child if it exists
        if (currentNode->right != NULL) {
            enqueue(q, currentNode->right);
        }
    }
    free(q); // Free the queue structure
}

// Utility function to free all nodes in the tree to prevent memory leaks
void freeTree(struct TreeNode* node) {
    if (node == NULL) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

// --- Main Function ---

int main() {
    int nums[] = {-10, -3, 0, 5, 9};
    int numsSize = sizeof(nums) / sizeof(nums[0]);

    printf("Original sorted array: ");
    for (int i = 0; i < numsSize; i++) {
        printf("%d ", nums[i]);
    }
    printf("\n\n");

    // Convert the array to a BST
    struct TreeNode* root = sortedArrayToBST(nums, numsSize);

    // An in-order traversal of a BST should yield the original sorted array.
    printf("✅ In-order traversal (verification): ");
    printInOrder(root);
    printf("\n");

    // A level-order traversal shows the tree's breadth and structure.
    printf("🔎 Level-order traversal (visualization): ");
    printLevelOrder(root);
    printf("\n\n");
    
    printf("Visual representation of the tree:\n");
    printf("      0\n");
    printf("     / \\\n");
    printf("   -3   9\n");
    printf("   /   /\n");
    printf(" -10  5\n");

    // Clean up allocated memory
    freeTree(root);
    
    return 0;
}