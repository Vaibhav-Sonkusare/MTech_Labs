/* Problem to find k closest points to origin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Structure to represent a point in 2D space
typedef struct {
    int x;
    int y;
} Point;

void find_k_closest(Point points[], int low, int high, int k);
int partition(Point points[], int low, int high);
long long squared_distance(Point p);
void swap(Point* a, Point* b);

int main() {
    Point points[] = {{3, 3}, {5, -1}, {-2, 4}, {1, 1}, {-1, -1}, {2, 2}};
    int n = sizeof(points) / sizeof(points[0]);
    int k = 3;

    if (k > n || k <= 0) {
        printf("Invalid value of k.\n");
        return 1;
    }

    printf("Original points:\n");
    for (int i = 0; i < n; i++) {
        printf("(%d, %d) \n", points[i].x, points[i].y);
    }

    // Find the k closest points. The result will be the first k elements of the array.
    find_k_closest(points, 0, n - 1, k);

    printf("\nThe %d closest points to the origin are:\n", k);
    for (int i = 0; i < k; i++) {
        printf("(%d, %d) with squared distance %lld\n", points[i].x, points[i].y, squared_distance(points[i]));
    }

    return 0;
}

// Function to calculate the squared distance of a point from the origin.
// We use squared distance to avoid floating-point calculations and potential precision errors.
// The comparison of distances will be the same as with the actual distance.
long long squared_distance(Point p) {
    // Using long long to prevent overflow for large coordinate values
    return (long long)p.x * p.x + (long long)p.y * p.y;
}

// Function to swap two points in the array
void swap(Point* a, Point* b) {
    Point temp = *a;
    *a = *b;
    *b = temp;
}

// Partition function (similar to Lomuto partition scheme in Quicksort)
// This function takes the last element as the pivot, places the pivot element
// at its correct position in the sorted array, and places all smaller elements
// (smaller distance) to the left of the pivot and all greater elements to the right.
int partition(Point points[], int low, int high) {
    // Pivot is the last element
    long long pivot_distance = squared_distance(points[high]);
    int i = (low - 1); // Index of smaller element

    for (int j = low; j < high; j++) {
        // If current element's distance is smaller than or equal to pivot's distance
        if (squared_distance(points[j]) <= pivot_distance) {
            i++; // increment index of smaller element
            swap(&points[i], &points[j]);
        }
    }
    // Place the pivot element at the correct position
    swap(&points[i + 1], &points[high]);
    return (i + 1);
}

// Main function that implements the Quickselect algorithm (Divide and Conquer)
// It finds the k-th smallest element in the array of points.
void find_k_closest(Point points[], int low, int high, int k) {
    if (low < high) {
        // pi is the partitioning index, points[pi] is now at the right place
        int pi = partition(points, low, high);

        // If partition index is the k-th element, we are done for this part.
        // We recursively call on both sides as we need all elements up to k,
        // not just the k-th element. A full sort isn't needed, but this
        // ensures all k elements are in the first k positions.

        // If the partition index is exactly k-1, it means the first k elements
        // are the k closest, though not necessarily in sorted order among themselves.
        if (pi == k - 1) {
            return;
        }

        // If partition index is greater than k-1, recur for the left subarray
        if (pi > k - 1) {
            find_k_closest(points, low, pi - 1, k);
        } else { // Else recur for the right subarray
            find_k_closest(points, pi + 1, high, k);
        }
    }
}