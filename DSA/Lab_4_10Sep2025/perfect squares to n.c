/* Solve using Dynamic Programming approach:
	Given an integer n, return the least number of perfect square numbers that sum to n.*/

#include <stdio.h>
#include <limits.h>

int min(int, int);

int main() {
    int n = 370;

    int dp[n + 1];
    dp[0] = 0;
    int prev[n + 1];
    prev[0] = 0;

    for(int i=1; i<= n; i++) {
        dp[i] = INT_MAX;
        for (int j = 1; j * j <= i; j++) {
            int rem = i - (j * j);
            // dp[i] = min(dp[i], dp[rem] + 1);
            if (dp[i] > dp[rem] + 1) {
                dp[i] = dp[rem] + 1;
                prev[i] = rem;
            }
        }
    }

    printf("Min Perfect square to get %d is %d.\n", n, dp[n]);

    int num = n;
    printf("nums are: ");
    while (num > 0) {
        printf("%d, ", num - prev[num]);
        num = prev[num];
    }
    printf("\n");

    return 0;
}

int min (int a, int b) {
    return (a < b)? a: b;
}