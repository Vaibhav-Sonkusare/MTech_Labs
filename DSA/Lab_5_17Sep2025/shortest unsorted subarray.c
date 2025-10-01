#include <stdio.h>
#include <limits.h>

int findUnsortedSubarray(int *, int, int *, int *);

int main() {
    int nums[] = {2, 6, 4, 8, 10, 9, 15};
    int numsSize = sizeof(nums) / sizeof(nums[0]);
    int left, right;

    int length = findUnsortedSubarray(nums, numsSize, &left, &right);

    printf("Length of shortest unsorted subarray: %d\n", length);

    if (length > 0) {
        printf("The shortest unsorted subarray is: ");
        for (int i = left; i <= right; i++) {
            printf("%d ", nums[i]);
        }
        printf("\n");
    } else {
        printf("The array is already sorted.\n");
    }

    return 0;
}

int findUnsortedSubarray(int* nums, int numsSize, int* left, int* right) {
    *left = -1;
    *right = -1;
    int max_seen = INT_MIN;
    int min_seen = INT_MAX;

    for (int i = 0; i < numsSize; i++) {
        max_seen = (nums[i] > max_seen) ? nums[i] : max_seen;
        if (nums[i] < max_seen) {
            *right = i;
        }
    }

    for (int i = numsSize - 1; i >= 0; i--) {
        min_seen = (nums[i] < min_seen) ? nums[i] : min_seen;
        if (nums[i] > min_seen) {
            *left = i;
        }
    }

    if (*left == -1) {
        return 0;
    }

    return *right - *left + 1;
}
