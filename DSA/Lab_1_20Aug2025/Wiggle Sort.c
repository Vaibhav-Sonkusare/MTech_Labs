#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void wiggle_sort(int *arr, int arr_size);
void swap_int(int *, int *);

int main(int argc, char **argv) {
    int arr[] = {1, 3, 4, 8, 6};
    // int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    // int arr[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    size_t arr_size = sizeof(arr)/sizeof(int);

    printf("arr = ");
    for (int i=0; i< arr_size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    wiggle_sort(arr, arr_size);

    for (int i=0; i< arr_size; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    return 0;
}

void wiggle_sort(int *arr, int arr_size) {
	int toogle = 1;
	for (int i=1; i< arr_size; i++) {
		if (toogle) {
			if (arr[i-1] > arr[i]) {
				swap_int(arr + i -1, arr + i);
			}
		} else {
			if (arr[i-1] < arr[i]) {
				swap_int(arr + i - 1, arr + i);
			}
		}
		toogle = (toogle + 1) % 2;
	}
}

void swap_int(int *a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}
