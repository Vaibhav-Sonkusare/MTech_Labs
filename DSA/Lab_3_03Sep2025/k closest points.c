/* Problem to find k closest points to origin
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

    find_k_closest(points, 0, n - 1, k);

    printf("\nThe %d closest points to the origin are:\n", k);
    for (int i = 0; i < k; i++) {
        printf("(%d, %d) with squared distance %lld\n", points[i].x, points[i].y, squared_distance(points[i]));
    }

    return 0;
}

long long squared_distance(Point p) {
    return (long long)p.x * p.x + (long long)p.y * p.y;
}

void swap(Point* a, Point* b) {
    Point temp = *a;
    *a = *b;
    *b = temp;
}

int partition(Point points[], int low, int high) {
    long long pivot_distance = squared_distance(points[high]);
    int i = (low - 1);

    for (int j = low; j < high; j++) {
        if (squared_distance(points[j]) <= pivot_distance) {
            i++;
            swap(&points[i], &points[j]);
        }
    }
    swap(&points[i + 1], &points[high]);
    return (i + 1);
}


void find_k_closest(Point points[], int low, int high, int k) {
    if (low < high) {
        int pi = partition(points, low, high);

        if (pi == k - 1) {
            return;
        }

        if (pi > k - 1) {
            find_k_closest(points, low, pi - 1, k);
        } else {
            find_k_closest(points, pi + 1, high, k);
        }
    }
}