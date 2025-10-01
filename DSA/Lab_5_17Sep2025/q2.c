#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NUM 10000

bool isPossible(int *, int);

int main() {
    int nums[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5};
    int size = sizeof(nums) / sizeof(nums[0]);
    
    printf("Array: ");
    for (int i=0; i< size; i++) {
        printf("%d ", nums[i]);
    }
    if (isPossible(nums, size)) {
        printf("\nCan be split.\n");
    } else {
        printf("\nCannot be split.\n");
    }

    return 0;
}

bool isPossible(int* nums, int numsSize) {
    int count[MAX_NUM + 1] = {0};
    int end[MAX_NUM + 1] = {0};

    for (int i = 0; i < numsSize; i++) {
        count[nums[i]]++;
    }

    for (int i = 0; i < numsSize; i++) {
        int num = nums[i];

        if (count[num] == 0)
            continue;

        count[num]--;

        if (num > 0 && end[num - 1] > 0) {
            end[num - 1]--;
            end[num]++;
        } else if (count[num + 1] > 0 && count[num + 2] > 0) {
            count[num + 1]--;
            count[num + 2]--;
            end[num + 2]++;
        } else {
            return false;
        }
    }
    return true;
}
