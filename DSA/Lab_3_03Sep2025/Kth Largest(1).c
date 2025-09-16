/* This Program intends to find the Kth Largest (or the Kth smallest)
number from an array of integers in an average time complexity of O(n)*/

/* Here, we have modified the quicksort's partition function such that,
if an integer's correct position is index k, then that is our answer.*/

#include <stdio.h>

int kth_largest(int *, int, int, int);
int partition_modified(int *, int, int);
void swap_int(int *, int *);

int main(int argc, char **argv) {
    int arr[] = {30, 50, 20, 10, 35};
    int res = kth_largest(arr, 0, 4, 2);
    printf("arr[%d] = %d\n", res, arr[res]);
    for (int i=0; i<5; i++) printf("%d ", arr[i]);
    printf("\n");

    return 0;
}

int kth_largest(int *arr, int l, int h, int k) {
    if (l < h) {
        int p = partition_modified(arr, l, h);

        if (p == k) {
            return p;
        } else if (p < k) {
            return kth_largest(arr, p+1, h, k);
        } else { //p > k
            return kth_largest(arr, l, p-1, k);
        }
    } else if (l == h) {
        if (l == k) {
            return l;
        }
    }

    return -1;
}

int partition_modified(int *arr, int l, int h) {
    int pivot = arr[h];
    int i=-1, j=-1;

    while (j < h) {
        if (arr[++j] < pivot) {
            swap_int(arr + ++i, arr + j);
        }
    }

    swap_int(arr + ++i, arr + h);
    return i;
}

void swap_int(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}