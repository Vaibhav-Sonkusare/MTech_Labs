// C Program for Recursive Level Order Traversal
#include <stdio.h>
#include <stdlib.h>

// Define the structure for a binary tree node
typedef struct Node {
    int data;
    struct Node* left;
    struct Node* right;
} Node;

// Function to find the maximum of two integers
int max(int a, int b) {
    return (a > b) ? a : b;
}
// Create a new tree node
Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode) {
        newNode->data = data;
        newNode->left = NULL;
        newNode->right = NULL;
    }
    return newNode;
}

// Function to get the height of the tree
int height(Node* root) {
    if (root == NULL) return 0;
    int leftHeight = height(root->left);
    int rightHeight = height(root->right);
    return max(leftHeight,rightHeight) + 1;
}

// Function to print nodes of the tree at a given level
void printGivenLevel(Node* root, int level) {
    if (root == NULL) return;
    if (level == 1) printf("%d ", root->data);
    else if (level > 1) {
        printGivenLevel(root->left, level - 1);
        printGivenLevel(root->right, level - 1);
    }
}

// Function for level order traversal of the tree without using a queue
void levelOrderTraversal(Node* root) {
    int h = height(root);
    for (int i = 1; i <= h; i++) {
        printGivenLevel(root, i);
    }
}

// Example to demonstrate level order traversal without using a queue
int main() {
    // Create a simple binary tree
    Node* root = createNode(1);
    root->left = createNode(2);
    root->right = createNode(3);
    root->left->left = createNode(4);
    root->left->right = createNode(5);

    printf("Level Order Traversal:\n");
    // Perform the level order traversal on the tree without using a queue
    levelOrderTraversal(root);

    return 0;
}