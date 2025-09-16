#include <stdio.h>
#include <stdlib.h>

// Definition for a binary tree node.
struct TreeNode {
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

// Helper function to create a new tree node.
struct TreeNode* createNode(int val) {
    struct TreeNode* newNode = (struct TreeNode*)malloc(sizeof(struct TreeNode));
    newNode->val = val;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

/**
 * The core recursive function that builds the BST.
 * It implements the divide and conquer strategy.
 * * @param nums The original sorted array of numbers.
 * @param start The starting index of the subarray for the current recursive call.
 * @param end The ending index of the subarray for the current recursive call.
 * @return The root of the constructed BST (or sub-BST).
 */
struct TreeNode* buildBST(int* nums, int start, int end) {
    // Base case: If the start index crosses the end index, the subarray is empty.
    if (start > end) {
        return NULL;
    }

    // 1. Divide: Find the middle element of the current subarray.
    // This middle element will become the root of the current subtree.
    int mid = start + (end - start) / 2;
    struct TreeNode* root = createNode(nums[mid]);

    // 2. Conquer: Recursively build the left and right subtrees.
    // The left subtree is built from the left half of the subarray (start to mid - 1).
    root->left = buildBST(nums, start, mid - 1);

    // The right subtree is built from the right half of the subarray (mid + 1 to end).
    root->right = buildBST(nums, mid + 1, end);
    
    // 3. Combine: The root, with its left and right subtrees attached, is returned.
    return root;
}

/**
 * Main function to convert a sorted array to a height-balanced BST.
 * This function serves as a wrapper to start the recursion.
 * * @param nums Pointer to the sorted integer array.
 * @param numsSize The number of elements in the array.
 * @return The root of the newly created height-balanced BST.
 */
struct TreeNode* sortedArrayToBST(int* nums, int numsSize) {
    if (numsSize <= 0) {
        return NULL;
    }
    // Kick off the recursive building process with the entire array range.
    return buildBST(nums, 0, numsSize - 1);
}

// Utility function to print the tree in-order (for verification)
void printInOrder(struct TreeNode* node) {
    if (node == NULL) {
        return;
    }
    printInOrder(node->left);
    printf("%d ", node->val);
    printInOrder(node->right);
}

// Main function to test the implementation.
int main() {
    int nums[] = {-10, -3, 0, 5, 9};
    int numsSize = sizeof(nums) / sizeof(nums[0]);

    printf("Original sorted array: ");
    for (int i = 0; i < numsSize; i++) {
        printf("%d ", nums[i]);
    }
    printf("\n");

    // Convert the array to a BST
    struct TreeNode* root = sortedArrayToBST(nums, numsSize);

    // An in-order traversal of a BST should yield the original sorted array.
    printf("In-order traversal of the constructed BST: ");
    printInOrder(root);
    printf("\n");
    
    // Example visualization of the resulting tree:
    //       0
    //      / \
    //    -3   9
    //    /   /
    //  -10  5

    return 0;
}