/* Solve using Dynamic Programming approach:
 * Given an integer array 'nums', return the lenght of the longest strictly increasing subsequence.
 */

#include <stdio.h>

int max(int, int);

int main() {
    int nums[] = {8, 2, 5, 1, 6, 3, 4, 7, 6};
    int size = sizeof(nums) / sizeof(int);
    int max_len = 1, end = 0;

    int dp[size];
    int prev[size];

    for (int i=0; i< size; i++) {
        dp[i] = 1;
        prev[i] = -1;
    }

    for (int i=0; i< size; i++) {
        for (int j = i - 1; j >= 0; j--) {
            if (nums[i] > nums[j] && dp[i] < dp[j] + 1) {
                dp[i] = dp[j] + 1;
                prev[i] = j;
            }
            // printf("dp[%d] = %d\n", i, dp[i]);
        }
        if (dp[i] > max_len) {
            max_len = dp[i];
            end = i;
        }
    }

    printf("Max Length of Substring can be %d.\n", max_len);

    // Print the longest strictly increasing subsequence
    int seq[max_len];
    int i = max_len - 1;
    int curr = end;
    while (curr != -1) {
        seq[i] = nums[curr];
        i--;
        curr = prev[curr];
    }
    printf("Longest Increasing Subsequence: ");
    for (int i=0; i< max_len; i++) {
        printf("%d ", seq[i]);
    }
    printf("\n");

    return 0;
}

int max(int a, int b) {
    return (a > b)? a : b;
}
