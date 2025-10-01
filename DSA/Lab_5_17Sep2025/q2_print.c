#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NUM 10000
#define MAX_SUBSEQS 10000
#define MAX_LENGTH 10000

typedef struct {
    int nums[MAX_LENGTH];
    int length;
} Subsequence;

Subsequence subseqs[MAX_SUBSEQS];
int subseq_count = 0;

int find_end_subsequence(int num) {
    for (int i = 0; i < subseq_count; i++) {
        if (subseqs[i].length > 0 && subseqs[i].nums[subseqs[i].length - 1] == num) {
            return i;
        }
    }
    return -1;
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

            int index = find_end_subsequence(num - 1);
            if (index != -1) {
                subseqs[index].nums[subseqs[index].length] = num;
                subseqs[index].length++;
            }
        } else if (count[num + 1] > 0 && count[num + 2] > 0) {
            count[num + 1]--;
            count[num + 2]--;
            end[num + 2]++;

            subseqs[subseq_count].nums[0] = num;
            subseqs[subseq_count].nums[1] = num + 1;
            subseqs[subseq_count].nums[2] = num + 2;
            subseqs[subseq_count].length = 3;
            subseq_count++;
        } else {
            return false;
        }
    }
    return true;
}

void printSubsequences() {
    printf("Subsequences formed:\n");
    for (int i = 0; i < subseq_count; i++) {
        printf("[");
        for (int j = 0; j < subseqs[i].length; j++) {
            printf("%d", subseqs[i].nums[j]);
            if (j < subseqs[i].length - 1)
                printf(", ");
        }
        printf("]\n");
    }
}

int main() {
    int nums[] = {1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6};
    int size = sizeof(nums) / sizeof(nums[0]);

    if (isPossible(nums, size)) {
        printf("True\n");
        printSubsequences();
    } else {
        printf("False\n");
    }

    return 0;
}
