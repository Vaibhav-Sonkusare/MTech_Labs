#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void wiggle_sort(int *, int, int);
void merge(int *, int, int, int);

int main(int argc, char **argv) {
    // int arr[] = {1, 3, 4, 8, 6};
    int arr[] = {1, 2, 42, 3, 1, 20, 15};
    size_t arr_size = sizeof(arr)/sizeof(int);

    printf("arr = ");
    for (int i=0; i< arr_size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    wiggle_sort(arr, 0, arr_size - 1);

    for (int i=0; i< arr_size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}

void wiggle_sort(int *arr, int l, int h) {
    if (l < h) {
        int mid = (l + h) / 2;

        wiggle_sort(arr, l, mid);
        wiggle_sort(arr, mid + 1, h);
        merge(arr, l, mid, h);
    }
}

void merge(int *arr, int l, int mid, int h) {
    int n1 = mid - l + 1;
    int n2 = h - mid;

    int *a1 = calloc(n1, sizeof(int));
    if (a1 == NULL) {
        printf("Calloc failed!\n");
        exit(1);
    }
    int *a2 = calloc(n2, sizeof(int));
    if (a2 == NULL) {
        printf("Calloc failed!\n");
        exit(1);
    }

    int i = 0, j = 0, k = l;
    while (i< n1 && k<= mid) {
        a1[i] = arr[k];
        k++;
        i++;
    }
    while (j< n2 && k<= h) {
        a2[j] = arr[k];
        k++;
        j++;
    }

    i = 0;
    j = 0;
    k = l;
    bool toogle = true;
    while (i < n1 && j < n2 && k <= h) {
        if (toogle) {
            if (a1[i] < a2[j]) {
                arr[k] = a1[i];
                k++;
                i++;
            } else {
                arr[k] = a2[j];
                k++;
                j++;
            }
        } else {
            if (a1[i] > a2[j]) {
                arr[k] = a1[i];
                k++;
                i++;
            } else {
                arr[k] = a2[j];
                k++;
                j++;
            }
        }
        toogle = toogle ? false: true;
    }

    // int uw = 0;
    // if (k < h) {
    //     uw = k;
    // }

    while (i < n1 && k <= h) {
        arr[k] = a1[i];
        k++;
        i++;
    }

    while (j < n2 && k <= h) {
        arr[k] = a2[j];
        k++;
        j++;
    }

    // printf("l=%d mid=%d, h=%d, n1=%d, n2=%d, i=%d, j=%d, k=%d, uw=%d, a1= ", l, mid, h, n1, n2, i, j, k, uw);
    // for (i=0; i< n1; i++) printf("%d ", a1[i]);
    // printf("\t\t\ta2= ");
    // for (j=0; j< n2; j++) printf("%d ", a2[j]);
    // printf("\t\t\tarr= ");
    // for (k=l; k<= h; k++) printf("%d ", arr[k]);
    // printf("\n");

    free(a1);
    free(a2);
}