/*Program to reverse the bits of a given 32 bit signed integer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

int reverse_signed_32int(int);
void reverse_arr_divide_and_conquer(bool *arr, int low, int high);
void swap_halves(bool *, int, int, int);
bool *int32_to_bin_arr(int);
int bin_arr_to_int32(bool *arr);
void print_bit_arr(bool *);

int main(int argc, char **argv)
{
    int num = 536870912;
    int res = reverse_signed_32int(num);

    printf("Original Number: %d\n", num);
    printf("Reversed Number: %d\n", res);

    return 0;
}

int reverse_signed_32int(int num)
{
    // bool is_negative = false;
    // if (num < 0)
    // {
    //     num = num * (-1);
    //     is_negative = true;
    // }

    bool *arr = int32_to_bin_arr(num);
    print_bit_arr(arr);
    reverse_arr_divide_and_conquer(arr, 0, 31);
    print_bit_arr(arr);

    int ret_val = bin_arr_to_int32(arr);

    free(arr);
    return ret_val;
}

void reverse_arr_divide_and_conquer(bool *arr, int low, int high) {
    if (low < high) {
        int mid = (low + high) / 2;
        int half_size = (mid - low) + 1;

        if ((high - low + 1) % 2 != 0) {
            half_size = (high - low) / 2;
        }
        
        swap_halves(arr, low, mid + 1, half_size);

        // now recursively call our function on the two subarrays
        reverse_arr_divide_and_conquer(arr, low, mid);
        reverse_arr_divide_and_conquer(arr, mid + 1, high);
    }
}

/* swap all the bits from arr[start1] to arr[start1 + size] and arr[start2] to arr[start2 + size]
 * eg if arr = 1, 2, 3, 4, 5, 6, 7, 8; low = 0, high = 7
 * after swap, arr = 5, 6, 7, 8, 1, 2, 3, 4
 */
void swap_halves(bool *arr, int start1, int start2, int size) {
    for (int i=0; i< size; i++) {
        bool temp = arr[start1 + i];
        arr[start1 + i] = arr[start2 + i];
        arr[start2 + i] = temp;
    }
}

bool *int32_to_bin_arr(int num)
{
    bool *arr = calloc(32, sizeof(bool));
    for (int i = 0; i< 32; i++)
    {
        if ((num >> i) & 1U) {
            arr[31 - i] = true;
        }
    }

    return arr;
}

int bin_arr_to_int32(bool *arr) {
    int res = 0;
    for (int i=0; i<32; i++) {
        if (arr[i]) {
            res = (res * 2) + 1;
        } else {
            res = res * 2;
        }
        
    }

    return res;
}

void print_bit_arr(bool *arr)
{
    printf("\nBit arr: ");
    for (int i = 0; i < 32; i++)
        printf("%d ", arr[i]);
    printf("\n");
}