/* Solve using Dynamic Programming approach:
 * Given an integer array 'nums', return the lenght of the longest strictly increasing subsequence.
 */

#include <stdio.h>

int max(int, int);

int main() {
    int nums[] = {8, 2, 5, 1, 6, 3, 4, 7, 6};
    int size = sizeof(nums) / sizeof(int);
    int max_len = 0, end;

    int dp[size];

    for (int i=0; i< size; i++) {
        dp[i] = 1;
        for (int j = i - 1; j >= 0; j--) {
            if (nums[i] > nums[j]) {
                dp[i] = max(dp[i], dp[j] + 1);
                if (max_len < dp[i]) {
                    max_len = dp[i];
                    end = i;
                }
            }
            // printf("dp[%d] = %d\n", i, dp[i]);
        }
    }

    printf("Max Length of Substring can be %d.\n", max_len);


    return 0;
}

int max(int a, int b) {
    return (a > b)? a : b;
}